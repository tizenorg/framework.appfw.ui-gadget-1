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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

#include <Ecore.h>
#include <Ecore_X.h>

#include "ug.h"
#include "ug-manager.h"
#include "ug-engine.h"
#include "ug-dbg.h"

#define Idle_Cb Ecore_Cb

#define ugman_idler_add(func, data)  \
	ecore_job_add((Ecore_Cb) func, (void *)data);

#ifdef ENABLE_UG_CREATE_CB
typedef void (*fn_ug_trace_cb)(char *ug, char *mem, char *parent, void *user_data);
fn_ug_trace_cb g_create_cb = NULL;
void *g_create_cb_user_data = NULL;
#endif

struct ug_manager {
	ui_gadget_h root;
	ui_gadget_h fv_top;
	GSList *fv_list;

	void *win;
	Window win_id;
	Display *disp;

	enum ug_option base_opt;
	enum ug_event last_rotate_evt;

	int walking;

	int is_initted:1;
	int is_landscape:1;
	int destroy_all:1;

	struct ug_engine *engine;
};

static struct ug_manager ug_man;

static inline void job_start(void);
static inline void job_end(void);

static int ug_relation_add(ui_gadget_h p, ui_gadget_h c)
{
	c->parent = p;
	/* prepend element to avoid the inefficiency,
		which is to traverse the entire list to find the end*/
	p->children = g_slist_prepend(p->children, c);

	return 0;
}

static int ug_relation_del(ui_gadget_h ug)
{
	ui_gadget_h p;

	p = ug->parent;
	if (!p) {
		_WRN("ug_relation_del failed: no parent");
		return -1;
	}

	if(p->children) {
		p->children = g_slist_remove(p->children, ug);
	}

	if (ug->children) {
		g_slist_free(ug->children);
		ug->children = NULL;
	}

	ug->parent = NULL;

	return 0;
}

static int ug_fvlist_add(ui_gadget_h c)
{
	ug_man.fv_list = g_slist_prepend(ug_man.fv_list, c);
	ug_man.fv_top = c;

	return 0;
}

static int ug_fvlist_del(ui_gadget_h c)
{
	ui_gadget_h t;

	ug_man.fv_list = g_slist_remove(ug_man.fv_list, c);

	/* update fullview top ug*/
	t = g_slist_nth_data(ug_man.fv_list, 0);
	ug_man.fv_top = t;

	return 0;
}

static enum ug_event __ug_x_rotation_get(Display *dpy, Window win)
{
	int count = 0;
    unsigned char *prop_data = NULL;
	int angle = 0;
	int func_ret;

	int ret = ecore_x_window_prop_property_get(
			ug_man.win_id,
			ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
			ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);

	if (ret && prop_data) {
        memcpy(&angle, prop_data, sizeof(int));
		_DBG("window property get ok. angle is %d", angle);
	} else {
		_DBG("window property get fail. angle will be default(0)");
	}

	if(prop_data)
		free(prop_data);

	switch (angle) {
		case 0:
			func_ret = UG_EVENT_ROTATE_PORTRAIT;
			break;
		case 90:
			func_ret = UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN;
			break;
		case 180:
			func_ret = UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN;
			break;
		case 270:
			func_ret = UG_EVENT_ROTATE_LANDSCAPE;
			break;
		default:
			func_ret = UG_EVENT_ROTATE_PORTRAIT;
			break;
	}

	return func_ret;
}

static void ugman_tree_dump(ui_gadget_h ug)
{
	static int i;
	int lv;
	const char *name;
	GSList *child;
	ui_gadget_h c;

	if (!ug)
		return;

	name = ug->name;
	if (ug == ug_man.root) {
		i = 0;
		_DBG("============== TREE_DUMP =============");
		_DBG("ROOT: Manager(%p)",ug);
		name = "Manager";
	}

	child = ug->children;
	if (!child)
		return;

	i++;
	lv = i;

	while (child) {
		c = child->data;
		_DBG("[%d] %s [%c] (mem : %s) (ug : %p) (PARENT:  %s)",
		     lv,
		     c && c->name ? c->name : "NO CHILD INFO FIXIT!!!",
		     c && c->mode == UG_MODE_FULLVIEW ? 'F' : 'f',
			 c->module->addr, c, name);
		ugman_tree_dump(c);
		child = g_slist_next(child);
	}
}

static int ugman_ug_free(ui_gadget_h ug)
{
	if (!ug) {
		_ERR("ug free failed: Invalid ug");
		errno = EINVAL;
		return -1;
	}

	if (ug->module) {
		ug_module_unload(ug->module);
	}
	if (ug->name) {
		free((void *)ug->name);
		ug->name = NULL;
	}
	if (ug->app_control) {
		app_control_destroy(ug->app_control);
		ug->app_control = NULL;
	}
	free(ug);
	ug = NULL;
	return 0;
}


static int ugman_ug_find(ui_gadget_h p, ui_gadget_h ug)
{
	GSList *child = NULL;

	if (!p || !ug)
		return 0;
	child = p->children;

	while (child) {
		if (child->data == ug)
			return 1;
		if (ugman_ug_find(child->data, ug))
			return 1;
		child = g_slist_next(child);
	}

	return 0;
}

static void ugman_ug_start(void *data)
{
	ui_gadget_h ug = data;
	struct ug_module_ops *ops = NULL;

	if (!ug) {
		_ERR("ug is null");
		return;
	} else if (ug->state != UG_STATE_CREATED) {
		_DBG("start cb will be not invoked because ug(%p) state(%d) is not created", ug, ug->state);
		return;
	}

	_DBG("ug=%p", ug);

	ug->state = UG_STATE_RUNNING;

	if (ug->module)
		ops = &ug->module->ops;

	if (ops && ops->start)
		ops->start(ug, ug->app_control, ops->priv);

	return;
}

static int ugman_ug_pause(void *data)
{
	ui_gadget_h ug = data;
	struct ug_module_ops *ops = NULL;
	GSList *child = NULL;

	job_start();

	if (!ug) {
		_WRN("ug pointer is null");
		goto end;
	}

	if (ug->children) {
		child = ug->children;
		while (child) {
			ugman_ug_pause(child->data);
			child = g_slist_next(child);
		}
	}

	if (ug->state != UG_STATE_RUNNING) {
		if(ug != ug_man.root) {
			_WRN("ug(%p)->state : %d", ug, ug->state);
		}
		goto end;
	}

	if (ug->module)
		ops = &ug->module->ops;

	if (ops && ops->pause) {
		_DBG("call ug(%p) pause cb", ug);
		ops->pause(ug, ug->app_control, ops->priv);
	}

	ug->state = UG_STATE_STOPPED;

 end:
	job_end();
	return 0;
}

static int ugman_ug_resume(void *data)
{
	ui_gadget_h ug = data;
	struct ug_module_ops *ops = NULL;
	GSList *child = NULL;

	job_start();

	if (!ug) {
		_WRN("ug pointer is null");
		goto end;
	}

	if (ug->children) {
		child = ug->children;
		while (child) {
			ugman_ug_resume(child->data);
			child = g_slist_next(child);
		}
	}

	if (ug->state != UG_STATE_STOPPED) {
		if(ug != ug_man.root) {
			_WRN("ug(%p)->state : %d", ug, ug->state);
		}
	}

	switch (ug->state) {
		case UG_STATE_CREATED:
			ugman_ug_start(ug);
			goto end;
		case UG_STATE_STOPPED:
			break;
		default:
			goto end;
	}

	if (ug->module)
		ops = &ug->module->ops;

	if (ops && ops->resume) {
		_DBG("call ug(%p) resume cb", ug);
		ops->resume(ug, ug->app_control, ops->priv);
	}

	ug->state = UG_STATE_RUNNING;

 end:
	job_end();
	return 0;
}

static void _ugman_enable_indicator(Ecore_X_Display *dpy, Ecore_X_Window xwin, int enable)
{
	if(dpy == NULL)	{
		_DBG("ERROR.. Invald Display");
		return;
	}
	if(enable == 1)	{
		ecore_x_e_illume_indicator_state_set(xwin, ECORE_X_ILLUME_INDICATOR_STATE_ON);
	} else {
		ecore_x_e_illume_indicator_state_set(xwin, ECORE_X_ILLUME_INDICATOR_STATE_OFF);
	}
}

static int _ugman_get_indicator_state (Ecore_X_Display *dpy, Ecore_X_Window xwin)
{
	Ecore_X_Illume_Indicator_State state;
	int ret;
	if(dpy == NULL)	{
		_DBG("ERROR.. Invald Display");
		return -1;
	}
	state = ecore_x_e_illume_indicator_state_get(xwin);
	if (state == ECORE_X_ILLUME_INDICATOR_STATE_OFF) {
		ret = 0;
	} else if (state == ECORE_X_ILLUME_INDICATOR_STATE_ON) {
		ret = 1;
	} else {
		ret = -1;
	}
	return ret;
}

static int ugman_indicator_update(enum ug_option opt, enum ug_event event)
{
	int enable;
	int cur_state;

	cur_state = _ugman_get_indicator_state(ug_man.disp, ug_man.win_id);

	_DBG("indicator update opt(%d) cur_state(%d)", opt, cur_state);

	switch (opt) {
#ifndef ENABLE_UG_HANDLE_INDICATOR_HIDE
		case UG_OPT_INDICATOR_ENABLE:
		case UG_OPT_INDICATOR_PORTRAIT_ONLY:
		case UG_OPT_INDICATOR_LANDSCAPE_ONLY:
		case UG_OPT_INDICATOR_DISABLE:
			enable = 1;
			break;
#else
		case UG_OPT_INDICATOR_ENABLE:
			if (event == UG_EVENT_NONE)
				enable = 1;
			else
				enable = cur_state ? 1 : 0;
			break;
		case UG_OPT_INDICATOR_PORTRAIT_ONLY:
			enable = ug_man.is_landscape ? 0 : 1;
			break;
		case UG_OPT_INDICATOR_LANDSCAPE_ONLY:
			enable = ug_man.is_landscape ? 1 : 0;
			break;
		case UG_OPT_INDICATOR_DISABLE:
			enable = 0;
			break;
#endif
		case UG_OPT_INDICATOR_MANUAL:
			return 0;
		default:
			_ERR("update failed: Invalid opt(%d)", opt);
			return -1;
	}

	if(cur_state != enable) {
		_DBG("set indicator status as %d", enable);
		_ugman_enable_indicator(ug_man.disp, ug_man.win_id, enable);
	}

	return 0;
}

static int ugman_ug_getopt(ui_gadget_h ug)
{
	if (!ug)
		return -1;

	/* Indicator Option */
	if (ug->mode == UG_MODE_FULLVIEW) {
		ugman_indicator_update(ug->opt, UG_EVENT_NONE);
	}

	return 0;
}

static int ugman_ug_event(ui_gadget_h ug, enum ug_event event)
{
	struct ug_module_ops *ops = NULL;
	GSList *child = NULL;

	if (!ug)
		return 0;

	if (ug->children) {
		child = ug->children;
		while (child) {
			ugman_ug_event(child->data, event);
			child = g_slist_next(child);
		}
	}

	if (ug->module)
		ops = &ug->module->ops;

	if (ops && ops->event) {
		_DBG("ug_event_cb : ug(%p) / event(%d) / event cb(%p)", ug, event, ops->event);
		ops->event(ug, event, ug->app_control, ops->priv);
	}

	return 0;
}

static int ugman_ug_destroy(void *data)
{
	ui_gadget_h ug = data;
	struct ug_module_ops *ops = NULL;
	struct ug_cbs *cbs;

	job_start();

	if (!ug)
		goto end;

	_DBG("ug(%p) state(%d)", ug, ug->state);

	switch (ug->state) {
		case UG_STATE_CREATED:
		case UG_STATE_RUNNING:
		case UG_STATE_STOPPED:
		case UG_STATE_DESTROYING:
		case UG_STATE_PENDING_DESTROY:
			break;
		default:
			_WRN("ug(%p) state is already destroyed", ug);
			goto end;
	}

	ug->state = UG_STATE_DESTROYED;

	if((ug != ug_man.root) && (ug->layout) &&
		(ug->mode == UG_MODE_FULLVIEW) &&
		(ug->layout_state != UG_LAYOUT_DESTROY)) {
		/* ug_destroy_all case */
		struct ug_engine_ops *eng_ops = NULL;

		if (ug_man.engine)
			eng_ops = &ug_man.engine->ops;

		if (eng_ops && eng_ops->destroy)
			eng_ops->destroy(ug, NULL, NULL);
	}

	if (ug->module)
		ops = &ug->module->ops;

	if (ops && ops->destroy) {
		_DBG("ug(%p) module destory cb call", ug);
		ops->destroy(ug, ug->app_control, ops->priv);
	}

	cbs = &ug->cbs;
	if (cbs && cbs->end_cb) {
		_DBG("ug(%p) end cb will be invoked", ug);
		cbs->end_cb(ug, cbs->priv);
	}

	if((ug->parent) && (ug->parent->state == UG_STATE_PENDING_DESTROY)) {
		if((ug->parent->children) && (g_slist_length(ug->parent->children) == 1)) {
			_WRN("pended parent ug(%p) destroy job is added to loop", ug->parent);
			ugman_idler_add((Idle_Cb)ugman_ug_destroy, ug->parent);
		} else {
			_WRN("pended parent ug(%p) will be destroyed after another children is destroyed", ug->parent);
		}
	} else {
		if(ug->parent) {
			_DBG("ug parent(%p) state(%d)", ug->parent, ug->parent->state);
		} else {
			_WRN("ug parent is null");
		}
	}

	if (ug != ug_man.root)
		ug_relation_del(ug);

	if (ug->mode == UG_MODE_FULLVIEW) {
		if (ug_man.fv_top == ug) {
			ug_fvlist_del(ug);
			if(!ug_man.destroy_all)
				ugman_ug_getopt(ug_man.fv_top);
		} else {
			ug_fvlist_del(ug);
		}
	}

	_DBG("free ug(%p)", ug);
	ugman_ug_free(ug);

	if (ug_man.root == ug)
		ug_man.root = NULL;

	ugman_tree_dump(ug_man.root);
 end:
	job_end();

	return 0;
}

static void ug_hide_end_cb(void *data)
{
	ui_gadget_h ug = data;
	if (ug->children) {
		_WRN("child ug is still destroying. parent ug(%p) will be destroyed later", ug);
		ug->state = UG_STATE_PENDING_DESTROY;
	} else {
		ugman_idler_add((Idle_Cb)ugman_ug_destroy, (void *)ug);
	}
}

static int ugman_ug_create(void *data)
{
	ui_gadget_h ug = data;
	struct ug_module_ops *ops = NULL;
	struct ug_cbs *cbs;
	struct ug_engine_ops *eng_ops = NULL;
	void* conformant = NULL;
	void* conformant2 = NULL;


	if (!ug || ug->state != UG_STATE_READY) {
		_ERR("ug(%p) input param error", ug);
		return -1;
	}

	conformant = ugman_get_conformant();
	if(!conformant) {
		_ERR("conformant error. ug_create(%s) fail.", ug->name);
		return -1;
	}

	ug->state = UG_STATE_CREATED;

	if (ug->module)
		ops = &ug->module->ops;

	if (ug_man.engine)
		eng_ops = &ug_man.engine->ops;

	if (ops && ops->create) {
		_DBG("before ug(%s) create cb",ug->name);
		ug->layout = ops->create(ug, ug->mode, ug->app_control, ops->priv);
		_DBG("after ug(%s) create cb",ug->name);
		if (!ug->layout) {
			_ERR("ug(%p) layout is null", ug);
			return -1;
		}
		if (ug->mode == UG_MODE_FULLVIEW) {
			if (eng_ops && eng_ops->create) {
				conformant2 = eng_ops->create(ug_man.win, ug, ugman_ug_start);
				if((!conformant2) || (conformant != conformant2)) {
					_ERR("conformant(%p,%p) error. ug(%p) destory cb is invoked.",
							conformant,conformant2,ug);
					ops->destroy(ug, ug->app_control, ops->priv);
					return -1;
				}
			}
		}
		cbs = &ug->cbs;

		if (cbs && cbs->layout_cb)
			cbs->layout_cb(ug, ug->mode, cbs->priv);

		_DBG("after caller layout cb call");
		ugman_indicator_update(ug->opt, UG_EVENT_NONE);
	}

	if(ug_man.last_rotate_evt == UG_EVENT_NONE) {
		ug_man.last_rotate_evt = __ug_x_rotation_get(ug_man.disp, ug_man.win_id);
	}
	ugman_ug_event(ug, ug_man.last_rotate_evt);

	if(ug->mode == UG_MODE_FRAMEVIEW)
		ugman_ug_start(ug);

	ugman_tree_dump(ug_man.root);

#ifdef ENABLE_UG_CREATE_CB
	if(g_create_cb) {
		ui_gadget_h parent = ug->parent;

		_DBG("invoke trace create cb(%p)", g_create_cb);

		g_create_cb((char*)ug->name,(char*)ug->module->addr,parent ? (char*)parent->name : NULL, g_create_cb_user_data);
	}
#endif


	return 0;
}

static ui_gadget_h ugman_root_ug_create(void)
{
	ui_gadget_h ug;

	ug = calloc(1, sizeof(struct ui_gadget_s));
	if (!ug) {
		_ERR("ug root create failed: Memory allocation failed");
		return NULL;
	}

	ug->mode = UG_MODE_FULLVIEW;
	ug->state = UG_STATE_RUNNING;
	ug->children = NULL;

	return ug;
}

int ugman_ug_add(ui_gadget_h parent, ui_gadget_h ug)
{
	if (!ug_man.is_initted) {
		_ERR("failed: manager is not initted");
		return -1;
	}

	if (!ug_man.root) {
		if (parent) {
			_ERR("failed: parent has to be NULL w/o root");
			errno = EINVAL;
			return -1;
		}

		ug_man.root = ugman_root_ug_create();
		if (!ug_man.root) {
			_ERR("failed : ug root create fail");
			return -1;
		}
		ug_man.root->opt = ug_man.base_opt;
		ug_man.root->layout = ug_man.win;
		ug_fvlist_add(ug_man.root);
	}

	if (!parent) {
		_DBG("parent is null. parent of ug(%p) will be manager", ug);
		parent = ug_man.root;
	} else {
		switch (parent->state) {
			case UG_STATE_DESTROYING:
			case UG_STATE_PENDING_DESTROY:
			case UG_STATE_DESTROYED:
				_WRN("parent(%p) state(%d) error", parent, parent->state);
				return -1;
			default:;
		}
	}

	if (ug_relation_add(parent, ug)) {
		_ERR("failed : ug_relation_add fail");
		return -1;
	}

	if (ugman_ug_create(ug) == -1) {
		_ERR("failed : ugman_ug_create fail");
		ug_relation_del(ug);
		return -1;
	}
	if (ug->mode == UG_MODE_FULLVIEW)
		ug_fvlist_add(ug);

	return 0;
}

ui_gadget_h ugman_ug_load(ui_gadget_h parent,
				const char *name,
				enum ug_mode mode,
				app_control_h service, struct ug_cbs *cbs)
{
	int r;
	ui_gadget_h ug;

	ug = calloc(1, sizeof(struct ui_gadget_s));
	if (!ug) {
		_ERR("ug_create() failed: Memory allocation failed");
		return NULL;
	}

	_DBG("name : %s", name);

	ug->module = ug_module_load(name);
	if (!ug->module) {
		_ERR("ug_create() failed: Module loading failed");
		goto load_fail;
	}

	ug->name = strdup(name);

	ug->mode = mode;
	app_control_clone(&ug->app_control, service);
	ug->opt = ug->module->ops.opt;
	ug->state = UG_STATE_READY;
	ug->children = NULL;

	if (cbs)
		memcpy(&ug->cbs, cbs, sizeof(struct ug_cbs));

	r = ugman_ug_add(parent, ug);
	if (r) {
		_ERR("ugman ug add failed");
		goto load_fail;
	}

	return ug;

 load_fail:
	ugman_ug_free(ug);
	return NULL;
}

int ugman_ug_destroying(ui_gadget_h ug)
{
	struct ug_module_ops *ops = NULL;

	_DBG("ugman_ug_destroying start ug(%p)", ug);

	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ugman_ug_destroying failed: Invalid ug(%p)");
		errno = EINVAL;
		return -1;
	}

	switch (ug->state) {
		case UG_STATE_DESTROYING:
		case UG_STATE_PENDING_DESTROY:
		case UG_STATE_DESTROYED:
			_WRN("ug(%p) state(%d) is already on destroying", ug, ug->state);
			return 0;
		default:
			break;
	}

	ug->destroy_me = 1;
	ug->state = UG_STATE_DESTROYING;

	if (ug->module)
		ops = &ug->module->ops;

	if (ops && ops->destroying)
		ops->destroying(ug, ug->app_control, ops->priv);

	return 0;
}

int ugman_ug_del(ui_gadget_h ug)
{
	struct ug_engine_ops *eng_ops = NULL;

	_DBG("ugman_ug_del start ug(%p)", ug);

	if (!ug || !ugman_ug_exist(ug)) {
		_ERR("ugman_ug_del failed: Invalid ug(%p)");
		errno = EINVAL;
		return -1;
	}

	switch (ug->state) {
		case UG_STATE_DESTROYING:
		case UG_STATE_PENDING_DESTROY:
		case UG_STATE_DESTROYED:
			_WRN("ug(%p) state(%d) is already on destroying", ug, ug->state);
			return 0;
		default:
			break;
	}

	if (ug->destroy_me) {
		_WRN("ugman_ug_del failed: ug is alreay on destroying");
		return -1;
	}

	if (!ug_man.is_initted) {
		_WRN("ugman_ug_del failed: manager is not initted");
		return -1;
	}

	if (!ug_man.root) {
		_ERR("ugman_ug_del failed: no root");
		return -1;
	}

	if (ug->children) {
		GSList *child, *trail;

		child = ug->children;
		_DBG("ugman_ug_del ug(%p) has child(%p)", ug, child->data);
		while (child) {
			trail = g_slist_next(child);
			ugman_ug_del(child->data);
			child = trail;
		}
	}

	ugman_ug_destroying(ug);

	/* pre call for indicator update time issue */
	bool is_update = false;
	ui_gadget_h t = NULL;
	if (ug_man.fv_top == ug) {
		is_update = true;
		t = g_slist_nth_data(ug_man.fv_list, 1);
	} else {
		if (ug->children) {
			GSList *child;
			child = g_slist_last(ug->children);
			if(ug_man.fv_top == (ui_gadget_h)child->data) {
				is_update = true;
				t = g_slist_nth_data(ug_man.fv_list,
					g_slist_index(ug_man.fv_list,(gconstpointer)ug)+1);
			}
		}
	}

	if((is_update)&&(t)) {
		ugman_ug_getopt(t);
	}

	if (ug_man.engine)
		eng_ops = &ug_man.engine->ops;

	if (ug->mode == UG_MODE_FULLVIEW) {
		if (eng_ops && eng_ops->destroy)
			eng_ops->destroy(ug, ug_man.fv_top, ug_hide_end_cb);
		else
			ugman_idler_add((Idle_Cb)ugman_ug_destroy, ug);
	} else {
		_DBG("ug(%p) mode is frameview", ug);
		ug_hide_end_cb(ug);
	}

	_DBG("ugman_ug_del(%p) end", ug);

	return 0;
}


int ugman_ug_del_child(ui_gadget_h ug)
{
	GSList *child, *trail;

	if (ug->children) {
		child = ug->children;
		_DBG("ug destroy all. ug(%p) has child(%p)", ug, child->data);
		while (child) {
			trail = g_slist_next(child);
			ugman_ug_del_child(child->data);
			child = trail;
		}
	}

	ugman_ug_destroy(ug);

	return 0;
}

static void ugman_ug_unset_content(void)
{
	struct ug_engine_ops *eng_ops = NULL;

	if (ug_man.engine) {
		eng_ops = &ug_man.engine->ops;
	} else {
		_WRN("ui engine is not loaded");
		return;
	}

	if (eng_ops && eng_ops->create) {
		eng_ops->request(ug_man.win, NULL, UG_UI_REQ_UNSET_CONTENT);
	} else {
		_WRN("ui engine is not loaded");
	}

	return;
}

int ugman_ug_del_all(void)
{
	/*  Terminate */
	if (!ug_man.is_initted) {
		_ERR("ugman_ug_del_all failed: manager is not initted");
		return -1;
	}

	if (!ug_man.root) {
		_ERR("ugman_ug_del_all failed: no root");
		return -1;
	}

	_DBG("ug_del_all. root(%p) walking(%d) ", ug_man.root, ug_man.walking);

	if (ug_man.walking > 0) {
		ug_man.destroy_all = 1;
	} else {
		ugman_ug_unset_content();
		ugman_ug_del_child(ug_man.root);
	}

	return 0;
}

int ugman_init(Display *disp, Window xid, void *win, enum ug_option opt)
{
	ug_man.win = win;
	ug_man.disp = disp;
	ug_man.win_id = xid;
	ug_man.base_opt = opt;
	ug_man.last_rotate_evt = UG_EVENT_NONE;

	if (!ug_man.is_initted) {
		ug_man.engine = ug_engine_load();
	}

	ug_man.is_initted = 1;

	return 0;
}

int ugman_resume(void)
{
	/* RESUME */
	if (!ug_man.is_initted) {
		_ERR("ugman_resume failed: manager is not initted");
		return -1;
	}

	if (!ug_man.root) {
		_WRN("ugman_resume failed: no root");
		return -1;
	}

	_DBG("ugman_resume called");

	ugman_idler_add((Idle_Cb)ugman_ug_resume, ug_man.root);

	return 0;
}

int ugman_pause(void)
{
	/* PAUSE (Background) */
	if (!ug_man.is_initted) {
		_ERR("ugman_pause failed: manager is not initted");
		return -1;
	}

	if (!ug_man.root) {
		_WRN("ugman_pause failed: no root");
		return -1;
	}

	_DBG("ugman_pause called");

	ugman_idler_add((Idle_Cb)ugman_ug_pause, ug_man.root);

	return 0;
}

static int ugman_send_event_pre(void *data)
{
	job_start();

	ugman_ug_event(ug_man.root, (enum ug_event)data);

	job_end();

	return 0;
}

int ugman_send_event(enum ug_event event)
{
	int is_rotation = 1;

	/* Propagate event */
	if (!ug_man.is_initted) {
		_ERR("ugman_send_event failed: manager is not initted");
		return -1;
	}

	if (!ug_man.root) {
		_WRN("ugman_send_event failed: no root");
		return -1;
	}

	/* In case of rotation, indicator state has to be updated */
	switch (event) {
	case UG_EVENT_ROTATE_PORTRAIT:
	case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
		ug_man.last_rotate_evt = event;
		ug_man.is_landscape = 0;
		break;
	case UG_EVENT_ROTATE_LANDSCAPE:
	case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		ug_man.last_rotate_evt = event;
		ug_man.is_landscape = 1;
		break;
	default:
		is_rotation = 0;
	}

	ugman_idler_add((Idle_Cb)ugman_send_event_pre, (void *)event);

	if (is_rotation && ug_man.fv_top)
		ugman_indicator_update(ug_man.fv_top->opt, event);

	return 0;
}

static int ugman_send_key_event_to_ug(ui_gadget_h ug,
				      enum ug_key_event event)
{
	struct ug_module_ops *ops = NULL;

	if (!ug)
		return -1;

	if (ug->module) {
		ops = &ug->module->ops;
	} else {
		return -1;
	}

	if (ops && ops->key_event) {
		ops->key_event(ug, event, ug->app_control, ops->priv);
	}

	return 0;
}

int ugman_send_key_event(enum ug_key_event event)
{
	if (!ug_man.is_initted) {
		_ERR("ugman_send_key_event failed: manager is not initted");
		return -1;
	}

	if (!ug_man.fv_top || !ugman_ug_exist(ug_man.fv_top)
	    || ug_man.fv_top->state == UG_STATE_DESTROYED) {
		_ERR("ugman_send_key_event failed: full view top UG is invalid");
		return -1;
	}

	return ugman_send_key_event_to_ug(ug_man.fv_top, event);
}

int ugman_send_message(ui_gadget_h ug, app_control_h msg)
{
	struct ug_module_ops *ops = NULL;
	if (!ug || !ugman_ug_exist(ug) || ug->state == UG_STATE_DESTROYED) {
		_ERR("ugman_send_message failed: Invalid ug(%p)", ug);
		errno = EINVAL;
		return -1;
	}

	if (!msg) {
		_ERR("ugman_send_message failed: Invalid msg");
		errno = EINVAL;
		return -1;
	}

	if (ug->module)
		ops = &ug->module->ops;

	if (ops && ops->message)
		ops->message(ug, msg, ug->app_control, ops->priv);

	return 0;
}

void *ugman_get_window(void)
{
	return ug_man.win;
}

void *ugman_get_conformant(void)
{
	struct ug_engine_ops *eng_ops = NULL;
	void* ret = NULL;

	if (ug_man.engine) {
		eng_ops = &ug_man.engine->ops;
	} else {
		_WRN("ui engine is not loaded");
		return NULL;
	}

	if (eng_ops && eng_ops->create) {
		ret = eng_ops->request(ug_man.win, NULL, UG_UI_REQ_GET_CONFORMANT);
	} else {
		_WRN("ui engine is not loaded");
	}

	return ret;
}

static inline void job_start(void)
{
	ug_man.walking++;
}

static inline void job_end(void)
{
	ug_man.walking--;

	if (!ug_man.walking && ug_man.destroy_all) {
		ug_man.destroy_all = 0;
		if (ug_man.root) {
			_DBG("ug_destroy_all pneding job exist. ug_destroy_all begin");
			ugman_ug_del_all();
		}
	}

	if (ug_man.walking < 0)
		ug_man.walking = 0;
}

int ugman_ug_exist(ui_gadget_h ug)
{
	return ugman_ug_find(ug_man.root, ug);
}

#ifdef ENABLE_UG_CREATE_CB
int ugman_create_cb(void (*create_cb)(char*,char*,char*,void*), void *user_data)
{
	if(create_cb == NULL) {
		_DBG("disable trace create cb");
		g_create_cb_user_data = NULL;
		g_create_cb = NULL;
	} else {
		_DBG("enable trace create cb(%p)", create_cb);
		g_create_cb_user_data = user_data;
		g_create_cb = create_cb;
	}

	return 0;
}
#endif

