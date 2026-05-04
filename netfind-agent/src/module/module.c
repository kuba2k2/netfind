// Copyright (c) Kuba Szczodrzyński 2026-4-9.

#include "module.h"

module_t *module_init(devdb_t *devdb, const module_ops_t *ops) {
	void *mod = ops->init(devdb);
	if (!mod) {
		LT_E("Module '%s' init failed", ops->name);
		return NULL;
	}
	return mod;
}

void module_free(module_t *mod) {
	if (!mod)
		return;
	mod->ops->free(mod);
}

nf_err_t module_start(module_t *mod) {
	if (!mod)
		return NF_ERR_NULL;
	nf_err_t err;
	if ((err = mod->ops->start(mod)))
		LT_E("Module '%s' failed to start; err=%d", mod->ops->name, err);
	return err;
}

nf_err_t module_stop(module_t *mod) {
	if (!mod)
		return NF_ERR_NULL;
	nf_err_t err;
	if ((err = mod->ops->stop(mod)))
		LT_E("Module '%s' failed to stop; err=%d", mod->ops->name, err);
	return err;
}

nf_err_t module_option_update(module_t *mod, nf_cfg_option_t *opt) {
	if (!mod)
		return NF_ERR_NULL;
	nf_err_t err;
	if ((err = mod->ops->option_update(mod, opt)))
		LT_E("Module '%s' failed to update option '%s'; err=%d", mod->ops->name, opt->key, err);
	return err;
}

nf_err_t module_action_call(module_t *mod, nf_action_t *act) {
	if (!mod)
		return NF_ERR_NULL;
	nf_err_t err;
	if ((err = mod->ops->action_call(mod, act)))
		LT_E("Module '%s' failed to call action '%s'; err=%d", mod->ops->name, act->key, err);
	return err;
}
