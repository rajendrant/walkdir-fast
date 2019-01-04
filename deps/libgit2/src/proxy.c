/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "proxy.h"

#include "git2/proxy.h"

#ifdef DISABLED_FUNCTION
int git_proxy_init_options(git_proxy_options *opts, unsigned int version)
{
	GIT_INIT_STRUCTURE_FROM_TEMPLATE(
		opts, version, git_proxy_options, GIT_PROXY_OPTIONS_INIT);
	return 0;
}
#endif // DISABLED_FUNCTION

#ifdef DISABLED_FUNCTION
int git_proxy_options_dup(git_proxy_options *tgt, const git_proxy_options *src)
{
	if (!src) {
		git_proxy_init_options(tgt, GIT_PROXY_OPTIONS_VERSION);
		return 0;
	}

	memcpy(tgt, src, sizeof(git_proxy_options));
	if (src->url) {
		tgt->url = git__strdup(src->url);
		GITERR_CHECK_ALLOC(tgt->url);
	}

	return 0;
}
#endif // DISABLED_FUNCTION

#ifdef DISABLED_FUNCTION
void git_proxy_options_clear(git_proxy_options *opts)
{
	git__free((char *) opts->url);
	opts->url = NULL;
}
#endif // DISABLED_FUNCTION
