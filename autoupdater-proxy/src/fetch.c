#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <libubox/uclient.h>
#include <libubox/uloop.h>
#include <libubox/blobmsg.h>
#include <stdbool.h>

#include "fetch.h"
#include "http.h"

#define MAX_REDIRECTS 10
#define BUFF_SIZE 256
#define CONNECTION_TIMEOUT 10000

static void cancel(struct uclient* cl) {
	uclient_disconnect(cl);
	uloop_end();
}

static void header_done_cb(struct uclient *cl) {
	static int redirects = 0;
	if(redirects < MAX_REDIRECTS) {
		int err = uclient_http_redirect(cl);
		if(err < 0) {
			cancel(cl);
			return;
		}
		if(err > 0) {
			redirects++;
			return;
		}
	}

	switch(cl->status_code) {
		case(200): {
			if(FETCH_UC_TO_STATE(cl)->flags.spider) {
				FETCH_UC_TO_STATE(cl)->success = true;
				cancel(cl);
				return;
			}

			struct blobmsg_policy content_type = {
			 .name = "Content-Type",
			 .type = BLOBMSG_TYPE_STRING,
			};

			struct blob_attr* tb_content_type;
			blobmsg_parse(&content_type, 1, &tb_content_type, blob_data(cl->meta), blob_len(cl->meta));
			if(tb_content_type) {
				printf("Content-Type: %s\r\n", blobmsg_get_string(tb_content_type));
			}

			printf("\r\n");
			break;
		}
		default:
			cancel(cl);
	}
}

static void data_read_cb(struct uclient* cl) {
	char buff[BUFF_SIZE];
	ssize_t read_len;
	while((read_len = uclient_read(cl, buff, sizeof(buff))) > 0) {
		fwrite(buff, read_len, 1, stdout);
	}
	fflush(stdout);
}

static void data_eof_cb(struct uclient* cl) {
	FETCH_UC_TO_STATE(cl)->success = true;
	cancel(cl);
}

static void error_cb(struct uclient* cl, int err) {
	cancel(cl);
}

static int request(char* url, bool spider) {
	int err = 0;
	struct uclient_cb cb = {
		.header_done = header_done_cb,
		.data_read = data_read_cb,
		.data_eof = data_eof_cb,
		.error = error_cb,
	};

	struct fetch_state state = { 0 };
	state.flags.spider = spider;

	struct uclient* uc = uclient_new(url, NULL, &cb);
	if(!uc) {
		err = -ENOMEM;
		goto out;
	}

	uc->priv = &state;

	if((err = uclient_set_timeout(uc, CONNECTION_TIMEOUT))) {
		goto out_uc_alloc;
	}
	if((err = uclient_connect(uc))) {
		goto out_uc_alloc;
	}
	if((err = uclient_http_set_request_type(uc, HTTP_GET))) {
		goto out_uc_alloc;
	}
	if((err = uclient_http_reset_headers(uc))) {
		goto out_uc_alloc;
	}
	if((err = uclient_http_set_header(uc, "User-Agent", "MIAU proxy"))) {
		goto out_uc_alloc;
	}
	if((err = uclient_request(uc))) {
		goto out_uc_alloc;
	}

	uloop_run();

	if(!state.success) {
		err = 1;
	}

out_uc_alloc:
	uclient_free(uc);
out:
	return err;
}

int get_url(char* url) {
	return request(url, false);
}

int spider_url(char* url) {
	return request(url, true);
}
