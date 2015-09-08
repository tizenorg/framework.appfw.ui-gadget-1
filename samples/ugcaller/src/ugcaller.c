/*
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
#include <stdbool.h>

#include <Elementary.h>
#include <app.h>

#include <Evas.h>
#include <Ecore_X.h>

#include <ui-gadget-module.h>
#include <ui-gadget.h>

#include "ugcaller.h"

void _orient_changed(app_device_orientation_e orientation, void *user_data);

static void _win_del(void *data, Evas_Object *obj, void *event_info)
{
	elm_exit();
}

static Evas_Object* _create_win(const char *name)
{
	Evas_Object *eo;
	int w, h;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (eo)
	{
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		evas_object_smart_callback_add(eo, "delete,request", _win_del, NULL);
		ecore_x_window_size_get(ecore_x_window_root_first_get(),
				&w, &h);
		evas_object_resize(eo, w, h);
	}

	return eo;
}

void _ug_layout_cb(ui_gadget_h ug, enum ug_mode mode, void* priv)
{
	LOGD("ug Layout CB. UG=0x%08x Priv=0x%08x Mode=%d", ug, priv, mode);

	Evas_Object *base;

	if (!ug || !priv)
	{
		LOGE("Abnormal value. UG=0x%08x Priv=0x%08x Mode=%d", ug, priv, mode);
		return;
	}

	appdata *ap = (appdata *)priv;

	base = ug_get_layout(ug);
	if (!base)
	{
		LOGD( "ug_get_layout is fail");
		return;
	}
	ap->ug_layout = base;

	switch (mode)
	{
		case UG_MODE_FULLVIEW:
			evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			ug_disable_effect(ug);
			evas_object_show(base);
			break;
		default:
			LOGD("Unknow UG mode : %d", mode);
			break;
	}

}

void _ug_result_cb( ui_gadget_h ug, app_control_h result, void *priv)
{
	LOGD("UG Result CB.");

	if (!ug || !priv)
	{
		LOGE("Abnormal value. UG=0x%08x Priv=0x%08x", ug, priv);
		return;
	}

	appdata *ap = (appdata *)priv;

	int ret = -1;
	app_control_h reply = NULL;
	ret = app_control_create(&reply);
	if (ret != APP_CONTROL_ERROR_NONE)
	{
		LOGE("app_control_create failed! ");
		return;
	}

	LOGD("** UG Result **");
	app_control_clone(&reply, result);

	ret = app_control_reply_to_launch_request(reply, ap->app_control_handle, APP_CONTROL_RESULT_SUCCEEDED);
	if (ret != APP_CONTROL_ERROR_NONE)
	{
		LOGE("app_control_reply_to_launch_request failed!");
	}

	app_control_destroy(reply);
}

void _ug_destroy_cb( ui_gadget_h ug, void *priv)
{
	LOGD("UG Destroyed..");

	if (!ug || !priv)
	{
		LOGE("Abnormal value. UG=0x%08x Priv=0x%08x", ug, priv);
		return;
	}

	elm_exit();		// will tirgger app_terminated
}

static void load_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata *apd = (appdata *)data;
	struct ug_cbs cbs = { 0, };

	if (!data)
		return;

	cbs.layout_cb = _ug_layout_cb;
	cbs.result_cb = _ug_result_cb;
	cbs.destroy_cb = _ug_destroy_cb;
	cbs.priv = apd;

	if (apd->ug_name == NULL) {
		LOGE("no ug name");
		return;
	}
	apd->sub_ug = ug_create(apd->ug, apd->ug_name,
				UG_MODE_FULLVIEW, NULL, &cbs);
}

static void result_cb(void *data, Evas_Object *obj, void *event_info)
{
	app_control_h result;
	appdata *apd = (appdata *)data;
	int ret;

	ret = app_control_create(&result);

	app_control_add_extra_data(result, "name", "hello-UG");
	app_control_add_extra_data(result, "description", "sample UI gadget");

	ug_send_result(apd->ug, result);

	app_control_destroy(result);
}

static void back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata *apd = (appdata *)data;
	int i;

	for (i=0 ; i<3 ; i++) {
		ug_destroy_me(apd->ug);
	}
}

static void main_quit_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	elm_exit();
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

static bool _app_create(void *user_data)
{
	LOGD("App Create");

	appdata *apd = (appdata *) user_data;
	Evas_Object *ly;
	Evas_Object *win = _create_win(PACKAGE);

	if (win == NULL) {
		LOGE("Cannot create app. pkg=%s", PACKAGE);
		return false;
	}

	apd->win_main = win;

	UG_INIT_EFL(apd->win_main, UG_OPT_INDICATOR_ENABLE);	// Init UG module

	ly = load_edj(win, EDJ_FILE, GRP_MAIN);
	if (ly == NULL) {
		LOGE("Cannot load edj");
		return false;
	}

	elm_win_resize_object_add(win, ly);
	edje_object_signal_callback_add(elm_layout_edje_get(ly), "EXIT", "*", main_quit_cb, NULL);
	apd->ly_main = ly;
	evas_object_show(ly);

	_orient_changed(app_get_device_orientation(), user_data);		// Set initial degree

	evas_object_show(win);

	return true;
}

static Evas_Object *create_fullview(Evas_Object *parent)
{
	Evas_Object *base;

	base = elm_layout_add(parent);
	if (!base)
		return NULL;
	elm_layout_theme_set(base, "layout", "application", "default");
	edje_object_signal_emit(_EDJ(base), "elm,state,show,indicator", "elm");
	edje_object_signal_emit(_EDJ(base), "elm,state,show,content", "elm");

	return base;
}

static Evas_Object *create_content(Evas_Object *parent, appdata *apd)
{
	Evas_Object *bx, *eo;

	bx = elm_box_add(parent);

	eo = elm_label_add(parent);
	elm_object_text_set(eo, _("Hello UG Caller"));
	evas_object_size_hint_align_set(eo, 0.5, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND,
					EVAS_HINT_EXPAND);
	evas_object_show(eo);
	elm_box_pack_end(bx, eo);

	eo = elm_button_add(parent);
	elm_object_text_set(eo, _("Load UG"));
	evas_object_size_hint_align_set(eo, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(eo, "clicked", load_cb, apd);
	elm_object_style_set(eo, "bottom_btn");

	evas_object_show(eo);
	elm_box_pack_end(bx, eo);

	eo = elm_button_add(parent);
	elm_object_text_set(eo, _("Send result"));
	evas_object_size_hint_align_set(eo, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(eo, "clicked", result_cb, apd);
	elm_object_style_set(eo, "bottom_btn");

	evas_object_show(eo);
	elm_box_pack_end(bx, eo);

	eo = elm_button_add(parent);
	elm_object_text_set(eo, _("Back"));
	evas_object_size_hint_align_set(eo, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_smart_callback_add(eo, "clicked", back_cb, apd);
	elm_object_style_set(eo, "bottom_btn");

	evas_object_show(eo);
	elm_box_pack_end(bx, eo);

	return bx;
}

static void _app_service(app_control_h service, void *user_data)
{
	appdata *apd = (appdata *)user_data;
	Evas_Object *content;
	LOGD("%s", __func__);

	apd->base = create_fullview(apd->win_main);

	content = create_content(apd->win_main, apd);
	elm_object_part_content_set(apd->base, "elm.swallow.content", content);

	evas_object_size_hint_weight_set(apd->base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(apd->base);
}


void _app_pause(void *user_data)
{
	LOGD("%s", __func__);

	ug_pause();
}

void _app_resume(void *user_data)
{
	LOGD("%s", __func__);

	ug_resume();
}

void _app_terminate(void *user_data)
{
	LOGD("%s", __func__);

	appdata *ap = (appdata *)user_data;

	if(ap->app_control_handle) {
		app_control_destroy(ap->app_control_handle);
		ap->app_control_handle = NULL;
	}

	if(ap->ug)
	{
		ug_destroy(ap->ug);
		ap->ug = NULL;
	}

	if(ap->ug_layout)
	{
		evas_object_del(ap->ug_layout);
		ap->ug_layout = NULL;
	}

	LOGD("%s terminated.", __func__);
}

void _region_changed(void *user_data)
{
	LOGD("%s", __func__);
}

void _low_battery(void *user_data)
{
	LOGD("%s", __func__);
	ug_send_event( UG_EVENT_LOW_BATTERY );
}

void _low_memory(void *user_data)
{
	LOGD("%s", __func__);
	ug_send_event( UG_EVENT_LOW_MEMORY );

}

void _lang_changed(void *user_data)
{
	LOGD("%s", __func__);
	ug_send_event( UG_EVENT_LANG_CHANGE );

}

void _orient_changed(app_device_orientation_e orientation, void *user_data)
{
	LOGD("%s", __func__);
	appdata *ap = (appdata *)user_data;

	int degree = 0;
	enum ug_event evt = UG_EVENT_NONE;

	switch(orientation)
	{
	case APP_DEVICE_ORIENTATION_0:
		degree = 0;
		evt = UG_EVENT_ROTATE_PORTRAIT;
		break;
	case APP_DEVICE_ORIENTATION_90:
		degree = 90;
		evt = UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN;
		break;

	case APP_DEVICE_ORIENTATION_180:
		degree = 180;
		evt = UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN;
		break;

	case APP_DEVICE_ORIENTATION_270:
		evt = UG_EVENT_ROTATE_LANDSCAPE;
		degree = 270;
		break;
	}

	ug_send_event(evt);

	if (ap->win_main != NULL)	{
		elm_win_rotation_with_resize_set(ap->win_main, degree);
	}
}

int main(int argc, char *argv[])
{
	setenv("ELM_ENGINE", "gl", 1); //enabling the OpenGL as the backend of the EFL.
	setenv("PKG_NAME", "ugcaller", 1);
	appdata ad = {0,};

	app_event_callback_s event_callback = {0,};

	event_callback.create = _app_create;
	event_callback.terminate = _app_terminate;
	event_callback.pause = _app_pause;
	event_callback.resume = _app_resume;
	event_callback.app_control = _app_service;

	event_callback.low_memory = _low_memory;
	event_callback.low_battery = _low_battery;
	event_callback.device_orientation = _orient_changed;
	event_callback.language_changed = _lang_changed;
	event_callback.region_format_changed = NULL;

	if ( argc == 2 )		// For command line options.
	{
		LOGD("Parsing from cmd line. ug name=%s", argv[1]);
		ad.ug_name = strdup(argv[1]);
	}

	int ret = APP_ERROR_NONE;

	ret = app_efl_main(&argc, &argv, &event_callback, &ad);

	if ( ret != APP_ERROR_NONE )
	{
		LOGE("app_efl_main() is failed. err=%d", ret);
	}

	return ret;

}

