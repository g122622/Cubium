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

#pragma once

#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <nlohmann/json_fwd.hpp>

// Forward declarations
namespace mc {
class ItemStack;
class ServerPlayer;
class PlayerInventory;
} // namespace mc

namespace mc::server {
class PlayerAdvancements;
}

namespace mc::advancement {

// Forward declare the Instance first
class InventoryChangedTriggerInstance;

/**
 * @brief 物品栏变化触发器
 *
 * 当玩家物品栏发生变化时触发。
 */
class InventoryChangedTrigger : public AbstractCriterionTrigger<InventoryChangedTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:inventory_changed";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const noexcept override { return ResourceLocation(TRIGGER_ID); }

    /**
     * @brief 从JSON反序列化实例
     */
    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /**
     * @brief 触发检测（使用 ServerPlayer）
     * @param player 玩家
     * @param inventory 物品栏
     */
    void trigger(mc::ServerPlayer& player, const mc::PlayerInventory& inventory);

    /**
     * @brief 使用谓词触发检测
     *
     * 此方法允许服务端代码直接使用谓词触发检测。
     * 谓词接收 InventoryChangedTriggerInstance 实例，返回 true 表示条件满足。
     *
     * @param advancements 玩家成就进度
     * @param predicate 检测谓词
     */
    template <typename PredicateT>
    void triggerWithPredicate(::mc::server::PlayerAdvancements& advancements, PredicateT&& predicate)
    {
        AbstractCriterionTrigger<InventoryChangedTriggerInstance>::trigger(
            advancements, std::forward<PredicateT>(predicate));
    }

    // 静态工厂方法
    static std::shared_ptr<InventoryChangedTriggerInstance> hasItems(std::vector<ItemPredicate> items);
    static std::shared_ptr<InventoryChangedTriggerInstance> hasItem(const ItemPredicate& item);
};

/**
 * @brief 物品栏变化触发器实例
 */
class InventoryChangedTriggerInstance : public CriterionInstance<InventoryChangedTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = InventoryChangedTrigger::TRIGGER_ID;

    InventoryChangedTriggerInstance() = default;

    /**
     * @brief 构造实例
     * @param slotsOccupied 占用槽位范围
     * @param slotsFull 满槽位范围
     * @param slotsEmpty 空槽位范围
     * @param items 物品谓词列表
     */
    InventoryChangedTriggerInstance(
        IntBounds slotsOccupied, IntBounds slotsFull, IntBounds slotsEmpty, std::vector<ItemPredicate> items);

    /**
     * @brief 使用槽位访问器检查条件是否满足
     *
     * 此方法提供与具体物品栏实现解耦的检测接口，
     * 允许在不依赖 PlayerInventory 完整定义的情况下进行检测。
     *
     * @param totalSlots 总槽位数
     * @param getSlot 获取指定槽位物品的函数（返回值类型）
     * @return 是否满足
     */
    [[nodiscard]] bool testWithInventory(i32 totalSlots, const std::function<mc::ItemStack(i32)>& getSlot) const;

    /**
     * @brief 从JSON解析
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    // ========== Getters ==========

    [[nodiscard]] const IntBounds& getSlotsOccupied() const noexcept { return m_slotsOccupied; }
    [[nodiscard]] const IntBounds& getSlotsFull() const noexcept { return m_slotsFull; }
    [[nodiscard]] const IntBounds& getSlotsEmpty() const noexcept { return m_slotsEmpty; }
    [[nodiscard]] const std::vector<ItemPredicate>& getItems() const noexcept { return m_items; }

private:
    IntBounds m_slotsOccupied;
    IntBounds m_slotsFull;
    IntBounds m_slotsEmpty;
    std::vector<ItemPredicate> m_items;
};

} // namespace mc::advancement
