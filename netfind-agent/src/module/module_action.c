// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "module.h"

nf_action_t *nf_action_make(const char *key, const char *title, const char *description) {
	if (!key)
		return NULL;
	nf_action_t *act;
	NF_MALLOC(act, sizeof(*act), return NULL);
	act->key		 = strdup(key);
	act->title		 = title && *title ? strdup(title) : NULL;
	act->description = description && *description ? strdup(description) : NULL;
	return act;
}

nf_action_t *nf_action_clone(const nf_action_t *src) {
	if (!src)
		return NULL;
	nf_action_t *dst;
	dst = nf_action_make(src->key, src->title, src->description);
	if (!dst)
		return NULL;

	nf_cfg_option_t *opt = NULL;
	DL_FOREACH(src->options, opt) {
		nf_cfg_option_t *opt2 = nf_cfg_option_clone(opt);
		if (opt2)
			DL_APPEND(dst->options, opt2);
	}

	return dst;
}

void nf_action_dump(nf_action_t *act) {
	if (!act)
		return;
	nf_cfg_option_t *opt = NULL;
	LT_I("- %s - %s", act->key, act->title);
	DL_FOREACH(act->options, opt) {
		nf_cfg_option_dump(opt);
	}
}

void nf_action_free(nf_action_t *act) {
	if (!act)
		return;
	nf_cfg_option_t *opt, *tmp;
	DL_FOREACH_SAFE(act->options, opt, tmp) {
		DL_DELETE(act->options, opt);
		nf_cfg_option_free(opt);
	}
	free(act->key);
	free(act->title);
	free(act->description);
	free(act);
}
