#pragma once

#include "../MonsterEntity.hpp"
#include "../../../core/Types.hpp"
#include <memory>
#include <random>

namespace mc {

/**
 * @brief 史莱姆实体
 *
 * 弹跳的绿色果冻状怪物。
 *
 * 特性：
 * - 分裂：被杀死时分裂成小史莱姆
 * - 弹跳：持续弹跳移动
 * - 尺寸：有4种尺寸（微小、小、中、大）
 * - 掉落：粘液球
 * - 生成：只在特定区块
 *
 * 参考 MC 1.16.5 SlimeEntity
 */
class SlimeEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SlimeEntity(LegacyEntityType type, EntityId id);
    ~SlimeEntity() override = default;

    // 禁止拷贝
    SlimeEntity(const SlimeEntity&) = delete;
    SlimeEntity& operator=(const SlimeEntity&) = delete;

    // 允许移动
    SlimeEntity(SlimeEntity&&) = default;
    SlimeEntity& operator=(SlimeEntity&&) = default;

    /**
     * @brief 创建史莱姆实体
     * @param world 世界实例
     * @return 新的史莱姆实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 尺寸系统 ==========

    /**
     * @brief 获取史莱姆尺寸
     * 尺寸范围 1-4
     */
    [[nodiscard]] i32 getSlimeSize() const { return m_size; }

    /**
     * @brief 设置史莱姆尺寸
     */
    void setSlimeSize(i32 size);

    /**
     * @brief 是否是微小史莱姆
     */
    [[nodiscard]] bool isTiny() const { return m_size == 1; }

    /**
     * @brief 是否是大型史莱姆
     */
    [[nodiscard]] bool isLarge() const { return m_size >= 4; }

    // ========== 弹跳系统 ==========

    /**
     * @brief 是否正在弹跳
     */
    [[nodiscard]] bool isJumping() const { return m_jumping; }

    /**
     * @brief 设置弹跳状态
     */
    void setJumping(bool jumping) { m_jumping = jumping; }

    /**
     * @brief 获取弹跳计时器
     */
    [[nodiscard]] i32 getJumpTimer() const { return m_jumpTimer; }

    // ========== 分裂 ==========

    /**
     * @brief 分裂成小史莱姆
     */
    void split();

    /**
     * @brief 检查是否可以分裂
     */
    [[nodiscard]] bool canSplit() const { return m_size > 1; }

    // ========== 攻击 ==========

    /**
     * @brief 是否正在攻击
     */
    [[nodiscard]] bool isAttacking() const { return m_attacking; }

    /**
     * @brief 设置攻击状态
     */
    void setAttacking(bool attacking) { m_attacking = attacking; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 史莱姆不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 根据尺寸更新属性
     */
    void updateSizeAttributes();

private:
    // 尺寸
    i32 m_size = 1;

    // 弹跳状态
    bool m_jumping = false;
    i32 m_jumpTimer = 0;
    i32 m_jumpCooldown = 0;

    // 攻击状态
    bool m_attacking = false;
    i32 m_attackCooldown = 0;

    // 面向方向
    f32 m_facingAngle = 0.0f;

    // 常量
    static constexpr i32 JUMP_COOLDOWN_MIN = 20; // 最小弹跳冷却
    static constexpr i32 JUMP_COOLDOWN_MAX = 60; // 最大弹跳冷却
    static constexpr i32 ATTACK_COOLDOWN = 20;   // 攻击冷却
};

} // namespace mc
