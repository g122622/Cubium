/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "world/block/registry/VanillaBlocks.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/FireInfoRegistry.hpp"

#include "common/perfetto/TraceEvents.hpp"

#include <spdlog/spdlog.h>

namespace mc {

bool VanillaBlocks::s_initialized = false;

void VanillaBlocks::initialize()
{
    if (s_initialized) {
        return;
    }

    {
        MC_TRACE_EVENT("client.initialization", "registerBaseBlocks");
        block_registry::registerBaseBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerBuildingBlocks");
        block_registry::registerBuildingBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerCaveBlocks");
        block_registry::registerCaveBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerDeepslateBlocks");
        block_registry::registerDeepslateBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerCopperBlocks");
        block_registry::registerCopperBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerNetherBlocks");
        block_registry::registerNetherBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerVegetationBlocks");
        block_registry::registerVegetationBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerNaturalBlocks");
        block_registry::registerNaturalBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerColoredBlocks");
        block_registry::registerColoredBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerRedstoneBlocks");
        block_registry::registerRedstoneBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerSignBannerBlocks");
        block_registry::registerSignBannerBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerBuildingVariantBlocks");
        block_registry::registerBuildingVariantBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerCherryBlocks");
        block_registry::registerCherryBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerBambooBlocks");
        block_registry::registerBambooBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerTrailsBlocks");
        block_registry::registerTrailsBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerSculkBlocks");
        block_registry::registerSculkBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerMangroveBlocks");
        block_registry::registerMangroveBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerMudBlocks");
        block_registry::registerMudBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerWildBlocks");
        block_registry::registerWildBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerTuffBlocks");
        block_registry::registerTuffBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerTrialBlocks");
        block_registry::registerTrialBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerPaleGardenBlocks");
        block_registry::registerPaleGardenBlocks();
    }
    {
        MC_TRACE_EVENT("client.initialization", "registerGardenBlocks");
        block_registry::registerGardenBlocks();
    }

    // 初始化方块标签（必须在所有方块注册后）
    {
        MC_TRACE_EVENT("client.initialization", "InitializeBlockTags");
        spdlog::info("[VanillaBlocks] Initializing block tags...");
        BlockTags::initialize();
        spdlog::info("[VanillaBlocks] Block tags initialized");
    }

    // 初始化原版方块燃烧参数（必须在所有方块注册后）
    {
        MC_TRACE_EVENT("client.initialization", "InitializeVanillaFireInfos");
        blocks::FireInfoRegistry::instance().initializeVanillaFireInfos();
        spdlog::info("[VanillaBlocks] Fire info initialized");
    }

    s_initialized = true;
}

} // namespace mc
