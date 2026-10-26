/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ExplorationMapFunction.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/map/FilledMapItem.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/item/loot/functions/LootFunction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/map/MapDecoration.hpp"
#include <cmath>
#include <memory>
#include <string>
#include <string_view>

namespace mc {
namespace loot {

const ResourceLocation& ExplorationMapFunction::defaultDestination()
{
    static const ResourceLocation DEFAULT_DESTINATION("minecraft", "on_treasure_maps");
    return DEFAULT_DESTINATION;
}

ExplorationMapFunction::ExplorationMapFunction(const ResourceLocation& destination,
    world::map::DecorationType decoration,
    i8 zoom,
    i32 searchRadius,
    bool searchSkipsLoadedChunks)
    : m_destination(destination)
    , m_decoration(decoration)
    , m_zoom(zoom)
    , m_searchRadius(searchRadius)
    , m_searchSkipsLoadedChunks(searchSkipsLoadedChunks)
{
    MC_ASSERT_RELEASE_MSG(searchRadius >= 0, "exploration_map: search_radius must be non-negative");
}

ItemStack ExplorationMapFunction::run(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 原版只处理空地图（Items.MAP）；已是已填充地图或其它物品时原样返回
    if (stack.getItem() != Items::MAP) {
        return stack;
    }

    // 搜索中心取上下文 ORIGIN（世界坐标，对方块掉落与实体掉落/钓鱼等非方块来源同样生效），
    // 缺失时返回原物品（原版 LootContextParams.ORIGIN 缺失同样不处理）
    const auto* origin = context.get(LootParams::ORIGIN);
    if (origin == nullptr) {
        return stack;
    }
    const BlockPos searchOrigin(static_cast<i32>(std::floor(origin->x)),
        static_cast<i32>(std::floor(origin->y)),
        static_cast<i32>(std::floor(origin->z)));

    // 按结构标签搜索最近的目标结构；未找到时返回原物品
    IWorld& world = context.getWorld();
    const auto foundPos =
        world.findNearestMapStructure(searchOrigin, m_destination, m_searchRadius, m_searchSkipsLoadedChunks);
    if (!foundPos.has_value()) {
        return stack;
    }

    // 建图：trackingPosition 与 unlimitedTracking 均为 true
    ItemStack mapStack = item::items::FilledMapItem::setupNewMap(world, foundPos->x, foundPos->z, m_zoom, true, true);

    // TODO: 原版建图后调用 MapItem.renderBiomePreviewMap 绘制生物群系预览底图，
    //  本项目地图数据尚无生物群系颜色预览实现，探险地图目前缺少预览底图，
    //  待地图数据支持按生物群系填充预览像素后在此补上（在地图中心定下后、添加装饰前调用）。

    // 添加目标装饰标记
    item::items::FilledMapItem::addTargetDecoration(mapStack, foundPos.value(), "+", m_decoration);

    return mapStack;
}

std::unique_ptr<LootFunction> ExplorationMapFunction::clone() const noexcept
{
    auto func = std::make_unique<ExplorationMapFunction>(
        m_destination, m_decoration, m_zoom, m_searchRadius, m_searchSkipsLoadedChunks);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

std::optional<ResourceLocation> ExplorationMapFunction::parseDestination(std::string_view str)
{
    // 与其它战利品函数中的注册表引用一致：允许省略默认命名空间 minecraft
    if (str.empty()) {
        return std::nullopt;
    }
    ResourceLocation parsed = ResourceLocation::parse(str);
    if (parsed.path().empty()) {
        return std::nullopt;
    }
    return parsed;
}

} // namespace loot
} // namespace mc
