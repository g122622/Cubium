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

#include "PillagerOutpostPools.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/jigsaw/EmptyJigsawPiece.hpp"
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

using jigsaw::EmptyJigsawPiece;
using jigsaw::JigsawPlacementBehaviour;
using jigsaw::SingleJigsawPiece;
using jigsaw::TemplatePool;
using jigsaw::TemplatePoolRegistry;

// 初始化标志
static bool s_registered = false;

// ============================================================================
// PillagerOutpostPools 命名空间实现
// ============================================================================

void PillagerOutpostPools::registerAll(TemplatePoolRegistry& registry)
{
    if (s_registered) {
        return;
    }

    // ========================================================================
    // pillager_outpost/base_plate - 基座
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/base_plate"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/base_plate/base_plate_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/base_plate/base_plate_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/base_plate/base_plate_2", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/base_plate/base_plate_3", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/watchtower - 观察塔 (起始池)
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/watchtower"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/watchtower/watchtower_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/watchtower/watchtower_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/watchtower/watchtower_2", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/watchtower/watchtower_3", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/feature_cage_with_allays - 带悦灵的笼子特征
    // ========================================================================
    {
        auto pool =
            std::make_unique<TemplatePool>(ResourceLocation("minecraft", "pillager_outpost/feature_cage_with_allays"),
                ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/feature_cage_with_allays", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/feature_cage - 笼子特征
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/feature_cage"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/feature_cage", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/tent - 帐篷
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/tent"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:pillager_outpost/tent_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:pillager_outpost/tent_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:pillager_outpost/tent_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/tent_feature - 帐篷特征
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/tent_feature"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/tent_feature_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/tent_feature_1", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/logs - 原木堆
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/logs"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:pillager_outpost/logs_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:pillager_outpost/logs_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(
            std::make_unique<SingleJigsawPiece>("minecraft:pillager_outpost/logs_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/targets - 靶子
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/targets"), ResourceLocation("minecraft", "empty"));

        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/targets_0", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/targets_1", JigsawPlacementBehaviour::Rigid),
            1);
        pool->addPiece(std::make_unique<SingleJigsawPiece>(
                           "minecraft:pillager_outpost/targets_2", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/feature_trees - 树木特征
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/feature_trees"), ResourceLocation("minecraft", "empty"));

        // FeatureJigsawPiece::place() 已实现（可放置配置化地物），此处仍用空元素占位；
        // 后续可替换为真实 FeatureJigsawPiece（如 minecraft:oak_tree）。
        // EmptyJigsawPiece 是单例（clone 返回 nullptr），用 make_unique 创建临时实例，
        // TemplatePool::addPiece 检测到 isEmpty() 后存入单例指针并丢弃临时对象。
        pool->addPiece(std::make_unique<EmptyJigsawPiece>(), 1);

        registry.registerPool(std::move(pool));
    }

    // ========================================================================
    // pillager_outpost/feature_cage_with_allays_pillager - 带悦灵和掠夺者的笼子
    // ========================================================================
    {
        auto pool = std::make_unique<TemplatePool>(
            ResourceLocation("minecraft", "pillager_outpost/feature_cage_with_allays_pillager"),
            ResourceLocation("minecraft", "empty"));

        pool->addPiece(
            std::make_unique<SingleJigsawPiece>(
                "minecraft:pillager_outpost/feature_cage_with_allays_pillager", JigsawPlacementBehaviour::Rigid),
            1);

        registry.registerPool(std::move(pool));
    }

    s_registered = true;
}

bool PillagerOutpostPools::isRegistered()
{
    return s_registered;
}

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
