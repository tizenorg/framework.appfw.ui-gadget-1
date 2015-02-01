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
#include <ui-gadget-module.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <app_service.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_ug_send_result_func_01(void);
static void utc_ApplicationFW_ug_send_result_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_ug_send_result_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_ug_send_result_func_02, NEGATIVE_TC_IDX },
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

static Eina_Bool tst01(void *data)
{
	int r = 0;
	app_control_h result;
	ui_gadget_h ug = data;
	int ret;
	
	ret = app_control_create(&result);
	if (ret != APP_CONTROL_ERROR_NONE) {
		tet_infoline("app_control_create() failed");
		tet_result(TET_UNINITIATED);
		goto exit;
	}
	
	r = ug_send_result(ug, result);
	if (r) {
		tet_infoline("ug_send_result2() failed in positive test case");
		tet_result(TET_FAIL);
		goto exit;
	}
	tet_result(TET_PASS);

exit:
	elm_exit();
	return 0;
}

static void result_cb(ui_gadget_h ug, app_control_h reply, void *priv)
{
	// dummy result callback
}

/**
 * @brief Positive test case of ug_send_result()
 */
static void utc_ApplicationFW_ug_send_result_func_01(void)
{
	ui_gadget_h ug;
	struct ug_cbs cbs = {0, };

	if (init()) {
		tet_result(TET_UNINITIATED);
		return;
	}

	cbs.result_cb = result_cb;

	ug = ug_create(NULL, TESTUG, UG_MODE_FULLVIEW, NULL, &cbs);
	if (!ug) {
		tet_infoline("ug_create() failed");
		tet_result(TET_UNINITIATED);
	}
	
	tet_infoline("utc_ApplicationFW_ug_send_result_func_01");

	ecore_idler_add(tst01, ug);
	elm_run();
}

static Eina_Bool tst02(void *data)
{
	int r = 0;
	app_control_h result;
	int ret;
	ui_gadget_h ug = data;
	
	ret = app_control_create(&result);
	if (ret != APP_CONTROL_ERROR_NONE) {
		tet_infoline("app_control_create() failed");
		tet_result(TET_UNINITIATED);
		goto exit;
	}

	r = ug_send_result(NULL, NULL);
	if (!r) {
		tet_infoline("ug_send_result() failed in negative test case");
		tet_result(TET_FAIL);
		goto exit;
	}

	r = ug_send_result(ug, NULL);
	if (!r) {
		tet_infoline("ug_send_result() failed in negative test case");
		tet_result(TET_FAIL);
		goto exit;
	}

	r = ug_send_result(NULL, result);
	if (!r) {
		tet_infoline("ug_send_result() failed in negative test case");
		tet_result(TET_FAIL);
		goto exit;
	}

	tet_result(TET_PASS);
exit:
	elm_exit();
	return 0;
}


/**
 * @brief Negative test case of ug_init ug_send_result()
 */
static void utc_ApplicationFW_ug_send_result_func_02(void)
{
	ui_gadget_h ug;

	if (init()) {
		tet_result(TET_UNINITIATED);
		return;
	}

	ug = ug_create(NULL, TESTUG2, UG_MODE_FULLVIEW, NULL, NULL);
	if (!ug) {
		tet_infoline("ug_create() failed");
		tet_result(TET_UNINITIATED);
	}

	ecore_idler_add(tst02, ug);
	elm_run();
}