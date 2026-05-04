// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "module.h"

nf_cfg_option_t *nf_cfg_option_make(nf_cfg_type_t type, const char *key, const char *title, const char *description) {
	if (!key)
		return NULL;
	nf_cfg_option_t *opt;
	NF_MALLOC(opt, sizeof(*opt), return NULL);
	opt->type		 = type;
	opt->key		 = strdup(key);
	opt->title		 = title && *title ? strdup(title) : NULL;
	opt->description = description && *description ? strdup(description) : NULL;
	return opt;
}

nf_cfg_option_t *nf_cfg_option_clone(const nf_cfg_option_t *src) {
	if (!src)
		return NULL;
	nf_cfg_option_t *dst;
	dst = nf_cfg_option_make(src->type, src->key, src->title, src->description);
	if (!dst)
		return NULL;

	nf_cfg_choice_t *choice = NULL;
	DL_FOREACH(src->choices, choice) {
		nf_cfg_choice_t *choice2 = nf_cfg_choice_clone(choice);
		if (choice2)
			DL_APPEND(dst->choices, choice2);
	}

	return dst;
}

void nf_cfg_option_dump(nf_cfg_option_t *opt) {
	if (!opt)
		return;
	nf_cfg_choice_t *choice = NULL;
	switch (opt->type) {
		case CFG_TYPE_SWITCH:
			LT_I("  - %s = %s - %s", opt->key, opt->value ? "true" : "false", opt->title);
			break;
		case CFG_TYPE_TEXT:
			LT_I("  - %s = '%s' - %s", opt->key, opt->size ? (char *)opt->value : NULL, opt->title);
			break;
		case CFG_TYPE_NUMBER:
			LT_I("  - %s = %llu - %s", opt->key, (uintptr_t)opt->value, opt->title);
			break;
		case CFG_TYPE_SINGLE_CHOICE:
			LT_I("  - %s - %s (single)", opt->key, opt->title);
			DL_FOREACH(opt->choices, choice) {
				nf_cfg_choice_dump(choice);
			}
			break;
		case CFG_TYPE_MULTI_CHOICE:
			LT_I("  - %s - %s (multi)", opt->key, opt->title);
			DL_FOREACH(opt->choices, choice) {
				nf_cfg_choice_dump(choice);
			}
			break;
	}
}

void nf_cfg_option_free(nf_cfg_option_t *opt) {
	if (!opt)
		return;
	nf_cfg_choice_t *choice, *tmp;
	DL_FOREACH_SAFE(opt->choices, choice, tmp) {
		DL_DELETE(opt->choices, choice);
		nf_cfg_choice_free(choice);
	}
	free(opt->key);
	free(opt->title);
	free(opt->description);
	if (opt->size)
		free(opt->value);
	free(opt);
}

nf_cfg_option_t *nf_cfg_option_find(nf_cfg_option_t *opts, const char *key) {
	if (!opts || !key)
		return NULL;
	nf_cfg_option_t *el = NULL;
	DL_FOREACH(opts, el) {
		if (strcmp(key, el->key) == 0)
			return el;
		const char *end;
		if (el->key[0] == '(' && (end = strchr(el->key, ')')) && strcmp(key, end + 1) == 0)
			return el;
	}
	return NULL;
}

void nf_cfg_option_set_switch(nf_cfg_option_t *opt, bool value) {
	if (!opt || opt->type != CFG_TYPE_SWITCH)
		return;
	opt->value = (void *)value;
}

void nf_cfg_option_set_text(nf_cfg_option_t *opt, const char *value) {
	if (!opt || opt->type != CFG_TYPE_TEXT)
		return;
	size_t length = value ? strlen(value) : 0;
	if (opt->size)
		free(opt->value);
	if (length)
		opt->value = memdup(value, length + 1);
	else
		opt->value = NULL;
	opt->size = length && opt->value ? length + 1 : 0;
}

void nf_cfg_option_set_number(nf_cfg_option_t *opt, uint32_t value) {
	if (!opt || opt->type != CFG_TYPE_NUMBER)
		return;
	opt->value = (void *)(uintptr_t)value;
}

void nf_cfg_option_set_single(nf_cfg_option_t *opt, const char *key) {
	if (!opt || opt->type != CFG_TYPE_SINGLE_CHOICE || !opt->choices)
		return;
	const nf_cfg_choice_t *choice = nf_cfg_choice_find(opt->choices, key);
	nf_cfg_choice_t *el			  = NULL;
	DL_FOREACH(opt->choices, el) {
		el->selected = el == choice;
	}
}

void nf_cfg_option_set_multi(nf_cfg_option_t *opt, const char *key, bool value) {
	if (!opt || opt->type != CFG_TYPE_MULTI_CHOICE || !opt->choices)
		return;
	nf_cfg_choice_t *choice = nf_cfg_choice_find(opt->choices, key);
	if (choice)
		choice->selected = value;
}

nf_cfg_choice_t *nf_cfg_option_get_single(nf_cfg_option_t *opt) {
	if (!opt || opt->type != CFG_TYPE_SINGLE_CHOICE || !opt->choices)
		return NULL;
	nf_cfg_choice_t *el = NULL;
	DL_FOREACH(opt->choices, el) {
		if (el->selected)
			return el;
	}
	return NULL;
}

nf_cfg_option_t *nf_cfg_option_make_dev_mac() {
	return nf_cfg_option_make(
		CFG_TYPE_TEXT,
		"(dev-macaddr)macaddr",
		"Device MAC address",
		"Choose an existing device or enter a custom MAC address"
	);
}

nf_cfg_option_t *nf_cfg_option_make_dev_ip() {
	return nf_cfg_option_make(
		CFG_TYPE_TEXT,
		"(dev-ipaddr)ipaddr",
		"Device IP address",
		"Choose an existing device or enter a custom IP address"
	);
}
