#pragma once

#include "../basic/AnimalEntity.hpp"
#include "../../../interfaces/IJumpingMount.hpp"
#include "../../../interfaces/IEquipable.hpp"
#include "../../../core/DataParameter.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
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
 * 【重要】MC 1.16.5 中，AbstractHorseEntity 只实现 IJumpingMount，
 * 不实现 IRideable 接口。马的控制逻辑通过 MobEntity 的乘客系统实现，
 * 而不是像猪/炽足兽那样通过 IRideable::ride() 方法。
 *
 * 参考 MC 1.16.5 AbstractHorseEntity
 */
class AbstractHorseEntity : public AnimalEntity,
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

    // ========== IJumpingMount 接口实现 ==========

    void onJump() override;
    [[nodiscard]] i32 getJumpPower() const override { return m_jumpPower; }
    void setJumpPower(i32 power) override;
    [[nodiscard]] f32 getMaxJumpHeight() const override;
    [[nodiscard]] bool canJump() const override;
    void startJumping(i32 jumpPower) override;
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
     * @brief 由玩家驯服此马
     *
     * MC 1.16.5: setTamedBy(PlayerEntity player)
     * 设置主人UUID、设为已驯服、触发进度、发送爱心粒子
     *
     * @param player 驯服者
     * @return 是否成功
     */
    bool setTamedBy(Player* player);

    /**
     * @brief 让马愤怒（扬蹄并播放愤怒音效）
     *
     * MC 1.16.5: makeMad()
     * 当驯服失败或被激怒时调用。
     */
    void makeMad();

    /**
     * @brief 让马后腿站立（扬蹄）
     *
     * MC 1.16.5: makeHorseRear()
     */
    void makeHorseRear();

    /**
     * @brief 检查是否正在扬蹄
     * @return 是否正在扬蹄
     */
    [[nodiscard]] bool isRearing() const;

    /**
     * @brief 设置扬蹄状态
     * @param rearing 是否扬蹄
     */
    void setRearing(bool rearing);

    /**
     * @brief 获取愤怒音效
     *
     * 基类返回 nullptr，子类应重写提供具体音效。
     * MC 1.16.5: getAngrySound()
     *
     * @return 愤怒音效资源位置，如果没有返回空
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getAngrySound() const { return std::nullopt; }

    /**
     * @brief 获取主人UUID
     * @return 主人UUID字符串，如果没有主人返回空字符串
     */
    [[nodiscard]] std::string getOwnerUuid() const;

    /**
     * @brief 设置主人UUID
     * @param uuid 主人UUID字符串
     */
    void setOwnerUuid(const std::string& uuid);

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

    // ========== 鞍系统 ==========

    /**
     * @brief 检查是否装备了鞍
     * MC 1.16.5: AbstractHorseEntity.isHorseSaddled()
     */
    [[nodiscard]] bool hasSaddle() const { return m_saddled; }

    /**
     * @brief 设置鞍的状态
     */
    void setSaddle(bool saddle);

    /**
     * @brief 检查是否可以被控制方向
     * MC 1.16.5: 马需要鞍才能被控制
     */
    [[nodiscard]] bool canBeSteered() const { return m_saddled; }

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
    i32 m_jumpPower = 0;        // MC 1.16.5: 跳跃力度 (0-100)
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

    // MC 1.16.5 状态标志位
    // 参考 AbstractHorseEntity.java 行128-138
    // getHorseWatchableBoolean(int p_110233_1_) 使用位掩码检查状态
    // isTame() 使用 getHorseWatchableBoolean(2) -> bit 1
    // isHorseSaddled() 使用 getHorseWatchableBoolean(4) -> bit 2
    static constexpr i8 STATUS_FLAG_TAME = 2;        // bit 1: 已驯服
    static constexpr i8 STATUS_FLAG_SADDLE = 4;      // bit 2: 已装备鞍
    static constexpr i8 STATUS_FLAG_BRED = 8;        // bit 3: 已繁殖
    static constexpr i8 STATUS_FLAG_EATING = 16;     // bit 4: 正在吃
    static constexpr i8 STATUS_FLAG_REARING = 32;    // bit 5: 正在扬蹄
    static constexpr i8 STATUS_FLAG_MOUTH_OPEN = 64; // bit 6: 嘴张开

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
