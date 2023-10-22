#include <string.h>

#include "xr_bt_device.h"
#include "xr_bt_main.h"
#include "bt_manager.h"
#include "btmg_log.h"
#include "btmg_gap.h"
#include "bt_ctrl.h"
#include "btmg_common.h"
#include "btmg_a2dp_sink.h"
#include "btmg_a2dp_source.h"
#include "btmg_spp_client.h"
#include "btmg_spp_server.h"
#include "btmg_hfp_hf.h"
#include "btmg_hfp_ag.h"
#include "btmg_ble.h"
#include "btmg_avrc.h"

#if defined(CONFIG_DRIVERS_TRNG) && defined(CONFIG_ARCH_SUN20IW2)
#include <sunxi_hal_trng.h>
#endif

#define CONFIG_BT_ADDR_PATH "/data/bt_mac"

btmg_callback_t *btmg_cb_p[CB_MAX] = {};
btmg_profile_info_t *bt_pro_info = NULL;
btmg_profile_state_t bt_pro_state;
dev_list_t *connected_devices = NULL;

static btmg_adapter_state_t btmg_adapter_state = BTMG_ADAPTER_OFF;

#define CHECK_ADAPTER_STATE()                                                                      \
    do {                                                                                           \
        if (btmg_adapter_state != BTMG_ADAPTER_ON) {                                               \
            BTMG_ERROR("bluetooth adapter not turned on");                                         \
            return BT_ERR_NOT_ENABLED;                                                             \
        }                                                                                          \
    } while (0)


static btmg_err btmg_callback_set(btmg_cb_id_t cb_id, btmg_callback_t *cb)
{
    if (cb_id < 0 || cb_id >= CB_MAX) {
        return BT_FAIL;
    }

    btmg_cb_p[cb_id] = cb;

    return BT_OK;
}

static btmg_callback_t *btmg_callback_get(btmg_cb_id_t cb_id)
{
    if (cb_id < 0 || cb_id >= CB_MAX) {
        return NULL;
    }

    return btmg_cb_p[cb_id];
}


static void btmg_callback_reset(btmg_cb_id_t cb_id)
{

    if (cb_id < 0 || cb_id >= CB_MAX) {
        return;
    }

    btmg_cb_p[cb_id] = NULL;
}

btmg_err btmg_register_callback(btmg_callback_t *cb)
{
    return btmg_callback_set(CB_MAIN, cb);
}

void btmg_unregister_callback(void)
{
    btmg_callback_reset(CB_MAIN);
}

btmg_err btmg_audiosystem_register_cb(btmg_callback_t *cb)
{
    return btmg_callback_set(CB_MINOR, cb);
}

void btmg_audiosystem_unregister_cb(void)
{
    btmg_callback_reset(CB_MINOR);
}

btmg_err btmg_set_profile(int profile)
{
    BTMG_DEBUG("profile mask:%d", profile);

    if (profile & BTMG_A2DP_SINK) {
        BTMG_DEBUG("a2dp sink profile enable.");
        bt_pro_info->a2dp_sink_enabled = true;
    }
    if (profile & BTMG_A2DP_SOURCE) {
        BTMG_DEBUG("a2dp source profile enable.");
        bt_pro_info->a2dp_source_enabled = true;
    }
    if (profile & BTMG_HFP_HF) {
        BTMG_DEBUG("hfp client profile enable.");
        bt_pro_info->hfp_hf_enabled = true;
    }
    if (profile & BTMG_HFP_AG) {
        BTMG_DEBUG("hfp client profile enable.");
        bt_pro_info->hfp_ag_enabled = true;
    }
    if (profile & BTMG_GATT_SERVER) {
        BTMG_DEBUG("gatt server profile enable.");
        bt_pro_info->gatts_enabled = true;
    }
    if (profile & BTMG_GATT_CLIENT) {
        BTMG_DEBUG("gatt client profile enable.");
        bt_pro_info->gattc_enabled = true;
    }
    if (profile & BTMG_SPP_SERVER) {
        BTMG_DEBUG("spp server profile enable.");
        bt_pro_info->spps_enabled = true;
    }
    if (profile & BTMG_SPP_CLIENT) {
        BTMG_DEBUG("spp client profile enable.");
        bt_pro_info->sppc_enabled = true;
    }

    return BT_OK;
}

btmg_err btmg_core_init(void)
{
    if (bt_pro_info) {
        //already init
        return BT_OK;
    } else {
        bt_pro_info = (btmg_profile_info_t *)malloc(sizeof(btmg_profile_info_t));
        if (NULL == bt_pro_info) {
            BTMG_ERROR("malloc for bt_pro_info failed");
            return BT_ERR_NO_MEMORY;
        }

        memset(bt_pro_info, 0, sizeof(btmg_profile_info_t));
        connected_devices = btmg_dev_list_new();
        if (connected_devices == NULL) {
            BTMG_ERROR("connected_devices init fail");
            return BT_ERR_NO_MEMORY;
        }

        if (XR_OS_SemaphoreCreateBinary(&(connected_devices->sem))) {
            BTMG_ERROR("BT XR_OS_SemaphoreCreateBinary ERROR");
            return BT_FAIL;
        }

        return BT_OK;
    }
}

btmg_err btmg_core_deinit(void)
{
    if (bt_pro_info != NULL) {
        free(bt_pro_info);
        bt_pro_info = NULL;
    }

    if (connected_devices != NULL) {
        XR_OS_SemaphoreDelete(&(connected_devices->sem));
        btmg_dev_list_free(connected_devices);
        connected_devices = NULL;
    }

    return BT_OK;
}

static int bt_profile_init(void)
{
    btmg_profile_info_t profie_cmp;

    if (bt_pro_info == NULL) {
        BTMG_ERROR("btmanager core is not init");
        goto fail;
    }

    memset(&profie_cmp, 0, sizeof(btmg_profile_info_t));
    if (memcmp(bt_pro_info, &profie_cmp, sizeof(btmg_profile_info_t)) == 0) {
        BTMG_ERROR("all profiles are disable.");
        goto fail;
    }

#ifdef CONFIG_COMPONENTS_BLUEDROID
    if (!bt_pro_state.gap_register) {
        if (bt_gap_init() != BT_OK) {
            BTMG_ERROR("gap init fail");
            goto fail;
        };
        bt_pro_state.gap_register = true;
    }

    if (bt_pro_info->a2dp_sink_enabled) {
        if (!(bt_pro_state.a2dp_sink_register | bt_pro_state.a2dp_source_register)) {
            if (bt_avrc_init(BTMG_A2DP_SINK) != BT_OK) {
                BTMG_ERROR("avrc init fail");
                goto fail;
            }
            if (bt_a2dp_sink_init() != BT_OK) {
                BTMG_ERROR("a2dp sink init fail");
                bt_avrc_deinit(BTMG_A2DP_SINK);
                goto fail;
            }
            BTMG_INFO("a2dp sink profile register success");
            bt_pro_state.a2dp_sink_register = true;
        }
    }

    if (bt_pro_info->a2dp_source_enabled) {
        if (!(bt_pro_state.a2dp_sink_register | bt_pro_state.a2dp_source_register)) {
            if (bt_avrc_init(BTMG_A2DP_SOURCE) != BT_OK) {
                BTMG_ERROR("avrc init fail");
                goto fail;
            }
            if (bt_a2dp_source_init() != BT_OK) {
                BTMG_ERROR("a2dp source init fail");
                bt_avrc_deinit(BTMG_A2DP_SOURCE);
                goto fail;
            }
            BTMG_INFO("a2dp source profile register success");
            bt_pro_state.a2dp_source_register = true;
        }
    }

    if (bt_pro_info->hfp_hf_enabled && !bt_pro_state.hfp_hf_register) {
        if (bt_hfp_hf_init() != BT_OK) {
            BTMG_ERROR("hfp hf init fail");
            goto fail;
        }
        BTMG_INFO("hfp hf profile register success");
        bt_pro_state.hfp_hf_register = true;
    }

    if (bt_pro_info->hfp_ag_enabled && !bt_pro_state.hfp_ag_register) {
        if (bt_hfp_ag_init() != BT_OK) {
            BTMG_ERROR("hfp ag init fail");
            goto fail;
        }
        BTMG_INFO("hfp ag profile register success");
        bt_pro_state.hfp_ag_register = true;
    }

    if (bt_pro_info->sppc_enabled && !bt_pro_state.sppc_register) {
        if (bt_sppc_init() != BT_OK) {
            BTMG_ERROR("sppc init fail");
            goto fail;
        }
        BTMG_INFO("spp client profile register success");
        bt_pro_state.sppc_register = true;
    }

    if (bt_pro_info->spps_enabled && !bt_pro_state.spps_register) {
        if (bt_spps_init() != BT_OK) {
            BTMG_ERROR("spps init fail");
            goto fail;
        }
        BTMG_INFO("spp server profile register success");
        bt_pro_state.spps_register = true;
    }
#endif

#ifdef CONFIG_BLEHOST
    if (bt_pro_info->gattc_enabled) {
        if (!(bt_pro_state.gattc_register | bt_pro_state.gatts_register)) {
            if (btmg_ble_init() != BT_OK) {
                BTMG_ERROR("ble init fail");
                goto fail;
            }
            BTMG_INFO("gatt client profile register success");
            bt_pro_state.gattc_register = true;
        }
    }

    if (bt_pro_info->gatts_enabled) {
        if (!(bt_pro_state.gattc_register | bt_pro_state.gatts_register)) {
            if (btmg_ble_init() != BT_OK) {
                BTMG_ERROR("ble init fail");
                goto fail;
            }
            BTMG_INFO("gatt server profile register success");
            bt_pro_state.gatts_register = true;
        }
    }

#endif

    return BT_OK;

fail:
    memset(&bt_pro_state, 0, sizeof(btmg_profile_state_t));
    return BT_FAIL;
}

static int bt_profile_deinit(void)
{
#ifdef CONFIG_COMPONENTS_BLUEDROID
    if (bt_pro_info->a2dp_sink_enabled) {
        bt_avrc_deinit(BTMG_A2DP_SINK);
        bt_a2dp_sink_deinit();
    }

    if (bt_pro_info->a2dp_source_enabled) {
        bt_avrc_deinit(BTMG_A2DP_SOURCE);
        bt_a2dp_source_deinit();
    }

    if (bt_pro_info->hfp_hf_enabled) {
        bt_hfp_hf_deinit();
    }

    if (bt_pro_info->hfp_ag_enabled) {
        bt_hfp_ag_deinit();
    }

    if (bt_pro_info->sppc_enabled) {
        bt_sppc_deinit();
    }

    if (bt_pro_info->spps_enabled) {
        bt_spps_deinit();
    }
#endif

#ifdef CONFIG_BLEHOST
    if (bt_pro_info->gattc_enabled | bt_pro_info->gatts_enabled) {
#if defined(CONFIG_BT_DEINIT)
        btmg_ble_deinit();
    }
#endif
#endif
    memset(&bt_pro_state, 0, sizeof(btmg_profile_state_t));
    return BT_OK;

fail:
    return BT_FAIL;
}

static void bt_get_mac(uint8_t *mac)
{
    int i, ret;
    FILE* pFile = NULL;

    pFile = fopen(CONFIG_BT_ADDR_PATH, "ab+");

    if (pFile == NULL) {
        BTMG_ERROR("open %s failed", CONFIG_BT_ADDR_PATH);
    } else {
        fseek(pFile, 0, SEEK_SET);
        for (i = 0; i < 6; i++) {
             ret = fscanf(pFile, "%02x:", &(mac[i]));
             if (ret != 1)
                break;
            }
            if (i == 6) {
                fclose(pFile);
                return;
            }
    }

//If the saved address is not found, a new address is generated.
#if defined(CONFIG_DRIVERS_TRNG) && defined(CONFIG_ARCH_SUN20IW2)
    uint32_t rand[4];
    HAL_TRNG_Extract(0, rand);
    for (i = 0; i < 6; i++) {
        mac[i] = *((uint8_t *)rand + i);
    }
#else
    for (i = 0; i < 6; i++) {
        srand((unsigned int)XR_OS_GetTicks());
        mac[i] = rand() % 255 + 1;
    }
#endif

    if (pFile) {
        uint8_t tmp_mac[17] = { 0 };//xx:xx:xx:xx:xx:xx
        for (i = 0; i < 6; i++) {
            sprintf(&(tmp_mac[i * 3]), "%02x:", mac[i]);
        }
         //Saved new address.
        ret = fwrite(tmp_mac, 17, sizeof(uint8_t), pFile);
        if (ret != 1) {
            BTMG_ERROR("save mac addr failed %d", ret);
        }
        fclose(pFile);
    }
}

btmg_err btmg_adapter_enable(bool enable)
{
    int err = XR_FAIL;
    uint8_t mac_addr[6] = { 0 };

    if (enable == true) {
        if (btmg_adapter_state == BTMG_ADAPTER_ON) {
            //add new profile
            if (bt_profile_init() < 0) {
                bt_ctrl_disable();
                BTMG_ERROR("BT turn on fail");
                if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb)
                    btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb(BTMG_ADAPTER_OFF);
                btmg_adapter_state = BTMG_ADAPTER_OFF;
                goto fail;
            }
            return BT_OK;
        }

        if (btmg_adapter_state == BTMG_ADAPTER_TURNING_ON) {
            BTMG_WARNG("BT is TURNING ON!");
            return BT_FAIL;
        }

        if (btmg_adapter_state == BTMG_ADAPTER_TURNING_OFF) {
            BTMG_WARNG("BT is already in process of TURNING OFF!");
            return BT_FAIL;
        }
        btmg_adapter_state = BTMG_ADAPTER_TURNING_ON;
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb)
            btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb(BTMG_ADAPTER_TURNING_ON);
        if (bt_ctrl_enable() != 0) {
            BTMG_ERROR("Failed to initialize controller");
            if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb) {
                btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb(BTMG_ADAPTER_OFF);
                btmg_adapter_state = BTMG_ADAPTER_OFF;
                goto fail;
            }
        }
        BTMG_INFO("btmanager version: %s, builed time: %s-%s", BTMGVERSION, __DATE__, __TIME__);
#ifdef CONFIG_COMPONENTS_BLUEDROID
        bt_bluedroid_adapter_register();
        XR_OS_MSleep(100);
        if ((err = xr_bluedroid_init()) != XR_OK) {
            BTMG_ERROR("initialize bluedroid failed: %d", err);
            return BT_FAIL;
        }
        XR_OS_MSleep(100);
        bt_get_mac(mac_addr);
        BTMG_ERROR("Get mac: %02x:%02x:%02x:%02x:%02x:%02x\n", mac_addr[0], mac_addr[1],
                   mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        bt_ctrl_set_mac(mac_addr);
        if ((err = xr_bluedroid_enable()) != XR_OK) {
            BTMG_ERROR("enable bluedroid failed: %d", err);
            return BT_FAIL;
        }
        XR_OS_MSleep(100);
#endif

#ifdef CONFIG_BLEHOST
        bt_zephyr_adapter_register();
#endif
        if (bt_profile_init() < 0) {
            bt_ctrl_disable();
            BTMG_ERROR("BT turn on fail");
            if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb)
                btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb(BTMG_ADAPTER_OFF);
            btmg_adapter_state = BTMG_ADAPTER_OFF;
            goto fail;
        }

        btmg_adapter_state = BTMG_ADAPTER_ON;
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb)
            btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb(BTMG_ADAPTER_ON);
    } else {
        if (btmg_adapter_state == BTMG_ADAPTER_OFF) {
            BTMG_WARNG("BT is already OFF!");
            return BT_OK;
        } else if (btmg_adapter_state == BTMG_ADAPTER_TURNING_OFF) {
            BTMG_WARNG("BT is TURNING_OFF!");
            goto fail;
        } else if (btmg_adapter_state == BTMG_ADAPTER_TURNING_ON) {
            BTMG_WARNG("BT is TURNING ON!");
            goto fail;
        } else if (btmg_adapter_state != BTMG_ADAPTER_ON) {
            BTMG_WARNG("BT state is not right,try later!");
            goto fail;
        }

        btmg_disconnect_dev_list(connected_devices);

        btmg_adapter_state = BTMG_ADAPTER_TURNING_OFF;
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb)
            btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb(BTMG_ADAPTER_TURNING_OFF);

        if (bt_profile_deinit() < 0) {
            BTMG_ERROR("profile deinit fail");
            goto fail;
        }
#ifdef CONFIG_COMPONENTS_BLUEDROID
        if ((err = xr_bluedroid_disable()) != XR_OK) {
            BTMG_ERROR("disable bluedroid failed: %d", err);
            goto fail;
        }

        if ((err = xr_bluedroid_deinit()) != XR_OK) {
            BTMG_ERROR("deinitialize bluedroid failed: %d", err);
            goto fail;
        }
        bt_bluedroid_adapter_unregister();
#endif
#ifdef CONFIG_BLEHOST
        bt_zephyr_adapter_unregister();
#endif
        if (bt_ctrl_disable() != 0) {
            BTMG_ERROR("Bluetooth controller deinit failed");
        }

        btmg_adapter_state = BTMG_ADAPTER_OFF;
        if (btmg_cb_p[CB_MAIN] && btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb)
            btmg_cb_p[CB_MAIN]->btmg_adapter_cb.state_cb(BTMG_ADAPTER_OFF);
    }
    return BT_OK;

fail:
    btmg_adapter_state = BTMG_ADAPTER_OFF;
    return BT_FAIL;
}

btmg_err btmg_adapter_get_state(btmg_adapter_state_t *adapter_state)
{
    *adapter_state = btmg_adapter_state;

    return BT_OK;
}

btmg_err btmg_set_loglevel(btmg_log_level_t log_level)
{
    return bt_set_debug_level((bt_log_level_t)log_level);
}

btmg_err btmg_get_loglevel(btmg_log_level_t *log_level)
{
    *log_level = (btmg_log_level_t)bt_get_debug_level();

    return BT_OK;
}

btmg_err btmg_set_ex_debug(int mask)
{
    if (mask >= EX_DBG_MASK_MAX) {
        BTMG_ERROR("mask is Invalid.");
        return BT_ERR_INVALID_ARG;
    }

    bt_set_debug_mask(mask);

    return BT_OK;
}

btmg_err btmg_get_ex_debug(int *mask)
{
    *mask = bt_get_debug_mask();

    return BT_OK;
}

btmg_err btmg_get_connnected_dev_list(void *connected_list)
{
    CHECK_ADAPTER_STATE();

    memcpy((dev_list_t *)connected_list, connected_devices, sizeof(dev_list_t));
    return BT_OK;
}

btmg_err btmg_adapter_get_address(char *addr)
{
    CHECK_ADAPTER_STATE();

    xr_bd_addr_t adapter_addr;

    memcpy(adapter_addr, xr_bt_dev_get_address(), 6);
    bda2str(adapter_addr, addr);

    return BT_OK;
}

btmg_err btmg_adapter_set_name(const char *name)
{
    CHECK_ADAPTER_STATE();

    return xr_bt_dev_set_device_name(name);
}

btmg_err btmg_adapter_get_name()
{
    CHECK_ADAPTER_STATE();

    xr_bt_dev_get_device_name();
}

btmg_err btmg_adapter_set_scanmode(btmg_scan_mode_t mode)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_set_scan_mode(mode);
}

btmg_err btmg_adapter_start_scan(void)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_scan_start();
}

btmg_err btmg_adapter_stop_scan(void)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_scan_stop();
}

//RTOS bluedroid 没有此API
btmg_err btmg_adapter_pair_device(const char *addr)
{
    return BT_ERR_NOT_SUPPORTED;
}

btmg_err btmg_adapter_unpair_device(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_remove_bond_device(addr);
}

btmg_err btmg_get_paired_device_num(int *number)
{
    CHECK_ADAPTER_STATE();

    *number = bt_gap_get_bond_device_num();

    return BT_OK;
}

btmg_err btmg_get_paired_devices(int device_num, btmg_paired_device_t *paired_list)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_get_bond_device_list(device_num, paired_list);
}

btmg_err btmg_remove_device(const char *addr)
{
    return BT_ERR_NOT_SUPPORTED;
}

bool btmg_device_is_connected(const char *addr)
{
    dev_node_t *dev_node = NULL;

    dev_node = btmg_dev_list_find_device(connected_devices, addr);
    if (dev_node == NULL) {
        return false;
    }

    return true;
}

btmg_err btmg_set_page_timeout(int slots)
{
    return BT_ERR_NOT_SUPPORTED;
}

btmg_err btmg_set_link_supervision_timeout(const char *addr, int slots)
{
    return BT_ERR_NOT_SUPPORTED;
}

btmg_err btmg_adapter_set_io_capability(btmg_io_capability_t io_cap)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_set_io_capability(io_cap);
}

btmg_err btmg_device_pincode_reply(char *pincode)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_pincode_reply(pincode);
}

btmg_err btmg_device_passkey_reply(uint32_t passkey)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_ssp_passkey_reply(passkey);
}

btmg_err btmg_device_passkey_confirm(uint32_t passkey)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_ssp_passkey_confirm(passkey);
}

btmg_err btmg_device_pairing_confirm(void)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_pairing_confirm();
}

btmg_err btmg_device_get_name(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_gap_get_device_name(addr);
}

btmg_err btmg_a2dp_sink_connect(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_a2dp_sink_connect(addr);
}

btmg_err btmg_a2dp_sink_disconnect(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_a2dp_sink_disconnect(addr);
}

btmg_err btmg_a2dp_source_connect(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_a2dp_source_connect(addr);
}

btmg_err btmg_a2dp_source_disconnect(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_a2dp_source_disconnect(addr);
}

btmg_err btmg_a2dp_source_set_audio_param(uint8_t channels, uint32_t sampling)
{
#ifdef CONFIG_A2DP_USE_AUDIO_SYSTEM
    BTMG_INFO("Currently configured for AudioSystem to take over, api is invalid")
    return BT_ERR_NOT_SUPPORTED;
#endif
    CHECK_ADAPTER_STATE();

    return bt_a2dp_source_set_audio_param(channels, sampling);
}

int btmg_a2dp_source_send_data(uint8_t *data, int len)
{
#ifdef CONFIG_A2DP_USE_AUDIO_SYSTEM
    BTMG_INFO("Currently configured for AudioSystem to take over, please use AudioTrack")
    return BT_ERR_NOT_SUPPORTED;
#endif
    CHECK_ADAPTER_STATE();

    return bt_a2dp_source_send_data(data, len);
}

btmg_err btmg_a2dp_source_play_start(void)
{
    CHECK_ADAPTER_STATE();

    return bt_a2dp_source_play_start();
}

btmg_err btmg_a2dp_source_play_stop(bool drop)
{
    CHECK_ADAPTER_STATE();

    return bt_a2dp_source_play_stop(drop);
}

bool btmg_a2dp_source_is_ready(void)
{
    if (btmg_adapter_state != BTMG_ADAPTER_ON)  {
        BTMG_ERROR("bluetooth adapter not turned on");
        return false;
    }

    if (bt_pro_info->a2dp_source_enabled == false) {
        return false;
    }

    return bt_a2dp_source_is_ready();
}

btmg_err btmg_sppc_connect(const char *dst)
{
    CHECK_ADAPTER_STATE();

    return bt_sppc_connect(dst);
}

btmg_err btmg_sppc_disconnect(const char *dst)
{
    CHECK_ADAPTER_STATE();

    return bt_sppc_disconnect(dst);
}

btmg_err btmg_sppc_write(char *data, uint32_t len)
{
    CHECK_ADAPTER_STATE();

    return bt_sppc_write(data, len);
}

btmg_err btmg_spps_start(int scn)
{
    CHECK_ADAPTER_STATE();

    return bt_spps_start(scn);
}

btmg_err btmg_spps_stop(void)
{
    CHECK_ADAPTER_STATE();

    return bt_spps_stop();
}

btmg_err btmg_spps_write(char *data, uint32_t len)
{
    CHECK_ADAPTER_STATE();

    return bt_spps_write(data, len);
}

btmg_err btmg_spps_disconnect(const char *dst)
{
    CHECK_ADAPTER_STATE();

    return bt_spps_disconnect(dst);
}

btmg_err btmg_hfp_hf_start_voice_recognition(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_start_voice_recognition();
}

btmg_err btmg_hfp_hf_stop_voice_recognition(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_stop_voice_recognition();
}

btmg_err btmg_hfp_hf_spk_vol_update(int volume)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_spk_vol_update(volume);
}

btmg_err btmg_hfp_hf_mic_vol_update(int volume)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_mic_vol_update(volume);
}

btmg_err btmg_hfp_hf_dial(const char *number)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_dial(number);
}

btmg_err btmg_hfp_hf_dial_memory(int location)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_dial_memory(location);
}

btmg_err btmg_hfp_hf_send_chld_cmd(btmg_hf_chld_type_t chld, int idx)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_send_chld_cmd(chld, idx);
}

btmg_err btmg_hfp_hf_send_btrh_cmd(btmg_hf_btrh_cmd_t btrh)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_send_btrh_cmd(btrh);
}

btmg_err btmg_hfp_hf_answer_call(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_answer_call();
}

btmg_err btmg_hfp_hf_reject_call(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_reject_call();
}

btmg_err btmg_hfp_hf_query_calls(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_query_calls();
}

btmg_err btmg_hfp_hf_query_operator(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_query_operator();
}

btmg_err btmg_hfp_hf_query_number(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_query_number();
}

btmg_err btmg_hfp_hf_send_dtmf(char code)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_send_dtmf(code);
}

btmg_err btmg_hfp_hf_request_last_voice_tag_number(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_request_last_voice_tag_number();
}

btmg_err btmg_hfp_hf_send_nrec(void)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_send_nrec();
}

btmg_err btmg_hfp_hf_disconnect(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_disconnect(addr);
}

btmg_err btmg_hfp_hf_disconnect_audio(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_hf_disconnect_audio(addr);
}

btmg_err btmg_hfp_ag_connect(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_ag_connect(addr);
}

btmg_err btmg_hfp_ag_disconnect(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_ag_disconnect(addr);
}

btmg_err btmg_hfp_ag_connect_audio(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_ag_connect_audio(addr);
}

btmg_err btmg_hfp_ag_disconnect_audio(const char *addr)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_ag_disconnect_audio(addr);
}

btmg_err btmg_hfp_ag_spk_vol_update(const char *addr, int volume)
{
    CHECK_ADAPTER_STATE();

    return bt_hfp_ag_spk_vol_update(addr, volume);
}

btmg_err btmg_avrc_ct_send_passthrough_cmd(uint8_t key_code)
{
    CHECK_ADAPTER_STATE();

    return bt_avrc_ct_send_passthrough_cmd(key_code);
}

btmg_err btmg_avrc_set_absolute_volume(uint32_t volume)
{
    CHECK_ADAPTER_STATE();

    return bt_avrc_set_absolute_volume(volume);
}

btmg_err btmg_avrc_get_absolute_volume(uint32_t *value)
{
    CHECK_ADAPTER_STATE();

    return bt_avrc_get_absolute_volume(value);
}
