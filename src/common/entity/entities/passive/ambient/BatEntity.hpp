#pragma once

#include "AmbientEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include <memory>

namespace mc {

/**
 * @brief 蝙蝠实体
 *
 * 生活在洞穴中的飞行生物。
 *
 * 特性：
 * - 飞行：在空中飞行
 * - 倒挂：白天会倒挂在方块下
 * - 休息：休息时不发出声音
 * - 睡眠：白天睡眠，夜间活动
 *
 * 参考 MC 1.16.5 BatEntity
 */
class BatEntity : public AmbientEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    BatEntity(LegacyEntityType type, EntityId id);
    ~BatEntity() override = default;

    // 禁止拷贝
    BatEntity(const BatEntity&) = delete;
    BatEntity& operator=(const BatEntity&) = delete;

    // 允许移动
    BatEntity(BatEntity&&) = default;
    BatEntity& operator=(BatEntity&&) = default;

    /**
     * @brief 创建蝙蝠实体
     * @param world 世界实例
     * @return 新的蝙蝠实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 飞行状态 ==========

    /**
     * @brief 是否正在飞行
     */
    [[nodiscard]] bool isFlying() const { return m_flying; }

    /**
     * @brief 设置飞行状态
     */
    void setFlying(bool flying) { m_flying = flying; }

    // ========== 休息状态 ==========

    /**
     * @brief 是否正在休息（倒挂）
     */
    [[nodiscard]] bool isResting() const { return m_resting; }

    /**
     * @brief 设置休息状态
     */
    void setResting(bool resting) { m_resting = resting; }

    /**
     * @brief 是否可以休息
     * 检查上方是否有方块可以倒挂
     */
    [[nodiscard]] bool canRest() const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.1f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 飞行状态
    bool m_flying = true;
    bool m_resting = false;

    // 休息位置
    BlockPos m_restPos;
    f32 m_restAngle = 0.0f;

    // 飞行计时器
    i32 m_flyTimer = 0;

    // 常量
    static constexpr f32 FLY_SPEED = 0.1f;
};

} // namespace mc
