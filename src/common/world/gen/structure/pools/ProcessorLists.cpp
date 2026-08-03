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
 *
 */

#include "ProcessorLists.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/gen/feature/template/RuleTest.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/jigsaw/ProcessorListRegistry.hpp"

#include <memory>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

using feature::template_::BlackstoneReplacementProcessor;
using feature::template_::BlockAgeProcessor;
using feature::template_::BlockIgnoreStructureProcessor;
using feature::template_::GravityStructureProcessor;
using feature::template_::IntegrityProcessor;
using feature::template_::LavaSubmergingProcessor;
using feature::template_::RuleEntry;
using feature::template_::RuleStructureProcessor;
using feature::template_::StructureProcessorList;

// ============================================================================
// 静态存储
// ============================================================================

// 空处理器
StructureProcessorList ProcessorLists::EMPTY;

// 僵尸村庄处理器
StructureProcessorList ProcessorLists::ZOMBIE_PLAINS;
StructureProcessorList ProcessorLists::ZOMBIE_DESERT;
StructureProcessorList ProcessorLists::ZOMBIE_SAVANNA;
StructureProcessorList ProcessorLists::ZOMBIE_SNOWY;
StructureProcessorList ProcessorLists::ZOMBIE_TAIGA;

// 苔藓化处理器
StructureProcessorList ProcessorLists::MOSSIFY_10_PERCENT;
StructureProcessorList ProcessorLists::MOSSIFY_20_PERCENT;
StructureProcessorList ProcessorLists::MOSSIFY_70_PERCENT;

// 道路处理器
StructureProcessorList ProcessorLists::STREET_PLAINS;
StructureProcessorList ProcessorLists::STREET_SAVANNA;
StructureProcessorList ProcessorLists::STREET_SNOWY_TAIGA;

// 农场处理器
StructureProcessorList ProcessorLists::FARM_PLAINS;
StructureProcessorList ProcessorLists::FARM_SAVANNA;
StructureProcessorList ProcessorLists::FARM_SNOWY;
StructureProcessorList ProcessorLists::FARM_TAIGA;
StructureProcessorList ProcessorLists::FARM_DESERT;

// 掠夺者前哨站处理器
StructureProcessorList ProcessorLists::OUTPOST_ROT;

// 堡垒遗迹处理器
StructureProcessorList ProcessorLists::BASTION_BOTTOM_RAMPART;
StructureProcessorList ProcessorLists::BASTION_TREASURE_ROOMS;
StructureProcessorList ProcessorLists::BASTION_HOUSING;
StructureProcessorList ProcessorLists::BASTION_SIDE_WALL_DEGRADATION;
StructureProcessorList ProcessorLists::BASTION_STABLE_DEGRADATION;
StructureProcessorList ProcessorLists::BASTION_GENERIC_DEGRADATION;
StructureProcessorList ProcessorLists::BASTION_RAMPART_DEGRADATION;
StructureProcessorList ProcessorLists::BASTION_ENTRANCE_REPLACEMENT;
StructureProcessorList ProcessorLists::BASTION_BRIDGE;
StructureProcessorList ProcessorLists::BASTION_ROOF;
StructureProcessorList ProcessorLists::BASTION_HIGH_WALL;
StructureProcessorList ProcessorLists::BASTION_HIGH_RAMPART;

// 初始化标志
namespace {
bool s_initialized = false;

// ============================================================================
// 辅助函数：创建僵尸村庄规则
// ============================================================================

/**
 * @brief 创建僵尸村庄的方块替换规则
 *
 * 僵尸村庄的方块替换规则包括：
 * 1. 苔藓化效果
 * 2. 特定方块替换（如玻璃 -> 空气，门 -> 空气等）
 */
StructureProcessorList createZombieVillageProcessor(f32 mossiness)
{
    StructureProcessorList list;

    // 1. 添加苔藓化处理器
    list.addProcessor(std::make_unique<BlockAgeProcessor>(mossiness));

    // 2. 添加方块替换规则
    // 僵尸村庄会将部分方块替换为其他方块
    // 例如：玻璃 -> 空气，门 -> 空气等
    // 这里使用 BlockIgnoreStructureProcessor 来移除某些方块
    std::vector<u32> blocksToIgnore;

    // 获取方块注册表
    auto& blockRegistry = BlockRegistry::instance();

    // 添加要忽略的方块（如果已注册）
    // 玻璃类方块
    if (auto* glass = blockRegistry.getBlock(ResourceLocation("minecraft", "glass"))) {
        blocksToIgnore.push_back(glass->defaultState().blockId());
    }
    if (auto* glassPane = blockRegistry.getBlock(ResourceLocation("minecraft", "glass_pane"))) {
        blocksToIgnore.push_back(glassPane->defaultState().blockId());
    }
    if (auto* whiteGlass = blockRegistry.getBlock(ResourceLocation("minecraft", "white_stained_glass"))) {
        blocksToIgnore.push_back(whiteGlass->defaultState().blockId());
    }

    // 门类方块（僵尸村庄中门被破坏）
    if (auto* oakDoor = blockRegistry.getBlock(ResourceLocation("minecraft", "oak_door"))) {
        blocksToIgnore.push_back(oakDoor->defaultState().blockId());
    }
    if (auto* spruceDoor = blockRegistry.getBlock(ResourceLocation("minecraft", "spruce_door"))) {
        blocksToIgnore.push_back(spruceDoor->defaultState().blockId());
    }
    if (auto* birchDoor = blockRegistry.getBlock(ResourceLocation("minecraft", "birch_door"))) {
        blocksToIgnore.push_back(birchDoor->defaultState().blockId());
    }
    if (auto* jungleDoor = blockRegistry.getBlock(ResourceLocation("minecraft", "jungle_door"))) {
        blocksToIgnore.push_back(jungleDoor->defaultState().blockId());
    }
    if (auto* acaciaDoor = blockRegistry.getBlock(ResourceLocation("minecraft", "acacia_door"))) {
        blocksToIgnore.push_back(acaciaDoor->defaultState().blockId());
    }
    if (auto* darkOakDoor = blockRegistry.getBlock(ResourceLocation("minecraft", "dark_oak_door"))) {
        blocksToIgnore.push_back(darkOakDoor->defaultState().blockId());
    }

    // 如果有要忽略的方块，添加处理器
    if (!blocksToIgnore.empty()) {
        list.addProcessor(std::make_unique<BlockIgnoreStructureProcessor>(blocksToIgnore));
    }

    return list;
}

// ============================================================================
// 辅助函数：创建堡垒遗迹处理器
// ============================================================================

/**
 * @brief 创建堡垒遗迹的退化处理器
 *
 * @param integrity 完整度 (0.0 - 1.0)
 * @param includeBlackstoneReplacement 是否包含黑石替换
 */
StructureProcessorList createBastionProcessor(f32 integrity, bool includeBlackstoneReplacement = true)
{
    StructureProcessorList list;

    // 1. 完整度处理器（随机移除方块）
    list.addProcessor(std::make_unique<IntegrityProcessor>(integrity));

    // 2. 黑石替换处理器
    if (includeBlackstoneReplacement) {
        list.addProcessor(std::make_unique<BlackstoneReplacementProcessor>());
    }

    return list;
}

} // namespace

// ============================================================================
// 初始化函数
// ============================================================================

void ProcessorLists::initialize()
{
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    // ========================================================================
    // 苔藓化处理器
    // ========================================================================

    MOSSIFY_10_PERCENT.addProcessor(std::make_unique<BlockAgeProcessor>(0.1f));
    MOSSIFY_20_PERCENT.addProcessor(std::make_unique<BlockAgeProcessor>(0.2f));
    MOSSIFY_70_PERCENT.addProcessor(std::make_unique<BlockAgeProcessor>(0.7f));

    // ========================================================================
    // 僵尸村庄处理器
    // ========================================================================

    // 每种僵尸村庄有不同的苔藓化概率
    ZOMBIE_PLAINS = createZombieVillageProcessor(0.5f);
    ZOMBIE_DESERT = createZombieVillageProcessor(0.5f);
    ZOMBIE_SAVANNA = createZombieVillageProcessor(0.5f);
    ZOMBIE_SNOWY = createZombieVillageProcessor(0.5f);
    ZOMBIE_TAIGA = createZombieVillageProcessor(0.5f);

    // ========================================================================
    // 道路处理器
    // ========================================================================

    STREET_PLAINS.addProcessor(std::make_unique<BlockAgeProcessor>(0.1f));
    STREET_SAVANNA.addProcessor(std::make_unique<BlockAgeProcessor>(0.1f));
    STREET_SNOWY_TAIGA.addProcessor(std::make_unique<BlockAgeProcessor>(0.1f));

    // ========================================================================
    // 农场处理器
    // ========================================================================

    FARM_PLAINS.addProcessor(std::make_unique<BlockAgeProcessor>(0.05f));
    FARM_SAVANNA.addProcessor(std::make_unique<BlockAgeProcessor>(0.05f));
    FARM_SNOWY.addProcessor(std::make_unique<BlockAgeProcessor>(0.05f));
    FARM_TAIGA.addProcessor(std::make_unique<BlockAgeProcessor>(0.05f));
    FARM_DESERT.addProcessor(std::make_unique<BlockAgeProcessor>(0.05f));

    // ========================================================================
    // 掠夺者前哨站处理器
    // ========================================================================

    // 5% 完整度，大部分方块会被移除
    OUTPOST_ROT.addProcessor(std::make_unique<IntegrityProcessor>(0.05f));

    // ========================================================================
    // 堡垒遗迹处理器
    // ========================================================================

    // 底层城墙
    BASTION_BOTTOM_RAMPART = createBastionProcessor(0.9f, true);

    // 宝藏房间
    BASTION_TREASURE_ROOMS = createBastionProcessor(0.95f, true);

    // 住宅区域
    BASTION_HOUSING = createBastionProcessor(0.85f, true);

    // 侧墙退化
    BASTION_SIDE_WALL_DEGRADATION = createBastionProcessor(0.8f, true);

    // 马厩退化
    BASTION_STABLE_DEGRADATION = createBastionProcessor(0.85f, true);

    // 通用退化
    BASTION_GENERIC_DEGRADATION = createBastionProcessor(0.9f, true);

    // 城墙退化
    BASTION_RAMPART_DEGRADATION = createBastionProcessor(0.85f, true);

    // 入口替换
    BASTION_ENTRANCE_REPLACEMENT = createBastionProcessor(0.95f, true);

    // 桥梁
    BASTION_BRIDGE = createBastionProcessor(0.9f, true);

    // 屋顶
    BASTION_ROOF = createBastionProcessor(0.95f, true);

    // 高墙
    BASTION_HIGH_WALL = createBastionProcessor(0.9f, true);

    // 高城墙
    BASTION_HIGH_RAMPART = createBastionProcessor(0.85f, true);

    // ========================================================================
    // 注册到 ProcessorListRegistry
    // ========================================================================

    auto& registry = jigsaw::ProcessorListRegistry::instance();

    registry.registerList(ResourceLocation("minecraft", "empty"), EMPTY);
    registry.registerList(ResourceLocation("minecraft", "mossify_10_percent"), MOSSIFY_10_PERCENT);
    registry.registerList(ResourceLocation("minecraft", "mossify_20_percent"), MOSSIFY_20_PERCENT);
    registry.registerList(ResourceLocation("minecraft", "mossify_70_percent"), MOSSIFY_70_PERCENT);
    registry.registerList(ResourceLocation("minecraft", "street_plains"), STREET_PLAINS);
    registry.registerList(ResourceLocation("minecraft", "street_savanna"), STREET_SAVANNA);
    registry.registerList(ResourceLocation("minecraft", "street_snowy_or_taiga"), STREET_SNOWY_TAIGA);
    registry.registerList(ResourceLocation("minecraft", "farm_plains"), FARM_PLAINS);
    registry.registerList(ResourceLocation("minecraft", "farm_savanna"), FARM_SAVANNA);
    registry.registerList(ResourceLocation("minecraft", "farm_snowy"), FARM_SNOWY);
    registry.registerList(ResourceLocation("minecraft", "farm_taiga"), FARM_TAIGA);
    registry.registerList(ResourceLocation("minecraft", "farm_desert"), FARM_DESERT);
    registry.registerList(ResourceLocation("minecraft", "zombie_plains"), ZOMBIE_PLAINS);
    registry.registerList(ResourceLocation("minecraft", "zombie_desert"), ZOMBIE_DESERT);
    registry.registerList(ResourceLocation("minecraft", "zombie_savanna"), ZOMBIE_SAVANNA);
    registry.registerList(ResourceLocation("minecraft", "zombie_snowy"), ZOMBIE_SNOWY);
    registry.registerList(ResourceLocation("minecraft", "zombie_taiga"), ZOMBIE_TAIGA);
    registry.registerList(ResourceLocation("minecraft", "outpost_rot"), OUTPOST_ROT);
    registry.registerList(ResourceLocation("minecraft", "bastion_bottom_rampart"), BASTION_BOTTOM_RAMPART);
    registry.registerList(ResourceLocation("minecraft", "bastion_treasure_rooms"), BASTION_TREASURE_ROOMS);
    registry.registerList(ResourceLocation("minecraft", "bastion_housing"), BASTION_HOUSING);
    registry.registerList(
        ResourceLocation("minecraft", "bastion_side_wall_degradation"), BASTION_SIDE_WALL_DEGRADATION);
    registry.registerList(ResourceLocation("minecraft", "bastion_stable_degradation"), BASTION_STABLE_DEGRADATION);
    registry.registerList(ResourceLocation("minecraft", "bastion_generic_degradation"), BASTION_GENERIC_DEGRADATION);
    registry.registerList(ResourceLocation("minecraft", "bastion_rampart_degradation"), BASTION_RAMPART_DEGRADATION);
    registry.registerList(ResourceLocation("minecraft", "bastion_entrance_replacement"), BASTION_ENTRANCE_REPLACEMENT);
    registry.registerList(ResourceLocation("minecraft", "bastion_bridge"), BASTION_BRIDGE);
    registry.registerList(ResourceLocation("minecraft", "bastion_roof"), BASTION_ROOF);
    registry.registerList(ResourceLocation("minecraft", "bastion_high_wall"), BASTION_HIGH_WALL);
    registry.registerList(ResourceLocation("minecraft", "bastion_high_rampart"), BASTION_HIGH_RAMPART);

    // 堡垒遗迹处理器别名：MC 数据包中的模板池使用短名称（无 bastion_ 前缀），
    // 因此需要注册与数据包一致的名称
    registry.registerList(ResourceLocation("minecraft", "bottom_rampart"), BASTION_BOTTOM_RAMPART);
    registry.registerList(ResourceLocation("minecraft", "treasure_rooms"), BASTION_TREASURE_ROOMS);
    registry.registerList(ResourceLocation("minecraft", "housing"), BASTION_HOUSING);
    registry.registerList(ResourceLocation("minecraft", "side_wall_degradation"), BASTION_SIDE_WALL_DEGRADATION);
    registry.registerList(ResourceLocation("minecraft", "stable_degradation"), BASTION_STABLE_DEGRADATION);
    registry.registerList(ResourceLocation("minecraft", "rampart_degradation"), BASTION_RAMPART_DEGRADATION);
    registry.registerList(ResourceLocation("minecraft", "entrance_replacement"), BASTION_ENTRANCE_REPLACEMENT);
    registry.registerList(ResourceLocation("minecraft", "bridge"), BASTION_BRIDGE);
    registry.registerList(ResourceLocation("minecraft", "roof"), BASTION_ROOF);
    registry.registerList(ResourceLocation("minecraft", "high_wall"), BASTION_HIGH_WALL);
    registry.registerList(ResourceLocation("minecraft", "high_rampart"), BASTION_HIGH_RAMPART);

    spdlog::info("Registered {} processor lists in ProcessorListRegistry", 42);
}

bool ProcessorLists::isInitialized()
{
    return s_initialized;
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
