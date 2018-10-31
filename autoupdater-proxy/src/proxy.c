#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <libubox/uloop.h>
#include <time.h>

#include "fetch.h"
#include "http.h"
#include "mirrors.h"
#include "util.h"

static ssize_t query_string_decode_value(char* value) {
	char* search_pos = value, *limit = value + strlen(value);

	strtr(value, '+', ' ');

	while(search_pos < limit) {
		if(*search_pos != '%') {
			search_pos++;
			continue;
		}

		if(limit - search_pos < 2) {
			return -EINVAL;
		}

		*search_pos = hex_to_byte(search_pos + 1);
		search_pos++;
		limit -= 2;
		memmove(search_pos, search_pos + 2, limit - search_pos);
	}

	return limit - value;
}

/*
 * Searches a query string for a given key, writes a pointer to the value to retval, and returns it's length.
 * If key is not found a negative error value is returned.
 */
static ssize_t query_string_find_value(char** retval, const char* query_string, const char* key) {
	int err = ENOENT;
	const char* limit = query_string + strlen(query_string);
	const char* cursor = query_string;
	while(cursor < limit) {
		char* res = strstr(cursor, key);
		// Abort if string not found
		if(!res) {
			err = ENOENT;
			break;
		}

		res += strlen(key);
		// Abort if searching past key puts us outside the string
		if(res >= limit) {
			err = ENOENT;
			break;
		}

		// Find key delimiter
		if(*res++ == '=') {
			// Find value delimiter
			const char* end = strchr(res, '&');
			if(!end) {
				end = limit;
			}
			*retval = res;
			return end - res;
		}
		cursor = res;
	}

	return -err;
}

static char* query_string_get_value(const char* query_string, const char* key) {
	char* qry_prm;
	ssize_t len = query_string_find_value(&qry_prm, query_string, key);
	if(len < 0) {
		return NULL;
	}

	char* val = strndup(qry_prm, len);
	ssize_t end = query_string_decode_value(val);
	if(end < 0) {
		free(val);
		return NULL;
	}
	// Ensure NULL termination
	val[end] = 0;

	return val;
}

#define MAX_URL_LEN 256

static int download_file(char* branch, char* file) {
	int err = 0;
	char** mirrorlist;
	ssize_t num_mirrors = get_mirrorlist(&mirrorlist, branch);
	if(num_mirrors < 0) {
		err = num_mirrors;
		fprintf(stderr, "Failed to get mirrorlist: %s(%d)\n", strerror(-err), err);
		goto out;
	}

	srand((int)time(NULL));
	ARRAY_SHUFFLE(mirrorlist, num_mirrors);

	char** mirror_ptr = mirrorlist;
	size_t mirror_cnt = num_mirrors;
	while(mirror_cnt-- > 0) {
		char* mirror = *mirror_ptr++;
		char url[MAX_URL_LEN];
		if(snprintf(url, sizeof(url), "%s/%s", mirror, file) >= sizeof(url)) {
			fprintf(stderr, "Skipping mirror '%s' with overly long file URL\n", mirror);
			continue;
		}

		if((err = spider_url(url))) {
			fprintf(stderr, "Failed to find file on mirror '%s', url: '%s', skipping mirror\n", mirror, url);
			continue;
		}

		if((err = get_url(url))) {
			fprintf(stderr, "Failed to download file '%s' from mirror '%s', url: '%s', skipping mirror\n", file, mirror, url);
			continue;
		}
		break;
	}

	free_mirrorlist(mirrorlist, num_mirrors);

out:
	return err;
}


#define QUERY_BRANCH "branch"
#define QUERY_FILE "file"

#define LOCKFILE "/tmp/miau.lock"

int main(int argc, char** argv) {
	int err = 0;

	// Acquire exclusive lock
	int lockfd = open(LOCKFILE, O_CREAT | O_RDONLY);
	if(lockfd >= 0) {
		int lockok = flock(lockfd, LOCK_EX | LOCK_NB);
		if(lockok) {
			fprintf(stderr, "Failed to acquire lock, exiting\n");
			err = HTTP_503;
			goto out_lockfd;
		}
	} else {
		// Don't break functionality if lockfiles are broken
		fprintf(stderr, "Failed to open lockfile, continuing without lock!\n");
	}

	// Get query string
	char* query_string = getenv("QUERY_STRING");
	if(!query_string) {
		fprintf(stderr, "No query string found\n");
		err = HTTP_400;
		goto out_lock;
	}

	char* qry_prm_branch;
	qry_prm_branch = query_string_get_value(query_string, QUERY_BRANCH);
	if(!qry_prm_branch) {
		fprintf(stderr, "Failed to get param '%s' from query string\n", QUERY_BRANCH);
		err = HTTP_400;
		goto out_lock;
	}

	char* qry_prm_file;
	qry_prm_file = query_string_get_value(query_string, QUERY_FILE);
	if(!qry_prm_file) {
		fprintf(stderr, "Failed to get param '%s' from query string\n", QUERY_FILE);
		err = HTTP_400;
		goto out_branch_alloc;
	}

	uloop_init();

	download_file(qry_prm_branch, qry_prm_file);

	uloop_done();

	free(qry_prm_file);
out_branch_alloc:
	free(qry_prm_branch);
out_lock:
	if(lockfd >= 0) {
		flock(lockfd, LOCK_UN);
	}
out_lockfd:
	if(lockfd >= 0) {
		close(lockfd);
	}
	return err;
}
