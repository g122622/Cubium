/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/passive/tamable/TameableEntity.hpp"
#include "common/entity/interfaces/IEquipable.hpp"
#include "common/entity/interfaces/IJumpingMount.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class Player;
class ItemStack;
class LivingEntity;

/**
 * @brief 鹦鹉螺类实体抽象基类
 *
 * 所有鹦鹉螺类实体（Nautilus、ZombieNautilus）的抽象基类。
 * 实现：可骑乘跳跃、装备栏、水中移动、冲刺、气泡粒子等通用功能。
 *
 * 继承层次：
 * - 继承 TameableEntity 以获得驯服系统（MC 原版 AbstractNautilus 继承 TamableAnimal）
 * - 手动实现水生行为（覆写 canBreatheUnderwater、maxAir、updateAirSupply、getPathWeight）
 *   原因：TameableEntity 继承 AnimalEntity 而非 WaterMobEntity，两者父类不共享
 * - 实现 IJumpingMount 接口（冲刺跳跃）
 * - 实现 IEquipable 接口（鞍 + 鹦鹉螺铠甲）
 *
 * 装备槽位映射（与 MC 1.21.11 EquipmentSlot 一致）：
 * - IEquipable slot 0 → EquipmentSlot::Saddle（鞍）
 * - IEquipable slot 1 → EquipmentSlot::Body（鹦鹉螺铠甲）
 *
 * 骑乘效果：
 * - 玩家骑乘时获得水下呼吸效果（对应 MC 原版 BREATH_OF_THE_NAUTILUS 效果）
 *   项目未实现 BREATH_OF_THE_NAUTILUS 效果，简化为使用 WaterBreathing 效果
 *
 * 冲刺系统：
 * - 玩家骑乘时按跳跃键触发冲刺
 * - 冷却时间 40 tick
 * - 水中冲刺力度 1.2，陆地冲刺力度 0.5
 */
class AbstractNautilusEntity : public TameableEntity, public entity::IJumpingMount, public entity::IEquipable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    explicit AbstractNautilusEntity(EntityId id);

    ~AbstractNautilusEntity() override = default;

    // 禁止拷贝
    AbstractNautilusEntity(const AbstractNautilusEntity&) = delete;
    AbstractNautilusEntity& operator=(const AbstractNautilusEntity&) = delete;

    // 禁止移动
    AbstractNautilusEntity(AbstractNautilusEntity&&) = delete;
    AbstractNautilusEntity& operator=(AbstractNautilusEntity&&) = delete;

    // ========== IJumpingMount 接口实现 ==========

    void onJump() override;
    [[nodiscard]] i32 getJumpPower() const override { return m_dashCooldown; }
    void setJumpPower(i32 power) override;
    [[nodiscard]] f32 getMaxJumpHeight() const override;
    [[nodiscard]] bool canJump() const override;
    void startJumping(i32 jumpPower) override;
    void stopJumping() override;

    // ========== IEquipable 接口实现 ==========
    // 注意：IEquipable 使用 i32 槽位索引，而 LivingEntity 使用 EquipmentSlot 枚举。
    // 两套方法签名不同（参数类型不同），可以共存。下面的 using 声明用于避免
    // i32 版本隐藏 LivingEntity 中的 EquipmentSlot 版本。

    using LivingEntity::getEquipment;
    using LivingEntity::setEquipment;

    [[nodiscard]] i32 getEquipmentSlotCount() const override { return 2; }
    [[nodiscard]] ItemStack getEquipment(i32 slot) const override;
    void setEquipment(i32 slot, const ItemStack& item) override;
    [[nodiscard]] bool canEquip(const ItemStack& item, i32 slot) const override;

    // ========== 装备查询（基于 EquipmentSlot） ==========

    /**
     * @brief 检查是否装备了鞍
     *
     * 对应 MC 1.21.11 AbstractNautilus.isSaddled()：
     * 通过 EquipmentSlot::SADDLE 槽位是否有物品判断
     */
    [[nodiscard]] bool isSaddled() const;

    /**
     * @brief 检查是否可以装备鞍
     *
     * 对应 MC 1.21.11 AbstractNautilus.canUseSlot(SADDLE)：
     * 实体存活、非幼年、已驯服时才能装备鞍
     */
    [[nodiscard]] virtual bool canEquipSaddle() const;

    /**
     * @brief 检查是否可以装备身体护甲
     *
     * 对应 MC 1.21.11 AbstractNautilus.canUseSlot(BODY)：
     * 实体存活、非幼年、已驯服时才能装备鹦鹉螺铠甲
     */
    [[nodiscard]] virtual bool canEquipBodyArmor() const;

    // ========== 骑乘系统 ==========

    /**
     * @brief 让玩家骑乘鹦鹉螺
     *
     * 对应 MC 1.21.11 AbstractNautilus.doPlayerRide()
     */
    void doPlayerRide(Player& player);

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

    // ========== 冲刺系统 ==========

    /**
     * @brief 检查是否正在冲刺
     */
    [[nodiscard]] bool isDashing() const { return m_dataManager.get<bool>(DATA_DASH_PARAM); }

    /**
     * @brief 设置冲刺状态
     */
    void setDashing(bool dashing);

    /**
     * @brief 获取冲刺冷却剩余时间（ticks）
     */
    [[nodiscard]] i32 getDashCooldown() const { return m_dashCooldown; }

    /**
     * @brief 获取冲刺音效
     *
     * 子类应重写提供具体音效
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getDashSound() const { return std::nullopt; }

    /**
     * @brief 获取冲刺就绪音效
     *
     * 子类应重写提供具体音效
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getDashReadySound() const { return std::nullopt; }

    // ========== 水生行为 ==========

    /**
     * @brief 是否可以在水下呼吸
     *
     * 鹦鹉螺是水生动物，可以在水下呼吸
     */
    [[nodiscard]] bool canBreatheUnderwater() const override { return true; }

    /**
     * @brief 获取最大空气供应量
     *
     * 鹦鹉螺最大空气 300 tick（15秒）
     */
    [[nodiscard]] i32 maxAir() const override { return MAX_AIR_SUPPLY; }

    /**
     * @brief 是否会被水流推动
     *
     * 对应 MC 1.21.11 AbstractNautilus.isPushedByFluid()：返回 false
     *
     * 注意：项目基类 TameableEntity 继承自 AnimalEntity 而非 WaterMobEntity，
     * 因此基类中没有 canBePushedByWater() 虚函数可覆写。
     * 这里直接提供同名方法供本类及调用方使用（非覆写）。
     * 若水流推动系统需要查询该接口，应通过 dynamic_cast 检查。
     */
    [[nodiscard]] bool canBePushedByWater() const { return false; }

    /**
     * @brief 获取路径权重
     *
     * 对应 MC 1.21.11 AbstractNautilus.getWalkTargetValue()：返回 0.0f
     * 鹦鹉螺不通过标准路径权重系统选择位置
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 音效 ==========

    /**
     * @brief 获取进食音效
     *
     * 子类应重写提供具体音效
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getEatSound() const { return std::nullopt; }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 骑乘移动处理
     *
     * 实现骑乘时的移动逻辑，包括水中和陆地速度差异
     */
    void travel(f32 strafing, f32 vertical, f32 forward) override;

    /**
     * @brief 玩家交互
     *
     * 对应 MC 1.21.11 AbstractNautilus.mobInteract()：
     * 1. 幼年 → 交给基类
     * 2. 已驯服 + Shift → 打开背包界面
     * 3. 未驯服 + 食物 → 尝试驯服（1/3 概率）
     * 4. 已驯服 + 食物 + 未满血 → 喂食治疗
     * 5. 物品交互（如鞍、鹦鹉螺铠甲）
     * 6. 已驯服 + 非Shift + 非食物 → 骑乘
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    /**
     * @brief 打开鹦鹉螺背包界面
     *
     * 条件：服务端 && (无骑乘者 || 骑乘者是自身) && 已驯服
     */
    virtual void openInventory(Player& player);

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    /**
     * @brief 注册同步数据参数
     *
     * 注册 DATA_DASH_PARAM（冲刺状态）到 EntityDataManager。
     * 由于 C++ 虚函数在基类构造函数中不会派发到派生类，
     * 派生类构造函数必须显式调用 registerData()，参考 WolfEntity 模式。
     */
    void registerData() override;

    /**
     * @brief 注册 AI 目标
     *
     * 注册鹦鹉螺类实体的通用 AI 目标：
     * - 0: SwimGoal（水中上浮）
     * - 0: FindWaterGoal（寻找水源）
     * - 1: PanicGoal（恐慌逃跑）
     * - 2: BreedGoal（繁殖，仅 NautilusEntity）
     * - 3: TemptGoal（跟随食物）
     * - 4: MeleeAttackGoal（近战攻击）
     * - 5: RandomSwimmingGoal（随机游泳）
     * - 6: LookAtGoal（看向玩家）
     * - 7: LookRandomlyGoal（随机看向）
     */
    void registerGoals() override;

    /**
     * @brief 注册属性
     *
     * 鹦鹉螺基础属性：
     * - MAX_HEALTH: 15.0
     * - MOVEMENT_SPEED: 1.0
     * - ATTACK_DAMAGE: 3.0
     * - KNOCKBACK_RESISTANCE: 0.3
     */
    void registerAttributes() override;

    /**
     * @brief 创建物品栏
     *
     * 对应 MC 1.21.11 AbstractNautilus.createInventory()
     */
    void createInventory();

    /**
     * @brief 应用骑乘效果
     *
     * 给骑乘者添加水下呼吸效果
     * 对应 MC 1.21.11 AbstractNautilus.applyEffects()
     */
    void applyRiderEffects();

    /**
     * @brief 执行骑乘者跳跃（冲刺）
     *
     * 对应 MC 1.21.11 AbstractNautilus.executeRidersJump()
     *
     * @param jumpScale 跳跃力度比例 (0.0-1.0)
     * @param rider 骑乘者
     */
    virtual void executeRidersJump(f32 jumpScale, Player& rider);

    /**
     * @brief 获取水中骑乘速度修饰符
     *
     * 对应 MC 1.21.11 AbstractNautilus.getRiddenSpeed()
     * - 水中：0.0325 * MOVEMENT_SPEED
     * - 陆地：0.02 * MOVEMENT_SPEED
     */
    [[nodiscard]] f32 getRiddenSpeed() const;

    /**
     * @brief 尝试驯服
     *
     * 对应 MC 1.21.11 AbstractNautilus.tryToTame()
     * 1/3 概率驯服成功，播放爱心粒子；否则播放烟雾粒子
     */
    void tryToTame(Player& player);

    /**
     * @brief 检查物品是否为鹦鹉螺驯服物品
     *
     * 对应 MC 1.21.11 ItemTags.NAUTILUS_TAMING_ITEMS
     * 子类可重写以定义特定驯服物品
     */
    [[nodiscard]] virtual bool isTamingItem(const ItemStack& itemStack) const;

    /**
     * @brief 检查物品是否为鹦鹉螺食物
     *
     * 对应 MC 1.21.11 ItemTags.NAUTILUS_FOOD
     * 子类可重写以定义特定食物
     */
    [[nodiscard]] virtual bool isNautilusFood(const ItemStack& itemStack) const;

    /**
     * @brief 检查物品是否为鹦鹉螺铠甲
     *
     * 默认检查物品是否为各种鹦鹉螺铠甲
     */
    [[nodiscard]] virtual bool isNautilusArmor(const ItemStack& itemStack) const;

    /**
     * @brief 获取装备音效
     *
     * 对应 MC 1.21.11 AbstractNautilus.getEquipSound()
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getEquipSound(EquipmentSlot slot) const;

    /**
     * @brief 更新冲刺冷却
     */
    void updateDashCooldown();

    /**
     * @brief 在水中生成气泡粒子
     *
     * 对应 MC 1.21.11 AbstractNautilus.spawnBubbles()
     */
    void spawnBubbles();

    // ========== 成员变量 ==========

    /// 骑乘者指针（缓存，避免每次查找）
    Player* m_rider = nullptr;

    /// 物品栏（鞍槽 + 鹦鹉螺铠甲槽）
    std::unique_ptr<blockentity::SimpleInventory> m_inventory;

    /// 冲刺冷却剩余时间（ticks）
    i32 m_dashCooldown = 0;

    /// 玩家跳跃蓄力比例 (0.0-1.0)
    f32 m_playerJumpPendingScale = 0.0f;

private:
    // ========== 数据同步参数 ==========

    /// 冲刺状态同步参数
    /// 对应 MC 1.21.11 AbstractNautilus.DASH
    static entity::DataParameter<bool> DATA_DASH_PARAM;

    // ========== 常量 ==========

    /// 最大空气供应量（ticks）
    static constexpr i32 MAX_AIR_SUPPLY = 300;

    /// 冲刺冷却时间（ticks）
    static constexpr i32 DASH_COOLDOWN_TICKS = 40;

    /// 冲刺最小持续时间（ticks）
    static constexpr i32 DASH_MINIMUM_DURATION_TICKS = 5;

    /// 水中冲刺力度
    static constexpr f32 DASH_MOMENTUM_IN_WATER = 1.2f;

    /// 陆地冲刺力度
    static constexpr f32 DASH_MOMENTUM_ON_LAND = 0.5f;

    /// 水中骑乘速度修饰符
    static constexpr f32 RIDDEN_SPEED_MODIFIER_IN_WATER = 0.0325f;

    /// 陆地骑乘速度修饰符
    static constexpr f32 RIDDEN_SPEED_MODIFIER_ON_LAND = 0.02f;

    /// 水中速度修饰符
    static constexpr f32 IN_WATER_SPEED_MODIFIER = 0.011f;

    /// 水阻力
    static constexpr f32 WATER_RESISTANCE = 0.9f;

    /// 骑乘者效果持续时间（ticks）
    static constexpr i32 EFFECT_DURATION = 60;

    /// 骑乘者效果刷新间隔（ticks）
    static constexpr i32 EFFECT_REFRESH_RATE = 40;

    /// 驯服概率分母（1/N 概率驯服成功）
    static constexpr i32 TAME_PROBABILITY_DENOMINATOR = 3;
};

} // namespace mc
