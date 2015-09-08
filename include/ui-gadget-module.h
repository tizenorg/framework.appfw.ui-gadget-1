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

#ifndef __UI_GADGET_MODULE_H__
#define __UI_GADGET_MODULE_H__

#include "ui-gadget.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ui-gadget-module.h
 * @brief This file contains a module to develop a UI gadget. Callees (UI gadgets) use this modules and APIs. (callee -> caller).
 *
 * @section Header to use them:
 * @code
 * #include <ui-gadget-module.h>
 * @endcode
 */

/**
 * @internal
 * @addtogroup CORE_LIB_GROUP_UI_GADGET_MODULE_MODULE
 * @{
 */

/**
 * @brief The Structure type for UI gadget module operation type.
 * @since_tizen 2.3
 * @see @ref lifecycle_sec
 */
struct ug_module_ops {
    void *(*create) (ui_gadget_h ug, enum ug_mode mode, app_control_h app_control,
                    void *priv);                                                                   /**< Create operation */
    void (*start) (ui_gadget_h ug, app_control_h app_control, void *priv);                         /**< Start operations */
    void (*pause) (ui_gadget_h ug, app_control_h app_control, void *priv);                         /**< Pause operations */
    void (*resume) (ui_gadget_h ug, app_control_h app_control, void *priv);                        /**< Resume operations */
    void (*destroy) (ui_gadget_h ug, app_control_h app_control, void *priv);                       /**< Destroy operations */
    void (*message) (ui_gadget_h ug, app_control_h msg, app_control_h app_control, void *priv);    /**< Message operations */
    void (*event) (ui_gadget_h ug, enum ug_event event, app_control_h app_control,
                void *priv);                                                                       /**< Event operations */
    void (*key_event) (ui_gadget_h ug, enum ug_key_event event,
                    app_control_h app_control, void *priv);                                        /**< Key event operations */
    void (*destroying) (ui_gadget_h ug, app_control_h app_control, void *priv);                    /**< Destroying operations */
    void *reserved[3];                                                                             /**< Reserved operations */

    void *priv;                                                                                    /**< Private data */

    enum ug_option opt;                                                                            /**< Option */
};

/**
 * @brief Makes a request that the caller of the given UI gadget instance destroys the instance.
 *        It just makes a request, but does not destroy the UI gadget.
 *
 * @details @b Purpose: This function is used to send a request that the caller of the given UI gadget instance destroys the instance.
 *
 * @details @b Typical @b use @b case: The UI gadget developer who wants to send a request that the caller of the given UI gadget instance destroys the instance can use this function.
 *
 * @details @b Method @b of @b function @b operation: Destroys the callback which is registered by the caller with ug_create() is invoked.
 *
 * @details @b Context @b of @b function: This function should be called in the created UI gadget.
 *
 * @since_tizen 2.3
 *
 * @remarks The API just makes a request, but does not destroy the UI gadget.
 *
 * @param[in] ug The UI gadget
 *
 * @return  @c 0 on success, 
 *          otherwise @c -1 on error
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget-module.h>
 * ...
 * // send a "destroy me" request
 * ug_destroy_me(ug);
 * ...
 * @endcode.
 */
int ug_destroy_me(ui_gadget_h ug);

/**
 * @brief Sends the result to the caller of the given UI gadget instance.
 *
 * @details @b Purpose: This function is used to send the result to the caller of the given UI gadget instance. The result has to be composed with a service handle.
 *
 * @details @b Typical @b use @b case: The UI gadget developer who wants to send the result to the caller of the given UI gadget instance can use this function.
 *
 * @details @b Method @b of @b function @b operation: Result callback which is registered by ug_create() is invoked.
 *
 * @details @b Context @b of @b function: This function is supposed to be called in the created UI gadget.
 *
 * @since_tizen 2.3
 *
 * @remarks After your message is sent, app_control_destroy() should be released.
 *
 * @param[in] ug   The UI gadget
 * @param[in] send The result, which is a service type (see @ref app_control_PG "Tizen managed api reference guide")
 *
 * @return  @c 0 on success, 
 *          otherwise -1 on error
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget-module.h>
 * ...
 * // make a result with service
 * app_control_h result;
 * app_control_create(&result);
 * app_control_add_extra_data(result, "Content", "Hello");
 *
 * // send the result
 * ug_send_result(ug, result);
 *
 * // release the result
 * app_control_destroy(result);
 * ...
 * @endcode.
 */
int ug_send_result(ui_gadget_h ug, app_control_h send);


/**
 * @brief Sends the result to the caller of the given UI gadget instance.
 *
 * @details @b Purpose: This function is used for sending the result to caller of the given UI gadget instance. The result has to be composed with the service handle.
 *
 * @details @b Typical @b use @b case: The UI gadget developer who wants to send the result to the caller of the given UI gadget instance can use this function.
 *
 * @details @b Method @b of @b function @b operation: Result callback which is registered by caller with ug_create() is invoked.
 *
 * @details @b Context @b of @b function: This function is supposed to be called in the created UI gadget.
 *
 * @since_tizen 2.3
 *
 * @remarks After your message is sent, app_control_destroy() should be released.
 *
 * @param[in] ug      The UI gadget
 * @param[in] send    The Service handle in which the results of the callee \n
 *                    See @ref app_control_PG "Tizen managed api reference guide".
 * @param[in] result  The result code of the launch request \n
 *                    This is valid in case that @a ug is launched by appcontrol.
 *
 * @return  @c 0 on success, 
 *          otherwise @c -1 on error
 *
 * @par Sample code:
 * @code
 * #include <ui-gadget-module.h>
 * ...
 * // make a result with service
 * app_control_h result;
 * app_control_create(&result);
 * app_control_add_extra_data(result, "Content", "Hello");
 *
 * // send the result
 * ug_send_result_full(ug, result, APP_CONTROL_RESULT_SUCCEEDED);
 *
 * // release the result
 * app_control_destroy(result);
 * ...
 * @endcode
 */
int ug_send_result_full(ui_gadget_h ug, app_control_h send, app_control_result_e result);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif              /* __UI_GADGET_MODULE_H__ */
