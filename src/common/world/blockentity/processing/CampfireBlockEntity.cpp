/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do do so, subject to the following conditions:
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

#include "CampfireBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "entity/utils/ItemDropHelper.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/crafting/SmeltingRecipe.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockState.hpp"
#include "world/block/blocks/decorative/CampfireBlock.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

CampfireBlockEntity::CampfireBlockEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::Campfire, pos)
    , m_inventory(SLOT_COUNT, [this]() { ContainerBlockEntity::setChanged(); })
    , m_cookTimes{}
    , m_cookTimesTotal{}
{}

void CampfireBlockEntity::tick(IWorld& world)
{
    // 获取方块状态检查是否点燃
    const BlockState* statePtr = BlockEntity::getBlockState();
    if (statePtr == nullptr) {
        return;
    }

    bool isLit = blocks::CampfireBlock::isLit(*statePtr);

    if (world.isClientSide()) {
        // 客户端：粒子效果由渲染器处理
        return;
    }

    // 服务端：烹饪逻辑
    if (isLit) {
        _cookAndDrop(world);
    } else {
        _coolDown();
    }
}

void CampfireBlockEntity::_cookAndDrop(IWorld& world)
{
    // 遍历所有槽位，烹饪并掉落完成的物品

    bool anyChanged = false;

    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        ItemStack stack = m_inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        // 增加烹饪时间
        ++m_cookTimes[i];

        // 检查是否烹饪完成
        if (m_cookTimes[i] >= m_cookTimesTotal[i]) {
            // 查找匹配的营火烹饪配方
            auto& recipeManager = crafting::RecipeManager::instance();
            const crafting::SmeltingRecipe* recipe =
                recipeManager.getSmeltingRecipe(stack, crafting::RecipeType::CampfireCooking);

            if (recipe != nullptr) {
                // 获取结果物品
                ItemStack result = recipe->getResultItem().copy();

                // 在营火位置掉落结果物品
                f64 x = static_cast<f64>(m_pos.x) + 0.5;
                f64 y = static_cast<f64>(m_pos.y) + 0.5;
                f64 z = static_cast<f64>(m_pos.z) + 0.5;

                ItemDropHelper::spawnItemEntity(&world, result, x, y, z, m_rng);

                // 清空槽位
                m_inventory.setItem(i, ItemStack());
                m_cookTimes[i] = 0;
                m_cookTimesTotal[i] = 0;
                anyChanged = true;
            } else {
                // 没有匹配配方，清空槽位（不应该发生，但作为安全措施）
                m_inventory.setItem(i, ItemStack());
                m_cookTimes[i] = 0;
                m_cookTimesTotal[i] = 0;
                anyChanged = true;
            }
        }
    }

    if (anyChanged) {
        ContainerBlockEntity::setChanged();

        // 烹饪完成掉落物品后通知客户端方块实体数据更新
        // 参考 MC: CampfireBlockEntity.cookTick() 中在物品烹饪完成后调用
        // ServerLevel.sendBlockUpdated(pos, state, state, 3)
        world.notifyBlockUpdate(m_pos);
    }
}

void CampfireBlockEntity::_coolDown()
{
    // 熄灭时烹饪时间每tick减少2，直到减到0为止

    bool anyChanged = false;

    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        if (m_cookTimes[i] > 0) {
            m_cookTimes[i] = std::max(0, m_cookTimes[i] - 2);
            anyChanged = true;
        }
    }

    if (anyChanged) {
        ContainerBlockEntity::setChanged();
    }
}

std::optional<std::pair<const crafting::CampfireCookingRecipe*, i32>> CampfireBlockEntity::findMatchingRecipe(
    const ItemStack& stack) const
{
    // 仅在有空槽位时查找配方

    if (stack.isEmpty()) {
        return std::nullopt;
    }

    // 检查是否有空槽位
    bool hasEmptySlot = false;
    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        if (m_inventory.getItem(i).isEmpty()) {
            hasEmptySlot = true;
            break;
        }
    }

    if (!hasEmptySlot) {
        return std::nullopt;
    }

    // 查找营火烹饪配方
    auto& recipeManager = crafting::RecipeManager::instance();
    const crafting::SmeltingRecipe* smeltingRecipe =
        recipeManager.getSmeltingRecipe(stack, crafting::RecipeType::CampfireCooking);

    if (smeltingRecipe == nullptr) {
        return std::nullopt;
    }

    // 转换为 CampfireCookingRecipe 指针（安全转换，因为类型已确认）
    const crafting::CampfireCookingRecipe* campfireRecipe =
        static_cast<const crafting::CampfireCookingRecipe*>(smeltingRecipe);

    i32 cookTime = smeltingRecipe->getCookTime();
    return std::make_pair(campfireRecipe, cookTime);
}

bool CampfireBlockEntity::addItem(ItemStack& stack, i32 cookTime)
{
    if (stack.isEmpty()) {
        return false;
    }

    // 找到第一个空槽位
    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        if (m_inventory.getItem(i).isEmpty()) {
            // 分割出1个物品放入槽位
            ItemStack singleItem = stack.split(1);
            m_inventory.setItem(i, singleItem);

            // 设置烹饪时间
            m_cookTimes[i] = 0;
            m_cookTimesTotal[i] = cookTime > 0 ? cookTime : DEFAULT_COOK_TIME;

            _inventoryChanged();
            return true;
        }
    }

    return false;
}

void CampfireBlockEntity::dropAllItems(IWorld& world)
{
    // 掉落所有槽位中的物品

    std::vector<ItemStack> drops;

    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        ItemStack stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            drops.push_back(stack);
        }
    }

    if (!drops.empty()) {
        ItemDropHelper::spawnItemEntities(&world, m_pos, drops, m_rng);
    }

    // 清空槽位
    m_inventory.clear();
    m_cookTimes.fill(0);
    m_cookTimesTotal.fill(0);

    ContainerBlockEntity::setChanged();
}

void CampfireBlockEntity::clear()
{
    m_inventory.clear();
    m_cookTimes.fill(0);
    m_cookTimesTotal.fill(0);
    ContainerBlockEntity::setChanged();
}

f32 CampfireBlockEntity::getCookProgress(i32 slot) const noexcept
{
    if (!_isValidSlot(slot)) {
        return 0.0f;
    }

    ItemStack stack = m_inventory.getItem(slot);
    if (stack.isEmpty() || m_cookTimesTotal[slot] <= 0) {
        return 0.0f;
    }

    return static_cast<f32>(m_cookTimes[slot]) / static_cast<f32>(m_cookTimesTotal[slot]);
}

void CampfireBlockEntity::_inventoryChanged()
{
    ContainerBlockEntity::setChanged();

    // 通知客户端方块实体数据更新
    // 参考 MC: CampfireBlockEntity.markUpdated() 中调用 level.sendBlockUpdated(pos, state, state, 3)
    // 与 setBlockState 不同，notifyBlockUpdate 即使方块状态未改变也会触发客户端同步，
    // 这对于方块实体内部数据（如营火烹饪物品）变化后的客户端刷新至关重要
    if (m_world != nullptr) {
        m_world->notifyBlockUpdate(m_pos);
    }
}

bool CampfireBlockEntity::load(const nlohmann::json& data)
{
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载烹饪时间
    if (data.contains("CookingTimes") && data["CookingTimes"].is_array()) {
        const auto& times = data["CookingTimes"];
        for (size_t i = 0; i < std::min(times.size(), static_cast<size_t>(SLOT_COUNT)); ++i) {
            if (times[i].is_number()) {
                m_cookTimes[i] = times[i].get<i32>();
            }
        }
    }

    // 加载总烹饪时间
    if (data.contains("CookingTotalTimes") && data["CookingTotalTimes"].is_array()) {
        const auto& totalTimes = data["CookingTotalTimes"];
        for (size_t i = 0; i < std::min(totalTimes.size(), static_cast<size_t>(SLOT_COUNT)); ++i) {
            if (totalTimes[i].is_number()) {
                m_cookTimesTotal[i] = totalTimes[i].get<i32>();
            }
        }
    }

    return true;
}

void CampfireBlockEntity::save(nlohmann::json& data) const
{
    ContainerBlockEntity::save(data);

    // 保存烹饪时间
    nlohmann::json cookingTimes = nlohmann::json::array();
    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        cookingTimes.push_back(m_cookTimes[i]);
    }
    data["CookingTimes"] = cookingTimes;

    // 保存总烹饪时间
    nlohmann::json cookingTotalTimes = nlohmann::json::array();
    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        cookingTotalTimes.push_back(m_cookTimesTotal[i]);
    }
    data["CookingTotalTimes"] = cookingTotalTimes;
}

std::unique_ptr<BlockEntity> CampfireBlockEntity::clone() const
{
    auto cloned = std::make_unique<CampfireBlockEntity>(m_pos);

    // 复制库存
    for (i32 i = 0; i < SLOT_COUNT; ++i) {
        cloned->m_inventory.setItem(i, m_inventory.getItem(i));
    }

    // 复制烹饪时间
    cloned->m_cookTimes = m_cookTimes;
    cloned->m_cookTimesTotal = m_cookTimesTotal;

    return cloned;
}

} // namespace blockentity
} // namespace mc
