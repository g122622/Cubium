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

#include "world/blockentity/processing/BrewingStandEntity.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/potion/PotionBrewing.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

BrewingStandEntity::BrewingStandEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::BrewingStand, pos)
    , m_inventory(TOTAL_SLOTS)
{}

BrewingStandEntity::~BrewingStandEntity() = default;

void BrewingStandEntity::setFuelLevel(i32 fuel)
{
    m_fuel = std::max(0, std::min(fuel, FUEL_PER_BREW * 64));
    setChanged();
}

bool BrewingStandEntity::hasBottle(i32 slot) const
{
    if (slot < 0 || slot >= BOTTLE_SLOTS) {
        return false;
    }

    const ItemStack& stack = m_inventory.getItem(slot);
    if (stack.isEmpty()) {
        return false;
    }

    return potion::PotionBrewing::isPotionItem(stack);
}

i32 BrewingStandEntity::getComparatorSignal() const
{
    // 信号强度 = floor(平均填充率 * 14) + (有非空槽位 ? 1 : 0)
    // 槽位填充率 = 物品数量 / min(容器堆叠上限, 物品最大堆叠数)

    i32 nonEmptySlotCount = 0;
    f32 totalFillRatio = 0.0f;

    for (i32 slot = 0; slot < TOTAL_SLOTS; ++slot) {
        const ItemStack& stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            // 计算该槽位的填充率
            i32 stackLimit = std::min(getMaxStackSize(), stack.getMaxStackSize());
            totalFillRatio += static_cast<f32>(stack.getCount()) / static_cast<f32>(stackLimit);
            ++nonEmptySlotCount;
        }
    }

    // 计算平均填充率
    f32 averageFillRatio = totalFillRatio / static_cast<f32>(TOTAL_SLOTS);

    // 信号强度 = floor(平均填充率 * 14) + (有非空槽位 ? 1 : 0)
    i32 signal = static_cast<i32>(std::floor(averageFillRatio * 14.0f));
    if (nonEmptySlotCount > 0) {
        signal += 1;
    }

    return std::min(signal, 15);
}

void BrewingStandEntity::tick(IWorld& world)
{
    bool brewing = isBrewing();
    if (brewing != m_lastBrewing) {
        _updateBlockState(world);
        m_lastBrewing = brewing;
    }

    // 获取当前材料
    const ItemStack& ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);

    // 检测材料变化，重置酿造时间
    if (!ingredientStack.isSameItem(m_ingredientCache)) {
        m_ingredientCache = ingredientStack.copy();
        m_brewTime = 0;
        setChanged();
    }

    if (!_canBrew()) {
        m_brewTime = 0;
        return;
    }

    if (m_fuel <= 0) {
        ItemStack fuelStack = m_inventory.getItem(FUEL_SLOT);
        if (!fuelStack.isEmpty() && fuelStack.getItem() == Items::BLAZE_POWDER) {
            fuelStack.shrink(1);
            m_fuel += FUEL_PER_BREW;
            m_inventory.setItem(FUEL_SLOT, fuelStack);
            setChanged();
        }
    }

    if (m_fuel <= 0) {
        m_brewTime = 0;
        return;
    }

    if (m_brewTime > 0) {
        --m_brewTime;
        if (m_brewTime == 0) {
            _doBrew(world);
            _consumeFuel();
            _updateBlockState(world);
            setChanged();
        }
    } else {
        m_brewTime = 400;
        setChanged();
    }
}

bool BrewingStandEntity::_canBrew() const
{
    const ItemStack& ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);
    if (ingredientStack.isEmpty()) {
        return false;
    }

    if (!potion::PotionBrewing::isReagent(ingredientStack)) {
        return false;
    }

    for (i32 i = 0; i < BOTTLE_SLOTS; ++i) {
        const ItemStack& bottleStack = m_inventory.getItem(i);
        if (bottleStack.isEmpty()) {
            continue;
        }

        if (!potion::PotionBrewing::isPotionItem(bottleStack)) {
            continue;
        }

        if (potion::PotionBrewing::canBrew(bottleStack, ingredientStack)) {
            return true;
        }
    }

    return false;
}

void BrewingStandEntity::_doBrew(IWorld& world)
{
    ItemStack ingredientStack = m_inventory.getItem(INGREDIENT_SLOT);
    if (ingredientStack.isEmpty()) {
        return;
    }

    bool anyBrewed = false;

    for (i32 i = 0; i < BOTTLE_SLOTS; ++i) {
        ItemStack bottleStack = m_inventory.getItem(i);
        if (bottleStack.isEmpty()) {
            continue;
        }

        if (!potion::PotionBrewing::isPotionItem(bottleStack)) {
            continue;
        }

        if (potion::PotionBrewing::canBrew(bottleStack, ingredientStack)) {
            ItemStack result = potion::PotionBrewing::brew(bottleStack, ingredientStack);
            m_inventory.setItem(i, result);
            anyBrewed = true;
        }
    }

    // 酿造完成时播放音效
    if (!world.isClientSide() && anyBrewed) {
        world.playSound(
            SoundEvents::BLOCK_BREWING_STAND_BREW, sound::SoundCategory::Blocks, m_pos.center(), 1.0f, 1.0f);
    }

    ingredientStack.shrink(1);
    m_inventory.setItem(INGREDIENT_SLOT, ingredientStack);
}

void BrewingStandEntity::_consumeFuel()
{
    if (m_fuel > 0) {
        --m_fuel;
        setChanged();
    }
}

void BrewingStandEntity::_updateBlockState(IWorld& world)
{
    const BlockState* state = world.getBlockState(getPos());
    if (state == nullptr) {
        return;
    }

    if (!state->hasProperty(BlockStateProperties::HAS_BOTTLE_0()) ||
        !state->hasProperty(BlockStateProperties::HAS_BOTTLE_1()) ||
        !state->hasProperty(BlockStateProperties::HAS_BOTTLE_2())) {
        return;
    }

    const BlockState& updated = state->with(BlockStateProperties::HAS_BOTTLE_0(), hasBottle(0))
                                    .with(BlockStateProperties::HAS_BOTTLE_1(), hasBottle(1))
                                    .with(BlockStateProperties::HAS_BOTTLE_2(), hasBottle(2));

    world.setBlockState(getPos(), &updated, 3);
}

bool BrewingStandEntity::load(const nlohmann::json& data)
{
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    if (data.contains("brew_time")) {
        m_brewTime = data["brew_time"].get<i32>();
    }

    if (data.contains("fuel")) {
        m_fuel = data["fuel"].get<i32>();
    }

    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    return true;
}

void BrewingStandEntity::save(nlohmann::json& data) const
{
    ContainerBlockEntity::save(data);

    data["brew_time"] = m_brewTime;
    data["fuel"] = m_fuel;

    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
}

std::unique_ptr<BlockEntity> BrewingStandEntity::clone() const
{
    auto clone = std::make_unique<BrewingStandEntity>(m_pos);
    clone->m_brewTime = m_brewTime;
    clone->m_fuel = m_fuel;
    clone->m_lastBrewing = m_lastBrewing;
    clone->m_customName = m_customName;
    for (i32 slot = 0; slot < TOTAL_SLOTS; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            clone->m_inventory.setItem(slot, stack.copy());
        }
    }
    return clone;
}

// ========== ISidedInventory 接口实现 ==========

std::vector<i32> BrewingStandEntity::getSlotsForFace(Direction side) const
{
    switch (side) {
        case Direction::Up:
            // 上方：材料槽
            return {INGREDIENT_SLOT};
        case Direction::Down:
            // 下方：药水瓶槽 + 材料槽
            return {0, 1, 2, INGREDIENT_SLOT};
        default:
            // 侧面：药水瓶槽 + 燃料槽
            return {0, 1, 2, FUEL_SLOT};
    }
}

bool BrewingStandEntity::isSlotAccessibleForDirection(i32 slot, Direction direction) const
{
    const std::vector<i32> accessibleSlots = getSlotsForFace(direction);
    for (i32 accessibleSlot : accessibleSlots) {
        if (accessibleSlot == slot) {
            return true;
        }
    }
    return false;
}

bool BrewingStandEntity::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(direction);

    // 检查方向是否允许访问该槽位
    if (!isSlotAccessibleForDirection(slot, direction)) {
        return false;
    }

    return canPlaceItem(slot, stack);
}

bool BrewingStandEntity::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(direction);

    // 检查方向是否允许访问该槽位
    if (!isSlotAccessibleForDirection(slot, direction)) {
        return false;
    }

    // 材料槽（槽位 3）只能提取玻璃瓶
    if (slot == INGREDIENT_SLOT) {
        return stack.getItem() == Items::GLASS_BOTTLE;
    }

    return true;
}

bool BrewingStandEntity::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return false;
    }

    switch (slot) {
        case 0:
        case 1:
        case 2:
            // 药水瓶槽：接受药水或水瓶
            return potion::PotionBrewing::isPotionItem(stack) || stack.getItem() == Items::GLASS_BOTTLE;
        case INGREDIENT_SLOT:
            // 材料槽：接受酿造材料
            return potion::PotionBrewing::isReagent(stack);
        case FUEL_SLOT:
            // 燃料槽：只接受烈焰粉
            return stack.getItem() == Items::BLAZE_POWDER;
        default:
            return false;
    }
}

} // namespace blockentity
} // namespace mc