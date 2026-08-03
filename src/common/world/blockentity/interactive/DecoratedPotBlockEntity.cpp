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

#include "world/blockentity/interactive/DecoratedPotBlockEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/items/special/PotterySherdItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "common/world/blockentity/interactive/DecoratedPotPattern.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

// ============================================================================
// PotDecorations 常量
// ============================================================================

const PotDecorations PotDecorations::EMPTY;

// ============================================================================
// PotDecorations 实现
// ============================================================================

PotDecorations::PotDecorations()
    : m_patterns{DecoratedPotPattern::Blank,
          DecoratedPotPattern::Blank,
          DecoratedPotPattern::Blank,
          DecoratedPotPattern::Blank}
{}

PotDecorations::PotDecorations(
    DecoratedPotPattern back, DecoratedPotPattern left, DecoratedPotPattern right, DecoratedPotPattern front)
    : m_patterns{back, left, right, front}
{}

PotDecorations::PotDecorations(const std::vector<DecoratedPotPattern>& patterns)
{
    for (size_t i = 0; i < 4; ++i) {
        m_patterns[i] = (i < patterns.size()) ? patterns[i] : DecoratedPotPattern::Blank;
    }
}

bool PotDecorations::isEmpty() const noexcept
{
    for (const auto& pattern : m_patterns) {
        if (pattern != DecoratedPotPattern::Blank) {
            return false;
        }
    }
    return true;
}

bool PotDecorations::operator==(const PotDecorations& other) const noexcept
{
    return m_patterns == other.m_patterns;
}

PotDecorations PotDecorations::fromJson(const nlohmann::json& sherdsArray)
{
    std::vector<DecoratedPotPattern> patterns;

    if (sherdsArray.is_array()) {
        for (const auto& sherd : sherdsArray) {
            if (!sherd.is_string()) {
                patterns.push_back(DecoratedPotPattern::Blank);
                continue;
            }

            const std::string itemId = sherd.get<std::string>();

            // 检查是否为砖块（对应 Blank 图案）
            // 砖块的物品ID为 "minecraft:brick"
            if (itemId == "minecraft:brick") {
                patterns.push_back(DecoratedPotPattern::Blank);
                continue;
            }

            // 从物品ID提取图案名称
            // 物品ID格式为 "minecraft:{name}_pottery_sherd"
            // 图案名称为去掉 "_pottery_sherd" 后缀和 "minecraft:" 前缀的部分
            const std::string prefix = "minecraft:";
            const std::string suffix = "_pottery_sherd";

            std::string name;
            if (itemId.starts_with(prefix)) {
                name = itemId.substr(prefix.length());
            } else {
                name = itemId;
            }

            if (name.ends_with(suffix)) {
                name = name.substr(0, name.length() - suffix.length());
            }

            patterns.push_back(DecoratedPotPatterns::byName(name));
        }
    }

    return PotDecorations(patterns);
}

nlohmann::json PotDecorations::toJson() const
{
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& pattern : m_patterns) {
        if (DecoratedPotPatterns::isBlank(pattern)) {
            arr.push_back("minecraft:brick");
        } else {
            arr.push_back("minecraft:" + DecoratedPotPatterns::getName(pattern) + "_pottery_sherd");
        }
    }
    return arr;
}

PotDecorations PotDecorations::fromNBT(const nbt::tags::list_tag& sherdsTag)
{
    std::vector<DecoratedPotPattern> patterns;

    if (sherdsTag.element_id() == nbt::TagId::String) {
        const auto& stringList = dynamic_cast<const nbt::tags::string_list_tag&>(sherdsTag);
        for (const auto& itemId : stringList.value) {
            // 与 fromJson 相同的逻辑
            if (itemId == "minecraft:brick") {
                patterns.push_back(DecoratedPotPattern::Blank);
                continue;
            }

            const std::string prefix = "minecraft:";
            const std::string suffix = "_pottery_sherd";

            std::string name;
            if (itemId.starts_with(prefix)) {
                name = itemId.substr(prefix.length());
            } else {
                name = itemId;
            }

            if (name.ends_with(suffix)) {
                name = name.substr(0, name.length() - suffix.length());
            }

            patterns.push_back(DecoratedPotPatterns::byName(name));
        }
    }

    return PotDecorations(patterns);
}

nbt::tags::string_list_tag PotDecorations::toNBT() const
{
    nbt::tags::string_list_tag list;
    for (const auto& pattern : m_patterns) {
        if (DecoratedPotPatterns::isBlank(pattern)) {
            list.value.push_back("minecraft:brick");
        } else {
            list.value.push_back("minecraft:" + DecoratedPotPatterns::getName(pattern) + "_pottery_sherd");
        }
    }
    return list;
}

// ============================================================================
// DecoratedPotBlockEntity 实现
// ============================================================================

DecoratedPotBlockEntity::DecoratedPotBlockEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::DecoratedPot, pos)
    , m_inventory(1)
    , m_decorations()
    , m_wobbleStartedAtTick(0)
    , m_lastWobbleStyle(WobbleStyle::Positive)
{}

void DecoratedPotBlockEntity::setDecorations(const PotDecorations& decorations)
{
    m_decorations = decorations;
    setChanged();
}

ItemStack DecoratedPotBlockEntity::getItem() const
{
    return m_inventory.getItem(0);
}

void DecoratedPotBlockEntity::setItem(const ItemStack& stack)
{
    m_inventory.setItem(0, stack);
    setChanged();
}

bool DecoratedPotBlockEntity::hasItem() const
{
    return !m_inventory.getItem(0).isEmpty();
}

void DecoratedPotBlockEntity::wobble(WobbleStyle style)
{
    m_lastWobbleStyle = style;

    // 设置摇晃动画开始时间
    if (m_world != nullptr) {
        m_wobbleStartedAtTick = static_cast<i64>(m_world->getGameTime());
        // 通过 blockEvent 同步摇晃动画到客户端
        const BlockState* state = m_world->getBlockState(m_pos);
        if (state != nullptr) {
            m_world->blockEvent(m_pos, state->getBlock(), 1, static_cast<i32>(style));
        }
    }

    setChanged();
}

bool DecoratedPotBlockEntity::isWobbling(i64 currentTick) const
{
    if (m_wobbleStartedAtTick == 0) {
        return false;
    }

    const i64 elapsed = currentTick - m_wobbleStartedAtTick;
    const i64 duration = (m_lastWobbleStyle == WobbleStyle::Positive) ? 7 : 10;
    return elapsed < duration;
}

i32 DecoratedPotBlockEntity::getComparatorSignal() const
{
    const ItemStack stack = m_inventory.getItem(0);
    if (stack.isEmpty()) {
        return 0;
    }

    // 单物品容器比较器信号计算公式
    const i32 count = stack.getCount();
    const i32 maxStack = stack.getMaxStackSize();
    if (maxStack == 0) {
        return 0;
    }
    return 1 + (count - 1) * 15 / maxStack;
}

bool DecoratedPotBlockEntity::load(const nlohmann::json& data)
{
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    m_inventory.clear();

    // 加载图案数据 (sherds)
    if (data.contains("sherds") && data["sherds"].is_array()) {
        m_decorations = PotDecorations::fromJson(data["sherds"]);
    } else {
        m_decorations = PotDecorations::EMPTY;
    }

    // 加载物品 (item)
    if (data.contains("item") && data["item"].is_object() && !data["item"].empty()) {
        auto result = ItemStack::fromJson(data["item"]);
        if (result.success()) {
            m_inventory.setItem(0, result.value());
        }
    }

    // 加载摇晃动画时间
    if (data.contains("wobble_started_at_tick") && data["wobble_started_at_tick"].is_number_integer()) {
        m_wobbleStartedAtTick = data["wobble_started_at_tick"].get<i64>();
    }

    return true;
}

void DecoratedPotBlockEntity::save(nlohmann::json& data) const
{
    ContainerBlockEntity::save(data);

    // 保存图案数据 (sherds)
    data["sherds"] = m_decorations.toJson();

    // 保存物品 (item)
    const ItemStack stack = m_inventory.getItem(0);
    if (!stack.isEmpty()) {
        data["item"] = stack.toJson();
    } else {
        data["item"] = nlohmann::json::object();
    }

    // 保存摇晃动画时间
    data["wobble_started_at_tick"] = m_wobbleStartedAtTick;
}

bool DecoratedPotBlockEntity::loadFromNBT(const nbt::tags::compound_tag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    m_inventory.clear();

    // 加载图案数据 (sherds)
    auto* sherdsList = mc::entity::serialization::nbt_helper::tryGetList(tag, "sherds");
    if (sherdsList != nullptr && sherdsList->element_id() == nbt::TagId::String) {
        m_decorations = PotDecorations::fromNBT(*sherdsList);
    } else {
        m_decorations = PotDecorations::EMPTY;
    }

    // 加载物品 (item)
    auto itemIt = tag.value.find("item");
    if (itemIt != tag.value.end() && itemIt->second->id() == nbt::TagId::Compound) {
        const auto& itemTag = dynamic_cast<const nbt::tags::compound_tag&>(*itemIt->second);
        auto result = ItemStack::fromNbt(itemTag);
        if (result.success()) {
            m_inventory.setItem(0, result.value());
        }
    }

    // 加载摇晃动画时间
    if (auto wobbleTick = mc::entity::serialization::nbt_helper::tryGetInt(tag, "wobble_started_at_tick")) {
        m_wobbleStartedAtTick = static_cast<i64>(*wobbleTick);
    }

    return true;
}

void DecoratedPotBlockEntity::saveToNBT(nbt::tags::compound_tag& tag) const
{
    BlockEntity::saveToNBT(tag);

    // 保存图案数据 (sherds)
    auto sherdsList = std::make_unique<nbt::tags::string_list_tag>(m_decorations.toNBT());
    tag.value.emplace("sherds", std::move(sherdsList));

    // 保存物品 (item)
    const ItemStack stack = m_inventory.getItem(0);
    if (!stack.isEmpty()) {
        auto itemTag = std::make_unique<nbt::tags::compound_tag>();
        stack.toNbt(*itemTag);
        tag.value.emplace("item", std::move(itemTag));
    }

    // 保存摇晃动画时间
    if (m_wobbleStartedAtTick != 0) {
        tag.put("wobble_started_at_tick", static_cast<i32>(m_wobbleStartedAtTick));
    }
}

std::unique_ptr<BlockEntity> DecoratedPotBlockEntity::clone() const
{
    auto cloned = std::make_unique<DecoratedPotBlockEntity>(m_pos);
    cloned->m_decorations = m_decorations;
    cloned->m_inventory.setItem(0, m_inventory.getItem(0));
    cloned->m_wobbleStartedAtTick = m_wobbleStartedAtTick;
    cloned->m_lastWobbleStyle = m_lastWobbleStyle;
    return cloned;
}

bool DecoratedPotBlockEntity::triggerEvent(i32 id, i32 type)
{
    if (id == 1) {
        // 摇晃动画事件
        // type=0: Positive (放入物品时), type=1: Negative (空手交互时)
        m_lastWobbleStyle = (type == 0) ? WobbleStyle::Positive : WobbleStyle::Negative;

        // 客户端收到事件时，使用世界时间设置动画起始时刻
        if (m_world != nullptr) {
            m_wobbleStartedAtTick = static_cast<i64>(m_world->getGameTime());
        }
        setChanged();
        return true;
    }
    return false;
}

Direction DecoratedPotBlockEntity::getDirection() const
{
    const BlockState* state = getBlockState();
    if (state != nullptr && state->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        return state->get(BlockStateProperties::HORIZONTAL_FACING());
    }
    return Direction::North;
}

// ============================================================================
// 自由函数实现
// ============================================================================

DecoratedPotPattern getPatternFromItem(const Item* item)
{
    if (item == nullptr) {
        return DecoratedPotPattern::Blank;
    }

    // 检查是否是陶片物品
    const auto* sherdItem = dynamic_cast<const item::PotterySherdItem*>(item);
    if (sherdItem != nullptr) {
        return sherdItem->getPattern();
    }

    // 检查是否是砖块
    // 砖块物品ID为 "minecraft:bricks"，通过 ItemRegistry 查找
    const Item* brickItem = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bricks"));
    if (item == brickItem) {
        return DecoratedPotPattern::Blank;
    }

    // 其他物品默认返回 Blank
    return DecoratedPotPattern::Blank;
}

const Item* getItemFromPattern(DecoratedPotPattern pattern)
{
    if (DecoratedPotPatterns::isBlank(pattern)) {
        // Blank 图案对应砖块物品
        return ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bricks"));
    }

    // 非 Blank 图案，构造陶片物品ID并查找
    // 物品ID格式: "minecraft:{name}_pottery_sherd"
    const std::string name = DecoratedPotPatterns::getName(pattern);
    const ResourceLocation itemId("minecraft", name + "_pottery_sherd");
    return ItemRegistry::instance().getItem(itemId);
}

ItemStack createDecoratedPotItem(const PotDecorations& decorations)
{
    // 获取饰纹陶罐对应的方块物品
    // 通过 BlockItemRegistry 从 DecoratedPot 方块获取对应的 BlockItem
    if (block_registry::TrailsBlocks::DECORATED_POT == nullptr) {
        return ItemStack();
    }

    const BlockItem* blockItem =
        BlockItemRegistry::instance().getBlockItem(*block_registry::TrailsBlocks::DECORATED_POT);
    if (blockItem == nullptr) {
        return ItemStack();
    }

    // 创建物品堆
    ItemStack stack(*blockItem, 1);

    // 将图案数据存储到物品的 BlockEntityTag 中
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["BlockEntityTag"] = nlohmann::json::object();
    tag["BlockEntityTag"]["id"] = "minecraft:decorated_pot";
    tag["BlockEntityTag"]["sherds"] = decorations.toJson();

    return stack;
}

} // namespace blockentity
} // namespace mc
