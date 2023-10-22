/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "sys/defs.h"

#include "ble/bluetooth/bluetooth.h"
#include "ble/bluetooth/uuid.h"
#include "gatt_attr_manage.h"

typedef struct gatt_attr_priv {
	uint32_t attrIndex;
	gatt_attr_base base;
	uint8_t *entry;
	struct bt_gatt_attr *attrs;
	struct bt_uuid_16 *uuids;
	struct bt_gatt_chrc *chrc;
	struct _bt_gatt_ccc *ccc;
} gatt_attr_priv_t;
static gatt_attr_priv_t *gatt_attr = NULL;

#define GET_PRIV(base) ((gatt_attr_priv_t *)container_of(base, gatt_attr_priv_t, base))

static int gatt_attr_add(struct gatt_attr_base *base, bt_gatt_attr_t *param)
{
	gatt_attr_priv_t *priv = GET_PRIV(base);

	switch (param->type) {
	case GATT_ATTR_SERVICE: {
		struct bt_gatt_attr blink_attrs[] = {
			BT_GATT_PRIMARY_SERVICE(param->attr.user_data),
		};
		memcpy(&priv->attrs[priv->attrIndex], blink_attrs, sizeof(blink_attrs));
		memcpy(&priv->uuids[priv->attrIndex], priv->attrs[priv->attrIndex].uuid, sizeof(struct bt_uuid_16));
		priv->attrs[priv->attrIndex].uuid = (const struct bt_uuid *)&priv->uuids[priv->attrIndex];
		priv->attrIndex++;
		break;
	}
	case GATT_ATTR_CHARACTERISTIC: {
		struct bt_gatt_attr blink_attrs[] = {
			BT_GATT_CHARACTERISTIC(param->attr.uuid, param->properties, param->attr.perm, param->attr.read,
			                       param->attr.write, param->attr.user_data),
		};
		memcpy(&priv->attrs[priv->attrIndex], blink_attrs, sizeof(blink_attrs));
		memcpy(&priv->uuids[priv->attrIndex], priv->attrs[priv->attrIndex].uuid, sizeof(struct bt_uuid_16));
		priv->attrs[priv->attrIndex].uuid = (const struct bt_uuid *)&priv->uuids[priv->attrIndex];

		memcpy(&priv->chrc[priv->attrIndex], priv->attrs[priv->attrIndex].user_data, sizeof(struct bt_gatt_chrc));
		priv->attrs[priv->attrIndex].user_data = &priv->chrc[priv->attrIndex];
		priv->attrIndex = priv->attrIndex + 2;
		break;
	}
	case GATT_ATTR_CCC: {
		struct bt_gatt_attr blink_attrs[] = {
			BT_GATT_CCC(param->attr.user_data, param->attr.perm),
		};
		memcpy(&priv->attrs[priv->attrIndex], blink_attrs, sizeof(blink_attrs));
		memcpy(&priv->uuids[priv->attrIndex], priv->attrs[priv->attrIndex].uuid, sizeof(struct bt_uuid_16));
		priv->attrs[priv->attrIndex].uuid = (const struct bt_uuid *)&priv->uuids[priv->attrIndex];

		memcpy(&priv->ccc[priv->attrIndex], priv->attrs[priv->attrIndex].user_data, sizeof(struct _bt_gatt_ccc));
		priv->attrs[priv->attrIndex].user_data = &priv->ccc[priv->attrIndex];
		priv->attrIndex++;
		break;
	}
	default:
		return -1;
	}

	return 0;
}

struct bt_gatt_attr *gatt_attr_get(struct gatt_attr_base *base, uint32_t attr_index)
{
	gatt_attr_priv_t *priv = GET_PRIV(base);
	return &priv->attrs[attr_index];
}

static void gatt_attr_destroy(struct gatt_attr_base *base)
{
	gatt_attr_priv_t *priv = GET_PRIV(base);

	if (priv != NULL) {
		if (priv->entry) {
			free(priv->entry);
			priv->entry = NULL;
		}
		free(priv);
		gatt_attr = NULL;
	}
}

gatt_attr_base *gatt_attr_create(uint32_t attr_num)
{
	uint32_t all_size;
	uint32_t attrs_size, uuids_size, chrc_size, ccc_size;

	gatt_attr_priv_t *priv = gatt_attr;
	if (priv)
		return &priv->base;

	priv = malloc(sizeof(gatt_attr_priv_t));
	if (priv == NULL)
		return NULL;
	memset(priv, 0, sizeof(gatt_attr_priv_t));

	attrs_size = sizeof(struct bt_gatt_attr) * attr_num;
	uuids_size = sizeof(struct bt_uuid_16) * attr_num;
	chrc_size = sizeof(struct bt_gatt_chrc) * attr_num;
	ccc_size = sizeof(struct _bt_gatt_ccc) * attr_num;
	all_size = attrs_size + uuids_size + chrc_size + ccc_size;

	priv->entry = (uint8_t *)malloc(all_size);
	if (priv->entry == NULL) {
		free(priv);
		return NULL;
	}
	memset(priv->entry, 0, all_size);

	priv->attrs = (struct bt_gatt_attr *)priv->entry;
	priv->uuids = (struct bt_uuid_16 *)(priv->entry + attrs_size);
	priv->chrc = (struct bt_gatt_chrc *)(priv->entry + attrs_size + uuids_size);
	priv->ccc = (struct _bt_gatt_ccc *)(priv->entry + attrs_size + uuids_size + chrc_size);

	priv->base.add = gatt_attr_add;
	priv->base.get = gatt_attr_get;
	priv->base.destroy = gatt_attr_destroy;

	gatt_attr = priv;

	return &priv->base;
}

