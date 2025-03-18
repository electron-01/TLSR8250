/**
 * @file tuya_ble_event.h
 * @brief This is tuya_ble_event file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2023 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TUYA_BLE_EVENT_H__
#define __TUYA_BLE_EVENT_H__

#include "board.h"
#include "tuya_cloud_types.h"
#include "tuya_ble_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (!TUYA_BLE_USE_OS)

/***********************************************************************
 ********************* constant ( macro and enum ) *********************
 **********************************************************************/
#define TUYA_BLE_EVT_MAX_NUM         MAX_NUMBER_OF_TUYA_MESSAGE

#ifndef TUYA_BLE_EVT_SIZE
#define TUYA_BLE_EVT_SIZE             52 //64
#endif

enum {
    TUYA_BLE_EVT_SEND_SUCCESS      = 0,
    TUYA_BLE_EVT_SEND_NO_MEMORY    = 1,
    TUYA_BLE_EVT_SEND_FAIL         = 2,
};

#define TUYA_BLE_ERROR_HANDLER(ERR_CODE)

#define TUYA_BLE_ERROR_CHECK(ERR_CODE)

#define TUYA_BLE_ERROR_CHECK_BOOL(BOOLEAN_VALUE)

#define CEIL_DIV(A, B)      \
    (((A) + (B) - 1) / (B))

/**@brief Compute number of bytes required to hold the scheduler buffer.
 *
 * @param[in] EVENT_SIZE   Maximum size of events to be passed through the scheduler.
 * @param[in] QUEUE_SIZE   Number of entries in scheduler queue (i.e. the maximum number of events
 *                         that can be scheduled for execution).
 *
 * @return    Required scheduler buffer size (in bytes).
 */
#define TUYA_BLE_SCHED_BUF_SIZE(EVENT_SIZE, QUEUE_SIZE)                                                 \
            ((EVENT_SIZE) * ((QUEUE_SIZE) + 1))

/**@brief Macro for initializing the event scheduler.
 *
 * @details It will also handle dimensioning and allocation of the memory buffer required by the
 *          scheduler, making sure the buffer is correctly aligned.
 *
 * @param[in] EVENT_SIZE   Maximum size of events to be passed through the scheduler.
 * @param[in] QUEUE_SIZE   Number of entries in scheduler queue (i.e. the maximum number of events
 *                         that can be scheduled for execution).
 *
 * @note Since this macro allocates a buffer, it must only be called once (it is OK to call it
 *       several times as long as it is from the same location, e.g. to do a reinitialization).
 */
#define TUYA_BLE_SCHED_INIT(EVENT_SIZE, QUEUE_SIZE)                                                     \
    do                                                                                             \
    {                                                                                              \
        STATIC UINT32_T TUYA_BLE_SCHED_BUF[CEIL_DIV(TUYA_BLE_SCHED_BUF_SIZE((EVENT_SIZE), (QUEUE_SIZE)),     \
                                               SIZEOF(UINT32_T))];                                 \
        UINT32_T ERR_CODE = tuya_ble_sched_init((EVENT_SIZE), (QUEUE_SIZE), TUYA_BLE_SCHED_BUF);            \
        TUYA_BLE_ERROR_CHECK(ERR_CODE);                                                                 \
    } while (0)

/***********************************************************************
 ********************* struct ******************************************
 **********************************************************************/


/***********************************************************************
 ********************* variable ****************************************
 **********************************************************************/


/***********************************************************************
 ********************* function ****************************************
 **********************************************************************/

/**@brief Function for initializing the Scheduler.
 *
 * @details It must be called before entering the main loop.
 *
 * @param[in]   max_event_size   Maximum size of events to be passed through the scheduler.
 * @param[in]   queue_size       Number of entries in scheduler queue (i.e. the maximum number of
 *                               events that can be scheduled for execution).
 * @param[in]   p_evt_buffer   Pointer to memory buffer for holding the scheduler queue. It must
 *                               be dimensioned using the APP_SCHED_BUFFER_SIZE() macro. The buffer
 *                               must be aligned to a 4 byte boundary.
 *
 * @note Normally initialization should be done using the TUYA_SCHED_INIT() macro, as that will both
 *       allocate the scheduler buffer, and also align the buffer correctly.
 *
 * @retval      NRF_SUCCESS               Successful initialization.
 * @retval      NRF_ERROR_INVALID_PARAM   Invalid parameter (buffer not aligned to a 4 byte
 *                                        boundary).
 */
//UINT32_T tuya_ble_sched_init(UINT16_T max_event_size, UINT16_T queue_size, VOID_T * p_evt_buffer);

/**
 * @brief Function for executing all scheduled events.
 *
 * @details This function must be called from within the main loop. It will execute all events
 *          scheduled since the last time it was called.
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_sched_execute(VOID_T);

/**@brief Function for scheduling an event.
 *
 * @details Puts an event into the event queue.
 *
 * @param[in]   p_event_data   Pointer to event data to be scheduled.
 * @param[in]   event_size     Size of event data to be scheduled.
 * @param[in]   handler        Event handler to receive the event.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
//tuya_ble_status_t tuya_ble_sched_event_put(VOID_T CONST  * p_event_data, UINT16_T  event_data_size);

/**@brief Function for getting the current amount of free space in the queue.
 *
 * @details The real amount of free space may be less if entries are being added from an interrupt.
 *          To get the sxact value, this function should be called from the critical section.
 *
 * @return Amount of free space in the queue.
 */
UINT16_T tuya_ble_sched_queue_size_get(VOID_T);

/**
 * @brief tuya_ble_sched_queue_space_get
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT16_T tuya_ble_sched_queue_space_get(VOID_T);

/**
 * @brief tuya_ble_sched_queue_events_get
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
UINT16_T tuya_ble_sched_queue_events_get(VOID_T);

/**
 * @brief tuya_ble_event_queue_init
 *
 * @param[in] param: none
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
VOID_T tuya_ble_event_queue_init(VOID_T);

/**
 * @brief tuya_ble_message_send
 *
 * @param[in] *evt: *evt
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
tuya_ble_status_t tuya_ble_message_send(tuya_ble_evt_param_t *evt);

#endif


#ifdef __cplusplus
}
#endif

#endif /* __TUYA_BLE_EVENT_H__ */

