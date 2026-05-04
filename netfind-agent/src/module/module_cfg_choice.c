// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "module.h"

nf_cfg_choice_t *nf_cfg_choice_make(const char *key, const char *title, const char *description) {
	if (!key)
		return NULL;
	nf_cfg_choice_t *choice;
	NF_MALLOC(choice, sizeof(*choice), return NULL);
	choice->key			= strdup(key);
	choice->title		= title && *title ? strdup(title) : NULL;
	choice->description = description && *description ? strdup(description) : NULL;
	return choice;
}

nf_cfg_choice_t *nf_cfg_choice_clone(const nf_cfg_choice_t *src) {
	if (!src)
		return NULL;
	nf_cfg_choice_t *dst;
	dst = nf_cfg_choice_make(src->key, src->title, src->description);
	return dst;
}

void nf_cfg_choice_dump(nf_cfg_choice_t *choice) {
	if (!choice)
		return;
	LT_I("    - %s = %s - %s", choice->key, choice->selected ? "true" : "false", choice->title);
	if (choice->description)
		LT_I("      %s", choice->description);
}

void nf_cfg_choice_free(nf_cfg_choice_t *choice) {
	if (!choice)
		return;
	free(choice->key);
	free(choice->title);
	free(choice->description);
	free(choice);
}

nf_cfg_choice_t *nf_cfg_choice_find(nf_cfg_choice_t *choices, const char *key) {
	if (!choices || !key)
		return NULL;
	nf_cfg_choice_t *el = NULL;
	DL_FOREACH(choices, el) {
		if (strcmp(key, el->key) == 0)
			return el;
	}
	return NULL;
}
