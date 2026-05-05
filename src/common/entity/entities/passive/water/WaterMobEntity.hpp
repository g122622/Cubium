#pragma once

#include "../../../core/CreatureEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 水生生物基类
 *
 * 生活在水中的生物的基类。
 *
 * 特性：
 * - 水下呼吸：可以在水下呼吸
 * - 水外窒息：离开水会逐渐窒息
 * - 游泳行为：在水中游泳
 * - 陆地挣扎：在陆地上会扑腾
 *
 * 参考 MC 1.16.5 WaterMobEntity / WaterCreatureEntity
 */
class WaterMobEntity : public CreatureEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    WaterMobEntity(LegacyEntityType type, EntityId id);
    ~WaterMobEntity() override = default;

    // 禁止拷贝
    WaterMobEntity(const WaterMobEntity&) = delete;
    WaterMobEntity& operator=(const WaterMobEntity&) = delete;

    // 允许移动
    WaterMobEntity(WaterMobEntity&&) = default;
    WaterMobEntity& operator=(WaterMobEntity&&) = default;

    // ========== 水下状态 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 是否在水中或气泡中
     */
    [[nodiscard]] bool isInWaterOrBubble() const;

    /**
     * @brief 是否可以生成
     * 检查是否在适合生成的水域
     */
    [[nodiscard]] virtual bool canSpawnInWater() const { return true; }

    // ========== 呼吸系统 ==========

    /**
     * @brief 获取空气供应量
     */
    [[nodiscard]] i32 getAirSupply() const { return m_airSupply; }

    /**
     * @brief 设置空气供应量
     */
    void setAirSupply(i32 supply) { m_airSupply = supply; }

    /**
     * @brief 获取最大空气供应量
     */
    [[nodiscard]] i32 getMaxAirSupply() const { return m_maxAirSupply; }

    /**
     * @brief 设置最大空气供应量
     */
    void setMaxAirSupply(i32 maxSupply) { m_maxAirSupply = maxSupply; }

    /**
     * @brief 是否在窒息
     */
    [[nodiscard]] bool isDrowning() const { return m_airSupply <= 0; }

    // ========== 行为 ==========

    /**
     * @brief 是否可以游泳
     */
    [[nodiscard]] virtual bool canSwim() const { return true; }

    /**
     * @brief 是否会被水流推动
     */
    [[nodiscard]] virtual bool canBePushedByWater() const { return true; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 更新空气供应
     */
    void updateAirSupply();

    /**
     * @brief 当离开水时调用
     */
    virtual void onLeaveWater() {}

    /**
     * @brief 当进入水时调用
     */
    virtual void onEnterWater() {}

private:
    // 空气供应
    i32 m_airSupply = 300;     // 当前空气量
    i32 m_maxAirSupply = 300;  // 最大空气量（15秒）

    // 溺水伤害计时器
    i32 m_drownDamageTimer = 0;

    // 水状态追踪（用于 onEnterWater/onLeaveWater 回调）
    bool m_wasInWater = false;

    // 溺水伤害量（水生生物比玩家少）
    // MC 1.16.5: 水生生物每次受到 1.0F 伤害，玩家受到 2.0F
    static constexpr f32 DROWN_DAMAGE_AMOUNT = 1.0f;
};

} // namespace mc
