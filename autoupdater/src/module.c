#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "module_priv.h"
#include "util.h"

#define PATH_LEN_MAX 255

static const char* mau_module_dir = "/lib/mau.d/"

static char* get_fext(char* fname) {
	return strrchr(fname, '.') + 1;
}

int mau_run_modules() {
	int err = 0;
	DIR* moddir = opendir(mau_module_dir);
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

		char absname[PATH_LEN_MAX];
		memset(absname, 0, sizeof(absname));

		strncat(absname, )
	}

out_moddir:
	closedir(moddir);
out:
	return err;
}
