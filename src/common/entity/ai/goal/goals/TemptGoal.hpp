#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"
#include <functional>

namespace mc {

// 前向声明
class CreatureEntity;
class Player;
class ItemStack;

namespace entity::ai::goal {

/**
 * @brief 食物诱惑目标
 *
 * 当玩家手持特定物品时，动物会被诱惑跟随玩家。
 *
 * 参考 MC 1.16.5 TemptGoal
 */
class TemptGoal : public Goal {
public:
    /**
     * @brief 物品检查函数类型
     */
    using ItemPredicate = std::function<bool(const ItemStack&)>;

    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param speed 移动速度倍率
     * @param itemPredicate 物品检查函数
     * @param scaredByMovement 是否被玩家移动吓跑
     */
    TemptGoal(CreatureEntity* creature, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement = false);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 检查是否正在执行
     */
    [[nodiscard]] bool isRunning() const { return m_isRunning; }

    [[nodiscard]] String getTypeName() const override { return "TemptGoal"; }

protected:
    /**
     * @brief 检查玩家手持物品是否为诱惑物品
     * @param stack 物品堆
     * @return 是否为诱惑物品
     */
    [[nodiscard]] bool isTempting(const ItemStack& stack) const;

    /**
     * @brief 检查是否被玩家移动吓跑
     */
    [[nodiscard]] bool isScaredByPlayerMovement() const;

    /**
     * @brief 寻找附近手持诱惑物品的玩家
     * @return 玩家实体，如果没有则返回 nullptr
     */
    Player* findTemptingPlayer();

    CreatureEntity* m_creature;
    f64 m_speed;
    ItemPredicate m_itemPredicate;
    bool m_scaredByMovement;
    Player* m_temptingPlayer = nullptr;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    f64 m_prevPitch = 0.0;
    f64 m_prevYaw = 0.0;
    i32 m_delayTemptCounter = 0;
    bool m_isRunning = false;

    static constexpr f32 TEMPT_RANGE = 10.0f; // 诱惑范围
    static constexpr i32 TEMPT_COOLDOWN = 100; // 诱惑冷却（ticks）
};

} // namespace entity::ai::goal
} // namespace mc
