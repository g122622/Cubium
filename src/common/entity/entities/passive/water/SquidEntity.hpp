#pragma once

#include "../../../../core/Types.hpp"
#include "../water/WaterMobEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 鱿鱼实体
 *
 * 生活在海洋中的无脊椎动物。
 *
 * 特性：
 * - 喷墨：受到攻击时会喷出墨汁
 * - 游泳：在水中优雅地游动
 * - 挣扎：离开水会扑腾
 * - 掉落：墨囊
 *
 * 参考 MC 1.16.5 SquidEntity
 */
class SquidEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SquidEntity(LegacyEntityType type, EntityId id);
    ~SquidEntity() override = default;

    // 禁止拷贝
    SquidEntity(const SquidEntity&) = delete;
    SquidEntity& operator=(const SquidEntity&) = delete;

    // 允许移动
    SquidEntity(SquidEntity&&) = default;
    SquidEntity& operator=(SquidEntity&&) = default;

    /**
     * @brief 创建鱿鱼实体
     * @param world 世界实例
     * @return 新的鱿鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 游泳行为 ==========

    /**
     * @brief 是否在游泳
     */
    [[nodiscard]] bool isSwimming() const { return m_swimming; }

    /**
     * @brief 设置游泳状态
     */
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 获取游泳方向
     */
    [[nodiscard]] f32 getSwimAngle() const { return m_swimAngle; }

    /**
     * @brief 设置游泳方向
     */
    void setSwimAngle(f32 angle) { m_swimAngle = angle; }

    // ========== 喷墨 ==========

    /**
     * @brief 是否正在喷墨
     */
    [[nodiscard]] bool isSprayingInk() const { return m_sprayingInk; }

    /**
     * @brief 喷墨
     */
    void sprayInk();

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.4f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 游泳状态
    bool m_swimming = false;
    f32 m_swimAngle = 0.0f;
    f32 m_targetSwimAngle = 0.0f;

    // 喷墨状态
    bool m_sprayingInk = false;
    i32 m_sprayTimer = 0;

    // 游泳计时器
    i32 m_swimTimer = 0;
    i32 m_changeDirectionTimer = 0;

    // 常量
    static constexpr i32 SWIM_DURATION = 40;      // 每次游泳持续时间
    static constexpr i32 SPRAY_INK_DURATION = 20; // 喷墨持续时间
};

} // namespace mc
