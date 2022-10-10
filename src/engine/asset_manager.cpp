

#include "asset_manager.h"

#include "graphics/material.h"
#include "utils/profiler.h"

void AssetManager::refresh_dirty_assets() const
{
	STAT_DURATION("Check asset updates");
	for (const auto& material : materials)
	{
		material->check_updates();
	}
}
