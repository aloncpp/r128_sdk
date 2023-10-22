/*
 * Copyright (c) 2017, Realtek Semiconductor Corporation. All rights reserved.
 */

#ifndef __GAP_BOND_LEGACY_H__
#define __GAP_BOND_LEGACY_H__

#ifdef __cplusplus
extern "C"
{
#endif
#if F_BT_BREDR_SUPPORT
#include "gap.h"

/**
 * gap_bond_legacy.h
 *
 * \brief   GAP link key change type.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef enum
{
    GAP_LINK_KEY_ADD,
    GAP_LINK_KEY_DELETE
} T_GAP_LINK_KEY_INFO_TYPE;

/**
 * gap_bond_legacy.h
 *
 * \brief    Response of BR/EDR authentication request. It will be received in the
 *           callback function registered by \ref legacy_register_cb with message
 *           type as \ref LEGACY_PAIR_RSP.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint8_t         bd_addr[6]; /**< Bluetooth address of remote device */
    uint16_t        cause;      /**< Result of authentication procedure */
} T_GAP_AUTHEN_RSP;

/**
 * gap_bond_legacy.h
 *
 * \brief    Response of user passkey request reply. It will be received in the
 *           callback function registered by \ref legacy_register_cb with message
 *           type as \ref PASSKEY_REQ_REPLY_RSP.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint16_t    cause;      /**< Result of user passkey request reply. */
} T_GAP_USER_PASSKEY_REQ_REPLY_RSP;

/**
 * gap_bond_legacy.h
 *
 * \brief    Response of keypress notification request. It will be received in the
 *           callback function registered by \ref legacy_register_cb with message
 *           type as \ref KEYPRESS_NOTIF_RSP.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint16_t                    cause;              /**< Keypress notification result. */
    uint8_t                     bd_addr[6];         /**< Bluetooth address of remote device. */
    T_GAP_REMOTE_ADDR_TYPE      remote_addr_type;   /**< Address type of remote device. */
} T_GAP_KEYPRESS_NOTIF_RSP;

/**
 * gap_bond_legacy.h
 *
 * \brief    Information of remote keypress notification. It will be received in
 *           the callback function registered by \ref legacy_register_cb with message
 *           type as \ref KEYPRESS_NOTIF_INFO.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint8_t                     bd_addr[6];         /**< Bluetooth address of remote device. */
    T_GAP_REMOTE_ADDR_TYPE      remote_addr_type;   /**< Address type of remote device. */
    T_GAP_KEYPRESS_NOTIF_TYPE   event_type;         /**< Keypress notification type. */
} T_GAP_KEYPRESS_NOTIF_INFO;

/**
 * gap_bond_legacy.h
 *
 * \brief    Indication to request BR/EDR OOB data of remote device. It will be
 *           received in the callback function registered by \ref legacy_register_cb
 *           with message type as \ref LEGACY_OOB_INPUT.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint8_t         bd_addr[6];     /**< Bluetooth address of remote device */
} T_GAP_LEGACY_REMOTE_OOB_REQ_IND;

/**
 * gap_bond_legacy.h
 *
 * \brief    Indication to request user confirmation when pairing. It will be received
 *           in the callback function registered by \ref legacy_register_cb with
 *           message type as \ref USER_CONFIRM_REQ.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint8_t                     bd_addr[6];         /**< Bluetooth address of remote device. */
    T_GAP_REMOTE_ADDR_TYPE      remote_addr_type;   /**< Address type of remote device. */
    uint32_t                    display_value;      /**< Numeric value to be displayed. */
} T_GAP_USER_CFM_REQ_IND;

/**
 * gap_bond_legacy.h
 *
 * \brief    Indication to request user to input passkey. It will be received in
 *           the callback function registered by \ref legacy_register_cb with message
 *           type as \ref PASSKEY_INPUT.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint8_t                     bd_addr[6];         /**< Bluetooth address of remote device. */
    T_GAP_REMOTE_ADDR_TYPE      remote_addr_type;   /**< Address type of remote device. */
    bool                        key_press;          /**< Whether keypress is needed. */
} T_GAP_USER_PASSKEY_REQ_IND;

/**
 * gap_bond_legacy.h
 *
 * \brief    Information of user passkey notification. It will be received in the
 *           callback function registered by \ref legacy_register_cb with message
 *           type as \ref PASSKEY_DISPLAY.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint8_t                     bd_addr[6];         /**< Bluetooth address of remote device. */
    T_GAP_REMOTE_ADDR_TYPE      remote_addr_type;   /**< Address type of remote device. */
    uint32_t                    display_value;      /**< Passkey value. */
} T_GAP_USER_PASSKEY_NOTIF_INFO;

/**
 * gap_bond_legacy.h
 *
 * \brief    Response of reading local BR/EDR OOB data. It will be received in the
 *           callback function registered by \ref legacy_register_cb with message
 *           type as \ref LOCAL_OOB_RSP.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    uint8_t         data_c[16];     /**< OOB data simple pairing hash C. */
    uint8_t         data_r[16];     /**< OOB data simple pairing randonizer R. */
    uint16_t        cause;          /**< Result of getting local OOB data. */
} T_GAP_LOCAL_OOB_RSP;

/**
 * gap_bond_legacy.h
 *
 * \brief    Infomation of link key change(add/delete). It will be received in the
 *           callback function registered by \ref legacy_register_cb with message
 *           type as \ref LINK_KEY_CHANGE_INFO.
 *
 * \ingroup  GAP_LEGACY_BOND
 */
typedef struct
{
    T_GAP_LINK_KEY_INFO_TYPE    type;       /**< Link key change type */
    uint8_t                     bd_addr[6]; /**< Bluetooth address of remtoe device */
} T_GAP_LINK_KEY_CHANGE_INFO;

/**
 * gap_bond_legacy.h
 *
 * \brief    Send a request to start BR/EDR pairing with remote device. When
 *           pairing is done, \ref LEGACY_PAIR_RSP message will be get in GAP
 *           legacy callback function registed by \ref legacy_register_cb with
 *           a parameter points to \ref T_GAP_AUTHEN_RSP to indicate the result of pairing.
 *           <b>Pairing procedure can only be started after receiving \ref PROTO_REG_CMPL
 *           message in GAP legacy callback.</b>
 *
 * \param[in]  bd_addr    Bluetooth address of remote device.
 *
 * \return    The status of sending pairing request.
 * \retval    GAP_CAUSE_SUCCESS            Request was sent successfully.
 * \retval    GAP_CAUSE_SEND_REQ_FAILED    Request was fail to send.
 *
 * <b>Example usage</b>
   \code{.c}
   // GAP legacy callback function.
   void legacy_cb(void *buf, T_LEGACY_MSG legacy_msg)
   {
       switch (legacy_msg)
       {
       case PROTO_REG_CMPL:
           {
               uint8_t bd_addr[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
               legacy_bond_pair(bd_addr);
           }
           break;

       case LEGACY_PAIR_RSP:
           {
               T_GAP_AUTHEN_RSP *p_rsp = (T_GAP_AUTHEN_RSP *)buf;

               APP_PRINT_TRACE2("Result of pairing whith %s is 0x%04x",
                                TRACE_BDADDR(p_rsp->bd_addr), p_rsp->cause);
           }
           break;

       default:
           break;
       }
   }

   int test(void)
   {
       legacy_register_cb(legacy_cb);

       // ...
       // Create app task.
       // Start BT stack.
       // ...

       return 0;
   }
   \endcode
 *
 * \ingroup  GAP_LEGACY_BOND
 */
T_GAP_CAUSE legacy_bond_pair(uint8_t *bd_addr);

/**
 * gap_bond_legacy.h
 *
 * \brief    Input a passkey or reject to input passkey when pairing with passkey entry. This is used when
 *           receive \ref PASSKEY_INPUT message in GAP legacy callback function
 *           registed by \ref legacy_register_cb.
 *
 * \param[in]  bd_addr    Bluetooth address of remote device.
 * \param[in]  passkey    Input passkey value.
 * \param[in]  cause      Accept or reject passkey input.
 *
 * \return    The status of sending passkey to Bluetooth stack.
 * \retval    GAP_CAUSE_SUCCESS            Passkey was sent successfully.
 * \retval    GAP_CAUSE_SEND_REQ_FAILED    Passkey was fail to send.
 *
 * <b>Example usage</b>
   \code{.c}
   // GAP legacy callback function.
   void legacy_cb(void *buf, T_LEGACY_MSG legacy_msg)
   {
       switch (legacy_msg)
       {
       case PASSKEY_INPUT:
           {
               uint32_t passkey = 888888;
               T_GAP_USER_PASSKEY_REQ_IND *p_ind = (T_GAP_USER_PASSKEY_REQ_IND *)buf;

               legacy_bond_input_passkey(p_ind->bd_addr, passkey, GAP_CFM_CAUSE_ACCEPT);
           }
           break;

       default:
           break;
       }
   }

   int test(void)
   {
       legacy_register_cb(legacy_cb);

       return 0;
   }
   \endcode
 *
 * \ingroup  GAP_LEGACY_BOND
 */
T_GAP_CAUSE legacy_bond_input_passkey(uint8_t *bd_addr, uint32_t passkey, T_GAP_CFM_CAUSE cause);

/**
 * gap_bond_legacy.h
 *
 * \brief    Input BR/EDR remote OOB data when pairing with out of bond. This is used when
 *           receive \ref LEGACY_OOB_INPUT message in GAP legacy callback function
 *           registed by \ref legacy_register_cb.Before using this function, remote OOB
 *           data should be obtained from remote device.
 *
 * \param[in]  bd_addr    Bluetooth address of remote device.
 * \param[in]  oob_r      The OOB data simple pairing randonizer R of remote device.
 * \param[in]  oob_c      The OOB data simple pairing hash C of remote device.
 * \param[in]  cause      Accept or reject to input remote oob data.
 *
 * \return    The status of sending OOB data to Bluetooth stack.
 * \retval    GAP_CAUSE_SUCCESS            OOB data was sent successfully.
 * \retval    GAP_CAUSE_SEND_REQ_FAILED    OOB data was fail to send.
 *
 * <b>Example usage</b>
   \code{.c}
   #define GAP_OOB_LEN          16

   // GAP legacy callback function.
   void legacy_cb(void *buf, T_LEGACY_MSG legacy_msg)
   {
       switch (legacy_msg)
       {
       case LEGACY_OOB_INPUT:
           {
               uint8_t oob_r[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
               uint8_t oob_c[GAP_OOB_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
               T_GAP_LEGACY_REMOTE_OOB_REQ_IND *p_ind = (T_GAP_LEGACY_REMOTE_OOB_REQ_IND *)buf;

               legacy_bond_input_remote_oob(p_ind->bd_addr, oob_r, oob_c, GAP_CFM_CAUSE_ACCEPT);
           }
           break;

       default:
           break;
       }
   }

   // Register GAP legacy callback function.
   int test(void)
   {
       legacy_register_cb(legacy_cb);

       return 0;
   }

   \endcode
 *
 * \ingroup  GAP_LEGACY_BOND
 */
T_GAP_CAUSE legacy_bond_input_remote_oob(uint8_t *bd_addr, uint8_t *oob_r, uint8_t *oob_c,
                                         T_GAP_CFM_CAUSE cause);

/**
 * gap_bond_legacy.h
 *
 * \brief    Send user confirmation request result when pairing with numeric
 *           comparision. This is used when receive \ref USER_CONFIRM_REQ message
 *           in GAP legacy callback function registed by \ref legacy_register_cb.
 *
 * \param[in]  bd_addr    Bluetooth address of remote device.
 * \param[in]  result     User confirmation result. Type is \ref T_GAP_CFM_CAUSE and
 *                        \ref GAP_CFM_CAUSE_ACCEPT means accept while the others
 *                        mean not accpect for the pairing.
 *
 * \return    The status of sending user confirmation request result.
 * \retval    GAP_CAUSE_SUCCESS            Confirmation was sent successfully.
 * \retval    GAP_CAUSE_SEND_REQ_FAILED    Confirmation was fail to send.
 *
 * <b>Example usage</b>
   \code{.c}
   // GAP legacy callback function.
   void legacy_cb(void *buf, T_LEGACY_MSG legacy_msg)
   {
       switch (legacy_msg)
       {
       case USER_CONFIRM_REQ:
           {
               T_GAP_USER_CFM_REQ_IND *p_ind = (T_GAP_USER_CFM_REQ_IND *)p_buf;

               APP_PRINT_TRACE1("Numeric comparison value is %d", p_ind->display_value);
               legacy_bond_user_cfm(p_ind->bd_addr, GAP_CFM_CAUSE_ACCEPT);
           }
           break;

       default:
           break;
       }
   }

   // Register GAP legacy callback function.
   int test(void)
   {
       legacy_register_cb(legacy_cb);

       return 0;
   }

   \endcode
 *
 * \ingroup  GAP_LEGACY_BOND
 */
T_GAP_CAUSE legacy_bond_user_cfm(uint8_t *bd_addr, T_GAP_CFM_CAUSE result);

/**
 * gap_bond_legacy.h
 *
 * \brief   Read local oob data before pairing with out of bond. This is used before initiating a
 *          pairing process, or when remote device asks for oob data. \ref LOCAL_OOB_RSP message will
 *          be get in GAP legacy callback function registed by \ref legacy_register_cb with a parameter
 *          points to \ref T_GAP_LOCAL_OOB_RSP.
 *
 * \param[in]   None.
 *
 * \return    The status of reading local oob data.
 * \retval    GAP_CAUSE_SUCCESS              The request was sent successfully.
 * \retval    GAP_CAUSE_SEND_REQ_FAILED      Read local oob request was fail to send.
 *
 * <b>Example usage</b>
   \code{.c}

   // GAP legacy callback function.
   void legacy_cb(void *buf, T_LEGACY_MSG legacy_msg)
   {
       switch (legacy_msg)
       {
       case LOCAL_OOB_RSP:
           {
               T_GAP_LOCAL_OOB_RSP *p_rsp = (T_GAP_LOCAL_OOB_RSP *)p_buf;

               if(p_rsp->cause == GAP_CAUSE_SUCCESS)
               {
                    //app will save p_rsp->data_c, p_rsp->data_r
               }
               else
               {
                   APP_PRINT_ERROR1("Read local oob failed, cause = %d", p_rsp->cause);
               }
           }
           break;

       default:
           break;
       }
   }

   // Register GAP legacy callback function and send read local oob request.
   int test(void)
   {
       legacy_register_cb(legacy_cb);

       legacy_bond_read_local_oob();

       return 0;
   }
   \endcode
 *
 * \ingroup  GAP_LEGACY_BOND
 */
T_GAP_CAUSE legacy_bond_read_local_oob(void);

#endif
#ifdef __cplusplus
}
#endif

#endif /* GAP_BOND_LEGACY_H */
