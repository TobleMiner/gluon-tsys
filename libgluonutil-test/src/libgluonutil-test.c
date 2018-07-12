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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libubus.h>
#include <libgluonutil.h>

int main(int argc, char **argv) {
	int err = 0;
	struct ubus_context *ubus_ctx;

	ubus_ctx = ubus_connect(NULL);
	if(!ubus_ctx) {
		err = -ENOENT;
		fprintf(stderr, "Failed to connect to ubus\n");
		goto fail;
	}

	ubus_add_uloop(ubus_ctx);
	LIST_HEAD(interfaces);
	if((err = gluonutil_get_mesh_interfaces(ubus_ctx, &interfaces))) {
		fprintf(stderr, "Failed to get mesh interfaces: %s(%d)\n", strerror(-err), err);
		goto fail_ubus;
	}

	struct gluonutil_interface *iface;
	list_for_each_entry(iface, &interfaces, list) {
		printf("Interface %s:\n", iface->device);
		printf("\tindex: %u\n", iface->ifindex);
		printf("\tstate: %s\n", iface->up ? "up" : "down");
		printf("\tprotocol: %s\n", iface->proto);
	}

	gluonutil_free_interfaces(&interfaces);
	
fail_ubus:
	ubus_free(ubus_ctx);
fail:
	return err;
}

