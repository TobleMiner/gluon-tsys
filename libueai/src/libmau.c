#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <uci.h>

#define PACKAGE_AUTOUPDATER "autoupdater"
#define OPTION_MIRRORS "mirror"

ssize_t get_mirrorlist(char*** retval, char* branch) {
	int err = 0;
	struct uci_context* ctx = uci_alloc_context();
	if(!ctx) {
		err = -ENOMEM;
		goto fail;
	}

	// Fetch autoupdater config
	struct uci_package* p_au = NULL;
	if((err = -uci_load(ctx, PACKAGE_AUTOUPDATER, &p_au)) || !p_au) {
		goto fail_ctx_alloc;
	}

	// Get section corresponding to branch
	struct uci_section* sec_br = uci_lookup_section(ctx, p_au, branch);
	if(!sec_br) {
		err = -ENOENT;
		goto fail_ctx_alloc;
	}

	// Get mirrorlist option
	struct uci_option* opt_mirror = uci_lookup_option(ctx, sec_br, OPTION_MIRRORS);
	if(!opt_mirror) {
		err = -ENOENT;
		goto fail_ctx_alloc;
	}

	if(opt_mirror->type != UCI_TYPE_LIST) {
		err = -EINVAL;
		goto fail_ctx_alloc;
	}

	size_t num_mirrors = 0;
	struct uci_element* elem_mirror;

	uci_foreach_element(&opt_mirror->v.list, elem_mirror) {
		num_mirrors++;
	}

	char** mirrors = calloc(num_mirrors, sizeof(char*));
	if(!mirrors) {
		err = -ENOMEM;
		goto fail_ctx_alloc;
	}

	char** mirror_ptr = mirrors;
	uci_foreach_element(&opt_mirror->v.list, elem_mirror) {
		char* mirror = strdup(elem_mirror->name);
		if(!mirror) {
			err = -ENOMEM;
			goto fail_mirror_alloc;
		}
		*mirror_ptr++ = mirror;
	}

	*retval = mirrors;
	return num_mirrors;


fail_mirror_alloc:
	while(mirror_ptr-- > mirrors) {
		free(*mirror_ptr);
	}
	free(mirrors);
fail_ctx_alloc:
	uci_free_context(ctx);
fail:
	return err;
}

void free_mirrorlist(char** mirrors, size_t len) {
	while(len-- > 0) {
		free(mirrors++);
	}
}
