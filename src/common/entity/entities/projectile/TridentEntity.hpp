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
     * @brief 设置三叉戟物品堆
     */
    void setItemStack(const ItemStack& stack) { m_tridentStack = stack; }

    /**
     * @brief 是否在返回中（激流附魔）
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
    [[nodiscard]] BlockCoord hitBlockPos() const { return m_hitBlockPos; }

protected:
    void onEntityHit(const RayTraceResult& result) override;
    void onBlockHit(const RayTraceResult& result) override;

private:
    /**
     * @brief 处理返回逻辑
     */
    void tickReturning();

    ItemStack m_tridentStack;       // 三叉戟物品
    bool m_returning = false;       // 是否在返回
    bool m_hitBlock = false;        // 是否击中方块
    BlockCoord m_hitBlockPos;       // 击中方块的坐标
    i32 m_loyaltyLevel = 0;         // 忠诚附魔等级
    f32 m_dealtDamage = 0.0f;       // 已造成的伤害
};

} // namespace entity
} // namespace mc
