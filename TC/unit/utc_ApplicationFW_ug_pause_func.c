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

static void utc_ApplicationFW_ug_pause_func_01(void);

enum {
	POSITIVE_TC_IDX = 0x01,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_ug_pause_func_01, POSITIVE_TC_IDX },
	{ NULL, 0},
};

static ui_gadget_h ug;

static void startup(void)
{
	char *err;
	Evas_Object *win;

	elm_init(0, NULL);
	win = elm_win_add(NULL, "UI gadget test", ELM_WIN_BASIC);
	if (!win) {
		err = "Cannot create window";
		goto startup_err;
	}
	elm_win_conformant_set(win, EINA_TRUE);
	elm_conformant_add(win);

	if (UG_INIT_EFL(win, UG_OPT_INDICATOR_ENABLE)) {
		err = "ug_init() failed";
		goto startup_err;
	}

	ug = ug_create(NULL, TESTUG, UG_MODE_FULLVIEW, NULL, NULL);
	if (!ug) {
		err = "ug_create() failed";
		goto startup_err;
	}

	return;

startup_err:
	tet_infoline(err);
	tet_delete(POSITIVE_TC_IDX, err);
}

static void cleanup(void)
{
	elm_shutdown();
}

static Eina_Bool tst01(void *data)
{
	int r = 0;

	r = ug_pause();
	if (r) {
		tet_infoline("ug_pause() failed in positive test case");
		tet_result(TET_FAIL);
		goto exit;
	}
	tet_result(TET_PASS);

exit:
	elm_exit();
	return 0;
}

/**
 * @brief Positive test case of ug_pause()
 */
static void utc_ApplicationFW_ug_pause_func_01(void)
{
	ecore_idler_add(tst01, NULL);
	elm_run();
}
