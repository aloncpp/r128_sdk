/* SPDX-License-Identifier: Apache-2.0 */

/* Define an output section which will set up an iterable area
 * of equally-sized data structures. For use with Z_STRUCT_SECTION_ITERABLE.
 * Input sections will be sorted by name, per ld's SORT_BY_NAME.
 *
 * This macro should be used for read-only data.
 *
 * Note that this keeps the symbols in the image even though
 * they are not being directly referenced. Use this when symbols
 * are indirectly referenced by iterating through the section.
 */
//#include "ble/linker/linker-defs.h"
#include "linker-defs.h"

	Z_ITERABLE_SECTION_ROM(bt_l2cap_fixed_chan, 4)

#if defined(CONFIG_BT_BREDR)
	Z_ITERABLE_SECTION_ROM(bt_l2cap_br_fixed_chan, 4)
#endif

	Z_ITERABLE_SECTION_ROM(bt_gatt_service_static, 4)

#if defined(CONFIG_BT_MESH)
	Z_ITERABLE_SECTION_ROM(bt_mesh_subnet_cb, 4)
	Z_ITERABLE_SECTION_ROM(bt_mesh_app_key_cb, 4)

	Z_ITERABLE_SECTION_ROM(bt_mesh_hb_cb, 4)
#endif

#if defined(CONFIG_BT_MESH_FRIEND)
	Z_ITERABLE_SECTION_ROM(bt_mesh_friend_cb, 4)
#endif

#if defined(CONFIG_BT_MESH_LOW_POWER)
	Z_ITERABLE_SECTION_ROM(bt_mesh_lpn_cb, 4)
#endif

#if 0
#if defined(CONFIG_EC_HOST_CMD)
	Z_ITERABLE_SECTION_ROM(ec_host_cmd_handler, 4)
#endif
#endif

#if defined(CONFIG_SETTINGS)
	Z_ITERABLE_SECTION_ROM(settings_handler_static, 4)
#endif
