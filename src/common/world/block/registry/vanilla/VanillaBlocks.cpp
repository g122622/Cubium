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

#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/fire/FireInfoRegistry.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/HarvestToolTable.hpp"
#include "common/world/block/tags/BlockTags.hpp"

#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/block/registry/vanilla/AgriculturalBlocks.hpp"
#include "common/world/block/registry/vanilla/BambooBlocks.hpp"
#include "common/world/block/registry/vanilla/BaseBlocks.hpp"
#include "common/world/block/registry/vanilla/BuildingBlocks.hpp"
#include "common/world/block/registry/vanilla/BuildingVariantBlocks.hpp"
#include "common/world/block/registry/vanilla/CandleBlocks.hpp"
#include "common/world/block/registry/vanilla/CaveBlocks.hpp"
#include "common/world/block/registry/vanilla/CherryBlocks.hpp"
#include "common/world/block/registry/vanilla/ColoredBlocks.hpp"
#include "common/world/block/registry/vanilla/CopperBlocks.hpp"
#include "common/world/block/registry/vanilla/DeepslateBlocks.hpp"
#include "common/world/block/registry/vanilla/FlowerPotBlocks.hpp"
#include "common/world/block/registry/vanilla/GardenBlocks.hpp"
#include "common/world/block/registry/vanilla/MangroveBlocks.hpp"
#include "common/world/block/registry/vanilla/MudBlocks.hpp"
#include "common/world/block/registry/vanilla/NaturalBlocks.hpp"
#include "common/world/block/registry/vanilla/NetherBlocks.hpp"
#include "common/world/block/registry/vanilla/PaleGardenBlocks.hpp"
#include "common/world/block/registry/vanilla/RedstoneBlocks.hpp"
#include "common/world/block/registry/vanilla/SculkBlocks.hpp"
#include "common/world/block/registry/vanilla/ShelfBlocks.hpp"
#include "common/world/block/registry/vanilla/SignBannerBlocks.hpp"
#include "common/world/block/registry/vanilla/TrailsBlocks.hpp"
#include "common/world/block/registry/vanilla/TrialBlocks.hpp"
#include "common/world/block/registry/vanilla/TuffBlocks.hpp"
#include "common/world/block/registry/vanilla/VegetationBlocks.hpp"
#include "common/world/block/registry/vanilla/WildBlocks.hpp"

#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {

bool VanillaBlocks::s_initialized = false;

void VanillaBlocks::initialize()
{
    if (s_initialized) {
        return;
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBaseBlocks");
        block_registry::registerBaseBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBuildingBlocks");
        block_registry::registerBuildingBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCaveBlocks");
        block_registry::registerCaveBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerDeepslateBlocks");
        block_registry::registerDeepslateBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCopperBlocks");
        block_registry::registerCopperBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerNetherBlocks");
        block_registry::registerNetherBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerVegetationBlocks");
        block_registry::registerVegetationBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerNaturalBlocks");
        block_registry::registerNaturalBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerAgriculturalBlocks");
        block_registry::registerAgriculturalBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerColoredBlocks");
        block_registry::registerColoredBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerRedstoneBlocks");
        block_registry::registerRedstoneBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerSignBannerBlocks");
        block_registry::registerSignBannerBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBuildingVariantBlocks");
        block_registry::registerBuildingVariantBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCherryBlocks");
        block_registry::registerCherryBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerBambooBlocks");
        block_registry::registerBambooBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerTrailsBlocks");
        block_registry::registerTrailsBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerSculkBlocks");
        block_registry::registerSculkBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerMangroveBlocks");
        block_registry::registerMangroveBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerMudBlocks");
        block_registry::registerMudBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerWildBlocks");
        block_registry::registerWildBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerTuffBlocks");
        block_registry::registerTuffBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerTrialBlocks");
        block_registry::registerTrialBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerPaleGardenBlocks");
        block_registry::registerPaleGardenBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerGardenBlocks");
        block_registry::registerGardenBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerShelfBlocks");
        block_registry::registerShelfBlocks();
    }
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerCandleBlocks");
        block_registry::registerCandleBlocks();
    }
    {
        // 花盆方块必须在所有内容物方块（花卉、树苗、蘑菇、仙人掌、下界菌、杜鹃花等）注册之后
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "registerFlowerPotBlocks");
        block_registry::registerFlowerPotBlocks();
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
        spdlog::info("[VanillaBlocks] Block tags initialized");
    }

    // 初始化原版方块燃烧参数（必须在所有方块注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeVanillaFireInfos");
        blocks::FireInfoRegistry::instance().initializeVanillaFireInfos();
        spdlog::info("[VanillaBlocks] Fire info initialized");
    }

    s_initialized = true;
}

} // namespace mc
