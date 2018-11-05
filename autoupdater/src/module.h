#pragma once

#include <settings.h>
#include <stddef.h>

#define MAU_UPDATE_CTX_PRIV(ctx) \
	((ctx)->priv)

#define MAU_DOWNLOAD_CTX_PRIV(dlctx) \
	((dlctx)->ctx->priv)

#define MAU_FIRMWARE_CTX_PRIV(fwctx) \
	((fwctx)->dlctx->ctx->priv)

struct mau_update_ctx {
	void* priv;
	struct settings* settings;
};

struct mau_download_ctx {
	off_t offset;
	char* buffer;

	struct mau_update_ctx* ctx;
};

struct mau_firmware_ctx {
	char* fname;
	struct mau_fownload_ctx dlctx;
};

typedef int (*mau_ctx_init)(struct mau_update_ctx* ctx);
typedef void (*mau_ctx_destroy)(struct mau_update_ctx* ctx);

typedef int (*mau_data_download_cb)(struct mau_download_ctx* ctx, char* data, size_t data_len);
typedef int (*mau_firmware_download_cb)(struct mau_firmware_ctx* ctx, char* data, size_t data_len);

typedef int (*mau_module_download_manifest)(struct mau_download_ctx* ctx, mau_data_download_cb cb);
typedef int (*mau_module_download_firmware)(struct mau_firmware_ctx* ctx, mau_firmware_download_cb cb);

typedef char* (*mau_module_get_name)(void);

struct mau_module_ops {
	mau_ctx_init init,
	mau_ctx_destroy destroy,
	mau_module_download_manifest dl_manifest,
	mau_module_download_firmware dl_firmware,
	mau_module_get_name get_name,
};
