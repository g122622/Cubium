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
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/FireInfoRegistry.hpp"
#include "world/block/registry/HarvestToolTable.hpp"

#include <chrono>

#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/block/registry/AgriculturalBlocks.hpp"
#include "common/world/block/registry/BambooBlocks.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/BuildingBlocks.hpp"
#include "common/world/block/registry/BuildingVariantBlocks.hpp"
#include "common/world/block/registry/CandleBlocks.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/CherryBlocks.hpp"
#include "common/world/block/registry/ColoredBlocks.hpp"
#include "common/world/block/registry/CopperBlocks.hpp"
#include "common/world/block/registry/DeepslateBlocks.hpp"
#include "common/world/block/registry/FlowerPotBlocks.hpp"
#include "common/world/block/registry/GardenBlocks.hpp"
#include "common/world/block/registry/MangroveBlocks.hpp"
#include "common/world/block/registry/MudBlocks.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/PaleGardenBlocks.hpp"
#include "common/world/block/registry/RedstoneBlocks.hpp"
#include "common/world/block/registry/SculkBlocks.hpp"
#include "common/world/block/registry/ShelfBlocks.hpp"
#include "common/world/block/registry/SignBannerBlocks.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/TrialBlocks.hpp"
#include "common/world/block/registry/TuffBlocks.hpp"
#include "common/world/block/registry/VegetationBlocks.hpp"
#include "common/world/block/registry/WildBlocks.hpp"

#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

bool VanillaBlocks::s_initialized = false;

void VanillaBlocks::initialize()
{
    if (s_initialized) {
        return;
    }

    // TEMP-DIAG：定位 CI 上首次方块注册耗时（验证后回退）。
    const auto diagStart = std::chrono::steady_clock::now();
    const auto diagMark = [&diagStart](const char* step) {
        spdlog::info("[BlockTiming] {} {}ms",
            step,
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - diagStart)
                .count());
    };
    diagMark("initialize_enter");

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBaseBlocks");
        block_registry::registerBaseBlocks();
        diagMark("registerBaseBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBuildingBlocks");
        block_registry::registerBuildingBlocks();
        diagMark("registerBuildingBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCaveBlocks");
        block_registry::registerCaveBlocks();
        diagMark("registerCaveBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerDeepslateBlocks");
        block_registry::registerDeepslateBlocks();
        diagMark("registerDeepslateBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCopperBlocks");
        block_registry::registerCopperBlocks();
        diagMark("registerCopperBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerNetherBlocks");
        block_registry::registerNetherBlocks();
        diagMark("registerNetherBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerVegetationBlocks");
        block_registry::registerVegetationBlocks();
        diagMark("registerVegetationBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerNaturalBlocks");
        block_registry::registerNaturalBlocks();
        diagMark("registerNaturalBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerAgriculturalBlocks");
        block_registry::registerAgriculturalBlocks();
        diagMark("registerAgriculturalBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerColoredBlocks");
        block_registry::registerColoredBlocks();
        diagMark("registerColoredBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerRedstoneBlocks");
        block_registry::registerRedstoneBlocks();
        diagMark("registerRedstoneBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerSignBannerBlocks");
        block_registry::registerSignBannerBlocks();
        diagMark("registerSignBannerBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBuildingVariantBlocks");
        block_registry::registerBuildingVariantBlocks();
        diagMark("registerBuildingVariantBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCherryBlocks");
        block_registry::registerCherryBlocks();
        diagMark("registerCherryBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBambooBlocks");
        block_registry::registerBambooBlocks();
        diagMark("registerBambooBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerTrailsBlocks");
        block_registry::registerTrailsBlocks();
        diagMark("registerTrailsBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerSculkBlocks");
        block_registry::registerSculkBlocks();
        diagMark("registerSculkBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerMangroveBlocks");
        block_registry::registerMangroveBlocks();
        diagMark("registerMangroveBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerMudBlocks");
        block_registry::registerMudBlocks();
        diagMark("registerMudBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerWildBlocks");
        block_registry::registerWildBlocks();
        diagMark("registerWildBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerTuffBlocks");
        block_registry::registerTuffBlocks();
        diagMark("registerTuffBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerTrialBlocks");
        block_registry::registerTrialBlocks();
        diagMark("registerTrialBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerPaleGardenBlocks");
        block_registry::registerPaleGardenBlocks();
        diagMark("registerPaleGardenBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerGardenBlocks");
        block_registry::registerGardenBlocks();
        diagMark("registerGardenBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerShelfBlocks");
        block_registry::registerShelfBlocks();
        diagMark("registerShelfBlocks_done");
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCandleBlocks");
        block_registry::registerCandleBlocks();
        diagMark("registerCandleBlocks_done");
    }
    {
        // 花盆方块必须在所有内容物方块（花卉、树苗、蘑菇、仙人掌、下界菌、杜鹃花等）注册之后
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerFlowerPotBlocks");
        block_registry::registerFlowerPotBlocks();
        diagMark("registerFlowerPotBlocks_done");
    }

    // 刷新全部状态的光照缓存（必须在所有方块构造完成后）。
    // BlockState 在 StateContainer 构造期缓存 opacity/pSD，此时方块子类构造尚未完成，
    // 对重写 getOpacity/propagatesSkylightDown 的方块（冰、雪层等）会缓存基类判定，须在此重算。
    BlockRegistry::instance().forEachBlock([](Block& block) {
        block.refreshLightCache();
    });

    // 初始化方块标签表（必须在所有方块注册后）。标签内容本身来自数据包：客户端与服务端
    // 都由各自的启动流程在方块注册完成后调用 BlockTagLoader 加载，这里只切换状态。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeBlockTags");
        spdlog::info("[VanillaBlocks] Initializing block tags...");
        BlockTags::initialize();
        diagMark("block_tags_done");
        spdlog::info("[VanillaBlocks] Block tags initialized");
    }

    // 初始化原版方块燃烧参数（必须在所有方块注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeVanillaFireInfos");
        blocks::FireInfoRegistry::instance().initializeVanillaFireInfos();
        diagMark("fire_infos_done");
        spdlog::info("[VanillaBlocks] Fire info initialized");
    }

    s_initialized = true;
    diagMark("initialize_done");
}

} // namespace mc
