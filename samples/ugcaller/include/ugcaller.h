/*
 * image-viewer
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

#include <Elementary.h>
#include <dlog.h>

#undef LOG_TAG
#define LOG_TAG "UI_GADGET_CALLER"

#if !defined(PACKAGE)
#define PACKAGE "ugcaller"
#endif

#define _EDJ(o)			elm_layout_edje_get(o)
#define EDJ_FILE "/opt/apps/org.tizen.ugcaller/res/ugcaller.edj"
#define GRP_MAIN "main"

typedef struct {
	int win_w;
	int win_h;

	Evas *evas;
	Evas_Object *win_main;
	Evas_Object *base;
	Evas_Object *ly_main;

	Evas_Object* ug_layout;

	ui_gadget_h ug;
	ui_gadget_h sub_ug;

	app_control_h app_control_handle;
	char *ug_name;
} appdata;

