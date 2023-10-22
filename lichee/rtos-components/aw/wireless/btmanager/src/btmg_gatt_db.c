#include "btmg_common.h"
#include "bt_manager.h"
#include <errno.h>
#include "ble/bluetooth/gatt.h"
#include "hci_core.h"
#include "ble/bluetooth/conn.h"
#include "btmg_gatt_db.h"

static int gatt_attr_add(struct gatt_attr_base *base, ll_bt_gatt_attr_t *param)
{
    ll_stack_gatt_server_t *priv = GET_PRIV(base);

    switch (param->type) {
    case GATT_ATTR_SERVICE: {
        struct bt_gatt_attr blink_attrs[] = {
            BT_GATT_PRIMARY_SERVICE(param->attr.user_data),
        };
        memcpy(&priv->attrs[priv->attrIndex], blink_attrs, sizeof(blink_attrs));

        memcpy(&(priv->uuids[priv->attrIndex]), priv->attrs[priv->attrIndex].uuid,
               sizeof(struct bt_uuid_128));
        priv->attrs[priv->attrIndex].uuid = (const struct bt_uuid *)&(priv->uuids[priv->attrIndex]);

        memcpy(&priv->user_data[priv->attrIndex].uuid, priv->attrs[priv->attrIndex].user_data,
               sizeof(struct bt_uuid_128));
        priv->attrs[priv->attrIndex].user_data = &(priv->user_data[priv->attrIndex].uuid);

        priv->attrIndex++;
        priv->service.attr_count++;
        break;
    }
    case GATT_ATTR_CHARACTERISTIC: {
        struct bt_gatt_attr blink_attrs[] = {
            BT_GATT_CHARACTERISTIC(param->attr.uuid, param->properties, param->attr.perm,
                                   param->attr.read, param->attr.write, param->attr.user_data),
        };
        memcpy(&priv->attrs[priv->attrIndex], blink_attrs, sizeof(blink_attrs));

        memcpy(&(priv->uuids[priv->attrIndex]), priv->attrs[priv->attrIndex].uuid,
               sizeof(struct bt_uuid_128));
        priv->attrs[priv->attrIndex].uuid = (const struct bt_uuid *)&priv->uuids[priv->attrIndex];

        memcpy(&priv->user_data[priv->attrIndex].chrc, priv->attrs[priv->attrIndex].user_data,
               sizeof(struct bt_gatt_chrc));
        priv->attrs[priv->attrIndex].user_data = &(priv->user_data[priv->attrIndex].chrc);
        priv->user_data[priv->attrIndex].chrc.uuid =
                (const struct bt_uuid *)&priv->uuids[priv->attrIndex + 1];

        memcpy(&(priv->uuids[priv->attrIndex + 1]), priv->attrs[priv->attrIndex + 1].uuid,
               sizeof(struct bt_uuid_128));
        priv->attrs[priv->attrIndex + 1].uuid =
                (const struct bt_uuid *)&priv->uuids[priv->attrIndex + 1];

        priv->attrIndex = priv->attrIndex + 2;
        priv->service.attr_count = priv->service.attr_count + 2;
        break;
    }
    case GATT_ATTR_CCC: {
        struct bt_gatt_attr blink_attrs[] = {
            BT_GATT_CCC(param->attr.user_data, param->attr.perm),
        };
        memcpy(&priv->attrs[priv->attrIndex], blink_attrs, sizeof(blink_attrs));

        memcpy(&priv->uuids[priv->attrIndex], priv->attrs[priv->attrIndex].uuid,
               sizeof(struct bt_uuid_128));
        priv->attrs[priv->attrIndex].uuid = (const struct bt_uuid *)&priv->uuids[priv->attrIndex];

        memcpy(&priv->user_data[priv->attrIndex].ccc, priv->attrs[priv->attrIndex].user_data,
               sizeof(struct _bt_gatt_ccc));
        priv->attrs[priv->attrIndex].user_data = &(priv->user_data[priv->attrIndex].ccc);

        priv->attrIndex++;
        priv->service.attr_count++;
        break;
    }
    default:
        return -1;
    }
    if (priv->service.attr_count > priv->max_attr_count) {
        // WARNING
        priv->service.attr_count = priv->max_attr_count;
    }

    return 0;
}

static struct bt_gatt_attr *gatt_attr_get(struct gatt_attr_base *base, uint32_t attr_index)
{
    ll_stack_gatt_server_t *priv = GET_PRIV(base);
    return &priv->attrs[attr_index];
}

static void gatt_attr_destroy(struct gatt_attr_base *base)
{
    ll_stack_gatt_server_t *priv = GET_PRIV(base);

    if (priv->entry) {
        free(priv->entry);
        priv->entry = NULL;
    }
}

int ll_gatt_attr_create(ll_stack_gatt_server_t *gatt_attr, uint32_t attr_num)
{
    uint32_t all_size;
    uint32_t attrs_size, uuids_size, user_data_size;
    ll_stack_gatt_server_t *priv = gatt_attr;

    memset(priv, 0, sizeof(ll_stack_gatt_server_t));

    attrs_size = sizeof(struct bt_gatt_attr) * attr_num;
    uuids_size = sizeof(struct bt_uuid_128) * attr_num;
    user_data_size = sizeof(gatt_attr_user_data_t) * attr_num;
    all_size = attrs_size + uuids_size + user_data_size;

    priv->entry = (uint8_t *)malloc(all_size);
    if (priv->entry == NULL) {
        return -1;
    }
    memset(priv->entry, 0, all_size);

    priv->attrs = (struct bt_gatt_attr *)priv->entry;
    priv->uuids = (struct bt_uuid_128 *)(priv->entry + attrs_size);
    priv->user_data = (gatt_attr_user_data_t *)(priv->entry + attrs_size + uuids_size);

    priv->base.add = gatt_attr_add;
    priv->base.get = gatt_attr_get;
    priv->base.destroy = gatt_attr_destroy;
    priv->attrBase = &priv->base;
    priv->service.attrs = GATT_ATTR_GET(priv->attrBase, 0);
    priv->max_attr_count = attr_num;
    priv->service.attr_count = 0;
    // priv->service.attr_count = attr_num;

    return 0;
    // return &priv->base;
}

struct bt_gatt_attr *gatt_server_handle_to_attr(ll_stack_gatt_server_t *gatt_attr, uint16_t handle)
{
    ll_stack_gatt_server_t *priv = gatt_attr;

    if (priv == NULL || priv->entry == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < gatt_attr->attrIndex; i++) {
        if (gatt_attr->attrs[i].handle == handle) {
            return &(gatt_attr->attrs[i]);
        }
    }
    return NULL;
    // return &priv->base;
}
