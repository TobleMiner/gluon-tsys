/*
  Copyright (c) 2018, Tobias Schramm <tobleminer@gmail.com>
  Copyright (c) 2016, Matthias Schiffer <mschiffer@universe-factory.net>
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


#include <json-c/json.h>
#include <uci.h>
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

#include "libgluonutil.h"

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

/**
 * Merges two JSON objects
 *
 * Both objects are consumed. On conflicts, object b will be preferred.
 */
static struct json_object * merge_json(struct json_object *a, struct json_object *b) {
	if (!json_object_is_type(a, json_type_object) || !json_object_is_type(b, json_type_object)) {
		json_object_put(a);
		return b;
	}

	json_object *m = json_object_new_object();

	json_object_object_foreach(a, key_a, val_a)
		json_object_object_add(m, key_a, json_object_get(val_a));
	json_object_put(a);

	json_object_object_foreach(b, key_b, val_b) {
		struct json_object *val_m;

		if (json_object_object_get_ex(m, key_b, &val_m))
			val_m = merge_json(json_object_get(val_m), json_object_get(val_b));
		else
			val_m = json_object_get(val_b);

		json_object_object_add(m, key_b, val_m);
	}
	json_object_put(b);

	return m;
}

char * gluonutil_read_line(const char *filename) {
	FILE *f = fopen(filename, "r");
	if (!f)
		return NULL;

	char *line = NULL;
	size_t len = 0;

	ssize_t r = getline(&line, &len, f);

	fclose(f);

	if (r >= 0) {
		len = strlen(line);

		if (len && line[len-1] == '\n')
			line[len-1] = 0;
	}
	else {
		free(line);
		line = NULL;
	}

	return line;
}

char * gluonutil_get_sysconfig(const char *key) {
	if (strchr(key, '/'))
		return NULL;

	const char prefix[] = "/lib/gluon/core/sysconfig/";
	char path[strlen(prefix) + strlen(key) + 1];
	snprintf(path, sizeof(path), "%s%s", prefix, key);

	return gluonutil_read_line(path);
}

char * gluonutil_get_node_id(void) {
	char *node_id = gluonutil_get_sysconfig("primary_mac");
	if (!node_id)
		return NULL;

	char *in = node_id, *out = node_id;

	do {
		if (*in != ':')
			*out++ = *in;
	} while (*in++);

	return node_id;
}

char * gluonutil_get_interface_address(const char *ifname) {
	const char *format = "/sys/class/net/%s/address";
	char path[strlen(format) + strlen(ifname) - 1];

	snprintf(path, sizeof(path), format, ifname);

	return gluonutil_read_line(path);
}



struct json_object * gluonutil_wrap_string(const char *str) {
	if (!str)
		return NULL;

	return json_object_new_string(str);
}

struct json_object * gluonutil_wrap_and_free_string(char *str) {
	struct json_object *ret = gluonutil_wrap_string(str);
	free(str);
	return ret;
}


bool gluonutil_get_node_prefix6(struct in6_addr *prefix) {
	struct json_object *site = gluonutil_load_site_config();
	if (!site)
		return false;

	struct json_object *node_prefix = NULL;
	if (!json_object_object_get_ex(site, "node_prefix6", &node_prefix)) {
		json_object_put(site);
		return false;
	}

	const char *str_prefix = json_object_get_string(node_prefix);
	if (!str_prefix) {
		json_object_put(site);
		return false;
	}

	char *prefix_addr = strndup(str_prefix, strchrnul(str_prefix, '/')-str_prefix);

	int ret = inet_pton(AF_INET6, prefix_addr, prefix);

	free(prefix_addr);
	json_object_put(site);

	if (ret != 1)
		return false;

	return true;
}



bool gluonutil_has_domains(void) {
	return (access("/lib/gluon/domains/", F_OK) == 0);
}

char * gluonutil_get_domain(void) {
	if (!gluonutil_has_domains())
		return NULL;

	char *ret = NULL;

	struct uci_context *ctx = uci_alloc_context();
	if (!ctx)
		goto uci_fail;

	ctx->flags &= ~UCI_FLAG_STRICT;

	struct uci_package *p;
	if (uci_load(ctx, "gluon", &p))
		goto uci_fail;

	struct uci_section *s = uci_lookup_section(ctx, p, "core");
	if (!s)
		goto uci_fail;

	const char *domain_code = uci_lookup_option_string(ctx, s, "domain");
	if (!domain_code)
		goto uci_fail;

	ret = strdup(domain_code);

uci_fail:
	if (ctx)
		uci_free_context(ctx);

	return ret;
}


struct json_object * gluonutil_load_site_config(void) {
	char *domain_code = NULL;
	struct json_object *site = NULL, *domain = NULL;

	site = json_object_from_file("/lib/gluon/site.json");
	if (!site)
		return NULL;

	if (!gluonutil_has_domains())
		return site;

	domain_code = gluonutil_get_domain();
	if (!domain_code)
		goto err;

	{
		const char *domain_path_fmt = "/lib/gluon/domains/%s.json";
		char domain_path[strlen(domain_path_fmt) + strlen(domain_code)];
		snprintf(domain_path, sizeof(domain_path), domain_path_fmt, domain_code);
		free(domain_code);

		domain = json_object_from_file(domain_path);
	}
	if (!domain)
		goto err;

	return merge_json(site, domain);

err:
	json_object_put(site);
	free(domain_code);
	return NULL;
}
