#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "module_priv.h"
#include "util.h"

#define PATH_LEN_MAX 255
#define LINE_LEN_MAX 255

static const char* ueai_module_dir = "/lib/ueai.d/"

static char* get_fext(char* fname) {
	return strrchr(fname, '.') + 1;
}

static int ueai_download_manifest_cb(struct ueai_download_ctx* dlctx, char* buff, size_t len) {
	struct ueai_update_ctx* ctx = dlctx->ctx;

	char* newline;
	while((newline = memchr()
}

static int ueai_download_manifest(struct ueai_module_download_manifest dl_func, struct ueai_update_ctx* ctx) {
	char line[LINE_LEN_MAX + 1];
	memset(line, 0, LINE_LEN_MAX);

	struct ueai_download_ctx dlctx = {
		.buffer = line,
		.offset = 0,
		.ctx = ctx,
	};

	return dl_func(&dlctx, ueai_download_manifest_cb);
}

int ueai_run_modules(struct settings* settings) {
	int err = 0;
	DIR* moddir = opendir(ueai_module_dir);
	if(!moddir) {
		err = -errno;
		goto out;
	}

	struct dirent* moddir;
	DIRENT_FOR_EACH(ent, moddir) {
		// Skip eveything that is not a regular file
		if(ent->d_type != DT_REG) {
			continue;
		}

		// Local filename, must be expanded to global name
		char* fname = ent->d_name;

		char* fext = get_fext(fname);
		if(!fext || strcmp(fext, "so") != 0) {
			fprintf(stderr, "Found garbage in module directory, '%s' is not a shared object\n", fname);
			goto module_next;
		}

		char absname[PATH_LEN_MAX];
		memset(absname, 0, sizeof(absname));

		strncat(absname, ueai_module_dir, sizeof(absname));
		strncat(absname, fname, sizeof(absname));

		void* module_hndl = dlopen(absname, RTLD_LAZY);
		if(!module_hndl) {
			fprintf(stderr, "Failed to load module '%s', skipping\n", absname);
			goto module_next;
		}

		struct ueai_module_ops* ops = dlsym(module_hndl, UEAI_MODULE_OPS_SYM);
		if(!ops) {
			fprintf(stderr, "Failed to find symbol '%s' in '%s', skipping module\n", UEAI_MODULE_OPS_SYM, absname);
			goto module_done;
		}

		struct ueai_update_ctx ctx;
		memset(&ctx, 0, sizeof(ctx));

		if((err = ops->init(&ctx)) {
			fprintf(stderr, "Failed to initialize module '%s' (%d), skipping module\n", ops->get_name(), err);
			goto module_done;
		}

		// TODO: Download manifest & firmware

module_done_initialized:
		ops->destroy(&ctx);
module_done:
		dlclose(module_hndl);
module_next:
		continue;
	}

out_moddir:
	closedir(moddir);
out:
	return err;
}
