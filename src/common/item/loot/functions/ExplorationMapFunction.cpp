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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ExplorationMapFunction.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/map/FilledMapItem.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/map/MapData.hpp"

namespace mc {
namespace loot {

ExplorationMapFunction::ExplorationMapFunction(Destination destination, i32 zoom, bool skipKnownStructures)
    : m_destination(destination)
    , m_zoom(zoom)
    , m_skipKnownStructures(skipKnownStructures)
{}

ItemStack ExplorationMapFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 创建探险地图
    // 参考: net.minecraft.loot.functions.ExplorationMap
    IWorld& world = context.getWorld();

    // 创建已填充地图，缩放级别由配置决定
    ItemStack mapStack = item::items::FilledMapItem::setupNewMap(world, 0, 0, m_zoom, true, false);

    // 根据目的地类型添加装饰标记
    // 注意：实际的结构搜索需要ServerWorld和结构生成系统的支持
    // 当前先添加装饰类型标记，坐标搜索逻辑在结构系统实现后补充
    auto decorationType = destinationToDecorationType(m_destination);

    // 设置探险地图的搜索配置到NBT
    auto& tag = mapStack.getOrCreateTag();
    tag["map_destination"] = static_cast<i32>(m_destination);

    // 如果有已知结构坐标，添加目标装饰
    // TODO: 当结构生成系统实现后，搜索最近的结构并调用 addTargetDecoration
    // 目前标记地图类型，以便客户端识别为探险地图
    (void)decorationType;

    return mapStack;
}

std::unique_ptr<LootFunction> ExplorationMapFunction::clone() const
{
    auto func = std::make_unique<ExplorationMapFunction>(m_destination, m_zoom, m_skipKnownStructures);
    for (const auto& cond : m_conditions) {
        func->addCondition(cond->clone());
    }
    return func;
}

world::map::DecorationType ExplorationMapFunction::destinationToDecorationType(Destination destination)
{
    switch (destination) {
        case Destination::Mansion:
            return world::map::DecorationType::MANSION;
        case Destination::Monument:
            return world::map::DecorationType::MONUMENT;
        case Destination::BuriedTreasure:
        case Destination::Shipwreck:
        case Destination::RuinedPortal:
        default:
            return world::map::DecorationType::RED_X;
    }
}

} // namespace loot
} // namespace mc
