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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDecoration.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace mc {
namespace loot {

// ============================================================================
// 目的地字符串映射表
// ============================================================================

namespace {

const char* DESTINATION_STRINGS[] = {
    "minecraft:buried_treasure", // BuriedTreasure
    "minecraft:mansion",         // Mansion
    "minecraft:monument",        // Monument
    "minecraft:shipwreck",       // Shipwreck
    "minecraft:ruined_portal",   // RuinedPortal
};

} // anonymous namespace

ExplorationMapFunction::ExplorationMapFunction(Destination destination,
    std::optional<world::map::DecorationType> decoration,
    i32 zoom,
    i32 searchRadius,
    bool skipKnownStructures)
    : m_destination(destination)
    , m_decoration(decoration)
    , m_zoom(zoom)
    , m_searchRadius(searchRadius)
    , m_skipKnownStructures(skipKnownStructures)
{}

ItemStack ExplorationMapFunction::apply(ItemStack stack, LootContext& context) const
{
    if (stack.isEmpty()) {
        return stack;
    }

    // 如果物品不是空地图，不处理
    if (stack.getItem() != Items::MAP) {
        return stack;
    }

    // 获取上下文中的位置参数（宝箱位置）
    auto* blockPos = context.get(LootParams::BLOCK_POS);
    if (!blockPos) {
        return stack;
    }

    // 搜索最近的结构
    IWorld& world = context.getWorld();
    ResourceLocation structureId = destinationToResourceLocation(m_destination);
    auto foundPos = world.findNearestStructure(*blockPos, structureId, m_searchRadius, m_skipKnownStructures);

    if (!foundPos.has_value()) {
        // 未找到结构，返回原始物品
        return stack;
    }

    // 创建已填充地图
    ItemStack mapStack = item::items::FilledMapItem::setupNewMap(world, foundPos->x, foundPos->z, m_zoom, true, false);

    // 添加目标装饰标记
    auto decorationType = getEffectiveDecoration();
    item::items::FilledMapItem::addTargetDecoration(mapStack, foundPos.value(), "+", decorationType);

    return mapStack;
}

std::unique_ptr<LootFunction> ExplorationMapFunction::clone() const noexcept
{
    auto func = std::make_unique<ExplorationMapFunction>(
        m_destination, m_decoration, m_zoom, m_searchRadius, m_skipKnownStructures);
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

std::optional<ExplorationMapFunction::Destination> ExplorationMapFunction::destinationFromString(const std::string& str)
{
    // 支持带命名空间和不带命名空间两种格式
    std::string name = str;
    constexpr std::string_view PREFIX = "minecraft:";
    if (name.size() > PREFIX.size() && name.substr(0, PREFIX.size()) == PREFIX) {
        name = name.substr(PREFIX.size());
    }

    if (name == "buried_treasure") {
        return Destination::BuriedTreasure;
    }
    if (name == "mansion") {
        return Destination::Mansion;
    }
    if (name == "monument") {
        return Destination::Monument;
    }
    if (name == "shipwreck") {
        return Destination::Shipwreck;
    }
    if (name == "ruined_portal") {
        return Destination::RuinedPortal;
    }
    return std::nullopt;
}

const char* ExplorationMapFunction::destinationToString(Destination dest)
{
    auto index = static_cast<size_t>(dest);
    MC_ASSERT_RELEASE(index < sizeof(DESTINATION_STRINGS) / sizeof(DESTINATION_STRINGS[0]));
    return DESTINATION_STRINGS[index];
}

world::map::DecorationType ExplorationMapFunction::getEffectiveDecoration() const
{
    if (m_decoration.has_value()) {
        return m_decoration.value();
    }
    return destinationToDecorationType(m_destination);
}

ResourceLocation ExplorationMapFunction::destinationToResourceLocation(Destination destination)
{
    switch (destination) {
        case Destination::BuriedTreasure:
            return ResourceLocation("minecraft", "buried_treasure");
        case Destination::Mansion:
            return ResourceLocation("minecraft", "mansion");
        case Destination::Monument:
            return ResourceLocation("minecraft", "monument");
        case Destination::Shipwreck:
            return ResourceLocation("minecraft", "shipwreck");
        case Destination::RuinedPortal:
            return ResourceLocation("minecraft", "ruined_portal");
        default:
            return ResourceLocation("minecraft", "buried_treasure");
    }
}

} // namespace loot
} // namespace mc
