/*
 *  UI Gadget
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <glib.h>
#include <appcore-efl.h>
#include <ui-gadget.h>
#include <Ecore_X.h>
#include <dlog.h>
#include <aul.h>
#include <app.h>
#include <app_control_internal.h>
#include <vconf.h>

#include "ug-client.h"

#include <pthread.h>

#include <appsvc.h>

#include <dlfcn.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <pkgmgr-info.h>

#ifdef _APPFW_FEATURE_UG_PROCESS_POOL
#include <privilege-control.h>
#include <sys/prctl.h>
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "UI_GADGET_CLIENT"

#define UG_CLIENT_API	__attribute__((visibility("default")))

#define BROWSER_APP_ID	"com.samsung.browser"

extern int appsvc_request_transient_app(bundle *b, Ecore_X_Window callee_id, appsvc_host_res_fn cbfunc, void *data);

static int _ug_client_ug_create(bundle *b, void *data);

static bool is_win_lower_for_terminating = false;

static bool is_app_pause = false;

#ifdef USE_UG_DESTORY_FOR_RELAUNCH
static bool check_relaunch = false;
#endif

static int home_screen_pid = 0;

static DBusConnection *bus;
static int ug_dbus_signal_handler_initialized = 0;

#ifdef _APPFW_FEATURE_UG_PROCESS_POOL
static int g_argc;
static char **g_argv;
static bool is_ug_client_pool = false;
static int max_cmdline_size = 0;
#endif

#ifdef _APPFW_FEATURE_APP_CONTROL_LITE
struct ugdata {
	ui_gadget_h ug;
	char *name;
	app_control_h request;
	int app_control_reply_id;
};
typedef struct ugdata ugdata_s;
static GList *g_uglist;

static int _ug_client_ug_list_comp(gconstpointer a, gconstpointer b)
{
	ugdata_s *key1 = (ugdata_s *) a;
	ugdata_s *key2 = (ugdata_s *) b;

	if(key1->ug == key2->ug) {
		return 0;
	} else {
		return -1;
	}
}

static void _ug_client_free_ug_list_info(ugdata_s *n)
{
	g_free(n->name);
	n->name = NULL;
	if(n->request) {
		app_control_destroy(n->request);
		n->request = NULL;
	}
	n->ug = NULL;
}

static void *_ug_client_find_ug_list_info(ui_gadget_h ug)
{
	GList *uglist;
	ugdata_s t;

	t.ug = ug;

	uglist = g_list_find_custom(g_uglist, &t, (GCompareFunc)_ug_client_ug_list_comp);
	if(uglist) {
		return (void *)uglist->data;
	}

	return NULL;
}

static ui_gadget_h _ug_client_get_top_ug(void)
{
	GList *uglist;
	ugdata_s *n;

	uglist = g_list_last(g_uglist);
	if(uglist) {
		n = uglist->data;
		if(n)
			return n->ug;
	}

	return NULL;
}

static int _ug_client_destory_ug(ui_gadget_h ug, void *data)
{
	ugdata_s *n = NULL;
	struct appdata *ad = data;

	if(g_list_length(g_uglist) <= 1) {
		LOGD("last ug will be removed at app terminate");
		return 0;
	}

	n = (ugdata_s *)_ug_client_find_ug_list_info(ug);
	if(n) {
		g_uglist = g_list_remove(g_uglist, n);
		LOGD("ug(%s) is removed. current ug list length(%d)", n->name, g_list_length(g_uglist));
		_ug_client_free_ug_list_info(n);
	} else {
		LOGE("cannot find ug(%s) at list", n->name);
		return 0;
	}

	ug_destroy(ug);

	return 1;
}
#endif

static int _ug_client_transient_window_del_cb(void *data)
{
	LOGD("transient_window_del_cb");
	return 0;
}

static int _ug_client_send_rotate_event(int angle)
{
	int ret = -1;
	LOGD("_ug_client_send_rotate_event angle : %d", angle);
	switch(angle) {
		case 0 :
			ret = ug_send_event(UG_EVENT_ROTATE_PORTRAIT);
			break;
		case 90 :
			ret = ug_send_event(UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN);
			break;
		case 180 :
			ret = ug_send_event(UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN);
			break;
		case 270 :
			ret = ug_send_event(UG_EVENT_ROTATE_LANDSCAPE);
			break;
		default :
			LOGW("wrong angle(%d) for send rotate event",angle);
			break;
	}

	return ret;
}

static void _ug_client_rotate(void *data, Evas_Object *obj, void *event)
{
	int changed_angle = 0;

	changed_angle = elm_win_rotation_get((const Evas_Object *)obj);
	if(changed_angle == -1) {
		LOGE("elm_win_rotation_get error");
		return;
	}

	LOGD("rotate call back : changed angle(%d)", changed_angle);

	_ug_client_send_rotate_event(changed_angle);

	return;
}

#ifdef CHECK_HOME_SCREEN_USING_VCONF
static void _ug_client_home_screen_top_cb(keynode_t *node, void *data)
{
	struct appdata *ad = data;

	if (!node) {
		LOGE("home screen top cb node value is null");
		return;
	}

	if ((node->value.i == VCONFKEY_IDLE_SCREEN_TOP_TRUE) && (!ad->is_transient)) {
		LOGW("home key pressed. window is not transient. ug client will be terminated");
		elm_exit();
	}
}
#else

static void _ug_client_home_screen_top_cb(void *data)
{
	struct appdata *ad = data;

	if((!ad->is_transient) && (home_screen_pid)) {

#ifdef CHECK_FOCUS_WINDOW_IS_HOME_SCREEN
		int home_pid_by_x;
		Ecore_X_Window focus_window;

		focus_window = ecore_x_window_focus_get();
		if(focus_window) {
			if(ecore_x_netwm_pid_get(focus_window, &home_pid_by_x) == EINA_FALSE) {
				LOGW("get_pid from focus window is failed");
			} else {
				LOGD("window focus pid : %d", home_pid_by_x);

				if(home_screen_pid == home_pid_by_x) {
					LOGW("home key pressed. window is not transient. ug client will be terminated");
					elm_exit();
				}
			}
		}
#else
		LOGW("home key pressed. window is not transient. ug client will be terminated");
		elm_exit();
#endif
	}

	return;
}

static DBusHandlerResult
_ug_client_dbus_signal_filter(DBusConnection *conn, DBusMessage *message,
		       void *user_data)
{
	const char *sender;
	const char *interface;
	int home_pid_by_dbus;

	DBusError error;

	dbus_error_init(&error);

	sender = dbus_message_get_sender(message);
	if (sender == NULL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (dbus_bus_get_unix_user(conn, sender, &error) != 0) {
		LOGW("reject by security issue - no allowed sender\n");
		dbus_error_free(&error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	interface = dbus_message_get_interface(message);
	if (interface == NULL) {
		LOGW("reject by security issue - no interface\n");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (dbus_message_is_signal(
	  message, interface, "home_launch")) {

	  	LOGD("interface signal is home_launch");

		if (dbus_message_get_args(message, &error, DBUS_TYPE_UINT32,
		     &home_pid_by_dbus, DBUS_TYPE_INVALID) == FALSE) {
			LOGW("Failed to get data: %s", error.message);
			dbus_error_free(&error);
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		LOGD("pid : %d", home_pid_by_dbus);

		home_screen_pid = home_pid_by_dbus;

		if(is_app_pause) {
			LOGD("home_launch signal under app_pause.\
				if home screen is top, app will be terminated");
			_ug_client_home_screen_top_cb(user_data);
		}
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static int _ug_client_dbus_listen_signal(void *data)
{
	DBusError error;
	char rule[128];

	if (ug_dbus_signal_handler_initialized)
		return 0;

	dbus_threads_init_default();

	dbus_error_init(&error);
	bus = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
	if (!bus) {
		LOGW("Failed to connect to the D-BUS daemon: %s", error.message);
		dbus_error_free(&error);
		return -1;
	}
	dbus_connection_setup_with_g_main(bus, NULL);

	snprintf(rule, 128,
		 "path='%s',type='signal',interface='%s'", "/aul/dbus_handler",
		 "com.samsung.aul.signal");
	/* listening to messages */
	dbus_bus_add_match(bus, rule, &error);
	if (dbus_error_is_set(&error)) {
		LOGW("Fail to rule set: %s", error.message);
		dbus_error_free(&error);
		return -1;
	}

	if (dbus_connection_add_filter(bus,
		_ug_client_dbus_signal_filter, data, NULL) == FALSE) {
		LOGW("dbus conntaction add fileter fail");
		return -1;
	}

	LOGD("bus : %p / filter func pointer : %p", bus , _ug_client_dbus_signal_filter);

	ug_dbus_signal_handler_initialized = 1;

	return 0;
}

static void _ug_client_dbus_signal_handler_fini(void *data)
{
	DBusError error;
	char rule[128];

	if (!ug_dbus_signal_handler_initialized)
		return;

	if(!dbus_connection_get_is_connected(bus)) {
		LOGD("dbus connection(%p) is not connected", bus);
		goto func_out;
	}

	dbus_connection_remove_filter(bus, _ug_client_dbus_signal_filter, data);

	dbus_error_init(&error);

	snprintf(rule, 128,
		 "path='%s',type='signal',interface='%s'", "/aul/dbus_handler",
		 "com.samsung.aul.signal");
	dbus_bus_remove_match(bus, rule, &error);
	if (dbus_error_is_set(&error)) {
		LOGE("Fail to rule unset: %s", error.message);
		dbus_error_free(&error);
		goto func_out;
	}

	dbus_connection_close(bus);

	LOGD("ug dbus signal finialized");

func_out :
	ug_dbus_signal_handler_initialized = 0;
	bus = NULL;

	return;
}
#endif

static bool _ug_client_set_split_window_prop(bundle *b, void *data)
{
	struct appdata *ad = data;
	int ret;
	bool func_ret = false;
	char *metadata_value = NULL;

	char appid[256]={0,};
	ret = aul_app_get_appid_bypid(getpid(), appid, sizeof(appid));
	if(ret < 0) {
		LOGE("error in getting appid");
		return false;
	} else {
		SECURE_LOGD("appid : %s", appid);
	}

	pkgmgrinfo_appinfo_h handle;
	ret = pkgmgrinfo_appinfo_get_appinfo(appid, &handle);
	if (ret != PMINFO_R_OK) {
		LOGE("error in getting appinfo");
		return false;
	}
	ret = pkgmgrinfo_appinfo_get_metadata_value(handle,
		"http://developer.samsung.com/tizen/metadata/multiwindow",
		&metadata_value);
	if (ret != PMINFO_R_OK) {
		LOGE("error in get metadata from appinfo");
		pkgmgrinfo_appinfo_destroy_appinfo(handle);
		return false;
	} else {
		LOGD("multi window supported app");
	}
	pkgmgrinfo_appinfo_destroy_appinfo(handle);

	elm_win_wm_desktop_layout_support_set(ad->win, EINA_TRUE );

	const char* val_startup = NULL;
	const char* val_layout = NULL;
	const char* default_startup = "0";
	const char* default_layout = "-1";
	void* evas_data = NULL;
	int id = -1;

	val_startup = bundle_get_val(b, "window_startup_type");
	if(!val_startup) {
		val_startup = default_startup;
	}
	val_layout = bundle_get_val(b, "window_layout_id");
	if(!val_layout) {
		val_layout = default_layout;
	}

	LOGD("val_startup : %s, val_layout : %s", val_startup, val_layout);

	evas_data = evas_object_data_get(ad->win, "id_startup_by");
	if(!evas_data) {
		id = elm_win_aux_hint_add(ad->win, "wm.policy.win.startup.by", val_startup);
		if (id != -1) {
			evas_object_data_set(ad->win, "id_startup_by", (void*)id );
		} else {
			LOGE("fail to add win startup hint");
			goto func_out;
		}
	} else {
		if(!elm_win_aux_hint_val_set(ad->win,(const int)evas_data, val_startup)) {
			LOGE("fail to set win startup hint");
			goto func_out;
		}
	}

	evas_data = evas_object_data_get(ad->win, "id_layout_pos");
	if(!evas_data) {
		id = elm_win_aux_hint_add(ad->win, "wm.policy.win.zone.desk.layout.pos", val_layout);
		if (id != -1) {
			evas_object_data_set(ad->win, "id_layout_pos", (void*)id );
		} else {
			LOGE("fail to add win layout hint");
			goto func_out;
		}
	} else {
		if(!elm_win_aux_hint_val_set(ad->win,(const int)evas_data, val_layout)) {
			LOGE("fail to set win layout hint");
			goto func_out;
		}
	}

	func_ret = true;
	LOGD("mutil window aux hint success");

func_out :
	return func_ret;
}

static void _ug_client_ug_create_preset(bundle *b, void *data)
{
	int ret;
	Ecore_X_Window id2;
	const char *caller_appid = NULL;
	struct appdata *ad = data;

#ifdef _APPFW_FEATURE_APP_CONTROL_LITE
	if(g_uglist) { // Need to be invoked one time
		return;
	}
#endif

	if(_ug_client_set_split_window_prop(b,data)) {
		LOGD("multiwindow prop is set");
	}

	id2 = elm_win_xwindow_get(ad->win);

	caller_appid = bundle_get_val(b, AUL_K_CALLER_APPID);
	if (caller_appid && strncmp(caller_appid, BROWSER_APP_ID, strlen(BROWSER_APP_ID)) == 0) {
		SECURE_LOGD("If the caller is browser app, transient is not occurred.");
		ad->is_transient = 0;
	}
	else {
		ret = appsvc_request_transient_app(b, id2, _ug_client_transient_window_del_cb, NULL);
		if (ret) {
			LOGD("fail to request transient app: return value(%d)", ret);
			ad->is_transient = 0;
#ifdef CHECK_HOME_SCREEN_USING_VCONF
			if(vconf_notify_key_changed(VCONFKEY_IDLE_SCREEN_TOP, _ug_client_home_screen_top_cb, ad) != 0) {
				LOGW("home screen vconf key changed cb error");
			}
#else
			if(_ug_client_dbus_listen_signal(data) < 0) {
				LOGW("home screen dbus register error");
			}
#endif
		} else {
			/* check home screen raise */
			ad->is_transient = 1;
		}
	}
}

void _ug_client_layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	struct appdata *ad;
	Evas_Object *base;

	if (!ug || !priv)
		return;

	ad = priv;

	base = ug_get_layout(ug);
	if (!base) {
		LOGE("base layout is null");
		return;
	}

	switch (mode) {
	case UG_MODE_FULLVIEW:
		evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
		ug_disable_effect(ug);
		evas_object_show(ad->win);
		elm_object_content_set(ad->ly_main, base);
		evas_object_show(base);
		elm_win_activate(ad->win);
		break;
	case UG_MODE_FRAMEVIEW:
		elm_object_part_content_set(ad->ly_main, "content", base);
		break;
	default:
		break;
	}
}

void _ug_client_result_cb(ui_gadget_h ug, app_control_h reply, void *priv)
{
	struct appdata *ad = NULL;
	int ret;
	char* value = NULL;
	int result;

	if (!ug || !priv)
		return;

	ret = app_control_get_extra_data (reply, UG_APP_CONTROL_DATA_RESULT, &value);
	if((ret == APP_CONTROL_ERROR_NONE) && (value)) {
		result = atoi(value);
		LOGD("reply result is %d", result);
	} else {
		LOGW("get reply result error(%d) . result will be APP_CONTROL_RESULT_SUCCEEDED", ret);
		result = APP_CONTROL_RESULT_SUCCEEDED;
	}

	ad = priv;
	if (!ad) {
		LOGE("appdata is null");
		return;
	}

#ifdef _APPFW_FEATURE_APP_CONTROL_LITE
	ugdata_s *n = NULL;
	app_control_h request = NULL;

	n = (ugdata_s *)_ug_client_find_ug_list_info(ug);
	if(n && n->request) {
		request = n->request;
	}

	if(n->app_control_reply_id != -1) {

		LOGD("aul call ug result callback");

		bundle *bundle_reply;
		if (app_control_to_bundle(reply, &bundle_reply) != APP_CONTROL_ERROR_NONE) {
			LOGE("service to bundle error");
			return;
		}

		LOGD("n->app_control_reply_id : %d", n->app_control_reply_id);
		ret = aul_call_ug_result_callback(bundle_reply, 0, n->app_control_reply_id);
		if (ret != AUL_R_OK)
			LOGE("aul_call_ug_result_callback failed, %d", ret);
		else
			LOGE("aul_call_ug_result_callback OK");
	} else {

		LOGD("app_control_reply_to_launch_request");

		ret = app_control_reply_to_launch_request(reply, request, (app_control_result_e)result);
		if (ret != APP_CONTROL_ERROR_NONE)
			LOGE("app_control_reply_to_launch_request failed, %d", ret);
		else
			LOGE("app_control_reply_to_launch_request OK");
	}
#else
	ret = app_control_reply_to_launch_request(reply, ad->request, (app_control_result_e)result);
	if (ret != APP_CONTROL_ERROR_NONE)
		LOGE("app_control_reply_to_launch_request failed, %d", ret);
#endif
}

void _ug_client_destroy_cb(ui_gadget_h ug, void *priv)
{
	struct appdata *ad = NULL;

	if (!ug)
		return;

	ad = priv;
	if (!ad) {
		LOGE("appdata is null. elm exit");
		elm_exit();
		return;
	}

#ifdef _APPFW_FEATURE_APP_CONTROL_LITE
	if(!_ug_client_destory_ug(ug, priv)) {
		return;
	}
#endif

	if((ad->win) && (ad->is_transient == 1)) {
		LOGD("unsert prev transient");
		ecore_x_icccm_transient_for_unset(elm_win_xwindow_get(ad->win));
		ad->is_transient = 0;
	}
	LOGD("window lower");
	elm_win_lower(ad->win);
	is_win_lower_for_terminating= true;
	elm_exit();
}

void _ug_client_end_cb(ui_gadget_h ug, void *priv)
{
	if (!ug) {
		LOGW("ug param error");
		return;
	}

	LOGD("_ug_client_end_cb invoked");

#ifdef USE_UG_DESTORY_FOR_RELAUNCH
	if(check_relaunch) {
		_ug_client_ug_create(priv);
		check_relaunch = false;
	}
#endif
}

#ifdef _APPFW_FEATURE_APP_CONTROL_LITE
static int _ug_client_ug_create(bundle *b, void *data)
{
	struct appdata *ad = data;
	struct ug_cbs cbs = { 0, };
	enum ug_mode mode = UG_MODE_FULLVIEW;

	ui_gadget_h ug;
	app_control_h service;

	cbs.layout_cb = _ug_client_layout_cb;
	cbs.destroy_cb = _ug_client_destroy_cb;
	cbs.result_cb = _ug_client_result_cb;
	cbs.end_cb = _ug_client_end_cb;
	cbs.priv = ad;

	mode = ad->is_frameview ? UG_MODE_FRAMEVIEW : UG_MODE_FULLVIEW;

	const char *app_exec = bundle_get_val(b, "__AUL_UG_EXEC__");
	char *ug_name = NULL;
	LOGD("app_exec : %s", app_exec);
	if(app_exec) {
		char *rchr = strrchr(app_exec, '/');
		if(rchr == NULL) {
			LOGE("Invalid exec path");
			return -1;
		}
		ug_name = &rchr[1];
	} else {
		ug_name = ad->name;
	}

	const char *reply_id = bundle_get_val(b, "__AUL_UG_ID__");

	if (ad->data)	/* ug-launcher */ {
		service_create_event(ad->data, &service);
		LOGD("ug-launcher service create case");
	} else {
		service_create_event(b, &service);
		LOGD("service create from bundle");
	}

	ug = ug_create(NULL, ug_name, mode, service, &cbs);
	if (ug == NULL) {
		LOGE("ug_create fail: %s", ug_name);
		elm_exit();
		return -1;
	}

	ugdata_s *n;
	n = calloc(1, sizeof(ugdata_s));
	if(n == NULL) {
		LOGE("alloc error");
		return -1;
	}

	if(ug_name)
		n->name = strdup(ug_name);

	if(service) {
		app_control_clone(&n->request, service);
		app_control_destroy(service);
	}

	n->ug = ug;
	if(reply_id) {
		sscanf(reply_id, "%d", &(n->app_control_reply_id));
	} else {
		n->app_control_reply_id = -1;
	}

	LOGE("n->app_control_reply_id : %d", n->app_control_reply_id);

	g_uglist = g_list_append(g_uglist, n);
	if(!g_uglist) {
		LOGE("g_list_append fail");
		return -1;
	}
	return 0;
}
#else
static int _ug_client_ug_create(bundle *b, void *data)
{
	struct appdata *ad = data;
	struct ug_cbs cbs = { 0, };
	enum ug_mode mode = UG_MODE_FULLVIEW;

	app_control_h service;

	cbs.layout_cb = _ug_client_layout_cb;
	cbs.destroy_cb = _ug_client_destroy_cb;
	cbs.result_cb = _ug_client_result_cb;
	cbs.end_cb = _ug_client_end_cb;
	cbs.priv = ad;

	mode = ad->is_frameview ? UG_MODE_FRAMEVIEW : UG_MODE_FULLVIEW;

	if (ad->data)   /* ug-launcher */
			app_control_create_event(ad->data, &service);
    else
			app_control_create_event(b, &service);

    if(service) {
            if(ad->request) {
                    app_control_destroy(ad->request);
                    ad->request = NULL;
            }
            app_control_clone(&ad->request, service);
            app_control_destroy(service);
    }

	LOGD("ad->name : %s", ad->name);

	ad->ug = ug_create(NULL, ad->name, mode, ad->request, &cbs);
	if (ad->ug == NULL) {
		LOGE("ug_create fail: %s", ad->name);
		elm_exit();
		return -1;
	}
	return 0;
}
#endif


static void _ug_client_win_del(void *data, Evas_Object *obj, void *event)
{
	elm_exit();
}

static Evas_Object *_ug_client_create_win(const char *name)
{
	int w, h;

#ifdef _APPFW_FEATURE_PROCESS_POOL
	Evas_Object *eo = (Evas_Object*)aul_get_preinit_window(name);
	if (eo == NULL)
	{
		LOGD("preinit win is null. ug client will make win");
		eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	}
#else

#ifdef _APPFW_FEATURE_APP_CONTROL_LITE
	elm_config_preferred_engine_set("opengl_x11");
#endif

#ifdef _APPFW_FEATURE_UG_PROCESS_POOL
	elm_config_preferred_engine_set("opengl_x11");
#endif

	Evas_Object *eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
#endif
	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_conformant_set(eo, EINA_TRUE);
		evas_object_smart_callback_add(eo, "delete,request",
					       _ug_client_win_del, NULL);
		ecore_x_window_size_get(ecore_x_window_root_first_get(),
					&w, &h);
		evas_object_resize(eo, w, h);

		elm_win_indicator_mode_set(eo,ELM_WIN_INDICATOR_SHOW);
		elm_win_indicator_opacity_set(eo, ELM_WIN_INDICATOR_OPAQUE);
	}

	return eo;
}

static Evas_Object *_ug_client_load_edj(Evas_Object *parent, const char *file,
			     const char *group)
{
	Evas_Object *eo;
	int r;
	eo = elm_layout_add(parent);
	if (eo) {
		r = elm_layout_file_set(eo, file, group);
		if (!r) {
			evas_object_del(eo);
			return NULL;
		}
		evas_object_size_hint_weight_set(eo,
						 EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
	}
	return eo;
}

static int _ug_client_low_memory(void *dummy, void *data)
{
	return ug_send_event(UG_EVENT_LOW_MEMORY);
}

static int _ug_client_low_battery(void *dummy, void *data)
{
	return ug_send_event(UG_EVENT_LOW_BATTERY);
}

static int _ug_client_lang_changed(void *dummy, void *data)
{
	char* lang = NULL;

	lang = vconf_get_str(VCONFKEY_LANGSET);
	if(lang) {
		LOGD("lang : %s", lang);
		elm_language_set((const char*)lang);
		free(lang);
	} else {
		LOGW("language get error");
	}

	return ug_send_event(UG_EVENT_LANG_CHANGE);
}

static int _ug_client_region_changed(void *dummy, void *data)
{
	return ug_send_event(UG_EVENT_REGION_CHANGE);
}

static void _ug_client_ug_terminate(void *data)
{
	struct appdata *ad = data;

	LOGD("ug_terminate called");

	if((ad->win) && (ad->is_transient == 1)) {
		LOGD("unset prev transient");
		ecore_x_icccm_transient_for_unset(elm_win_xwindow_get(ad->win));
		ad->is_transient = 0;
	}

#ifdef USE_UG_DESTORY_FOR_RELAUNCH
	ug_destroy(ad->ug);
#else
	ug_destroy_all();
#endif
	ad->ug = NULL;

	LOGD("ug_terminate end");

	return;
}

static int _ug_client_ug_file_exist(const char *filename)
{
	FILE *file;
	if ((file = fopen(filename, "r"))) {
		fclose(file);
		return 1;
	}

	return 0;
}

static void _ug_client_ug_module_load(void *data)
{
	struct appdata *ad = data;
	char ug_file[PATH_MAX];

	LOGD("_ug_client_ug_module_load called");

	do {
		snprintf(ug_file, PATH_MAX, "/usr/ug/lib/libug-%s.so", ad->name);
		if (_ug_client_ug_file_exist(ug_file))
			break;
		snprintf(ug_file, PATH_MAX, "/opt/ug/lib/libug-%s.so", ad->name);
		if (_ug_client_ug_file_exist(ug_file))
			break;
		snprintf(ug_file, PATH_MAX, "/opt/usr/ug/lib/libug-%s.so", ad->name);
		if (_ug_client_ug_file_exist(ug_file))
			break;
	} while (0);

	dlopen(ug_file, RTLD_LAZY);
	return;
}

static void _ug_client_prt_usage(const char *cmd)
{
	fprintf(stderr, "Usage: %s [-f] [-F] -n <UG NAME> [-d <Arguments>]\n",
		cmd);
	fprintf(stderr, "   Options:\n");
	fprintf(stderr, "            -d argument\n");
	fprintf(stderr, "            -F Fullview mode (Default)\n");
	fprintf(stderr, "            -f Frameview mode\n");
	fprintf(stderr, "   Example:\n");
	fprintf(stderr,
		"            %s -F -n helloUG-efl -d \"name,John Doe\" -d \"age,30\"\n",
		cmd);

}

static void _ug_client_main_quit_cb(void *data, Evas_Object *obj,
			 const char *emission, const char *source)
{
	elm_exit();
}

static int _ug_client_update_argument(const char *optarg, struct appdata *ad)
{
	const char *key;
	const char *val;
	key = strtok((char *)optarg, ",");
	if (!key)
		return -1;

	val = optarg + strlen(key) + 1;

	if (!ad->data)
		ad->data = bundle_create();
	if (!ad->data)
		return -1;
	bundle_add(ad->data, key, val);
	return 0;
}

#ifdef _APPFW_FEATURE_UG_PROCESS_POOL
static inline int _ug_client_change_cmdline(const char *cmdline)
{
	if ((!cmdline) || ((strlen(cmdline) > max_cmdline_size + 1))) {
		LOGW("input cmdline error");
		return -1;
	}

	memset(g_argv[0], '\0', max_cmdline_size);
	snprintf(g_argv[0], max_cmdline_size, "%s", cmdline);

	return 0;
}

static int _ug_client_set_window_name(const Evas_Object *preinit_window, const char *win_name)
{
	int ret = -1;
	if(!win_name) {
		LOGW("invalid window name");
		return -1;
	}

	const Evas *e = evas_object_evas_get(preinit_window);
    if(e) {
            Ecore_Evas *ee = ecore_evas_ecore_evas_get(e);
            if(ee) {
                    ecore_evas_name_class_set(ee, win_name, win_name);
                    LOGD("name class set success : %s", win_name);
                    ret = 0;
            }
    }

	return ret;
}

static void _ug_client_set_env(bundle * kb)
{
	const char *str;

	setenv("PKG_NAME", bundle_get_val(kb, AUL_K_PKG_NAME), 1);

	str = bundle_get_val(kb, AUL_K_STARTTIME);
	if (str != NULL)
		setenv("APP_START_TIME", str, 1);

	setenv("HWACC", bundle_get_val(kb, AUL_K_HWACC), 1);
	setenv("TASKMANAGE", bundle_get_val(kb, AUL_K_TASKMANAGE), 1);
}
#endif

static int app_create(void *data)
{
	struct appdata *ad = data;
	Evas_Object *win;
	Evas_Object *ly;
	Evas_Object *conform;
	Evas_Object *bg;

	LOGD("app_create");

	elm_app_base_scale_set(1.8);

	/* create window */
	win = _ug_client_create_win(PACKAGE);
	if (win == NULL)
		return -1;
	ad->win = win;
	UG_INIT_EFL(ad->win, UG_OPT_INDICATOR_ENABLE);

#ifdef _APPFW_FEATURE_PROCESS_POOL
	bg = (Evas_Object*)aul_get_preinit_background();
	if (bg == NULL)
	{
		bg = elm_bg_add(win);
		evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(win, bg);
	}
#else
	bg = elm_bg_add(win);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, bg);
#endif
	evas_object_show(bg);

#ifdef _APPFW_FEATURE_PROCESS_POOL
	conform = (Evas_Object*)aul_get_preinit_conformant();
	if (conform == NULL)
	{
		conform = elm_conformant_add(win);
	}
#else
	conform = elm_conformant_add(win);
#endif
	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	ad->conform = conform;

	bg = elm_bg_add(conform);
	elm_object_style_set(bg, "indicator/headerbg");
	elm_object_part_content_set(conform, "elm.swallow.indicator_bg", bg);
	evas_object_show(bg);

	/* load edje */
	ly = _ug_client_load_edj(conform, EDJ_FILE, GRP_MAIN);
	if (ly == NULL)
		return -1;
	elm_win_resize_object_add(win, conform);

	evas_object_show(conform);
	elm_object_content_set(conform, ly);
	edje_object_signal_callback_add(elm_layout_edje_get(ly),
					"EXIT", "*", _ug_client_main_quit_cb, NULL);
	ad->ly_main = ly;
	_ug_client_lang_changed(NULL, ad);

	/* rotate notice */
	int angle = -1;
	angle = elm_win_rotation_get((const Evas_Object *)win);
	LOGE("rotate : %d", angle);
	if(angle != -1) {
		_ug_client_send_rotate_event(angle);
	} else {
		LOGE("elm win rotation get error");
	}
	if(elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, (const int*)&rots, 4);
	} else {
		LOGW("wm rotation supported get error");
	}
	evas_object_smart_callback_add(win, "wm,rotation,changed", _ug_client_rotate, data);

	appcore_set_event_callback(APPCORE_EVENT_LOW_MEMORY, _ug_client_low_memory, ad);
	appcore_set_event_callback(APPCORE_EVENT_LOW_BATTERY, _ug_client_low_battery, ad);
	appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE, _ug_client_lang_changed, ad);
	appcore_set_event_callback(APPCORE_EVENT_REGION_CHANGE, _ug_client_region_changed, ad);

	ad->is_transient = 0;

	return 0;
}

static int app_terminate(void *data)
{
	struct appdata *ad = data;

	LOGD("app_terminate called");

	/* dl open for blocking bs in case that unremoved cb is called after dl close */
	_ug_client_ug_module_load(data);

	_ug_client_dbus_signal_handler_fini(data);

	evas_object_smart_callback_del(ad->win, "wm,rotation,changed", _ug_client_rotate);

	_ug_client_ug_terminate(data);

	if (ad->win) {
		evas_object_del(ad->win);
		ad->win = NULL;
	}

	if (ad->name) {
		free(ad->name);
	}

#ifndef _APPFW_FEATURE_APP_CONTROL_LITE
	if(ad->request) {
		LOGD("service handle destoroy");
		app_control_destroy(ad->request);
		ad->request = NULL;
	}
#endif

	LOGD("app_terminate end");

	return 0;
}

static int app_pause(void *data)
{
	if(is_win_lower_for_terminating) {
		LOGD("terminating. pause event is ignored");
		return 0;
	} else {
		LOGD("app_pause called");
	}

	ug_pause();

#ifndef CHECK_HOME_SCREEN_USING_VCONF
	_ug_client_home_screen_top_cb(data);
#endif

	is_app_pause = true;

	return 0;
}

static int app_resume(void *data)
{
	if(is_win_lower_for_terminating) {
		LOGD("terminating. resume event is ignored");
	} else {
		LOGD("app_resume called");
		ug_resume();
	}

	is_app_pause = false;

	return 0;
}

static long getoftime()
{
        struct timeval tv;
        long curtime = 0;

        gettimeofday(&tv, NULL);
        curtime = tv.tv_sec * 1000000 + tv.tv_usec;

        return curtime;
}

#ifdef _APPFW_FEATURE_UG_PROCESS_POOL
static int __get_app_dir_name(const char *pkgid, char **dir_name)
{
	char dirname[PATH_MAX];
	int r;

	if(!pkgid) {
		LOGD("pkgid is null");
	}

	r = snprintf(dirname, PATH_MAX, "/opt/usr/apps/%s/res/locale",pkgid);
	if (r < 0)
		return -1;
	if (access(dirname, R_OK) == 0)
		goto func_out;

	r = snprintf(dirname, PATH_MAX, "/usr/apps/%s/res/locale",pkgid);
	if (r < 0)
		return -1;

func_out:
	if(strlen(dirname) > 0) {
		*dir_name = strdup(dirname);
	}
	return 0;
}
#endif


static int app_reset(bundle *b, void *data)
{
	struct appdata *ad = data;
	int ret = 0;

	LOGD("app_reset invoked");

	if(!b) {
		LOGW("invalid input param(bundle) error");
		return 0;
	}

#ifdef _APPFW_FEATURE_UG_PROCESS_POOL

	if((ad->name) && (strcmp(ad->name, "ug-client") == 0) 
		&& (!bundle_get_val(b, AUL_K_EXEC))) {
		LOGD("ug-client pool reset ready");
		is_ug_client_pool = true;
		return 0;
	}

	LOGD("ad->name : %s", ad->name);

	if(is_ug_client_pool) {

		long t1 = getoftime();
		
		char *app_path = NULL;
		const char *pkg_name = NULL;
		const char *pkg_type = NULL;
		const char *pkg_exec = NULL;
		char process_name[16] = { 0, };

		LOGD("ug-client pool reset continue");

		pkg_name = bundle_get_val(b, AUL_K_PKG_NAME);
		pkg_type = bundle_get_val(b, AUL_K_PACKAGETYPE);
		pkg_exec = bundle_get_val(b, AUL_K_EXEC);

		if((!pkg_name) || (!pkg_type) || (!pkg_exec)) {
			LOGW("wrong pkg info");
			return 0;
		}

		long t3 = getoftime();

		/* SET PRIVILEGES */
		SECURE_LOGD("[candidate] pkg_name : %s / pkg_type : %s / app_path : %s ", 
				pkg_name, pkg_type, pkg_exec);
		perm_app_set_privilege(pkg_name, pkg_type, pkg_exec);

		long t4 = getoftime();
		LOGD("PRIVILEGES time : %ld", t4-t3);

		/* SET UG NAME */
		app_path = strrchr(pkg_exec, '/');
		if(app_path) {
			if(ad->name) free(ad->name);
			ad->name = strdup(&app_path[1]);
		}

		/* SET PROCESS NAME */
		snprintf(process_name, 16, "%s", ad->name);
		prctl(PR_SET_NAME, process_name);
	
		/* SET ENV */	
		_ug_client_set_env(b);

		/* SET CMDLINE */
		if(_ug_client_change_cmdline(pkg_exec) < 0) {
			LOGW("change cmdline error");
			return 0;
		}

		/* SET I18N */
		t3 = getoftime();
		const char* pkg_id = bundle_get_val(b, AUL_K_PKGID);
		LOGD("pkg_id : %s", pkg_id);
		char *dir_name = NULL;
		__get_app_dir_name(pkg_id, &dir_name);
		LOGD("dir name : %s", dir_name);
		appcore_set_i18n((const char*)dir_name, (const char*)pkg_name);
		if(dir_name) free(dir_name);
		t4 = getoftime();
		LOGD("total I18N time : %ld", t4-t3);

		/* SET WINDOW NAME */
		if(_ug_client_set_window_name(ad->win, pkg_name) < 0) {
			LOGW("change cmdline error");
			return 0;
		}

		long t2 = getoftime();
		LOGD("init time : %ld", t2-t1);
	}

	int rc = system("touch /home/app/abc.txt");
	LOGD("system rc : %d / errno : %d", rc, errno);
#endif


#ifndef _APPFW_FEATURE_APP_CONTROL_LITE
	if(ad->ug) {
		LOGD("single instance ug. new ug will be launched after old ug is destroyed");
		_ug_client_ug_terminate(data);
		_ug_client_ug_create_preset(b,data);
#ifdef USE_UG_DESTORY_FOR_RELAUNCH
		check_relaunch = true;
#else
		_ug_client_ug_create(b,data);
#endif
		return 0;
	}
#endif

	_ug_client_ug_create_preset(b,data);
	ret = _ug_client_ug_create(b,data);
	if((ret == 0) && (ad->win)) {
		evas_object_show(ad->win);
		elm_win_activate(ad->win);
	}

	return 0;
}

UG_CLIENT_API int main(int argc, char *argv[])
{
	int opt;
	struct appdata ad;
	struct appcore_ops ops = {
		.create = app_create,
		.terminate = app_terminate,
		.pause = app_pause,
		.resume = app_resume,
		.reset = app_reset,
	};
	int cmdlen = 0;

#ifdef _APPFW_FEATURE_UG_PROCESS_POOL
	int i;

	g_argc = argc;
	g_argv = argv;
	for (i = 0; i < argc; i++) {
		max_cmdline_size += (strlen(argv[i]) + 1);
	}

	setenv("HOME", "/home/app", 1);
#endif

	memset(&ad, 0x0, sizeof(struct appdata));
	ops.data = &ad;

	cmdlen = strlen(argv[0]);
	if (strncmp(argv[0], "ug-launcher", cmdlen) == 0
		|| strncmp(argv[0], "/usr/bin/ug-launcher", cmdlen) == 0) {

		setenv("HOME", "/home/app", 1);
		
		while ((opt = getopt(argc, argv, "n:d:")) != -1) {
			switch (opt) {
			case 'n':
				if (optarg)
					ad.name = strdup(optarg);
				break;
			case 'f':
				ad.is_frameview = 1;
				break;
			case 'F':
				ad.is_frameview = 0;
				break;
			case 'd':
				if (_ug_client_update_argument(optarg, &ad)) {
					if (ad.data)
						bundle_free(ad.data);
					_ug_client_prt_usage(argv[0]);
					return -1;
				}
				break;
			default:
				_ug_client_prt_usage(argv[0]);
				return -1;
			}
		}

		if (!ad.name) {
			_ug_client_prt_usage(argv[0]);
			return -1;
		}
		argc = 1; // remove appsvc bundle
	} else {	/* ug-client */
		char *name = NULL;
		name = strrchr(argv[0], '/');
		if (name == NULL)
			return -1;
		/* .../bin/{name} */
		ad.name = strdup(&name[1]);
	}
	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
