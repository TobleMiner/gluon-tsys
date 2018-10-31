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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libgluonutil.h>
#include <libubox/list.h>
#include <libubus.h>
#include <json-c/json.h>

#include "libmeshneighbour.h"

#define IPV6_MCAST_ALL_NODES (struct in6_addr){ .s6_addr = { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } }

static int get_neighbours_common(struct ubus_context *ubus_ctx, struct mesh_neighbour_ctx *neigh_ctx) {
	neigh_ctx->neighbours = (struct list_head)LIST_HEAD_INIT(neigh_ctx->neighbours);
	neigh_ctx->interfaces = (struct list_head)LIST_HEAD_INIT(neigh_ctx->interfaces);

	return gluonutil_get_mesh_interfaces(ubus_ctx, &neigh_ctx->interfaces);
}

static void mesh_free_respondd_neighbour(struct mesh_neighbour *neigh) {
	if(neigh->nodeid) {
		free(neigh->nodeid);
	}
	free(neigh);
}

void mesh_free_respondd_neighbours(struct list_head *neighbours) {
	struct mesh_neighbour *neigh, *next;
	list_for_each_entry_safe(neigh, next, neighbours, list) {
		list_del(&neigh->list);
		mesh_free_respondd_neighbour(neigh);
	}
}

void mesh_free_respondd_neighbours_ctx(struct mesh_neighbour_ctx *ctx) {
	mesh_free_respondd_neighbours(&ctx->neighbours);
	gluonutil_free_interfaces(&ctx->interfaces);
}

struct mesh_respondd_ctx {
	struct gluonutil_interface *iface;
	struct list_head *neighbours;
	void *cb_priv;
	neighbour_cb cb;
};

static struct mesh_neighbour *find_neighbour_nodeid(const char *nodeid, const struct list_head *neighbours) {
	struct mesh_neighbour *neighbour;
	list_for_each_entry(neighbour, neighbours, list) {
		if(!strcmp(nodeid, neighbour->nodeid)) {
			return neighbour;
		}
	}
	return NULL;
}

static int mesh_respondd_cb(const char *json_data, size_t data_len, const struct librespondd_pkt_info *pktinfo, void *priv) {
	// pktinfo not set, something is not right
	if(!pktinfo->ifindex) {
		goto out;
	}

	struct mesh_respondd_ctx *ctx = priv;

	struct json_object *json_root = json_tokener_parse(json_data);
	if(!json_root) {
		goto out;
	}
	
	struct json_object *json_nodeid;
	if(!json_object_object_get_ex(json_root, "node_id", &json_nodeid)) {
		goto out_json;
	}

	const char *nodeid = json_object_get_string(json_nodeid);
	if(!nodeid) {
		goto out_json;
	}

	if(find_neighbour_nodeid(nodeid, ctx->neighbours)) {
		goto out_json;
	}


	struct mesh_neighbour *neighbour = malloc(sizeof(struct mesh_neighbour));
	if(!neighbour) {
		goto out_json;
	}
	memset(neighbour, 0, sizeof(*neighbour));

	neighbour->nodeid = strdup(nodeid);
	if(!neighbour->nodeid) {
		goto out_neighbour;
	}

	neighbour->iface = ctx->iface;
	neighbour->addr = pktinfo->src_addr;

	if(ctx->cb) {
		if(ctx->cb(json_root, pktinfo, neighbour, ctx->cb_priv)) {
			goto out_neighbour;
		}
	}

	list_add(&neighbour->list, ctx->neighbours);

	return RESPONDD_CB_OK;

out_neighbour:
	mesh_free_respondd_neighbour(neighbour);
out_json:
	json_object_put(json_root);
out:
	return RESPONDD_CB_OK;
}

static int mesh_get_neighbours_respondd_interfaces(struct list_head *interfaces, struct list_head* neighbours, unsigned short respondd_port, neighbour_cb cb, void *priv) {
	struct gluonutil_interface *iface;
	struct sockaddr_in6 sock_addr;
	sock_addr.sin6_family = AF_INET6;
	sock_addr.sin6_port = htons(respondd_port);
	sock_addr.sin6_flowinfo = 0;
	sock_addr.sin6_addr = IPV6_MCAST_ALL_NODES;

	struct timeval timeout = { 3, 0 };

	int err = 0;
	list_for_each_entry(iface, interfaces, list) {
		if(!iface->up) {
			continue;
		}

		sock_addr.sin6_scope_id = iface->ifindex;

		LIST_HEAD(neighbours_if);

		struct mesh_respondd_ctx ctx = {
			.iface = iface,
			.neighbours = &neighbours_if,
			.cb_priv = priv,
			.cb = cb,
		};
		int ret = respondd_request(&sock_addr, "nodeinfo", &timeout, mesh_respondd_cb, &ctx);
		if(ret) {
			err = ret;
		}

		list_splice(&neighbours_if, neighbours);
	}

	return err;
}

static int mesh_get_neighbours_respondd_ubus(struct ubus_context *ubus_ctx, struct mesh_neighbour_ctx *neigh_ctx, unsigned short respondd_port, neighbour_cb cb, void *priv) {
	int err = get_neighbours_common(ubus_ctx, neigh_ctx);
	if(err) {
		goto fail;
	}

	err = mesh_get_neighbours_respondd_interfaces(&neigh_ctx->interfaces, &neigh_ctx->neighbours, respondd_port, cb, priv);
	if(err) {
		goto fail_interfaces;
	}

	return 0;

fail_interfaces:
	gluonutil_free_interfaces(&neigh_ctx->interfaces);
fail:
	return err;
}

int mesh_get_neighbours_respondd(struct mesh_neighbour_ctx *neigh_ctx, unsigned short respondd_port, neighbour_cb cb, void *priv) {
        struct ubus_context *ubus_ctx = ubus_connect(NULL);
        if(!ubus_ctx) {
                return -ECONNREFUSED;
        }

        int err = mesh_get_neighbours_respondd_ubus(ubus_ctx, neigh_ctx, respondd_port, cb, priv);

        ubus_free(ubus_ctx);

        return err;
}
