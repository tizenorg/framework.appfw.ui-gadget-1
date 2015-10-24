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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>


#include "ug.h"
#include "ug-module.h"
#include "ug-manager.h"
#include "ug-dbg.h"

#include <Ecore_X.h>
#include <Elementary.h>
#include <aul.h>
#include <pkgmgr-info.h>

#ifndef UG_API
#define UG_API __attribute__ ((visibility("default")))
#endif

UG_API ui_gadget_h ug_create(ui_gadget_h parent,
				   const char *name,
				   enum ug_mode mode,
				   app_control_h app_control, struct ug_cbs *cbs)
{
	if (!name) {
		_ERR("ug_create() failed: Invalid name");
		errno = EINVAL;
		return NULL;
	}

	if (mode < UG_MODE_FULLVIEW || mode >= UG_MODE_INVALID) {
		_ERR("ug_create() failed: Invalid mode");
		errno = EINVAL;
		return NULL;
	}

	SECURE_LOGD("name : %s", name);

	return ugman_ug_load(parent, name, mode, app_control, cbs);
}

UG_API int UG_INIT_EFL(void *win, enum ug_option opt)
{
	return ug_init((Display *)ecore_x_display_get(),
		elm_win_xwindow_get(win), win, opt);
}

UG_API int ug_init(void *disp, unsigned long xid, void *win, enum ug_option opt)
{
	if (!win || !xid || !disp) {
		_ERR("ug_init() failed: Invalid arguments");
		return -1;
	}

	if (opt < UG_OPT_INDICATOR_ENABLE || opt >= UG_OPT_MAX) {
		_ERR("ug_init() failed: Invalid option");
		return -1;
	}

	return ugman_init((Display *)disp, (Window)xid, win, opt);
}

UG_API int ug_pause(void)
{
	return ugman_pause();
}

UG_API int ug_pause_ug(ui_gadget_h ug)
{
	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_pause_ug() failed: Invalid ug");
		errno = EINVAL;
		return -1;
	}

	return ugman_pause_ug(ug);
}

UG_API int ug_resume(void)
{
	return ugman_resume();
}

UG_API int ug_resume_ug(ui_gadget_h ug)
{
	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_resume_ug() failed: Invalid ug");
		errno = EINVAL;
		return -1;
	}

	return ugman_resume_ug(ug);
}

UG_API int ug_destroy(ui_gadget_h ug)
{
	return ugman_ug_del(ug);
}

UG_API int ug_destroy_all(void)
{
	return ugman_ug_del_all();
}

UG_API int ug_destroy_me(ui_gadget_h ug)
{
	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_destroy_me() failed: Invalid ug");
		errno = EINVAL;
		return -1;
	}

	switch (ug->state)
	{
		case UG_STATE_CREATED:
		case UG_STATE_RUNNING:
		case UG_STATE_STOPPED:
			break;
		default:
		{
			_WRN("ug_destory_me() failed:ug(%p) status(%d) error for ug_destory_me",
				ug, ug->state);
			return -1;
		}
	}

	if (!ug->cbs.destroy_cb) {
		_WRN("ug_destroy_me() failed: destroy callback does not exist");
		return -1;
	}

	ug->cbs.destroy_cb(ug, ug->cbs.priv);
	return 0;
}

UG_API void *ug_get_layout(ui_gadget_h ug)
{
	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_get_layout() failed: Invalid ug");
		errno = EINVAL;
		return NULL;
	}
	return ug->layout;
}

UG_API void *ug_get_parent_layout(ui_gadget_h ug)
{
	ui_gadget_h parent;
	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_get_parent_layout() failed: Invalid ug");
		errno = EINVAL;
		return NULL;
	}

	parent = ug->parent;

	if (parent)
		return parent->layout;
	return NULL;
}

UG_API enum ug_mode ug_get_mode(ui_gadget_h ug)
{
	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_get_mode() failed: Invalid ug");
		errno = EINVAL;
		return UG_MODE_INVALID;
	}

	return ug->mode;
}

UG_API void *ug_get_window(void)
{
	return ugman_get_window();
}

UG_API void *ug_get_conformant(void)
{
	return ugman_get_conformant();
}

UG_API int ug_send_event(enum ug_event event)
{
	if (event <= UG_EVENT_NONE || event >= UG_EVENT_MAX) {
		_ERR("ug_send_event() failed: Invalid event");
		return -1;
	}

	return ugman_send_event(event);
}

UG_API int ug_send_key_event(enum ug_key_event event)
{
	if (event <= UG_KEY_EVENT_NONE || event >= UG_KEY_EVENT_MAX) {
		_ERR("ug_send_key_event() failed: Invalid event");
		return -1;
	}

	return ugman_send_key_event(event);
}

UG_API int ug_send_result(ui_gadget_h ug, app_control_h send)
{
	app_control_h send_dup = NULL;

	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_send_result() failed: Invalid ug");
		errno = EINVAL;
		return -1;
	}

	if (!ug->cbs.result_cb) {
		_ERR("ug_send_result() failed: result callback does not exist");
		return -1;
	}

	if (send) {
		app_control_clone(&send_dup, send);
		if (!send_dup) {
			_ERR("ug_send_result() failed: app_control_destroy failed");
			return -1;
		}
	} else {
		_ERR("send sevice handle null error");
		return -1;
	}

	ug->cbs.result_cb(ug, send_dup, ug->cbs.priv);

	if (send_dup)
		app_control_destroy(send_dup);

	return 0;
}

UG_API int ug_send_result_full(ui_gadget_h ug, app_control_h send, app_control_result_e result)
{
	app_control_h send_dup = NULL;
	char tmp_result[4] = {0,};
	int ret;

	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ug_send_result() failed: Invalid ug");
		errno = EINVAL;
		return -1;
	}

	if (!ug->cbs.result_cb) {
		_ERR("ug_send_result() failed: result callback does not exist");
		return -1;
	}

	if (send) {
		app_control_clone(&send_dup, send);
		if (!send_dup) {
			_ERR("ug_send_result() failed: app_control_destroy failed");
			return -1;
		}
	} else {
		_ERR("send sevice handle null error");
		return -1;
	}

	snprintf(tmp_result, 4, "%d", result);

	ret = app_control_add_extra_data(send_dup, UG_APP_CONTROL_DATA_RESULT, (const char*)tmp_result);
	if(ret != APP_CONTROL_ERROR_NONE) {
		_WRN("app control add exter data error(%d)", ret);
	}

	ug->cbs.result_cb(ug, send_dup, ug->cbs.priv);

	if (send_dup)
		app_control_destroy(send_dup);

	return 0;
}

UG_API int ug_send_message(ui_gadget_h ug, app_control_h msg)
{
	int r;

	app_control_h msg_dup = NULL;
	if (msg) {
		app_control_clone(&msg_dup, msg);
		if (!msg_dup) {
			_ERR("ug_send_message() failed: app_control_destroy failed");
			return -1;
		}
	}

	r = ugman_send_message(ug, msg_dup);

	if (msg_dup)
		app_control_destroy(msg_dup);

	return r;
}

UG_API int ug_disable_effect(ui_gadget_h ug)
{
	if(!ug) {
		_ERR("ug input param is null");
		return -1;
	}
	if (ug->layout_state != UG_LAYOUT_INIT) {
		_ERR("ug_disable_effect() failed: ug has already been shown");
		return -1;
	}
	ug->layout_state = UG_LAYOUT_NOEFFECT;

	return 0;
}

UG_API int ug_is_installed(const char *name)
{
	if(name == NULL){
		_ERR("name is null");
		return -1;
	}

	return ug_exist(name);
}

#ifdef ENABLE_UG_CREATE_CB
UG_API int ug_create_cb(void (*create_cb)(char*,char*,char*,void*), void *user_data)
{
	int ret;

	ret = ugman_create_cb(create_cb, user_data);
	if(ret == -1) {
		_ERR("trace cb register fail");
	}

	return ret;
}
#endif


