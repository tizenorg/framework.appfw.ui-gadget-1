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
#include <tet_api.h>
#include <ui-gadget.h>
#include <Elementary.h>
#include <Ecore_X.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_ug_disable_effect_func_01(void);
static void utc_ApplicationFW_ug_disable_effect_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_ug_disable_effect_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_ug_disable_effect_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0},
};


static void startup(void)
{
	elm_init(0, NULL);
}

static void cleanup(void)
{
	elm_shutdown();
}

static int init(void)
{
	Evas_Object *win;
	win = elm_win_add(NULL, "UI gadget test", ELM_WIN_BASIC);
	if (!win) {
		tet_infoline("Cannot create window");
		return -1;
	}
	elm_win_conformant_set(win, EINA_TRUE);
	elm_conformant_add(win);

	if (UG_INIT_EFL(win, UG_OPT_INDICATOR_ENABLE)) {
		tet_infoline("ug_init() failed");
		return -1;
	}
	return 0;
}

void layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	int ret;
	ret = ug_disable_effect(ug);
	if (ret == -1) {
		tet_infoline("ug_disable_effect() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Positive test case of ug_disable_effect()
 */
static void utc_ApplicationFW_ug_disable_effect_func_01(void)
{
	ui_gadget_h ug;
	struct ug_cbs cbs = {0, };

	if (init()) {
		tet_result(TET_UNINITIATED);
		return;
	}
	
	cbs.layout_cb = layout_cb;

	ug = ug_create(NULL, TESTUG, UG_MODE_FULLVIEW, NULL, &cbs);
	if (!ug) {
		tet_infoline("ug_create() failed");
		tet_result(TET_UNINITIATED);
		return;
	}
}

void layout2_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
{
	int ret;
	ret = ug_disable_effect(NULL);
	if (ret == 0) {
		tet_infoline("ug_disable_effect() failed in negative test case");
		tet_result(TET_FAIL);	
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init ug_disable_effect()
 */
static void utc_ApplicationFW_ug_disable_effect_func_02(void)
{
	ui_gadget_h ug;
	struct ug_cbs cbs = {0, };

	if (init()) {
		tet_result(TET_UNINITIATED);
		return;
	}
	
	cbs.layout_cb = layout2_cb;

	ug = ug_create(NULL, TESTUG2, UG_MODE_FULLVIEW, NULL, &cbs);
	if (!ug) {
		tet_infoline("ug_create() failed");
		tet_result(TET_UNINITIATED);
		return;
	}
}
