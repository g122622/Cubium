#pragma once

#include "../CriterionTrigger.hpp"
#include "../conditions/ItemPredicate.hpp"
#include <vector>

namespace mc::advancement {

// Forward declarations
class ServerPlayer;
class PlayerInventory;

// Forward declare the Instance
class InventoryChangedTriggerInstance;

/**
 * @brief 物品栏变化触发器
 *
 * 当玩家物品栏发生变化时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.InventoryChangeTrigger
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
    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    /**
     * @brief 从JSON反序列化实例
     */
    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /**
     * @brief 触发检测
     * @param player 玩家
     * @param inventory 物品栏
     */
    void trigger(ServerPlayer& player, const PlayerInventory& inventory);

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
        IntBounds slotsOccupied,
        IntBounds slotsFull,
        IntBounds slotsEmpty,
        std::vector<ItemPredicate> items
    );

    /**
     * @brief 检查条件是否满足
     * @param player 玩家
     * @param inventory 物品栏
     * @return 是否满足
     */
    [[nodiscard]] bool test(ServerPlayer& player, const PlayerInventory& inventory) const;

    /**
     * @brief 从JSON解析
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    IntBounds m_slotsOccupied;
    IntBounds m_slotsFull;
    IntBounds m_slotsEmpty;
    std::vector<ItemPredicate> m_items;
};

} // namespace mc::advancement
