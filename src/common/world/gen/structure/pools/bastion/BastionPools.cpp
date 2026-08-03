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

#include "BastionPools.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/jigsaw/SingleJigsawPiece.hpp"
#include "common/world/gen/jigsaw/TemplatePool.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

using jigsaw::JigsawPlacementBehaviour;
using jigsaw::SingleJigsawPiece;
using jigsaw::TemplatePool;
using jigsaw::TemplatePoolRegistry;

// 初始化标志
static bool s_registered = false;

// ============================================================================
// BastionPools 命名空间实现
// ============================================================================

void BastionPools::registerAll(TemplatePoolRegistry& registry)
{
    if (s_registered) {
        return;
    }

    // 注册各类型堡垒遗迹
    BastionUnitsPools::registerAll(registry);
    BastionStablesPools::registerAll(registry);
    BastionTreasurePools::registerAll(registry);
    BastionBridgePools::registerAll(registry);

    s_registered = true;
}

bool BastionPools::isRegistered()
{
    return s_registered;
}

// ============================================================================
// BastionUnitsPools - 单元型堡垒遗迹
// ============================================================================

void BastionUnitsPools::registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // bastion/units/start - 起始池
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/start"), ResourceLocation("minecraft", "empty"));

        // 起始结构只有 1 个
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/center_0", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/center_pieces - 中心部件
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/center_pieces"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/center_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/center_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/center_bridges - 中心桥梁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/center_bridges"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/bridge_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/bridge_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/bridge_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/stages - 层级
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/stages"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/stage_0", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/stage_1", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/stage_2", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/stage_3", JigsawPlacementBehaviour::Rigid), 1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/walls - 墙壁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/walls"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_0", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_1", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_2", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_3", JigsawPlacementBehaviour::Rigid), 1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/walls/edges - 墙壁边缘
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/walls/edges"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/edge_0", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/edge_1", JigsawPlacementBehaviour::Rigid), 1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/walls/walls - 墙壁连接
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/walls/walls"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_base_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_base_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_base_2", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/wall_base_3", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/roofs - 屋顶
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/roofs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/roof_0", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/roof_1", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/roof_2", JigsawPlacementBehaviour::Rigid), 1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/roof_3", JigsawPlacementBehaviour::Rigid), 1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/ramparts - 城墙
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/ramparts"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/rampart_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/rampart_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/rampart_2", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/rampart_3", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/units/rampart_4", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/units/rampart_plates - 城墙平台
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/units/rampart_plates"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/units/rampart_plate_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/units/rampart_plate_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }
}

// ============================================================================
// BastionStablesPools - 猪灵兽栏
// ============================================================================

void BastionStablesPools::registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // bastion/stables/start - 起始池
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/start"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/starting_pieces/stable_0", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/stables/legs - 腿部
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/legs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/legs/leg_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/legs/leg_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/legs/leg_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/stables/walls - 墙壁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/walls"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/walls/wall_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/walls/wall_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/walls/wall_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/stables/ramparts - 城墙
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/ramparts"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/ramparts/large_rampart_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/ramparts/large_rampart_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/ramparts/large_rampart_2", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/ramparts/rampart_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/ramparts/rampart_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/ramparts/rampart_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/stables/air_base - 空中基础
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/air_base"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/air_base/base_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/air_base/base_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/air_base/base_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/stables/bridges - 桥梁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/bridges"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/bridges/bridge_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/bridges/bridge_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/stables/roofs - 屋顶
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/roofs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/roofs/roof_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/roofs/roof_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/stables/connectors - 连接器
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/stables/connectors"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/connectors/connector_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/stables/connectors/connector_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }
}

// ============================================================================
// BastionTreasurePools - 宝藏型堡垒遗迹
// ============================================================================

void BastionTreasurePools::registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // bastion/treasure/start - 起始池
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/start"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/bases/centers/center_0", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/legs - 腿部
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/legs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/legs/leg_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/legs/leg_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/legs/leg_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/walls - 墙壁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/walls"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/walls/wall_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/walls/wall_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/walls/wall_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/ramparts - 城墙
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/ramparts"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/ramparts/large_rampart_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/ramparts/large_rampart_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/ramparts/rampart_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/ramparts/rampart_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/air_base - 空中基础
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/air_base"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/air_base/base_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/air_base/base_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/air_base/base_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/bridges - 桥梁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/bridges"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/bridges/bridge_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/bridges/bridge_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/roofs - 屋顶
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/roofs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/roofs/roof_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/roofs/roof_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/extensions - 扩展
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/extensions"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/extensions/extension_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/extensions/extension_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/extensions/extension_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/treasure/centers - 中心
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/treasure/centers"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/bases/centers/center_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/bases/centers/center_2", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/treasure/bases/centers/center_3", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }
}

// ============================================================================
// BastionBridgePools - 桥梁型堡垒遗迹
// ============================================================================

void BastionBridgePools::registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // bastion/bridge/start - 起始池
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/start"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/starting_piece/entrance_base", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/starting_piece/entrance_0", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/bridge/legs - 腿部
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/legs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/bridge/legs/leg_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:bastion/bridge/legs/leg_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/bridge/walls - 墙壁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/walls"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/walls/wall_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/walls/wall_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/bridge/ramparts - 城墙
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/ramparts"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/ramparts/rampart_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/ramparts/rampart_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/bridge/air_base - 空中基础
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/air_base"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/air_base/base_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/air_base/base_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/bridge/bridges - 桥梁
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/bridges"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/bridges/bridge_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/bridges/bridge_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/bridges/bridge_2", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/bridges/bridge_3", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/bridge/roofs - 屋顶
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/roofs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/roofs/roof_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/roofs/roof_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // bastion/bridge/connectors - 连接器
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "bastion/bridge/connectors"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/connectors/connector_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:bastion/bridge/connectors/connector_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
