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
#include <appcore-efl.h>
#include <ui-gadget.h>
#include <Ecore_X.h>
#include <dlog.h>
#include <aul.h>
#include <appsvc.h>
#include <runtime_info.h>

#include "ug-client.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "UGClient"

static void prt_usage(const char *cmd)
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

static void win_del(void *data, Evas_Object *obj, void *event)
{
	elm_exit();
}

static void main_quit_cb(void *data, Evas_Object *obj,
			 const char *emission, const char *source)
{
	elm_exit();
}

static int rotate(enum appcore_rm m, void *data)
{
	struct appdata *ad = data;
	int r;
	bool is_rotation_lock = false;

	if (ad == NULL || ad->win == NULL)
		return 0;

	/* rotation lock */
	r = runtime_info_get_value_bool(RUNTIME_INFO_KEY_ROTATION_LOCK_ENABLED, &is_rotation_lock);
	if ( !r && is_rotation_lock)
		m = APPCORE_RM_PORTRAIT_NORMAL;

	switch (m) {
	case APPCORE_RM_PORTRAIT_NORMAL:
		ug_send_event(UG_EVENT_ROTATE_PORTRAIT);
		r = 0;
		break;
	case APPCORE_RM_PORTRAIT_REVERSE:
		r = 180;
		ug_send_event(UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN);
		break;
	case APPCORE_RM_LANDSCAPE_NORMAL:
		ug_send_event(UG_EVENT_ROTATE_LANDSCAPE);
		r = 270;
		break;
	case APPCORE_RM_LANDSCAPE_REVERSE:
		ug_send_event(UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN);
		r = 90;
		break;
	default:
		r = -1;
		break;
	}

	return 0;
}

void layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	struct appdata *ad;
	Evas_Object *base;

	if (!ug || !priv)
		return;

	ad = priv;

	base = ug_get_layout(ug);
	if (!base)
		return;

	switch (mode) {
	case UG_MODE_FULLVIEW:
		evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
		elm_win_resize_object_add(ad->win, base);
		ug_disable_effect(ug);
		evas_object_show(base);
		break;
	case UG_MODE_FRAMEVIEW:
		elm_object_part_content_set(ad->ly_main, "content", base);
		break;
	default:
		break;
	}
}

void result_cb(ui_gadget_h ug, service_h result, void *priv)
{
	struct appdata *ad;
	int ret;

	if (!ug || !priv)
		return;
	ad = priv;

	ret = service_reply_to_launch_request(result, ad->request, SERVICE_RESULT_SUCCEEDED);
	if (ret != SERVICE_ERROR_NONE)
		LOGE("service_reply_to_launch_request failed, %d\n", ret);
}

void destroy_cb(ui_gadget_h ug, void *priv)
{
	if (!ug)
		return;

	ug_destroy(ug);
	elm_exit();
}

static Evas_Object *create_win(const char *name)
{
	Evas_Object *eo;
	int w, h;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		evas_object_smart_callback_add(eo, "delete,request",
					       win_del, NULL);
		ecore_x_window_size_get(ecore_x_window_root_first_get(),
					&w, &h);
		evas_object_resize(eo, w, h);
	}

	return eo;
}

static Evas_Object *load_edj(Evas_Object *parent, const char *file,
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

static int low_memory(void *data)
{
	return ug_send_event(UG_EVENT_LOW_MEMORY);
}

static int low_battery(void *data)
{
	return ug_send_event(UG_EVENT_LOW_BATTERY);
}

static int lang_changed(void *data)
{
	return ug_send_event(UG_EVENT_LANG_CHANGE);
}

static int app_create(void *data)
{
	struct appdata *ad = data;
	enum appcore_rm rm;
	Evas_Object *win;
	Evas_Object *ly;

	/* create window */
	win = create_win(PACKAGE);
	if (win == NULL)
		return -1;
	ad->win = win;
	UG_INIT_EFL(ad->win, UG_OPT_INDICATOR_ENABLE);

	/* load edje */
	ly = load_edj(win, EDJ_FILE, GRP_MAIN);
	if (ly == NULL)
		return -1;
	elm_win_resize_object_add(win, ly);
	edje_object_signal_callback_add(elm_layout_edje_get(ly),
					"EXIT", "*", main_quit_cb, NULL);
	ad->ly_main = ly;

	lang_changed(ad);

	appcore_set_rotation_cb(rotate, ad);
	appcore_set_event_callback(APPCORE_EVENT_LOW_MEMORY, low_memory, ad);
	appcore_set_event_callback(APPCORE_EVENT_LOW_BATTERY, low_battery, ad);
	appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE, lang_changed, ad);

	if (appcore_get_rotation_state(&rm) == 0)
		rotate(rm, ad);

	return 0;
}

static int app_terminate(void *data)
{
	struct appdata *ad = data;

	ug_destroy_all();
	if (ad->ly_main)
		evas_object_del(ad->ly_main);

	if (ad->win)
		evas_object_del(ad->win);

	return 0;
}

static int app_pause(void *data)
{
	ug_pause();
	return 0;
}

static int app_resume(void *data)
{
	ug_resume();
	return 0;
}

static int svc_cb(void *data)
{
	LOGD("svc_cb called\n");
	return 0;
}

extern int service_create_event(bundle *data, service_h *service);
extern int appsvc_request_transient_app(bundle *b, Ecore_X_Window callee_id, appsvc_host_res_fn cbfunc, void *data);

static int app_reset(bundle *b, void *data)
{
	struct appdata *ad = data;
	struct ug_cbs cbs = { 0, };
	service_h service;
	enum ug_mode mode = UG_MODE_FULLVIEW;

	Ecore_X_Window id2 = elm_win_xwindow_get(ad->win);
	appsvc_request_transient_app(b, id2, svc_cb, "svc test");

	if (ad->win) {
		elm_win_activate(ad->win);
		evas_object_show(ad->win);
	}

	if (ad->data)	/* ug-launcher */
		service_create_event(ad->data, &service);
	else
		service_create_event(b, &service);

	service_clone(&ad->request, service);

	cbs.layout_cb = layout_cb;
	cbs.destroy_cb = destroy_cb;
	cbs.result_cb = result_cb;
	cbs.priv = ad;

	ad->ug = ug_create(NULL, ad->name, mode, service, &cbs);
	if (ad->ug == NULL) {
		LOGE("ug_create fail: %s\n", ad->name);
		elm_exit();
	}

	return 0;
}

static int update_argument(const char *optarg, struct appdata *ad)
{
	const char *key;
	const char *val;
	key = strtok((char *)optarg, ",");
	if (!key)
		return -1;

	val = optarg + strlen(key) + 1;
	if (!val)
		return -1;

	if (!ad->data)
		ad->data = bundle_create();
	if (!ad->data)
		return -1;
	bundle_add(ad->data, key, val);
	return 0;
}

int main(int argc, char *argv[])
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

	setenv("ELM_ENGINE", "gl", 1); //enabling the OpenGL as the backend of the EFL.

	memset(&ad, 0x0, sizeof(struct appdata));
	ops.data = &ad;

	cmdlen = strlen(argv[0]);
	if (strncmp(argv[0], "ug-launcher", cmdlen) == 0
		|| strncmp(argv[0], "/usr/bin/ug-launcher", cmdlen) == 0) {
		while ((opt = getopt(argc, argv, "n:d:")) != -1) {
			switch (opt) {
			case 'n':
				if (optarg)
					ad.name = strdup(optarg);
				break;
			case 'd':
				if (update_argument(optarg, &ad)) {
					if (ad.data)
						bundle_free(ad.data);
					prt_usage(argv[0]);
					return -1;
				}
				break;
			default:
				prt_usage(argv[0]);
				return -1;
			}
		}

		if (!ad.name) {
			prt_usage(argv[0]);
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
