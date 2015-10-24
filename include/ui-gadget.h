/*
 *  UI Gadget
 *
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Jinwoo Nam <jwoo.nam@samsung.com>
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

#ifndef __UI_GADGET_H__
#define __UI_GADGET_H__

#include <app.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file ui-gadget.h
 * @brief This file contains a module to use a UI gadget. Caller uses this module and APIs.
 *
 * @section Header To Use Them:
 * @code
 * #include <ui-gadget.h>
 * @endcode
 */

/**
 * @addtogroup CORE_LIB_GROUP_UI_GADGET_MODULE
 * @{
 */

/**
 * @brief The structure type for UI gadget handle.
 *
 * @since_tizen 2.3
 *
 * @see ug_create()
 * @see ug_destroy()
 * @see ug_get_layout()
 * @see ug_get_parent_layout()
 * @see ug_get_mode()
 */
typedef struct ui_gadget_s *ui_gadget_h;

/**
 * @brief Enumeration for UI gadget mode.
 *
 * @since_tizen 2.3
 *
 * @see ug_create()
 * @see ug_get_mode()
 */
enum ug_mode {
    UG_MODE_FULLVIEW,   /**< Fullview mode */
    UG_MODE_FRAMEVIEW,  /**< Frameview mode */
    UG_MODE_INVALID,    /**< Invalid mode */
    UG_MODE_MAX         /**< Max value */
};

/**
 * @brief Enumeration for UI gadget event.
 *
 * @since_tizen 2.3
 *
 * @see ug_send_event()
 */
enum ug_event {
    UG_EVENT_NONE = 0x00,                    /**< No event */
    UG_EVENT_LOW_MEMORY,                     /**< Low memory event */
    UG_EVENT_LOW_BATTERY,                    /**< Low battery event */
    UG_EVENT_LANG_CHANGE,                    /**< Language change event */
    UG_EVENT_ROTATE_PORTRAIT,                /**< Rotate event: Portrait */
    UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN,     /**< Rotate event: Portrait upsidedown */
    UG_EVENT_ROTATE_LANDSCAPE,               /**< Rotate event: Landscape */
    UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN,    /**< Rotate event: Landscape upsidedown */
    UG_EVENT_REGION_CHANGE,                  /**< Region change event */
    UG_EVENT_MAX                             /**< Max value */
};

/**
 * @brief Enumeration for UI gadget key event.
 *
 * @since_tizen 2.3
 *
 * @see ug_send_key_event()
 */
enum ug_key_event {
    UG_KEY_EVENT_NONE = 0x00,   /**< No event */
    UG_KEY_EVENT_END,           /**< End key event */
    UG_KEY_EVENT_MAX            /**< Max value */
};

/**
 * @brief Enumeration for UI gadget option.
 *
 * @since_tizen 2.3
 *
 * @see ug_init()
 */
enum ug_option {
    UG_OPT_INDICATOR_ENABLE = 0x00,
            /**< Indicator option:
            Enable with both portrait and landscape window */
    UG_OPT_INDICATOR_PORTRAIT_ONLY = 0x01,
            /**< Indicator option: Enable with portrait window */
    UG_OPT_INDICATOR_LANDSCAPE_ONLY = 0x02,
            /**< Indicator option: Enable with landscape window */
    UG_OPT_INDICATOR_DISABLE = 0x03,
            /**< Indicator option:
            Disable with both portrait and landscape view window */
    UG_OPT_INDICATOR_MANUAL = 0x04,
            /**< Indicator option:
            Indicator will be handled manually */
    UG_OPT_MAX    /**< Max value */
};

/**
 * @brief Definition for key string for the extra data of app_control that is used for result state of ui-gadget. 
 *        When ug callee (UI gadgets) sends result state using ug_send_result_full(), the ug caller can get the 
 *        result state from ug callee in result parameter of the result_cb().
 *
 * @since_tizen 2.3
 */
#define UG_APP_CONTROL_DATA_RESULT "__UG_SEND_RESULT__"



/**
 * @brief The Structure type for UI gadget callback type.
 *
 * @since_tizen 2.3
 *
 * @see ug_create()
 */
struct ug_cbs {
    void (*layout_cb) (ui_gadget_h ug, enum ug_mode mode,
                void *priv);
    /**< Layout callback */
    void (*result_cb) (ui_gadget_h ug, app_control_h result, void *priv);
    /**< Result callback */
    void (*destroy_cb) (ui_gadget_h ug, void *priv);
    /**< Destroy callback */
    void (*end_cb) (ui_gadget_h ug, void *priv);
    /**< End callback */
    void *priv;
    /**< Private data */
    void *reserved[3];
    /**< Reserved operations */
};

/**
 * @brief Definition for easy-to-use api of ug_init() for EFL.
 *
 * @since_tizen 2.3
 *
 * @see ug_init()
 */
int UG_INIT_EFL(void *win, enum ug_option opt);

/**
 * @brief Initializes default window, display, xwindow ID and indicator state.
 *
 * @details @b Purpose: First of all, to use UI gadgets in an application, the default window to draw the 
 *          UI gadgets on has to be registered. Besides, to change the indicator state for the full-view UI 
 *          gadget, display and xwindow ID have to be registered, and to restore the application's indicator 
 *          state, default indicator option has to be registered. This function is used for registering them.
 *
 * @details @b Typical @b use @b case: Application developers who want to use UI gadget MUST register display, 
 *          xwindow ID, default window, and option with the function at first.
 *
 * @details @b Method @b of @b function @b operation: Register display, xwindow ID, default window, and option.
 *
 * @details @b Context @b of @b function: None
 *
 * @since_tizen 2.3
 *
 * @remarks If you are unfamiliar with display and xwindow ID, please use following api: UG_INIT_EFL. 
 *          The macros kindly generate proper functions to get display and xwindow ID.
 *
 * @param[in] disp  The default display
 * @param[in] xid   The default xwindow ID of default window
 * @param[in] win   The Default window object, it is void pointer for supporting both GTK (GtkWidget *) and EFL (Evas_Object *)
 * @param[in] opt   The Default indicator state to restore application's indicator state
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @see UG_INIT_EFL()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * Evas_Object *win;
 * ...
 * // create window
 * ...
 * ug_init((Display *)ecore_x_display_get(), elm_win_xwindow_get(win), win, UG_OPT_INDICATOR_ENABLE);
 * // for convenience you can use following macro: UG_INIT_EFL(win, UG_OPT_INDICATOR_ENABLE);
 * ...
 * @endcode
 */
int ug_init(void *disp, unsigned long xid, void *win, enum ug_option opt);

/**
 * @brief Creates a UI gadget.
 *
 * @details @b Purpose: This function is used to create a UI gadget instance. In addition, the following 
 *          callbacks can be registered with the function: layout callback, result callback, and destroy callback. (See struct ug_cbs)
 *
 * @details @b Typical @b use @b case: Anyone who wants to create a UI gadget can use this function.
 *
 * @details @b Method @b of @b function @b operation: First, the UI gadget with the given name is dynamically loaded(dlopen). 
 *          Next, state operations of loaded UI gadget are invoked according to its lifecycle. There are three callbacks 
 *          which can be registered with the function: layout callback, result callback, and destroy callback. If the state 
 *          is changed to "Create", the layout callback is invoked for layout arrangement. If ug_send_result() is invoked 
 *          in the loaded UI gadget, the result callback is invoked. If ug_destroy_me() is invoked in the loaded UI gadget, 
 *          the destroy callback is invoked.
 *
 * @details @b Context @b of @b function: This function should be called after successful initialization by ug_init().
 *
 * @since_tizen 2.3
 *
 * @remarks If "app_control" is passed, app_control_destroy() should be released after ug_create().
 *
 * @param[in] parent       The parent's UI gadget \n
 *                         If the UI gadget uses the function, the parent has to be the UI gadget \n
 *                         Otherwise, if an application uses the function, the parent has to be @c NULL.
 * @param[in] name         The name of the UI gadget
 * @param[in] mode         The mode of the UI gadget (UG_MODE_FULLVIEW | UG_MODE_FRAMEVIEW)
 * @param[in] app_control  The argument for the UI gadget (see @ref app_control_PG "Tizen managed api reference guide")
 *
 * @return  The pointer of UI gadget,
 *          otherwise @c NULL on error
 *
 * @pre ug_init() should be called.
 *
 * @see struct ug_cbs
 * @see ug_mode
 *
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * app_control_h app_control;
 * ui_gadget_h ug;
 * struct ug_cbs cbs = {0, };
 *
 * // set callbacks: layout callback, result callback, destroy callback
 * cbs.layout_cb = _layout_cb;
 * cbs.result_cb = _result_cb;
 * cbs.destroy_cb = _destroy_cb;
 * cbs.priv = user_data;
 *
 * // create arguments
 * app_control_create(&app_control);
 * app_control_add_extra_data(app_control, "Content", "Hello");
 *
 * // create "helloUG-efl" UI gadget instance
 * ug = ug_create(NULL, "helloUG-efl", UG_MODE_FULLVIEW, app_control, &cbs);
 *
 * // release arguments
 * app_control_destroy(bontext @b of @b function:

 * ...
 * @endcode
 */
ui_gadget_h ug_create(ui_gadget_h parent, const char *name,
                    enum ug_mode mode, app_control_h app_control,
                    struct ug_cbs *cbs);

/**
 * @brief Pauses all UI gadgets.
 *
 * @details @b Purpose: This function is used to pause UI gadgets in the "Running" state. Eventually, the state of the UI gadgets will be "Stopped".
 *
 * @details @b Typical @b use @b case: Application developers who want to pause loaded UI gadgets can use this function.
 *
 * @details @b Method @b of @b function @b operation: "Pause" state operations of UI gadgets with the "Running" state in the 
 *          UI gadget tree are invoked by post-order traversal.
 *
 *          @b Context @b of @b function: This function is supposed to be called after successful initialization with ug_init().
 *
 * @since_tizen 2.3
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called.
 *
 * @see ug_resume()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // pause all UI gadget instances
 * ug_pause();
 * ...
 * @endcode
 */
int ug_pause(void);

/**
 * @brief Resumes all UI gadgets.
 *
 * @details @b Purpose: This function is used for resuming UI gadgets in the "Stopped" state. Eventually, the state of all UI gadgets will be "Running".
 *
 * @details @b Typical @b use @b case: Application developers who want to resume loaded UI gadgets can use this function.
 *
 * @details @b Method @b of @b function @b operation: "Resume" state operations of UI gadgets with the "Stopped" state in the UI gadget tree are invoked by post-order traversal.
 *
 * @details @b Context @b of @b function: This function should be called after successful initialization by ug_init().
 *
 * @since_tizen 2.3
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called.
 *
 * @see ug_pause()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // resume all UI gadget instances
 * ug_resume();
 * ...
 * @endcode
 */
int ug_resume(void);

/**
 * @brief Destroys the given UI gadget instance.
 *
 * @details @b Purpose: This function is used for destroying the given UI gadget instance and its children. Eventually, the state of the instance will be "Destroyed".
 *
 * @details @b Typical @b use @b case: Anyone who want to destroy specific UI gadget can use this function.
 *
 * @details @b Method @b of @b function @b operation: "Destroy" state operations of the given UI gadget instance and its children are invoked.
 *
 * @details @b Context @b of @b function: This function is supposed to be called after successful initialization of ug_init() and creation of ug_create().
 *
 * @since_tizen 2.3
 *
 * @param[in] ug The UI gadget
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called \n
 *      ug_create() should be used to create the @a ug gadget.
 *
 * @see ug_destroy_all()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // destroy UI gadget instance
 * ug_destroy(ug);
 * ...
 * @endcode
 */
int ug_destroy(ui_gadget_h ug);

/**
 * @brief Destroys all UI gadgets of an application.
 *
 * @details @b Purpose: This function is used for destroying all UI gadgets. Eventually, the state of all UI gadgets will be "Destroyed".
 *
 * @b Typical @b use @b case: Application developers who want to destroy loaded UI gadgets can use this function.
 *
 * @details @b Method @b of @b function @b operation: "Destroy" state operations of all UI gadgets in the UI gadget tree are invoked by post-order traversal.
 *
 * @details @b Context @b of @b function: This function should be called after successful initialization of ug_init().
 *
 * @since_tizen 2.3
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called.
 *
 * @see ug_destroy()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // destroy all UI gadget instances
 * ug_destroy_all();
 * ...
 * @endcode
 */
int ug_destroy_all(void);

/**
 * @brief Gets the base layout of the given UI gadget instance.
 *
 * @details @b Purpose: This function is used to get the base layout pointer of the given UI gadget instance.
 *
 * @details @b Typical @b use @b case: Anyone who wants to get the base layout of a UI gadget can use this function.
 *
 * @details @b Method @b of @b function @b operation: This function returns the base layout pointer which is created in the "Create" operation of the given UI gadget instance.
 *
 * @details @b Context @b of @b function: This function should be called after successful initialization of ug_init() and creation of ug_create().
 *
 * @since_tizen 2.3
 *
 * @param[in] ug The UI gadget
 *
 * @return  The pointer of the base layout,
 *          otherwise @c NULL on error \n
 *          The result value is a void pointer that supports both GTK (GtkWidget *) and EFL (Evas_Object *).
 *
 * @pre ug_init() should be called. \n
 *      ug_create() should be used to create the @a ug gadget.
 *
 * @see ug_get_parent_layout()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * Evas_Object *ly;
 * // get a base layout
 * ly = (Evas_Object *)ug_get_layout(ug);
 * ...
 * @endcode
 */
void *ug_get_layout(ui_gadget_h ug);

/**
 * @brief Gets the base layout of the parent of the given UI gadget instance.
 *
 * @details @b Purpose: This function is used to get the base layout pointer of the parent of the given UI gadget instance.
 *
 * @details @b Typical @b use @b case: Anyone who wants to get the base layout of a UI gadget's parent can use this function.
 *
 * @details @b Method @b of @b function @b operation: This function returns the base layout pointer which is created in "Create" operation of parent of the given UI gadget instance.
 *
 * @details @b Context @b of @b function: This function is supposed to be called after successful initialization of ug_init() and creation of ug_create().
 *
 * @since_tizen 2.3
 *
 * @param[in] ug The UI gadget
 *
 * @return  The pointer of the base layout,
 *          otherwise @c NULL on error \n
            The result value is a void pointer that supports both GTK (GtkWidget *) and EFL (Evas_Object *).
 *
 * @pre ug_init() should be called. \n
 *      ug_create() should be used to create the @a ug gadget.
 *
 * @see ug_get_layout()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * Evas_Object *ly;
 * // get a base layout of parent of the given UI gadget instance
 * ly = (Evas_Object *)ug_get_parent_layout(ug);
 * ...
 * @endcode
 */
void *ug_get_parent_layout(ui_gadget_h ug);

/**
 * @brief Gets a default window.
 *
 * @details @b Purpose: This function is used to get the default window which is registered with ug_init().
 *
 * @details @b Typical @b use @b case: Anyone who want to get the default window can use this function.
 *
 * @details @b Method @b of @b function @b operation: This function returns the default window pointer which is registered with ug_init().
 *
 * @details @b Context @b of @b function: This function should be called after successful initialization of ug_init().
 *
 * @since_tizen 2.3
 *
 * @return  The pointer of a default window,
 *          otherwise @c NULL on error \n
 *           The result value is void pointer that supports both GTK (GtkWidget *) and EFL (Evas_Object *).
 *
 * @pre ug_init() should be called.
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * Evas_Object *win;
 * // get default window
 * win = (Evas_Object *)ug_get_window();
 * ...
 * @endcode
 */
void *ug_get_window(void);

/**
 * @brief Gets the ug conformant.
 *
 * @details @b Purpose: This function is used for getting the ug conformant.
 *
 * @details @b Typical @b use @b case: Anyone who wants to get the ug conformant can use this function.
 *
 * @details @b Method @b of @b function @b operation: This function returns the ug conformant pointer.
 *
 * @details @b Context @b of @b function: This function is supposed to be called after successful initialization with ug_init().
 *
 * @since_tizen 2.3
 *
 * @return  The pointer of the default window,
 *          otherwise @c NULL on error \n
 *          The result value is a void pointer for supporting both GTK (GtkWidget *) and EFL (Evas_Object *).
 *
 * @pre ug_init() should be called.
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * Evas_Object *conform;
 * // get default window
 * conform = (Evas_Object *)ug_get_conformant();
 * ...
 * @endcode
 */
void *ug_get_conformant(void);

/**
 * @brief Gets the mode of the given UI gadget instance.
 *
 * @details @b Purpose: This function is used to get the mode of the given UI gadget instance. The Mode can be UG_MODE_FULLVIEW or UG_MODE_FRAMEVIEW.
 *
 * @details @b Typical @b use @b case: Anyone who wants to get the mode of a UI gadget can use this function.
 *
 * @details @b Method @b of @b function @b operation: This function returns the mode which is registered with ug_create().
 *
 * @details @b Context @b of @b function: This function is supposed to be called after successful initialization of ug_init() and creation of ug_create().
 *
 * @since_tizen 2.3
 *
 * @param[in] ug The UI gadget
 *
 * @return  The UI gadget mode of the given UI gadget instance (UG_MODE_FULLVIEW | UG_MODE_FRAMEVIEW)
 *
 * @pre ug_init() should be called, \n
 *      ug_create() should be used to create the @a ug gadget.
 *
 * @see enum ug_mode
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * enum ug_mode mode;
 * // get mode (UG_MODE_FULLVIEW | UG_MODE_FRAMEVIEW)
 * mode = ug_get_mode(ug);
 * ...
 * @endcode
 */
enum ug_mode ug_get_mode(ui_gadget_h ug);

/**
 * @brief Propagates the given system event to all UI gadgets.
 *
 * @details @b Purpose: This function is used to propagate the given system event. Available system events are low memory, low battery, language changed and window rotate event.
 *
 * @details @b Typical @b use @b case: Application developers who want to propagate system event to all UI gadgets can use this function.
 *
 * @details @b Method @b of @b function @b operation: Event operations of all UI gadgets in the UI gadget tree are invoked by post-order traversal.
 *
 * @details @b Context @b of @b function: This function is supposed to be called after successful initialization with ug_init().
 *
 * @since_tizen 2.3
 *
 * @param[in]  The event UI gadget event (see enum ug_event)
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called.
 *
 * @see enum ug_event
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // propagate low battery event to all UI gadget instances
 * ug_send_event(UG_EVENT_LOW_BATTERY);
 * ...
 * @endcode
 */
int ug_send_event(enum ug_event event);

/**
 * @brief Sends key event to full view top UI gadget.
 *
 * @details @b Purpose: This function is used to send a key event to the full view top UI gadget. Available key events are end events.
 *
 * @details @b Typical @b use @b case: Application developers who want to send key event to full view top UI gadget can use this function.
 *
 * @details @b Method @b of @b function @b operation: Key event operation of full view top UI gadget in the UI gadget tree are invoked.
 *
 * @details @b Context @b of @b function: This function is supposed to be called after successful initialization with ug_init().
 *
 * @since_tizen 2.3
 *
 * @param[in] event The UI gadget key event (see enum ug_key_event)
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called.
 *
 * @see enum ug_key_event
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // send key event callback to full view top UI gadget instances
 * ug_send_key_event(UG_KEY_EVENT_END);
 * ...
 * @endcode
 */
int ug_send_key_event(enum ug_key_event event);

/**
 * @brief Sends a message to the given UI gadget instance.
 *
 * @details @b Purpose: This function is used for sending a message to a created UI gadget. The message has to be composed with app_control handle.
 *
 * @details @b Typical @b use @b case: Anyone who wants to send a message to a created UI gadget.
 *
 * @details @b Method @b of @b function @b operation: Message operation of the given UI gadget instance is invoked.
 *
 * @details @b Context @b of @b function: This function is supposed to be called after successful initialization with ug_init() and creation UI gadget with ug_create().
 *
 * @since_tizen 2.3
 *
 * @remarks After message is sent, app_control_destroy() should be released.
 *
 * @param[in] ug   The UI gadget
 * @param[in] msg  The Message to send, which is app_control type (see @ref app_control_PG "Tizen managed api reference guide")
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called. \n
 *      ug_create() should be used to create the @a ug gadget.
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // make a message with app_control
 * app_control_h msg;
 * app_control_create(&msg)
 * app_control_add_extra_data(msg, "Content", "Hello");
 *
 * // send the message
 * ug_send_message(ug, msg);
 *
 * // release the message
 * app_control_destroy(msg);
 * ...
 * @endcode
 */
int ug_send_message(ui_gadget_h ug, app_control_h msg);

/**
 * @brief Disables the transition effect of the given UI gadget instance.
 *
 * @details @b Purpose: This function is used for disabling the transition effect of a created UI gadget.
 *
 * @details @b Typical @b use @b case: Anyone who wants to disable the transition effect of a created UI gadget.
 *
 * @details @b Method @b of @b function @b operation: No transition effect of the given UI gadget is invoked.
 *
 * @details @b Context @b of @b function: This function should be called after successful initialization of ug_init() and creation of ug_create().
 *
 * @since_tizen 2.3
 *
 * @remarks Before showing layout of given UI gadget, ug_disable_effect() should be called.
 *
 * @param[in] ug The UI gadget
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called. \n
 *      ug_create() should be used to create the @a ug gadget.
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * static void layout_cb(ui_gadget_h ug, enum ug_mode mode, void *priv)
 * {
 * ...
 * base = ug_get_layout(ug);
 * switch (mode) {
 * case UG_MODE_FULLVIEW:
 * // disable effect
 * ug_disable_effect(ug);
 * evas_object_show(base);
 * ...
 * @endcode
 */
int ug_disable_effect(ui_gadget_h ug);

/**
 * @brief Checks whether the given ug is installed.
 *
 * @details @b Purpose: This function is used for checking whether the given ug is installed or not
 *
 * @details @b Typical @b use @b case: Anyone who wants to know whether the given ug is installed or not.
 *
 * @details @b Method @b of @b function @b operation: This function returns a value indicating whether the ug is installed or not.
 *
 * @details @b Context @b of @b function: N/A.
 *
 * @since_tizen 2.3
 *
 * @param[in] name The UI gadget's name
 *
 * @return  @c 1 if installed,
 *          @c 0 if not installed,
 *          otherwise @c -1 in case of error
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * ret = ug_is_installed(ug);
 * @endcode
 */
int ug_is_installed(const char *name);

/**
 * @brief Pauses the given UI gadget instance.
 *
 * @details @b Purpose: This function is used to pause the specified UI gadget instance and its children in the "Running" state. Eventually, the state of the UI gadget instance will be "Stopped".
 *
 * @details @b Typical @b use @b case: Application developers who want to pause the specified UI gadget instance can use this function.
 *
 * @details @b Method @b of @b function @b operation: "Pause" state operations of the UI gadgets with the "Running" state in the sub-tree that has the specified UI gadget as the root node are invoked by post-order traversal.
 *
 *          @b Context @b of @b function: This function is supposed to be called after successful initialization with ug_init() and creation of ug_create().
 *
 * @since_tizen 2.4
 *
 * @param[in] ug The UI gadget
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called.
 *	ug_create() should be used to create the @a ug gadget.
 *
 * @see ug_resume_ug()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // pauses the given UI gadget instance.
 * ug_pause_ug(ug);
 * ...
 * @endcode
 */
int ug_pause_ug(ui_gadget_h ug);

/**
 * @brief Resumes the given UI gadget instance.
 *
 * @details @b Purpose: This function is used to resume the specified UI gadget instance and its children in the "Stopped" state. Eventually, the state of the UI gadget instance will be "Running".
 *
 * @details @b Typical @b use @b case: Application developers who want to resume the specified UI gadget instance can use this function.
 *
 * @details @b Method @b of @b function @b operation: "Resume" state operations of the UI gadgets with the "Stopped" state in the sub-tree that has specified UI gadget as the root node are invoked by post-order traversal.
 *
 * @details @b Context @b of @b function: This function should be called after successful initialization by ug_init() and creation of ug_create().
 *
 * @since_tizen 2.4
 *
 * @param[in] ug The UI gadget
 *
 * @return  @c 0 on success,
 *          otherwise @c -1 on error
 *
 * @pre ug_init() should be called.
 *	ug_create() should be used to create the @a ug gadget.
 *
 * @see ug_pause_ug()
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget.h>
 * ...
 * // resumes the given UI gadget instance.
 * ug_resume_ug(ug);
 * ...
 * @endcode
 */
int ug_resume_ug(ui_gadget_h ug);

/**
 * @}
 */

#ifdef __cplusplus
}

#endif


#endif              /* __UI_GADGET_H__ */
