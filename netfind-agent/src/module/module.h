// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#include <netfind.h>

#include "module_types.h"

// ethcap/ethcap.c
extern module_ops_t ethcap_ops;

// module.c
module_t *module_init(devdb_t *devdb, const module_ops_t *ops);
void module_free(module_t *mod);
nf_err_t module_start(module_t *mod);
nf_err_t module_stop(module_t *mod);
nf_err_t module_option_update(module_t *mod, nf_cfg_option_t *opt);
nf_err_t module_action_call(module_t *mod, nf_action_t *act);

// module_action.c
nf_action_t *nf_action_make(const char *key, const char *title, const char *description);
nf_action_t *nf_action_clone(const nf_action_t *src);
void nf_action_dump(nf_action_t *act);
void nf_action_free(nf_action_t *act);

// module_cfg_option.c
nf_cfg_option_t *nf_cfg_option_make(nf_cfg_type_t type, const char *key, const char *title, const char *description);
nf_cfg_option_t *nf_cfg_option_clone(const nf_cfg_option_t *src);
void nf_cfg_option_dump(nf_cfg_option_t *opt);
void nf_cfg_option_free(nf_cfg_option_t *opt);
nf_cfg_option_t *nf_cfg_option_find(nf_cfg_option_t *opts, const char *key);
void nf_cfg_option_set_switch(nf_cfg_option_t *opt, bool value);
void nf_cfg_option_set_text(nf_cfg_option_t *opt, const char *value);
void nf_cfg_option_set_number(nf_cfg_option_t *opt, uint32_t value);
void nf_cfg_option_set_single(nf_cfg_option_t *opt, const char *key);
void nf_cfg_option_set_multi(nf_cfg_option_t *opt, const char *key, bool value);
nf_cfg_choice_t *nf_cfg_option_get_single(nf_cfg_option_t *opt);
nf_cfg_option_t *nf_cfg_option_make_dev_mac();
nf_cfg_option_t *nf_cfg_option_make_dev_ip();

// module_cfg_choice.c
nf_cfg_choice_t *nf_cfg_choice_make(const char *key, const char *title, const char *description);
nf_cfg_choice_t *nf_cfg_choice_clone(const nf_cfg_choice_t *src);
void nf_cfg_choice_dump(nf_cfg_choice_t *choice);
void nf_cfg_choice_free(nf_cfg_choice_t *choice);
nf_cfg_choice_t *nf_cfg_choice_find(nf_cfg_choice_t *choices, const char *key);
