/*
  Copyright (c) 2018, Tobias Schramm <tobleminer@gmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <libubus.h>
#include <libubox/blobmsg.h>
#include <libubox/list.h>

#include "libmeshutil.h"

typedef struct {
	struct list_head *interfaces;
	int err;
} interface_ctx;

void gluonutil_free_interface(struct gluonutil_interface *iface) {
	if(iface->device) {
		free(iface->device);
	}
	if(iface->proto) {
		free(iface->proto);
	}
	free(iface);
}

void gluonutil_free_interfaces(struct list_head *interfaces) {
	struct gluonutil_interface *cursor, *next;
	list_for_each_entry_safe(cursor, next, interfaces, list) {
		list_del(&cursor->list);
		gluonutil_free_interface(cursor);
	}
}

static struct gluonutil_interface *find_interface_ifindex(unsigned int ifindex, struct list_head *interfaces) {
	struct gluonutil_interface *cursor;
	list_for_each_entry(cursor, interfaces, list) {
		if(cursor->ifindex == ifindex) {
			return cursor;
		}
	}
	return NULL;
}

enum {
	GLUONUTIL_IFACE_ATTR_DEVICE,
	GLUONUTIL_IFACE_ATTR_UP,
	GLUONUTIL_IFACE_ATTR_PROTO,
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_arr) (sizeof((_arr)) / sizeof((*_arr)))
#endif

static int parse_blob_interface(struct blob_attr *iface_blobmsg, struct gluonutil_interface *iface) {
	int err = 0;

	const struct blobmsg_policy iface_policy[] = {
		[GLUONUTIL_IFACE_ATTR_DEVICE] = {
			.name = "device",
			.type = BLOBMSG_TYPE_STRING,
		},
		[GLUONUTIL_IFACE_ATTR_UP] = {
			.name = "up",
			.type = BLOBMSG_TYPE_BOOL,
		},
		[GLUONUTIL_IFACE_ATTR_PROTO] = {
			.name = "proto",
			.type = BLOBMSG_TYPE_STRING,
		},
	};

	int policy_len = ARRAY_SIZE(iface_policy);
	struct blob_attr *attrs[policy_len];
	if(blobmsg_parse(iface_policy, policy_len, attrs, blobmsg_data(iface_blobmsg), blobmsg_len(iface_blobmsg))) {
		err = -EINVAL;
		goto fail;
	}

	if(!attrs[GLUONUTIL_IFACE_ATTR_DEVICE]) {
		return -ENOENT;
	}

	iface->device = strdup(blobmsg_get_string(attrs[GLUONUTIL_IFACE_ATTR_DEVICE]));
	iface->ifindex = if_nametoindex(iface->device);

	if(attrs[GLUONUTIL_IFACE_ATTR_UP]) {
		iface->up = blobmsg_get_bool(attrs[GLUONUTIL_IFACE_ATTR_UP]);
	}
	if(attrs[GLUONUTIL_IFACE_ATTR_PROTO]) {
		iface->proto = strdup(blobmsg_get_string(attrs[GLUONUTIL_IFACE_ATTR_PROTO]));
	}

fail:
	return err;
}

static int parse_blob_interfaces(struct list_head *interfaces, struct blob_attr *attr, int len) {
	int err = 0;
	if(!blobmsg_check_attr(attr, true)) {
		err = -EINVAL;
		goto fail;
	}

	struct blob_attr *table = blobmsg_data(attr);
	struct blob_attr *cursor;
	int table_len = blobmsg_data_len(attr);
	struct gluonutil_interface *iface = NULL;
	if(!blobmsg_check_attr(table, false)) {
		err = -EINVAL;
		goto fail;
	}
	__blob_for_each_attr(cursor, table, table_len) {
		if(!blobmsg_check_attr(cursor, false)) {
			err = -EINVAL;
			goto fail_interfaces;
		}
		iface = malloc(sizeof(struct gluonutil_interface));
		if(!iface) {
			goto fail_interfaces;
		}
		memset(iface, 0, sizeof(struct gluonutil_interface));
		if((err = parse_blob_interface(cursor, iface))) {
			gluonutil_free_interface(iface);
			continue;
		}

		if(find_interface_ifindex(iface->ifindex, interfaces)) {
			gluonutil_free_interface(iface);
			continue;
		}

		list_add(&iface->list, interfaces);
	}

	return 0;

fail_interfaces:
	gluonutil_free_interfaces(interfaces);
fail:
	return err;
}

static bool proto_is_mesh(char* proto) {
	if(!proto) {
		return false;
	}

	for(int i = 0; i < ARRAY_SIZE(gluonutil_mesh_protocols); i++) {
		if(!strcmp(proto, gluonutil_mesh_protocols[i])) {
			return true;
		}
	}

	return false;
}

static void get_mesh_interfaces_cb(struct ubus_request *req, int type, struct blob_attr *msg) {
	interface_ctx *ctx = req->priv;

	ctx->err = parse_blob_interfaces(ctx->interfaces, blob_data(msg), blob_len(msg));
}

/**
 * Fills supplied list with all known mesh interfaces
 */
int gluonutil_get_mesh_interfaces(struct ubus_context* ubus_ctx, struct list_head *interfaces) {
	int err = 0;
	uint32_t id;
	if(ubus_lookup_id(ubus_ctx, "network.interface", &id)) {
		err = -EINVAL;
		goto fail;
	}

	struct blob_buf buf;
	memset(&buf, 0, sizeof(buf));
	blob_buf_init(&buf, 0);
	interface_ctx result;
	result.err = -ENOENT;
	result.interfaces = interfaces;
	if(ubus_invoke(ubus_ctx, id, "dump", buf.head, get_mesh_interfaces_cb,
			&result, 30 * 1000) != UBUS_STATUS_OK) {
		err = -EINVAL;
		goto fail;
	}

	if(result.err) {
		err = result.err;
		goto fail;
	}

	struct gluonutil_interface *cursor, *next;
	list_for_each_entry_safe(cursor, next, interfaces, list) {
		if(!proto_is_mesh(cursor->proto)) {
			list_del(&cursor->list);
			gluonutil_free_interface(cursor);
		}
	}

fail:
	return err;
}
