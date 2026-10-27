/*
visual-sidechain
Copyright (C) 2026 Jeffrey A. Brick
SPDX-License-Identifier: GPL-2.0-or-later

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>
#include <string.h>

#include "obs/modulator-filter.h"
#include "obs/shader-filter.h"
#include "obs/vendor-api.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static obs_hotkey_id g_panic_hotkey_id = OBS_INVALID_HOTKEY_ID;
static obs_hotkey_id g_restore_hotkey_id = OBS_INVALID_HOTKEY_ID;

static bool enum_panic_sources_cb(void *param, obs_source_t *source)
{
	UNUSED_PARAMETER(param);
	if (!source)
		return true;
	const char *id = obs_source_get_unversioned_id(source);
	if (!id)
		return true;
	if (strcmp(id, "visual_sidechain_shader_filter") == 0) {
		vs_shader_filter_apply_panic(source);
	} else if (strcmp(id, "visual_sidechain_modulator_filter") == 0) {
		vs_modulator_filter_apply_panic(source);
	}
	return true;
}

static bool enum_restore_sources_cb(void *param, obs_source_t *source)
{
	UNUSED_PARAMETER(param);
	if (!source)
		return true;
	const char *id = obs_source_get_unversioned_id(source);
	if (!id)
		return true;
	if (strcmp(id, "visual_sidechain_shader_filter") == 0) {
		vs_shader_filter_apply_restore(source);
	} else if (strcmp(id, "visual_sidechain_modulator_filter") == 0) {
		vs_modulator_filter_apply_restore(source);
	}
	return true;
}

static void on_panic_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (!pressed)
		return;
	vs_fader_bridge_trigger_panic();
	obs_enum_all_sources(enum_panic_sources_cb, NULL);
	obs_log(LOG_INFO,
		"Panic hotkey triggered: shader filters set to pass-through and all strobe, flash, and modulation amounts zeroed");
}

static void on_restore_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);
	if (!pressed)
		return;
	vs_fader_bridge_trigger_restore();
	obs_enum_all_sources(enum_restore_sources_cb, NULL);
	obs_log(LOG_INFO,
		"Restore hotkey triggered: shader filters restored from pass-through and pre-panic amounts restored");
}

/* Shown by the OBS 33 Plugin Manager when data/manifest.json gives no display name. Keep both strings in sync
 * with buildspec.json (displayName) and cmake/common/manifest.json.in (description). */
const char *obs_module_name(void)
{
	return "Visual Sidechain";
}

const char *obs_module_description(void)
{
	return "FFT-reactive GPU shader filters and sibling-filter Audio Modulator for OBS Studio 33.";
}

bool obs_module_load(void)
{
	vs_register_modulator_filter();
	vs_register_shader_filter();
	g_panic_hotkey_id = obs_hotkey_register_frontend("visual_sidechain_panic", obs_module_text("Hotkey.Panic"),
							 on_panic_hotkey, NULL);
	g_restore_hotkey_id = obs_hotkey_register_frontend("visual_sidechain_restore",
							   obs_module_text("Hotkey.Restore"), on_restore_hotkey, NULL);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	vs_vendor_api_register();
}

void obs_module_unload(void)
{
	if (g_panic_hotkey_id != OBS_INVALID_HOTKEY_ID) {
		obs_hotkey_unregister(g_panic_hotkey_id);
		g_panic_hotkey_id = OBS_INVALID_HOTKEY_ID;
	}
	if (g_restore_hotkey_id != OBS_INVALID_HOTKEY_ID) {
		obs_hotkey_unregister(g_restore_hotkey_id);
		g_restore_hotkey_id = OBS_INVALID_HOTKEY_ID;
	}
	vs_fader_bridge_detach();
	obs_log(LOG_INFO, "plugin unloaded");
}
