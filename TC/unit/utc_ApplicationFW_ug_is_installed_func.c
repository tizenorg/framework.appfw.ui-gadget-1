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

static Evas_Object *win;

static void startup(void);
static void cleanup(void);

static void utc_ApplicationFW_ug_is_installed_func_01(void);
static void utc_ApplicationFW_ug_is_installed_func_02(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_ug_is_installed_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_ug_is_installed_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0},
};

static void startup(void)
{
	char *err;
	elm_init(0, NULL);
	win = elm_win_add(NULL, "UI gadget test", ELM_WIN_BASIC);
	if (!win) {
		err = "Cannot create window";
		tet_infoline(err);
		tet_delete(POSITIVE_TC_IDX, err);
	}
	elm_win_conformant_set(win, EINA_TRUE);
	elm_conformant_add(win);
}

static void cleanup(void)
{
	if (win)
		evas_object_del(win);
	elm_shutdown();
}

/**
 * @brief Positive test case of ug_is_installed API
 */
static void utc_ApplicationFW_ug_is_installed_func_01(void)
{
	int ret;

	ret = ug_is_installed(TESTUG);
	if(ret <= 0) {
		tet_infoline("ug_is_installed() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_is_installed API
 */
static void utc_ApplicationFW_ug_is_installed_func_02(void)
{
	int ret;

        ret = ug_is_installed("hello");
        if(ret == 1) {
                tet_infoline("ug_is_installed() failed in negative test case");
                tet_result(TET_FAIL);
                return;
        }
        tet_result(TET_PASS);
}
