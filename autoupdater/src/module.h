#pragma once

#include <settings.h>
#include <stddef.h>

#define UEAI_MODULE_OPS_SYM "ueai_ops"

#define UEAI_UPDATE_CTX_PRIV(ctx) \
	((ctx)->priv)

#define UEAI_DOWNLOAD_CTX_PRIV(dlctx) \
	((dlctx)->ctx->priv)

#define UEAI_FIRMWARE_CTX_PRIV(fwctx) \
	((fwctx)->dlctx->ctx->priv)

struct ueai_update_ctx {
	void* priv;
	struct settings* settings;
	struct manifest manifest;

};

struct ueai_download_ctx {
	off_t offset;
	char* buffer;

	struct ueai_update_ctx* ctx;
};

struct ueai_firmware_ctx {
	char* fname;
	struct ueai_fownload_ctx dlctx;
};

typedef int (*ueai_ctx_init)(struct ueai_update_ctx* ctx);
typedef void (*ueai_ctx_destroy)(struct ueai_update_ctx* ctx);

typedef int (*ueai_data_download_cb)(struct ueai_download_ctx* ctx, char* data, size_t data_len);
typedef int (*ueai_firmware_download_cb)(struct ueai_firmware_ctx* ctx, char* data, size_t data_len);

typedef int (*ueai_module_download_manifest)(struct ueai_download_ctx* ctx, ueai_data_download_cb cb);
typedef int (*ueai_module_download_firmware)(struct ueai_firmware_ctx* ctx, ueai_firmware_download_cb cb);

typedef char* (*ueai_module_get_name)(void);

struct ueai_module_ops {
	ueai_ctx_init init,
	ueai_ctx_destroy destroy,
	ueai_module_download_manifest dl_manifest,
	ueai_module_download_firmware dl_firmware,
	ueai_module_get_name get_name,
};
