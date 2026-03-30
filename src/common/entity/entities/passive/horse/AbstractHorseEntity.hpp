#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../interfaces/IRideable.hpp"
#include "../../../interfaces/IJumpingMount.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;

/**
 * @brief 马类实体基类
 *
 * 所有马类实体（马、驴、骡、羊驼、骷髅马、僵尸马）的抽象基类。
 * 实现可骑乘、可跳跃、装备栏等通用功能。
 *
 * 参考 MC 1.16.5 AbstractHorseEntity
 */
class AbstractHorseEntity : public AnimalEntity,
                            public entity::IRideable,
                            public entity::IJumpingMount {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    AbstractHorseEntity(LegacyEntityType type, EntityId id);
    ~AbstractHorseEntity() override = default;

    // 禁止拷贝
    AbstractHorseEntity(const AbstractHorseEntity&) = delete;
    AbstractHorseEntity& operator=(const AbstractHorseEntity&) = delete;

    // 允许移动
    AbstractHorseEntity(AbstractHorseEntity&&) = default;
    AbstractHorseEntity& operator=(AbstractHorseEntity&&) = default;

    // ========== IRideable 接口实现 ==========

    [[nodiscard]] bool hasSaddle() const override { return m_saddled; }
    void setSaddle(bool saddle) override;
    void onPlayerStartRiding(Player* player) override;
    void onPlayerStopRiding(Player* player) override;
    [[nodiscard]] f32 getSteeringSpeed() const override;
    bool boost() override;
    [[nodiscard]] i32 getBoostTime() const override { return m_boostTime; }
    void setBoostTime(i32 time) override { m_boostTime = time; }

    // ========== IJumpingMount 接口实现 ==========

    void onJump() override;
    [[nodiscard]] f32 getJumpPower() const override { return m_jumpPower; }
    void setJumpPower(f32 power) override;
    [[nodiscard]] f32 getMaxJumpHeight() const override;
    [[nodiscard]] bool canJump() const override;
    void startJumping() override;
    void stopJumping() override;

    // ========== 骑乘系统 ==========

    /**
     * @brief 检查是否正在被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const;

    /**
     * @brief 检查玩家是否可以骑乘
     * @param player 玩家
     * @return 是否可以骑乘
     */
    [[nodiscard]] bool canBeRiddenBy(Player* player) const;

    /**
     * @brief 获取骑乘者（玩家）
     * @return 骑乘者指针，如果没有则返回 nullptr
     */
    [[nodiscard]] Player* getRider() const { return m_rider; }

    /**
     * @brief 设置骑乘者
     * @param rider 骑乘者
     */
    void setRider(Player* rider) { m_rider = rider; }

    // ========== 驯服系统 ==========

    /**
     * @brief 是否已驯服
     */
    [[nodiscard]] bool isTame() const { return m_tame; }

    /**
     * @brief 设置驯服状态
     * @param tame 是否驯服
     */
    void setTame(bool tame);

    /**
     * @brief 获取驯服进度 (0-100)
     */
    [[nodiscard]] i32 getTemper() const { return m_temper; }

    /**
     * @brief 增加驯服进度
     * @param amount 增加量
     * @return 是否达到驯服阈值
     */
    bool increaseTemper(i32 amount);

    /**
     * @brief 获取最大驯服进度
     */
    [[nodiscard]] i32 getMaxTemper() const { return m_maxTemper; }

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 是否可用于驯服
     */
    [[nodiscard]] virtual bool isTameItem(const ItemStack& itemStack) const;

    // ========== 装备系统 ==========

    /**
     * @brief 获取装备栏大小
     */
    [[nodiscard]] virtual i32 getInventorySize() const { return 2; } // 鞍槽 + 马铠槽

    /**
     * @brief 是否有马铠
     */
    [[nodiscard]] bool hasArmor() const { return m_hasArmor; }

    /**
     * @brief 设置马铠状态
     */
    void setArmor(bool armor) { m_hasArmor = armor; }

    // ========== 速度和跳跃 ==========

    /**
     * @brief 获取移动速度
     */
    [[nodiscard]] f32 getSpeed() const;

    /**
     * @brief 获取跳跃强度
     */
    [[nodiscard]] f32 getJumpStrength() const { return m_jumpStrength; }

    /**
     * @brief 设置跳跃强度
     */
    void setJumpStrength(f32 strength) { m_jumpStrength = strength; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerAttributes() override;

    /**
     * @brief 更新骑乘状态
     */
    void updateRiding();

    /**
     * @brief 更新跳跃蓄力
     */
    void updateJumpPower();

    /**
     * @brief 执行跳跃
     */
    void performJump();

    /**
     * @brief 更新加速状态
     */
    void updateBoost();

    /**
     * @brief 初始化随机属性
     *
     * 马的跳跃、速度、生命值在生成时随机确定
     */
    void initRandomAttributes();

private:
    // 骑乘状态
    Player* m_rider = nullptr;
    bool m_saddled = false;
    bool m_hasArmor = false;

    // 驯服状态
    bool m_tame = false;
    i32 m_temper = 0;
    i32 m_maxTemper = 100;

    // 跳跃状态
    f32 m_jumpPower = 0.0f;
    f32 m_jumpStrength = 0.0f;  // 基础跳跃强度
    bool m_isJumping = false;
    i32 m_jumpCooldown = 0;

    // 加速状态
    i32 m_boostTime = 0;
    bool m_isBoosting = false;

    // 属性（马特有）
    f32 m_speed = 0.0f;
    f32 m_jumpHeight = 0.0f;
    f32 m_health = 0.0f;

    // 常量
    static constexpr f32 MIN_SPEED = 0.1127f;       // 最小速度
    static constexpr f32 MAX_SPEED = 0.3375f;       // 最大速度
    static constexpr f32 MIN_JUMP = 0.4f;           // 最小跳跃
    static constexpr f32 MAX_JUMP = 1.0f;           // 最大跳跃
    static constexpr f32 MIN_HEALTH = 15.0f;        // 最小生命值
    static constexpr f32 MAX_HEALTH = 30.0f;        // 最大生命值
    static constexpr i32 MAX_BOOST_TIME = 300;      // 最大加速时间（ticks）
    static constexpr f32 JUMP_POWER_SCALE = 0.98f;  // 跳跃蓄力缩放
};

} // namespace mc
