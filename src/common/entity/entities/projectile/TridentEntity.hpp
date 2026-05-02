#pragma once

#include "AbstractArrowEntity.hpp"
#include "../../../item/core/ItemStack.hpp"
#include <memory>

namespace mc {
namespace entity {

/**
 * @brief 三叉戟实体
 *
 * 三叉戟是一种特殊的投掷武器，可以被玩家拾取并具有特殊攻击模式。
 *
 * 参考 MC 1.16.5 TridentEntity
 */
class TridentEntity : public AbstractArrowEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    TridentEntity(LegacyEntityType type, EntityId id);

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.5f; }
    [[nodiscard]] f32 height() const override { return 0.5f; }

    void tick() override;

    // ========== 三叉戟属性 ==========

    /**
     * @brief 获取三叉戟物品堆
     */
    [[nodiscard]] ItemStack getItemStack() const { return m_tridentStack; }

    /**
     * @brief 设置三叉戟物品堆（同时更新附魔等级）
     */
    void setItemStack(const ItemStack& stack);

    /**
     * @brief 是否在返回中（忠诚附魔）
     */
    [[nodiscard]] bool isReturning() const { return m_returning; }

    /**
     * @brief 设置返回状态
     */
    void setReturning(bool returning) { m_returning = returning; }

    /**
     * @brief 是否已击中方块（插入方块）
     */
    [[nodiscard]] bool hasHitBlock() const { return m_hitBlock; }

    /**
     * @brief 击中方块的坐标
     */
    [[nodiscard]] BlockPos hitBlockPos() const { return m_hitBlockPos; }

    /**
     * @brief 获取忠诚附魔等级
     */
    [[nodiscard]] u8 loyaltyLevel() const { return m_loyaltyLevel; }

    /**
     * @brief 设置忠诚附魔等级
     */
    void setLoyaltyLevel(u8 level) { m_loyaltyLevel = level; }

    /**
     * @brief 获取水中阻力
     * 参考 MC 1.16.5: 三叉戟水中阻力为 0.99
     */
    [[nodiscard]] f32 getWaterDrag() const override;

    /**
     * @brief 根据发射者设置附魔效果
     */
    void setEnchantmentEffectsFrom(LivingEntity& shooter, f32 baseVelocity);

    /**
     * @brief 玩家拾取三叉戟
     */
    bool onPlayerPickup(Player& player) override;

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

    /**
     * @brief 三叉戟在方块中的tick处理
     * 参考 MC 1.16.5: 如果有忠诚附魔，不超时移除
     */
    void tickInGroundTrident();

private:
    /**
     * @brief 处理返回逻辑
     */
    void tickReturning();

    /**
     * @brief 检查是否应该返回到射手
     * 参考 MC 1.16.5 TridentEntity.shouldReturnToThrower()
     */
    bool shouldReturnToThrower();

    ItemStack m_tridentStack;       // 三叉戟物品
    bool m_hitBlock = false;        // 是否击中方块
    bool m_returning = false;       // 是否在返回中
    BlockPos m_hitBlockPos;         // 击中方块的坐标
    u8 m_loyaltyLevel = 0;          // 忠诚附魔等级
    i32 m_returningTicks = 0;       // 返回计时器
};

} // namespace entity
} // namespace mc
