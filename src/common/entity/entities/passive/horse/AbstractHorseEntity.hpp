#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../interfaces/IRideable.hpp"
#include "../../../interfaces/IJumpingMount.hpp"
#include "../../../interfaces/IEquipable.hpp"
#include "../../../core/DataParameter.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../world/blockentity/core/SimpleInventory.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

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
                            public entity::IJumpingMount,
                            public entity::IEquipable {
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

    // ========== IEquipable 接口实现 ==========

    /**
     * @brief 获取装备槽数量
     */
    [[nodiscard]] i32 getEquipmentSlotCount() const override { return getInventorySize(); }

    /**
     * @brief 获取指定槽位的装备
     */
    [[nodiscard]] ItemStack getEquipment(i32 slot) const override;

    /**
     * @brief 设置指定槽位的装备
     */
    void setEquipment(i32 slot, const ItemStack& item) override;

    /**
     * @brief 检查是否可以装备指定物品
     */
    [[nodiscard]] bool canEquip(const ItemStack& item, i32 slot) const override;

    // ========== IRideable travelTowards ==========

    /**
     * @brief 执行骑乘移动逻辑
     * MC 1.16.5: travelTowards()
     */
    void travelTowards(const Vector3& travelVec) override;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 骑乘移动处理
     * MC 1.16.5: travel(Vector3d)
     */
    void travel(f32 strafing, f32 vertical, f32 forward) override;

protected:
    void registerGoals() override;
    void registerAttributes() override;
    void registerData() override;

    // ========== 状态标志辅助方法 ==========

    /**
     * @brief 获取状态标志
     * MC 1.16.5: getHorseWatchableBoolean()
     */
    [[nodiscard]] bool getHorseWatchableBoolean(i8 flag) const;

    /**
     * @brief 设置状态标志
     * MC 1.16.5: setHorseWatchableBoolean()
     */
    void setHorseWatchableBoolean(i8 flag, bool value);

    // ========== 尺寸 ==========
    // 子类应该重写这些方法以提供正确的尺寸

    [[nodiscard]] f32 getBaseWidth() const override { return 1.3964844f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 1.6f; }

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

    /**
     * @brief 初始化马背包
     * MC 1.16.5: initHorseChest()
     */
    void initHorseChest();

protected:
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
    bool m_allowStandSliding = false;  // MC 1.16.5: 允许站立滑动
    i32 m_jumpCooldown = 0;

    // 加速状态
    i32 m_boostTime = 0;
    bool m_isBoosting = false;

    // 属性（马特有）
    f32 m_speed = 0.0f;
    f32 m_jumpHeight = 0.0f;
    f32 m_horseHealth = 0.0f;  // 改名避免与基类冲突

    // 库存（鞍槽 + 马铠槽）
    std::unique_ptr<blockentity::SimpleInventory> m_inventory;

    // 动画状态
    i32 m_eatingCounter = 0;
    i32 m_openMouthCounter = 0;
    i32 m_jumpRearingCounter = 0;
    i32 m_tailCounter = 0;
    i32 m_sprintCounter = 0;
    f32 m_headLean = 0.0f;
    f32 m_prevHeadLean = 0.0f;
    f32 m_rearingAmount = 0.0f;
    f32 m_prevRearingAmount = 0.0f;
    f32 m_mouthOpenness = 0.0f;
    f32 m_prevMouthOpenness = 0.0f;

private:
    // MC 1.16.5 数据参数
    static entity::DataParameter<i8> STATUS_PARAM;  // 使用 i8 代替 u8（DataValue 支持的类型）
    static entity::DataParameter<i64> OWNER_UUID_PARAM;  // 0 表示无主人

    // 状态标志位
    static constexpr i8 STATUS_FLAG_SADDLE = 0b00000001;
    static constexpr i8 STATUS_FLAG_TAME = 0b00000010;
    static constexpr i8 STATUS_FLAG_BRED = 0b00000100;
    static constexpr i8 STATUS_FLAG_EATING = 0b00001000;
    static constexpr i8 STATUS_FLAG_REARING = 0b00010000;
    static constexpr i8 STATUS_FLAG_MOUTH_OPEN = 0b00100000;

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
