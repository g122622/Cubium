#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
using BlockId = u32;  // 方块ID类型

/**
 * @brief 末影人实体
 *
 * 可以瞬移的中立型怪物。
 *
 * 特性：
 * - 瞬移：被攻击或看眼睛时会瞬移
 * - 搬方块：可以搬起和放置方块
 * - 中立：通常中立，被激怒后攻击
 * - 怕水：接触水会瞬移并受到伤害
 *
 * 参考 MC 1.16.5 EndermanEntity
 */
class EndermanEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    EndermanEntity(LegacyEntityType type, EntityId id);
    ~EndermanEntity() override = default;

    // 禁止拷贝
    EndermanEntity(const EndermanEntity&) = delete;
    EndermanEntity& operator=(const EndermanEntity&) = delete;

    // 允许移动
    EndermanEntity(EndermanEntity&&) = default;
    EndermanEntity& operator=(EndermanEntity&&) = default;

    /**
     * @brief 创建末影人实体
     * @param world 世界实例
     * @return 新的末影人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 愤怒系统 ==========

    /**
     * @brief 是否愤怒
     */
    [[nodiscard]] bool isAngry() const { return m_angry; }

    /**
     * @brief 设置愤怒状态
     */
    void setAngry(bool angry) { m_angry = angry; }

    /**
     * @brief 是否被激怒
     */
    [[nodiscard]] bool isProvoked() const { return m_provoked; }

    /**
     * @brief 设置激怒状态
     */
    void setProvoked(bool provoked) { m_provoked = provoked; }

    // ========== 瞬移系统 ==========

    /**
     * @brief 尝试瞬移
     * @param reason 瞬移原因
     * @return 是否成功瞬移
     */
    bool teleport();

    /**
     * @brief 尝试瞬移到目标附近
     */
    bool teleportToTarget();

    /**
     * @brief 尝试瞬移避开水
     */
    bool teleportAwayFromWater();

    // ========== 搬方块系统 ==========

    /**
     * @brief 是否拿着方块
     */
    [[nodiscard]] bool isHoldingBlock() const { return m_holdingBlock; }

    /**
     * @brief 获取拿着的方块
     */
    [[nodiscard]] BlockId getHeldBlock() const { return m_heldBlock; }

    /**
     * @brief 设置拿着的方块
     */
    void setHeldBlock(BlockId block) { m_heldBlock = block; m_holdingBlock = true; }

    /**
     * @brief 放下方块
     */
    void placeHeldBlock();

    /**
     * @brief 拿起方块
     */
    void pickUpBlock();

    // ========== 阳光燃烧 ==========

    /**
     * @brief 末影人不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 水伤害 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.55f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 愤怒状态
    bool m_angry = false;
    bool m_provoked = false;
    i32 m_angerTime = 0;

    // 搬方块
    bool m_holdingBlock = false;
    BlockId m_heldBlock = 0;

    // 瞬移冷却
    i32 m_teleportCooldown = 0;

    // 常量
    static constexpr i32 TELEPORT_COOLDOWN = 50;    // 瞬移冷却
    static constexpr i32 ANGER_DURATION = 600;      // 愤怒持续时间
    static constexpr f32 WATER_DAMAGE = 1.0f;       // 水伤害
    static constexpr i32 WATER_DAMAGE_INTERVAL = 10; // 水伤害间隔
};

} // namespace mc
