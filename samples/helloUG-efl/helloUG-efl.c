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

#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#include <Elementary.h>
#include <ui-gadget-module.h>
#include <dlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "helloUG-efl.h"
#define POPUP_TITLE_MAX (128)

struct ug_data *g_ugd;

#ifdef ENABLE_UG_CREATE_CB
extern int ug_create_cb(void (*create_cb)(char*,char*,char*,void*), void *user_data);
#endif

bool __helloUG_get_extra_data_cb(app_control_h service, const char *key, void * user_data);

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "UI_GADGET_HELLOUG"

struct ug_data {
	Evas_Object *base;
	Evas_Object *pu;
	ui_gadget_h ug;
	ui_gadget_h sub_ug;
	app_control_h service;
};

static int count_test = 0;
static int pool_test = 0;

#ifdef ENABLE_UG_CREATE_CB
static void create_cb(char *ug, char* mem, char* parent, void* user_data)
{
	LOGD("ug : %s, mem : %s, parent : %s, data : %p", ug, mem, parent, user_data);
}
#endif

static void __helloUG_multi_close_popup(void *data, Evas_Object *obj, void *event_info)
{
	evas_object_del(obj);
}

static Evas_Object *__helloUG_multi_show_popup(Evas_Object *win, const char *title)
{
	Evas_Object *pu;

	if (!title || !win)
		return NULL;

	pu = elm_popup_add(win);
	if (!pu)
		return NULL;

	evas_object_size_hint_weight_set(pu, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	elm_popup_timeout_set(pu, 1);
	elm_object_text_set(pu, title);
	evas_object_smart_callback_add(pu, "response", __helloUG_multi_close_popup, NULL);
	evas_object_show(pu);
	return pu;
}

static void __helloUG_multi_layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	Evas_Object *base, *parent;

	LOGD("__helloUG_multi_layout_cb");

	if (!ug || !priv)
		return;

	base = ug_get_layout(ug);
	if (!base)
		return;

	switch (mode) {
	case UG_MODE_FULLVIEW:
		parent = ug_get_parent_layout(ug);
		if (!parent)
			return;
		evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(base);
		break;
	default:
		break;
	}
}

static void __helloUG_multi_result_cb(ui_gadget_h ug, app_control_h result, void *priv)
{
	Evas_Object *win;
	struct ug_data *ugd;

	if (!ug || !priv)
		return;

	ugd = priv;
	win = ug_get_window();
	if (!win)
		return;

	ugd->pu = __helloUG_multi_show_popup(win, "multipleUG-efl");
	ug_destroy(ug);
}

static void __transition_finished(void *data, Evas_Object *obj, void *event_info)
{
	evas_object_smart_callback_del(obj, "transition,finished", __transition_finished);

	LOGD("%s : called\n", __func__);
}

static void __helloUG_multi_destroy_cb(ui_gadget_h ug, void *priv)
{
	struct ug_data *ugd;

	LOGD("%s : called\n", __func__);

	if (!ug || !priv)
		return;

	Evas_Object *con = ug_get_conformant();
	if(!con)
		LOGE("conformant is null");
	else
		LOGD("conformant(%p)", con);

    Evas_Object *ug_navi = elm_object_part_content_get(con, "elm.swallow.ug");
	if(!ug_navi)
		LOGE("ug_navi is null");

    evas_object_smart_callback_add(ug_navi, "transition,finished", __transition_finished, NULL);


	ugd = priv;
	ug_destroy(ug);

	if (ugd->sub_ug == ug)
		ugd->sub_ug = NULL;
}

static void __helloUG_show_cb(void *data, Evas_Object *obj, void *event_info)
{
	LOGD("%s : called\n", __func__);

	app_control_h svc_handle = NULL;
	app_control_create(&svc_handle);
	app_control_set_app_id(svc_handle, (char*)data);
	app_control_add_extra_data(svc_handle, "show", "y");
	app_control_send_launch_request(svc_handle, NULL, NULL);
	app_control_destroy(svc_handle);
}

static void __helloUG_pool_launch_cb(void *data, Evas_Object *obj, void *event_info)
{
	LOGD("%s : called\n", __func__);

	app_control_h svc_handle = NULL;
	app_control_create(&svc_handle);
	app_control_set_app_id(svc_handle, (char *)data);
	app_control_add_extra_data(svc_handle, "pool", "y");
	app_control_send_launch_request(svc_handle, NULL, NULL);
	app_control_destroy(svc_handle);

	LOGD("pool_test : %d", pool_test);
	
	if(pool_test == 0) {
		pool_test++;
		Evas_Object *win;
		win = ug_get_window();
		if (!win)
			return;
		__helloUG_multi_show_popup(win,"pool will be ready. launch one more");
	}
}

static void __helloUG_launch_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	LOGD("%s : called\n", __func__);

	__helloUG_get_extra_data_cb(reply, "http://samsung.com/appcontrol/data/permission_control", NULL);
}


static void __helloUG_launch_cb(void *data, Evas_Object *obj, void *event_info)
{
	LOGD("%s : called\n", __func__);

	app_control_h svc_handle = NULL;
	app_control_create(&svc_handle);
	app_control_set_app_id(svc_handle, (char*)data);
	app_control_send_launch_request(svc_handle, __helloUG_launch_reply_cb, NULL);
	app_control_destroy(svc_handle);
}

static void __helloUG_ug_create_setting_storage_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct ug_data *ugd;
	struct ug_cbs cbs = { 0, };

	LOGD("%s : called\n", __func__);

	if (!data)
		return;

	ugd = data;

	cbs.layout_cb = __helloUG_multi_layout_cb;
	cbs.result_cb = __helloUG_multi_result_cb;
	cbs.destroy_cb = __helloUG_multi_destroy_cb;
	cbs.priv = ugd;

	ugd->sub_ug = ug_create(ugd->ug, "setting-storage-efl", UG_MODE_FULLVIEW, NULL, &cbs);
}

static void __helloUG_ug_create_helloworld(void *data, Evas_Object *obj, void *event_info)
{
	struct ug_data *ugd;
	struct ug_cbs cbs = { 0, };

	LOGD("%s : called\n", __func__);

	if (!data)
		return;

	ugd = data;

	cbs.layout_cb = __helloUG_multi_layout_cb;
	cbs.result_cb = __helloUG_multi_result_cb;
	cbs.destroy_cb = __helloUG_multi_destroy_cb;
	cbs.priv = ugd;

	ugd->sub_ug = ug_create(ugd->ug, "com.samsung.helloworld", UG_MODE_FULLVIEW, NULL, &cbs);
}

static void __helloUG_helloworldug_load_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct ug_data *ugd;
	struct ug_cbs cbs = { 0, };
	//const char* ug_name = "helloUG-efl";
	const char* ug_name = "com.samsung.helloworld";

	LOGD("%s : called\n", __func__);

	if (!data)
		return;

	ugd = data;

	cbs.layout_cb = __helloUG_multi_layout_cb;
	cbs.result_cb = __helloUG_multi_result_cb;
	cbs.destroy_cb = __helloUG_multi_destroy_cb;
	cbs.priv = ugd;

	if(count_test % 2) {
		LOGD("%s : called / FULLVIEW \n", __func__);

#ifdef ENABLE_UG_CREATE_CB
		ug_create_cb(NULL, NULL);
#endif

		ugd->sub_ug = ug_create(ugd->ug, ug_name,
				UG_MODE_FULLVIEW, NULL, &cbs);
	} else {
		LOGD("%s : called / FRAMEVIEW \n", __func__);

#ifdef ENABLE_UG_CREATE_CB
		ug_create_cb(create_cb, NULL);
#endif
		ugd->sub_ug = ug_create(ugd->ug, ug_name,
				UG_MODE_FRAMEVIEW, NULL, &cbs);
	}
}

static void __helloUG_helloug_load_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct ug_cbs cbs = { 0, };
	//const char* ug_name = "com.samsung.helloworld";
	int mode;

	LOGD("%s : called\n", __func__);

	if (!data)
		return;

	cbs.layout_cb = __helloUG_multi_layout_cb;
	cbs.result_cb = __helloUG_multi_result_cb;
	cbs.destroy_cb = __helloUG_multi_destroy_cb;
	cbs.priv = g_ugd;

	if(count_test % 2) {
		mode = UG_MODE_FULLVIEW;
		LOGD("%s : called / FULLVIEW \n", __func__);
	} else {
		mode = UG_MODE_FRAMEVIEW;
		LOGD("%s : called / FRAMEVIEW \n", __func__);
	}

	ug_create_cb(NULL, NULL);

	//ug_create(NULL, data, UG_MODE_FULLVIEW, NULL, &cbs);
	ug_create(g_ugd->ug, data, UG_MODE_FULLVIEW, NULL, &cbs);	
}

static void __helloUG_result_cb(void *data, Evas_Object *obj, void *event_info)
{
	app_control_h result;
	struct ug_data *ugd;
	int ret;

	if (!data)
		return;

	ugd = data;

	LOGD("__helloUG_result_cb");

	ret = app_control_create(&result);

	app_control_add_extra_data(result, "name", "hello-UG");
	app_control_add_extra_data(result, "description", "sample UI gadget");
	app_control_add_extra_data(result, "__UG_SEND_REUSLT__", APP_CONTROL_RESULT_SUCCEEDED);
	app_control_add_extra_data(result, "http://samsung.com/appcontrol/data/permission_control", "no");
	//ug_send_result_full(ugd->ug, result, APP_CONTROL_RESULT_SUCCEEDED);

	ret = app_control_reply_to_launch_request(result, ugd->service, (app_control_result_e)APP_CONTROL_RESULT_SUCCEEDED);
	if (ret != APP_CONTROL_ERROR_NONE)
		LOGE("app_control_reply_to_launch_request failed, %d", ret);

	app_control_destroy(result);
}

static void __helloUG_overlap_cb(void *data, Evas_Object *obj, void *event_info)
{
	/* indicator test */
	Evas_Object *conform = NULL;
	conform = (Evas_Object *)ug_get_conformant();

	LOGD("conform pointer(%p)\n", conform);

	if(count_test % 2) {
		LOGD("%s : overlap\n", __func__);
		elm_object_signal_emit(conform, "elm,state,indicator,overlap", "");
	} else {
		LOGD("%s : nooverlap\n", __func__);
		elm_object_signal_emit(conform, "elm,state,indicator,nooverlap", "");
	}
}

static void __helloUG_hide_cb(void *data, Evas_Object *obj, void *event_info)
{
	LOGD("helloUG window lower");
	elm_win_lower(ug_get_window());
}

static void __helloUG_exit_cb(void *data, Evas_Object *obj, void *event_info)
{
	elm_exit();
}


static void __helloUG_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	struct ug_data *ugd;
	int i;

	LOGD("%s : called\n", __func__);

	if (!data)
		return;

	ugd = data;
	for (i=0 ; i<3 ; i++) {
		ug_destroy_me(ugd->ug);
	}
}

static Evas_Object *__helloUG_create_content(Evas_Object *parent, struct ug_data *ugd)
{
	Evas_Object *list;
	char lable[124] = {0,};

	/* List */
	list = elm_list_add(parent);
	elm_list_mode_set(list, ELM_LIST_COMPRESS);

	/* Main Menu Items Here */
	sprintf(lable,"Hello UI Gadget / PID : %d", getpid());
	elm_list_item_append(list, lable, NULL, NULL, NULL, NULL);
	memset(lable,0x00,124);

	/* hello UG launched by ug_create */
#if 0
	elm_list_item_append(list, "Create shared helloUG2", NULL, NULL, __helloUG_helloug_load_cb, "helloUG2-efl");
	elm_list_item_append(list, "Create priv helloug(appid)", NULL, NULL, __helloUG_helloug_load_cb, "helloUG-efl");
	elm_list_item_append(list, "Create priv helloug(appid)", NULL, NULL, __helloUG_helloug_load_cb, "com.samsung.helloUG-efl");
	elm_list_item_append(list, "Create shared wifi", NULL, NULL, __helloUG_helloug_load_cb, "wifi-efl-UG");
#endif

	elm_list_item_append(list, "Launch Hello UGAPP", NULL, NULL, __helloUG_launch_cb, "com.samsung.hello");
	elm_list_item_append(list, "Launch Hello UGAPP(POOL)", NULL, NULL, __helloUG_launch_cb, "com.samsung.helloworld");
	elm_list_item_append(list, "Launch Hello UGAPP(UG POOL)", NULL, NULL, __helloUG_launch_cb, "com.samsung.helloUG-efl");

	//elm_list_item_append(list, "Launch Storage UGAPP", NULL, NULL, __helloUG_launch_cb, "com.samsung.set-storage");
	//elm_list_item_append(list, "Launch Storage UGAPP(POOL)", NULL, NULL, __helloUG_launch_cb, "com.samsung.set-storage-pool");
	//elm_list_item_append(list, "Launch Storage UGAPP(UG POOL)", NULL, NULL, __helloUG_launch_cb, "setting-storage-efl");
	elm_list_item_append(list, "Launch Setting UGAPP", NULL, NULL, __helloUG_launch_cb, "com.samsung.set-time");
	elm_list_item_append(list, "Launch Setting UGAPP", NULL, NULL, __helloUG_launch_cb, "com.samsung.set-time-pool");
	elm_list_item_append(list, "Launch Setting UGAPP", NULL, NULL, __helloUG_launch_cb, "setting-time-efl");

#if 0
	//elm_list_item_append(list, "Create Hello UG", NULL, NULL, __helloUG_helloug_load_cb, ugd);
	//elm_list_item_append(list, "Create HelloWorld APP UG", NULL, NULL, __helloUG_helloug_load_cb, ugd);
	elm_list_item_append(list, "Launch Hello UGAPP(PROCESS POOL)", NULL, NULL, __helloUG_launch_cb, "com.samsung.helloworld");
	//elm_list_item_append(list, "Launch Hello UGAPP(UG-CLIENT POOL)", NULL, NULL, __helloUG_pool_launch_cb, "com.samsung.helloworld");
	elm_list_item_append(list, "Show UG APP", NULL, NULL, __helloUG_show_cb, "com.samsung.helloworld");

	elm_list_item_append(list, "Launch WIFI UGAPP(PROCESS POOL)", NULL, NULL, __helloUG_launch_cb, "wifi-efl-ug-lite");
	//elm_list_item_append(list, "Launch WIFI UGAPP(UG-CLIENT POOL)", NULL, NULL, __helloUG_pool_launch_cb, "wifi-efl-ug-lite");
	elm_list_item_append(list, "Show WIFI APP", NULL, NULL, __helloUG_show_cb, "wifi-efl-ug-lite");
	//elm_list_item_append(list, "UG Create setting stroage", NULL, NULL, __helloUG_ug_create_setting_storage_cb, ugd);


	elm_list_item_append(list, "UG Create by self launch request", NULL, NULL, __helloUG_ug_create_helloworld, ugd);
	elm_list_item_append(list, "UG Send result", NULL, NULL, __helloUG_result_cb, ugd);
	elm_list_item_append(list, "overlap test", NULL, NULL, __helloUG_overlap_cb, ugd);
#endif
	
	elm_list_item_append(list, "Back", NULL, NULL, __helloUG_back_cb, ugd);
	elm_list_item_append(list, "Hide", NULL, NULL, __helloUG_hide_cb, ugd);
	elm_list_item_append(list, "Exit", NULL, NULL, __helloUG_exit_cb, ugd);

	elm_list_go(list);
	
#if 0
	/* permission test */
	FILE *file;
	if ((file = fopen("/opt/usr/apps/com.samsung.helloworld/data/ui-gadget_doc.h", "rw"))) {
		LOGD("helloworld file open success");
	} else {
		LOGE("\x1b[31m helloworld file open fail (%d) \x1b[0m", errno);
	}
#endif	

	return list;
}

bool __helloUG_get_extra_data_cb(app_control_h service, const char *key, void * user_data)
{
	char *value;
	int ret;

	ret = app_control_get_extra_data(service, key, &value);
	if (ret) {
		LOGE("__get_extra_data_cb: error get data(%d)\n", ret);
		return false;
	}

	LOGD("extra data : %s, %s\n", key, value);
	free(value);

	return true;
}

static Evas_Object *__helloUG_create_fullview(Evas_Object *parent, struct ug_data *ugd)
{
	Evas_Object *base;

	base = elm_layout_add(parent);
	if (!base)
		return NULL;
	evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_theme_set(base, "layout", "application", "default");
	edje_object_signal_emit(_EDJ(base), "elm,state,show,indicator", "elm");
	edje_object_signal_emit(_EDJ(base), "elm,state,show,content", "elm");

	return base;
}

static Evas_Object *__helloUG_create_frameview(Evas_Object *parent, struct ug_data *ugd)
{
	Evas_Object *base;

	base = elm_layout_add(parent);
	if (!base)
		return NULL;
	elm_layout_theme_set(base, "standard", "window", "integration");
	edje_object_signal_emit(_EDJ(base), "elm,state,show,content", "elm");

	return base;
}

static void *__helloUG_on_create(ui_gadget_h ug, enum ug_mode mode, app_control_h service,
		       void *priv)
{
	Evas_Object *parent;
	Evas_Object *content;
	struct ug_data *ugd;
	char *operation = NULL;

	if (!ug || !priv)
		return NULL;

	bindtextdomain("helloUG-efl", "/opt/ug/res/locale");

	ugd = priv;
	ugd->ug = ug;
	ugd->service = service;

	g_ugd = ugd;

	app_control_get_operation(service, &operation);

	if (operation == NULL) {
		/* ug called by ug_create */
		LOGD("ug called by ug_create\n");
	} else {
		/* ug called by service request */
		LOGD("ug called by service request :%s\n", operation);
		free(operation);
	}

	app_control_foreach_extra_data(service, __helloUG_get_extra_data_cb, NULL);

	parent = ug_get_parent_layout(ug);
	if (!parent)
		return NULL;

	if (mode == UG_MODE_FULLVIEW)
		ugd->base = __helloUG_create_fullview(parent, ugd);
	else
		ugd->base = __helloUG_create_frameview(parent, ugd);

	evas_object_color_set(ugd->base, 255, 255, 255, 255);
	evas_object_color_set(elm_bg_add(ugd->base), 255, 255, 255, 255);

	if (ugd->base) {
		content = __helloUG_create_content(parent, ugd);
		elm_object_part_content_set(ugd->base, "elm.swallow.content", content);
		evas_object_color_set(content, 255, 255, 255, 255);
	}

	return ugd->base;
}

static void __helloUG_on_start(ui_gadget_h ug, app_control_h service, void *priv)
{
#if 0
	int i;
	pthread_t p_thread[10];
	int thr_id;
	int status;
	int a = 1;

	for (i=0; i<10; i++) {
		thr_id = pthread_create(&p_thread[i], NULL, __helloUG_start_t_func, (void*)&a);
		if (thr_id < 0) {
			perror("thread create error: ");
			exit(0);
		}
		pthread_detach(p_thread[i]);
	}
#endif
}

static void __helloUG_on_pause(ui_gadget_h ug, app_control_h service, void *priv)
{

}

static void __helloUG_on_resume(ui_gadget_h ug, app_control_h service, void *priv)
{
	LOGD("%s : called\n", __func__);
}

static void __helloUG_on_destroy(ui_gadget_h ug, app_control_h service, void *priv)
{
	struct ug_data *ugd;
	LOGD("%s : called\n", __func__);
	if (!ug || !priv)
		return;

	ugd = priv;
	evas_object_del(ugd->pu);
	evas_object_del(ugd->base);
	ugd->base = NULL;
}

static void __helloUG_on_message(ui_gadget_h ug, app_control_h msg, app_control_h service,
		      void *priv)
{
}

static void __helloUG_on_event(ui_gadget_h ug, enum ug_event event, app_control_h service,
		    void *priv)
{
	int degree = -1;

	struct ug_data *ugd = priv;

	switch (event) {
	case UG_EVENT_LOW_MEMORY:
		break;
	case UG_EVENT_LOW_BATTERY:
		break;
	case UG_EVENT_LANG_CHANGE:
		break;
	case UG_EVENT_ROTATE_PORTRAIT:
		elm_layout_theme_set(ugd->base, "layout", "application", "default");
		degree = 0;
		break;
	case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
		elm_layout_theme_set(ugd->base, "layout", "application", "default");
		degree = 180;
		break;
	case UG_EVENT_ROTATE_LANDSCAPE:
		elm_layout_theme_set(ugd->base, "layout", "application", "noindicator");
		degree = 270;
		break;
	case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		elm_layout_theme_set(ugd->base, "layout", "application", "noindicator");
		degree = 90;
		break;
	case UG_EVENT_REGION_CHANGE:
		break;
	default:
		break;
	}

	if (degree > -1)
		elm_win_rotation_with_resize_set(ug_get_window(), degree);
}

static void __helloUG_on_key_event(ui_gadget_h ug, enum ug_key_event event,
			 app_control_h service, void *priv)
{
	if (!ug)
		return;

	switch (event) {
	case UG_KEY_EVENT_END:
		ug_destroy_me(ug);
		break;
	default:
		break;
	}
}

static void __helloUG_on_destroying(ui_gadget_h ug, app_control_h service, void *priv)
{
	LOGD("%s : called\n", __func__);
}

UG_MODULE_API int UG_MODULE_INIT(struct ug_module_ops *ops)
{
	struct ug_data *ugd;

	if (!ops)
		return -1;

	ugd = calloc(1, sizeof(struct ug_data));
	if (!ugd)
		return -1;

	ops->create = __helloUG_on_create;
	ops->start = __helloUG_on_start;
	ops->pause = __helloUG_on_pause;
	ops->resume = __helloUG_on_resume;
	ops->destroy = __helloUG_on_destroy;
	ops->message = __helloUG_on_message;
	ops->event = __helloUG_on_event;
	ops->key_event = __helloUG_on_key_event;
	ops->destroying = __helloUG_on_destroying;
	ops->priv = ugd;

	count_test++;

	if(count_test % 2) {
		ops->opt = UG_OPT_INDICATOR_PORTRAIT_ONLY;
		LOGD("%s : overlap UG OPT(%d) SET\n", __func__, (int)ops->opt);
	} else {
		ops->opt = UG_OPT_INDICATOR_PORTRAIT_ONLY;
		LOGD("%s : no overlap UG OPT(%d) SET\n", __func__, (int)ops->opt);
	}

	return 0;
}

UG_MODULE_API void UG_MODULE_EXIT(struct ug_module_ops *ops)
{
	struct ug_data *ugd;

	if (!ops)
		return;

	ugd = ops->priv;
	if (ugd)
		free(ugd);
}
