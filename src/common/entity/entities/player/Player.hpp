#pragma once

#include "../../core/Entity.hpp"
#include "../../movement/AutoJump.hpp"
#include "../../inventory/PlayerInventory.hpp"
#include "../../experience/ExperienceManager.hpp"
#include "../../effect/EffectInstance.hpp"
#include "../../../network/packet/ProtocolPackets.hpp"
#include <array>
#include <memory>

namespace mc {

// ============================================================================
// 玩家能力标志
// ============================================================================

struct PlayerAbilities {
    bool invulnerable = false;      // 无敌
    bool flying = false;            // 正在飞行
    bool canFly = false;            // 允许飞行
    bool creativeMode = false;      // 创造模式
    bool allowEdit = true;          // 允许编辑方块
    f32 flySpeed = 0.05f;          // 飞行速度 (MC默认: 0.05F)
    f32 walkSpeed = 0.1f;           // 行走速度

    void serialize(network::PacketSerializer& ser) const {
        u8 flags = 0;
        if (invulnerable) flags |= 0x01;
        if (flying) flags |= 0x02;
        if (canFly) flags |= 0x04;
        if (creativeMode) flags |= 0x08;
        ser.writeU8(flags);
        ser.writeF32(flySpeed);
        ser.writeF32(walkSpeed);
    }

    [[nodiscard]] static Result<PlayerAbilities> deserialize(network::PacketDeserializer& deser) {
        PlayerAbilities abilities;

        auto flagsResult = deser.readU8();
        if (flagsResult.failed()) return flagsResult.error();

        u8 flags = flagsResult.value();
        abilities.invulnerable = (flags & 0x01) != 0;
        abilities.flying = (flags & 0x02) != 0;
        abilities.canFly = (flags & 0x04) != 0;
        abilities.creativeMode = (flags & 0x08) != 0;

        auto flySpeedResult = deser.readF32();
        if (flySpeedResult.failed()) return flySpeedResult.error();
        abilities.flySpeed = flySpeedResult.value();

        auto walkSpeedResult = deser.readF32();
        if (walkSpeedResult.failed()) return walkSpeedResult.error();
        abilities.walkSpeed = walkSpeedResult.value();

        return abilities;
    }
};

// ============================================================================
// 饥饿系统
// ============================================================================

struct FoodStats {
    i32 foodLevel = 20;             // 饥饿值 (0-20)
    f32 saturationLevel = 5.0f;     // 饱和度
    f32 exhaustionLevel = 0.0f;     // 消耗累积值
    i32 foodTimer = 0;              // 计时器

    void addExhaustion(f32 amount) {
        exhaustionLevel += amount;
        while (exhaustionLevel >= 4.0f) {
            exhaustionLevel -= 4.0f;
            if (saturationLevel > 0.0f) {
                saturationLevel = std::max(0.0f, saturationLevel - 1.0f);
            } else if (foodLevel > 0) {
                foodLevel--;
            }
        }
    }

    void addStats(i32 food, f32 saturation) {
        foodLevel = std::min(20, foodLevel + food);
        saturationLevel = std::min(20.0f, saturationLevel + saturation);
    }

    bool needsFood() const {
        return foodLevel < 20;
    }

    void serialize(network::PacketSerializer& ser) const {
        ser.writeI32(foodLevel);
        ser.writeF32(saturationLevel);
        ser.writeF32(exhaustionLevel);
    }

    [[nodiscard]] static Result<FoodStats> deserialize(network::PacketDeserializer& deser) {
        FoodStats stats;

        auto foodResult = deser.readI32();
        if (foodResult.failed()) return foodResult.error();
        stats.foodLevel = foodResult.value();

        auto satResult = deser.readF32();
        if (satResult.failed()) return satResult.error();
        stats.saturationLevel = satResult.value();

        auto exhResult = deser.readF32();
        if (exhResult.failed()) return exhResult.error();
        stats.exhaustionLevel = exhResult.value();

        return stats;
    }
};

// ============================================================================
// 玩家类
// ============================================================================

/**
 * @brief 玩家实体类
 *
 * 继承自Entity，添加玩家特有的属性和能力：
 * - 玩家尺寸常量（宽度、高度、眼睛高度）
 * - 游戏模式、生命值、饥饿值
 * - 经验系统
 * - 能力标志（飞行、无敌等）
 * - 物理移动支持（步进、跳跃）
 *
 * 物理系统参考MC Java版实现：
 * - LivingEntity.aiStep() - 主tick循环
 * - LivingEntity.travel() - 物理更新
 * - Entity.move() - 碰撞检测
 */
class Player : public Entity {
public:
    // 玩家尺寸常量
    static constexpr f32 PLAYER_WIDTH = 0.6f;
    static constexpr f32 PLAYER_HEIGHT = 1.8f;
    static constexpr f32 PLAYER_EYE_HEIGHT = 1.62f;
    static constexpr f32 PLAYER_CROUCH_HEIGHT = 1.5f;
    static constexpr f32 PLAYER_SWIM_HEIGHT = 0.6f;
    static constexpr f32 PLAYER_STEP_HEIGHT = 0.6f;  // 步进高度

    // MC物理常量
    static constexpr f32 MOTION_THRESHOLD = 0.003f;  // 速度阈值，低于此值归零
    static constexpr i32 JUMP_COOLDOWN = 10;          // 跳跃冷却(ticks)
    static constexpr f32 SNEAK_EDGE_DISTANCE = 0.05f; // 潜行边缘检测距离

    Player(EntityId id, const String& username);
    ~Player() override;

    // 禁止拷贝
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // 允许移动
    Player(Player&&) = default;
    Player& operator=(Player&&) = default;

    // ========== 玩家特有属性 ==========

    [[nodiscard]] const String& username() const { return m_username; }
    [[nodiscard]] PlayerId playerId() const { return m_playerId; }
    void setPlayerId(PlayerId id) { m_playerId = id; }

    // 游戏模式
    [[nodiscard]] GameMode gameMode() const { return m_gameMode; }
    void setGameMode(GameMode mode);

    // 维度
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dim) { m_dimension = dim; }

    // 生命值和饥饿
    [[nodiscard]] f32 health() const { return m_health; }
    [[nodiscard]] f32 maxHealth() const { return m_maxHealth; }
    void setHealth(f32 health);
    void heal(f32 amount);
    void damage(f32 amount);

    [[nodiscard]] const FoodStats& foodStats() const { return m_foodStats; }
    FoodStats& foodStats() { return m_foodStats; }

    // ========== 经验系统 ==========

    /**
     * @brief 获取经验管理器
     */
    [[nodiscard]] const entity::experience::ExperienceManager& experienceManager() const { return *m_experienceManager; }
    entity::experience::ExperienceManager& experienceManager() { return *m_experienceManager; }

    // 经验相关便捷方法（委托给 ExperienceManager）
    [[nodiscard]] i32 experienceLevel() const { return m_experienceManager->getLevel(); }
    [[nodiscard]] f32 experienceProgress() const { return m_experienceManager->getProgress(); }
    [[nodiscard]] i32 totalExperience() const { return m_experienceManager->getTotalExperience(); }
    [[nodiscard]] i32 xpSeed() const { return m_experienceManager->getXpSeed(); }

    /**
     * @brief 添加经验值
     * @param amount 经验值数量
     */
    virtual void addExperience(i32 amount);

    /**
     * @brief 设置经验等级
     * @param level 目标等级
     */
    virtual void setExperienceLevel(i32 level);

    /**
     * @brief 添加经验等级
     * @param levels 要添加的等级数（可以为负数）
     */
    void addExperienceLevels(i32 levels);

    /**
     * @brief 消耗经验值
     * @param amount 要消耗的经验值
     * @return 是否成功消耗
     */
    [[nodiscard]] bool consumeExperience(i32 amount);

    /**
     * @brief 消耗经验等级（用于附魔）
     * @param levels 要消耗的等级数
     * @return 是否成功消耗
     */
    [[nodiscard]] bool consumeExperienceLevels(i32 levels);

    /**
     * @brief 当前等级填满经验条需要的经验值
     */
    [[nodiscard]] i32 experienceBarCapacity() const;

    /**
     * @brief 设置完整的经验状态
     * @param level 等级
     * @param progress 进度 (0.0-1.0)
     * @param totalExperience 总经验值
     */
    void setExperience(i32 level, f32 progress, i32 totalExperience);

    /**
     * @brief 掉落经验（死亡时调用）
     *
     * 玩家死亡时掉落 min(level * 7, 100) 点经验。
     */
    void dropExperience();

    // ========== XP 冷却 ==========

    /**
     * @brief 获取 XP 冷却时间
     * @return 剩余冷却 ticks
     */
    [[nodiscard]] i32 xpCooldown() const { return m_xpCooldown; }

    /**
     * @brief 设置 XP 冷却时间
     * @param cooldown 冷却 ticks
     */
    void setXpCooldown(i32 cooldown) { m_xpCooldown = cooldown; }

    /**
     * @brief 检查是否可以拾取 XP
     */
    [[nodiscard]] bool canPickupXp() const { return m_xpCooldown <= 0; }

    // 能力
    [[nodiscard]] const PlayerAbilities& abilities() const { return m_abilities; }
    PlayerAbilities& abilities() { return m_abilities; }

    // 状态
    [[nodiscard]] bool isOnGround() const { return m_onGround; }
    [[nodiscard]] bool isSprinting() const { return m_isSprinting; }
    [[nodiscard]] bool isSneaking() const { return m_isSneaking; }
    [[nodiscard]] bool isSwimming() const { return m_isSwimming; }
    [[nodiscard]] bool isSleeping() const { return m_isSleeping; }
    [[nodiscard]] bool isDead() const { return m_health <= 0.0f; }
    [[nodiscard]] bool isJumping() const { return m_isJumping; }

    void setSprinting(bool sprinting);
    void setSneaking(bool sneaking);
    void setSwimming(bool swimming);
    void setSleeping(bool sleeping);

    /**
     * @brief 切换飞行状态
     *
     * 仅当 canFly 为 true 时才能切换。
     * 在飞行和非飞行状态之间切换。
     */
    void toggleFlying();

    // ========== 重写尺寸方法 ==========

    [[nodiscard]] f32 width() const override { return PLAYER_WIDTH; }
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] f32 eyeHeight() const override;
    [[nodiscard]] f32 stepHeight() const override { return PLAYER_STEP_HEIGHT; }

    // ========== 更新 ==========

    void tick() override;
    void update() override;

    /**
     * @brief 处理传送门 tick
     *
     * 玩家需要 80 tick (4秒) 在传送门中才能传送。
     * 传送后设置 300 tick (15秒) 冷却。
     *
     * @return true 如果应该触发传送
     */
    bool tickPortal() override;

    // ========== 物理/移动 ==========

    /**
     * @brief 处理移动输入
     *
     * 根据MC Java版 Entity.getAbsoluteMotion() 的逻辑：
     * - MC坐标系: yaw=0 看向 +Z, yaw=90 看向 -X
     * - forward: 正值向前走, 负值向后走
     * - strafe: 正值向右走, 负值向左走
     *
     * @param forward 前后移动 (-1到1，负为后退)
     * @param strafe 左右移动 (-1到1，负为左)
     * @param jumping 是否跳跃
     * @param sneaking 是否潜行
     */
    void handleMovementInput(f32 forward, f32 strafe, bool jumping, bool sneaking);

    /**
     * @brief 执行跳跃
     *
     * 只有在地面上且跳跃冷却为0时才能跳跃。
     * 跳跃速度为 JUMP_VELOCITY (0.42)。
     * 跳跃后设置冷却为 JUMP_COOLDOWN (10 ticks)。
     */
    void jump();

    /**
     * @brief 更新玩家物理
     *
     * 每帧调用，处理：
     * - 应用速度到位置（带碰撞检测）
     * - 重力
     * - 跳跃
     * - 阻力
     * - 速度阈值处理
     * - 跳跃冷却
     */
    void updatePhysics();

    /**
     * @brief 检查潜行时是否可以移动到边缘
     *
     * 参考MC的 maybeBackOffFromEdge
     * 潜行时防止玩家走到方块边缘掉落
     *
     * @param movement 期望移动向量
     * @return 修正后的移动向量
     */
    [[nodiscard]] Vector3 maybeBackOffFromEdge(const Vector3& movement) const;

    // ========== 网络同步 ==========

    [[nodiscard]] network::PlayerPosition playerPosition() const;

    // ========== 背包 ==========

    /**
     * @brief 获取玩家背包
     */
    [[nodiscard]] const PlayerInventory& inventory() const { return m_inventory; }
    PlayerInventory& inventory() { return m_inventory; }

    /**
     * @brief 获取手持物品
     * @param hand 主手或副手
     * @return 物品堆引用
     */
    [[nodiscard]] ItemStack getHeldItem(Hand hand) const;
    ItemStack& getHeldItem(Hand hand);

    /**
     * @brief 设置创造模式背包
     *
     * 为创造模式玩家添加常见方块到背包。
     * 清空当前背包并填入所有已注册的方块物品。
     */
    void setCreativeModeInventory();

    /**
     * @brief 获取吸收伤害值（金苹果效果）
     */
    [[nodiscard]] f32 absorptionAmount() const { return m_absorptionAmount; }
    void setAbsorptionAmount(f32 amount) { m_absorptionAmount = amount; }

    /**
     * @brief 获取护甲值
     */
    [[nodiscard]] i32 armorValue() const;

    /**
     * @brief 获取跳跃因子
     *
     * 跳跃因子影响玩家能跳多高。正常方块返回 1.0，
     * 蜂蜜块返回 0.5（降低跳跃高度）。
     *
     * @return 跳跃因子（0.0 - 1.0）
     */
    [[nodiscard]] virtual f32 getJumpFactor() const { return 1.0f; }

    /**
     * @brief 获取自动跳跃系统
     */
    [[nodiscard]] entity::movement::AutoJump& autoJump() { return m_autoJump; }
    [[nodiscard]] const entity::movement::AutoJump& autoJump() const { return m_autoJump; }

    // ========== 重生 ==========

    void respawn();

    // ========== 效果系统 ==========
    // Player 不继承 LivingEntity，但需要效果系统来支持不祥之兆等效果

    /**
     * @brief 添加效果
     * @param effect 效果实例
     * @return 是否成功添加
     */
    bool addEffect(const entity::effect::EffectInstance& effect);

    /**
     * @brief 移除效果
     * @param type 效果类型
     */
    void removeEffect(entity::effect::EffectType type);

    /**
     * @brief 移除所有效果
     */
    void removeAllEffects();

    /**
     * @brief 检查是否有效果
     * @param type 效果类型
     */
    [[nodiscard]] bool hasEffect(entity::effect::EffectType type) const;

    /**
     * @brief 获取效果实例
     * @param type 效果类型
     * @return 效果实例指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const entity::effect::EffectInstance* getEffect(entity::effect::EffectType type) const;

    /**
     * @brief 获取所有效果
     */
    [[nodiscard]] const std::vector<entity::effect::EffectInstance>& getAllEffects() const { return m_effects; }

    // ========== 序列化 ==========

    void serialize(network::PacketSerializer& ser) const;
    [[nodiscard]] static Result<std::unique_ptr<Player>> deserialize(network::PacketDeserializer& deser);

private:
    /**
     * @brief 应用移动速度修正
     */
    void applyMovementSpeed(f32& speed, bool sneaking) const;

    /**
     * @brief 重置过小的速度为零
     * 参考MC: if (Math.abs(motion) < 0.003) motion = 0
     */
    void clampMotion();

    String m_username;
    PlayerId m_playerId = 0;
    GameMode m_gameMode = GameMode::Survival;

    f32 m_health = 20.0f;
    f32 m_maxHealth = 20.0f;
    f32 m_absorptionAmount = 0.0f;

    FoodStats m_foodStats;
    PlayerAbilities m_abilities;
    PlayerInventory m_inventory{this};  // 玩家背包

    // 经验管理器（唯一数据源）
    std::unique_ptr<entity::experience::ExperienceManager> m_experienceManager;

    // XP 冷却（拾取经验球的延迟）
    i32 m_xpCooldown = 0;

    bool m_isSprinting = false;
    bool m_isSneaking = false;
    bool m_isSwimming = false;
    bool m_isSleeping = false;
    bool m_isJumping = false;        // 当前帧是否在跳跃

    i32 m_jumpTicks = 0;             // 跳跃冷却
    i32 sleepTimer = 0;
    i32 hurtTime = 0;
    i32 deathTime = 0;

    // 自动跳跃系统
    entity::movement::AutoJump m_autoJump;

    // 效果列表（Player 不继承 LivingEntity，独立管理效果）
    std::vector<entity::effect::EffectInstance> m_effects;
};

} // namespace mc
