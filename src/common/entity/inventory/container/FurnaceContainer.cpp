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

#include "entity/inventory/container/FurnaceContainer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/world/block/BlockPos.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace mc {
namespace blockentity {

// ========== 构造函数 ==========

FurnaceContainer::FurnaceContainer(ContainerId id,
    PlayerInventory* playerInventory,
    IInventory* furnaceInventory,
    AbstractFurnaceEntity* furnaceEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_furnaceInventory(furnaceInventory)
    , m_furnaceInventoryOwner()
    , m_furnaceEntity(furnaceEntity)
{

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(furnaceInventory != nullptr);
    MC_ASSERT(furnaceInventory->getContainerSize() == FURNACE_SLOTS);

    // 初始化槽位布局
    _initSlots(playerInventory);
    _initTrackedInts();
}

FurnaceContainer::FurnaceContainer(ContainerId id,
    PlayerInventory* playerInventory,
    std::shared_ptr<IInventory> furnaceInventoryOwner,
    AbstractFurnaceEntity* furnaceEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_furnaceInventory(furnaceInventoryOwner.get())
    , m_furnaceInventoryOwner(std::move(furnaceInventoryOwner))
    , m_furnaceEntity(furnaceEntity)
{

    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(m_furnaceInventory != nullptr);
    MC_ASSERT(m_furnaceInventory->getContainerSize() == FURNACE_SLOTS);

    _initSlots(playerInventory);
    _initTrackedInts();
}

// ========== 经验相关 ==========

f32 FurnaceContainer::getStoredExperience() const
{
    if (m_furnaceEntity) {
        return m_furnaceEntity->getStoredExperience();
    }
    return 0.0f;
}

f32 FurnaceContainer::extractStoredExperience()
{
    if (m_furnaceEntity) {
        return m_furnaceEntity->extractStoredExperience();
    }
    return 0.0f;
}

void FurnaceContainer::_grantExperienceForOutput(i32 extractedCount)
{
    // 只有在有玩家且有累积经验时才发放
    if (!m_player || !m_furnaceEntity) {
        return;
    }

    f32 storedXp = m_furnaceEntity->getStoredExperience();
    if (storedXp <= 0.0f) {
        return;
    }

    // 每次取出发放全部累积经验
    f32 xpToGrant = m_furnaceEntity->extractStoredExperience();
    if (xpToGrant > 0.0f) {
        m_player->addExperience(static_cast<i32>(std::floor(xpToGrant)));
    }

    MC_UNUSED(extractedCount);
}

// ========== 快速移动 ==========

bool FurnaceContainer::stillValid(const Player& player) const
{
    // 如果没有关联的方块实体，背包可访问
    if (m_furnaceEntity == nullptr) {
        return true;
    }

    // 检查玩家是否在熔炉附近（8格范围内）
    const BlockPos pos = m_furnaceEntity->getPos();
    return player.distanceSqTo(static_cast<f32>(pos.x) + 0.5f,
               static_cast<f32>(pos.y) + 0.5f,
               static_cast<f32>(pos.z) + 0.5f) <= 64.0f; // 8^2 = 64
}

ItemStack FurnaceContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack originalStack = slotStack.copy();

    if (slotIndex < FURNACE_SLOTS) {
        if (slotIndex == SLOT_OUTPUT) {
            // Shift+点击输出槽：移动到玩家背包
            // 经验发放由 FurnaceResultSlot::onTake 处理
            if (!moveItemToRange(slotStack, FURNACE_SLOTS, getSlotCount() - 1, true)) {
                return ItemStack();
            }
        } else {
            if (!moveItemToRange(slotStack, FURNACE_SLOTS, getSlotCount() - 1, false)) {
                return ItemStack();
            }
        }
    } else {
        if (!moveItemToRange(slotStack, SLOT_INPUT, SLOT_INPUT, false)) {
            if (!moveItemToRange(slotStack, SLOT_FUEL, SLOT_FUEL, false)) {
                return ItemStack();
            }
        }
    }

    slot->set(slotStack.isEmpty() ? ItemStack() : slotStack);

    return originalStack;
}

// ========== 私有方法 ==========

void FurnaceContainer::_initSlots(PlayerInventory* playerInventory)
{
    // ========== 熔炉槽位 ==========

    // 输入槽（顶部中央）
    addSlot(std::make_unique<Slot>(m_furnaceInventory, SLOT_INPUT, 56, 17));

    // 燃料槽（中部中央）- 使用 FurnaceFuelSlot 限制只能放入燃料
    addSlot(std::make_unique<FurnaceFuelSlot>(m_furnaceInventory, SLOT_FUEL, 56, 53));

    // 输出槽（底部中央）- 使用 FurnaceResultSlot 处理经验发放
    // 从 PlayerInventory 获取玩家指针
    Player* player = playerInventory->getPlayer();
    addSlot(std::make_unique<FurnaceResultSlot>(player, // Player* 用于发放经验
        m_furnaceInventory,                             // IInventory* 熔炉背包
        SLOT_OUTPUT,                                    // 槽位索引
        116,
        35,             // 显示坐标
        m_furnaceEntity // AbstractFurnaceEntity* 用于提取累积经验
        ));

    // 同步设置 m_player（向后兼容）
    m_player = player;

    // ========== 玩家主背包（3行9列）==========

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col; // 玩家背包从索引9开始
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = PLAYER_INV_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏（1行9列）==========

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col; // 快捷栏从索引0开始
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = HOTBAR_Y;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }
}

void FurnaceContainer::_initTrackedInts()
{
    // tracked int 绑定到菜单内独立存储成员，与实体解耦：
    // 服务端 syncProgressFromEntity 每 tick 把实体值刷进成员，detectAndSendChanges 检测变化下推；
    // 客户端 setTrackedInt 直接写成员（实体为 nullptr 时仍持久化）。
    trackInt([this]() { return m_dataLitTime; }, [this](i32 v) { m_dataLitTime = v; });         // DATA_LIT_TIME = 0
    trackInt([this]() { return m_dataLitDuration; }, [this](i32 v) { m_dataLitDuration = v; }); // DATA_LIT_DURATION = 1
    trackInt([this]() { return m_dataCookingProgress; },
        [this](i32 v) { m_dataCookingProgress = v; }); // DATA_COOKING_PROGRESS = 2
    trackInt([this]() { return m_dataCookingTotalTime; },
        [this](i32 v) { m_dataCookingTotalTime = v; }); // DATA_COOKING_TOTAL_TIME = 3
}

void FurnaceContainer::syncProgressFromEntity()
{
    if (m_furnaceEntity == nullptr) {
        return;
    }
    m_dataLitTime = m_furnaceEntity->getBurnTime();
    m_dataLitDuration = m_furnaceEntity->getBurnTimeTotal();
    m_dataCookingProgress = m_furnaceEntity->getCookTime();
    m_dataCookingTotalTime = m_furnaceEntity->getCookTimeTotal();
}

f32 FurnaceContainer::getLitProgress() const
{
    const i32 burnTime = (getTrackedInt(DATA_LIT_TIME) != nullptr) ? getTrackedInt(DATA_LIT_TIME)->get() : 0;
    const i32 burnTimeTotal =
        (getTrackedInt(DATA_LIT_DURATION) != nullptr) ? getTrackedInt(DATA_LIT_DURATION)->get() : 0;
    if (burnTimeTotal <= 0) {
        return 0.0f;
    }
    return std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
}

f32 FurnaceContainer::getBurnProgress() const
{
    const i32 cookTime =
        (getTrackedInt(DATA_COOKING_PROGRESS) != nullptr) ? getTrackedInt(DATA_COOKING_PROGRESS)->get() : 0;
    const i32 cookTimeTotal =
        (getTrackedInt(DATA_COOKING_TOTAL_TIME) != nullptr) ? getTrackedInt(DATA_COOKING_TOTAL_TIME)->get() : 0;
    if (cookTimeTotal <= 0 || cookTime <= 0) {
        return 0.0f;
    }
    return std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
}

} // namespace blockentity
} // namespace mc
