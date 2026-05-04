// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#pragma once

#include <netfind.h>

typedef struct devdb_t devdb_t;
typedef struct module_t module_t;

typedef enum nf_cfg_type_t {
	CFG_TYPE_SWITCH = 1,
	CFG_TYPE_TEXT,
	CFG_TYPE_NUMBER,
	CFG_TYPE_SINGLE_CHOICE = 10,
	CFG_TYPE_MULTI_CHOICE  = 11,
} nf_cfg_type_t;

typedef struct nf_cfg_choice_t {
	char *key;
	char *title;
	char *description;
	bool selected;
	void *data;

	struct nf_cfg_choice_t *prev, *next;
} nf_cfg_choice_t;

typedef struct nf_cfg_option_t {
	nf_cfg_type_t type;
	char *key;
	char *title;
	char *description;

	void *value;   //!< Value data
	uint32_t size; //!< Size of memory allocated to value

	nf_cfg_choice_t *choices;
	struct nf_cfg_option_t *prev, *next;
} nf_cfg_option_t;

typedef struct nf_action_t {
	char *key;
	char *title;
	char *description;
	nf_cfg_option_t *options;
	struct nf_action_t *prev, *next;
} nf_action_t;

typedef module_t *(*module_init_t)(devdb_t *devdb);
typedef void (*module_free_t)(module_t *module);
typedef nf_err_t (*module_start_t)(module_t *module);
typedef nf_err_t (*module_stop_t)(module_t *module);
typedef nf_err_t (*module_option_update_t)(module_t *module, nf_cfg_option_t *opt);
typedef nf_err_t (*module_action_call_t)(module_t *module, nf_action_t *call);

typedef struct module_ops_t {
	const char *name;
	module_init_t init;
	module_free_t free;
	module_start_t start;
	module_stop_t stop;
	module_option_update_t option_update;
	module_action_call_t action_call;
} module_ops_t;

typedef struct module_t {
	module_ops_t *ops;
	nf_cfg_option_t *cfg;
	nf_action_t *act;
	pthread_t thread;
} module_t;
