#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../interfaces/IRideable.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;

/**
 * @brief 炽足兽实体
 *
 * 生活在下界的被动生物，可以在熔岩上行走。
 *
 * 特性：
 * - 熔岩行走：可以在熔岩表面行走
 * - 骑乘：可以被玩家骑乘，使用 warped fungus on a stick 控制
 * - 冷却：离开熔岩后会发抖，需要回到熔岩
 * - 繁殖：使用诡异菌繁殖
 * - 乘骑：小炽足兽会骑在成年炽足兽头上
 *
 * 参考 MC 1.16.5 StriderEntity
 */
class StriderEntity : public AnimalEntity, public entity::IRideable {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    StriderEntity(LegacyEntityType type, EntityId id);
    ~StriderEntity() override = default;

    // 禁止拷贝
    StriderEntity(const StriderEntity&) = delete;
    StriderEntity& operator=(const StriderEntity&) = delete;

    // 允许移动
    StriderEntity(StriderEntity&&) = default;
    StriderEntity& operator=(StriderEntity&&) = default;

    /**
     * @brief 创建炽足兽实体
     * @param world 世界实例
     * @return 新的炽足兽实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 熔岩状态 ==========

    /**
     * @brief 是否在熔岩中
     */
    [[nodiscard]] bool isInLava() const override;

    /**
     * @brief 是否在熔岩表面
     */
    [[nodiscard]] bool isOnLavaSurface() const { return m_onLavaSurface; }

    /**
     * @brief 设置是否在熔岩表面
     */
    void setOnLavaSurface(bool surface) { m_onLavaSurface = surface; }

    /**
     * @brief 是否寒冷（不在熔岩中）
     */
    [[nodiscard]] bool isCold() const { return m_coldTimer > 0; }

    /**
     * @brief 获取寒冷计时器
     */
    [[nodiscard]] i32 getColdTimer() const { return m_coldTimer; }

    /**
     * @brief 设置寒冷计时器
     */
    void setColdTimer(i32 timer) { m_coldTimer = timer; }

    // ========== 骑乘系统 (IRideable) ==========

    [[nodiscard]] bool hasSaddle() const override { return m_saddled; }
    void setSaddle(bool saddle) override;
    void onPlayerStartRiding(mc::Player* player) override { (void)player; m_isBeingRidden = true; }
    void onPlayerStopRiding(mc::Player* player) override { (void)player; m_isBeingRidden = false; }
    [[nodiscard]] f32 getSteeringSpeed() const override;
    bool boost() override;

    /**
     * @brief 是否被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const { return m_isBeingRidden; }

    /**
     * @brief 是否可以骑乘
     */
    [[nodiscard]] bool canBeRidden() const { return true; }

    // ========== 加速系统 ==========

    /**
     * @brief 是否正在加速
     */
    [[nodiscard]] bool isBoosting() const { return m_boosting; }

    /**
     * @brief 设置加速状态
     */
    void setBoosting(bool boosting) { m_boosting = boosting; }

    /**
     * @brief 获取加速时间
     */
    [[nodiscard]] i32 getBoostTime() const override { return m_boostTime; }

    /**
     * @brief 设置加速时间
     */
    void setBoostTime(i32 time) override { m_boostTime = time; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 炽足兽使用诡异菌繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.5f : 1.0f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 熔岩状态
    bool m_onLavaSurface = false;
    i32 m_coldTimer = 0;

    // 骑乘状态
    bool m_saddled = false;
    bool m_isBeingRidden = false;
    u64 m_riderId = 0;

    // 加速状态
    bool m_boosting = false;
    i32 m_boostTime = 0;
    i32 m_boostCooldown = 0;

    // 常量
    static constexpr i32 COLD_DURATION = 100; // 5秒冷却
    static constexpr i32 BOOST_DURATION_MIN = 140; // 最小加速时间
    static constexpr i32 BOOST_DURATION_MAX = 700; // 最大加速时间
    static constexpr f32 BOOST_SPEED = 0.3f; // 加速速度
};

} // namespace mc
