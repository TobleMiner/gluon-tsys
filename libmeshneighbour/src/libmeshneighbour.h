#ifndef _LIBMESHNEIGHBOUR_H_
#define _LIBMESHNEIGHBOUR_H_

#include <libgluonutil.h>
#include <librespondd.h>
#include <libubox/list.h>

struct mesh_neighbour {
	struct in6_addr addr;
	struct gluonutil_interface *iface;
	char *nodeid;
	void *priv;

	struct list_head list;
};

struct mesh_neighbour_ctx {
	struct list_head neighbours;
	struct list_head interfaces;
};

typedef int (*neighbour_cb)(struct json_object *json, const struct librespondd_pkt_info *pktinfo, struct mesh_neighbour *neigh, void* priv);

int mesh_get_neighbours_respondd(struct mesh_neighbour_ctx *neigh_ctx, unsigned short respondd_port, neighbour_cb cb, void *priv);

void mesh_free_respondd_neighbours_ctx(struct mesh_neighbour_ctx *ctx);

#endif
