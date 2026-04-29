#pragma once

#include "../../core/Entity.hpp"
#include "../../movement/AutoJump.hpp"
#include "../../inventory/PlayerInventory.hpp"
#include "../../experience/ExperienceManager.hpp"
#include "../../effect/EffectInstance.hpp"
#include "../../food/FoodStats.hpp"
#include "../../../network/packet/ProtocolPackets.hpp"
#include "../../../world/GlobalPos.hpp"
#include "ChatVisibility.hpp"
#include "PlayerModelPart.hpp"
#include "common/world/block/BlockPos.hpp"
#include "spdlog/spdlog.h"

#include <array>
#include <memory>
#include <optional>

namespace mc {

class AbstractContainerMenu;

// ============================================================================
// 玩家能力标志
// ============================================================================

struct PlayerAbilities {
    bool invulnerable = false;      // 无敌
    bool flying = false;            // 正在飞行
    bool canFly = false;            // 允许飞行
    bool creativeMode = false;      // 创造模式
    bool allowEdit = true;          // 允许编辑方块
    // src\common\entity\entities\player\GameModeUtils.cpp 才是真正设置这些速度的来源
    f32 flySpeed = 0;          // 飞行速度 
    f32 walkSpeed = 0;           // 行走速度

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
        spdlog::info("[PlayerAbilities::deserialize] invulnerable={}, flying={}, canFly={}, creativeMode={}, flySpeed={}, walkSpeed={}",
                     abilities.invulnerable, abilities.flying, abilities.canFly, abilities.creativeMode,
                     abilities.flySpeed, abilities.walkSpeed);

        return abilities;
    }
};

// FoodStats 类已移至 food/FoodStats.hpp

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

    [[nodiscard]] ChatVisibility chatVisibility() const { return m_chatVisibility; }
    void setChatVisibility(ChatVisibility visibility) { m_chatVisibility = visibility; }

    [[nodiscard]] u8 playerModelParts() const { return m_playerModelParts; }
    void setPlayerModelParts(u8 modelPartsMask) { m_playerModelParts = modelPartsMask; }

    [[nodiscard]] bool isWearing(PlayerModelPart part) const
    {
        return (m_playerModelParts & getPlayerModelPartMask(part)) != 0;
    }

    void setModelPartEnabled(PlayerModelPart part, bool enabled)
    {
        if (enabled) {
            m_playerModelParts = static_cast<u8>(m_playerModelParts | getPlayerModelPartMask(part));
            return;
        }

        m_playerModelParts = static_cast<u8>(m_playerModelParts & ~getPlayerModelPartMask(part));
    }

    // 游戏模式
    [[nodiscard]] GameMode gameMode() const { return m_gameMode; }
    void setGameMode(GameMode mode);

    // 维度
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dim) { m_dimension = dim; }

    [[nodiscard]] sound::SoundCategory getSoundCategory() const override { return sound::SoundCategory::Players; }

    /**
     * @brief 设置位置并重置步距采样
     */
    void setPosition(f32 x, f32 y, f32 z);
    void setPosition(const Vector3& pos) { setPosition(pos.x, pos.y, pos.z); }

    // 生命值和饥饿
    [[nodiscard]] f32 health() const { return m_health; }
    [[nodiscard]] f32 maxHealth() const { return m_maxHealth; }
    void setHealth(f32 health);
    void heal(f32 amount);
    void damage(f32 amount);

    [[nodiscard]] const FoodStats& foodStats() const { return m_foodStats; }
    FoodStats& foodStats() { return m_foodStats; }

    // ========== 饥饿消耗 ==========

    /**
     * @brief 饥饿消耗常量
     * 参考 MC 1.16.5 PlayerEntity 和 FoodStats
     */
    /// 疾跑每米消耗
    static constexpr f32 EXHAUSTION_SPRINT_PER_METER = 0.1f;
    /// 普通跳跃消耗
    static constexpr f32 EXHAUSTION_JUMP = 0.05f;
    /// 疾跑跳跃消耗
    static constexpr f32 EXHAUSTION_SPRINT_JUMP = 0.2f;
    /// 游泳每米消耗
    static constexpr f32 EXHAUSTION_SWIM_PER_METER = 0.01f;
    /// 水下行走每米消耗
    static constexpr f32 EXHAUSTION_UNDERWATER_WALK_PER_METER = 0.01f;
    /// 水面行走每米消耗
    static constexpr f32 EXHAUSTION_WATER_WALK_PER_METER = 0.01f;
    /// 攻击实体消耗
    static constexpr f32 EXHAUSTION_ATTACK = 0.1f;
    /// 受到伤害消耗（基础值，根据伤害源可能不同）
    static constexpr f32 EXHAUSTION_DAMAGE = 0.1f;
    /// 挖掘方块消耗（每 tick）
    static constexpr f32 EXHAUSTION_MINE_PER_TICK = 0.005f;

    /**
     * @brief 添加饥饿消耗值
     * @param exhaustion 消耗值
     * @note 只有生存模式和冒险模式才会消耗
     */
    void addExhaustion(f32 exhaustion);

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
    [[nodiscard]] bool isSneaking() const override { return m_isSneaking; }
    [[nodiscard]] bool isSwimming() const { return m_isSwimming; }
    [[nodiscard]] bool isSleeping() const { return m_isSleeping; }
    [[nodiscard]] bool isDead() const { return m_health <= 0.0f; }
    [[nodiscard]] bool isJumping() const { return m_isJumping; }

    void setSprinting(bool sprinting);
    void setSneaking(bool sneaking);
    void setSwimming(bool swimming);
    void setSleeping(bool sleeping);

    // ========== 睡眠系统 ==========

    /**
     * @brief 获取当前睡眠位置
     * @return 床位位置，如果不在睡眠则返回空
     */
    [[nodiscard]] std::optional<BlockPos> getSleepingPosition() const { return m_sleepingPosition; }

    /**
     * @brief 设置睡眠位置
     * @param pos 床位位置
     */
    void setSleepingPosition(const BlockPos& pos) { m_sleepingPosition = pos; }

    /**
     * @brief 清除睡眠位置
     */
    void clearSleepingPosition() { m_sleepingPosition = std::nullopt; }

    /**
     * @brief 检查玩家是否完全入睡
     *
     * 玩家需要睡眠 100 ticks (5秒) 才算完全入睡。
     * 只有完全入睡的玩家才计入夜间跳过计数。
     *
     * @return true 如果睡眠计时器 >= 100
     */
    [[nodiscard]] bool isPlayerFullyAsleep() const { return m_isSleeping && sleepTimer >= 100; }

    /**
     * @brief 获取睡眠计时器
     * @return 睡眠计时器值 (0-100 完全入睡后保持)
     */
    [[nodiscard]] i32 getSleepTimer() const { return sleepTimer; }

    /**
     * @brief 设置睡眠计时器
     * @param value 计时器值
     */
    void setSleepTimer(i32 value) { sleepTimer = value; }

    /**
     * @brief 开始睡眠
     *
     * 设置睡眠状态和位置，切换到睡眠姿态。
     *
     * @param pos 床位位置
     */
    void startSleeping(const BlockPos& pos);

    /**
     * @brief 停止睡眠
     *
     * 清除睡眠状态和位置，切换到站立姿态。
     */
    void stopSleeping();

    // ========== 重生点系统 ==========

    /**
     * @brief 获取重生点
     * @return 重生点位置（维度+方块位置），如果未设置返回空
     */
    [[nodiscard]] std::optional<GlobalPos> getSpawnPoint() const { return m_spawnPoint; }

    /**
     * @brief 设置重生点
     *
     * @param dimension 维度ID
     * @param pos 重生点位置
     * @param forced 是否强制重生点（指南针指向该点）
     */
    void setSpawnPoint(DimensionId dimension, const BlockPos& pos, bool forced = false);

    /**
     * @brief 清除重生点
     */
    void clearSpawnPoint() { m_spawnPoint = std::nullopt; }

    /**
     * @brief 检查重生点是否强制
     * @return true 如果重生点被强制设置
     */
    [[nodiscard]] bool isSpawnForced() const { return m_spawnForced; }

    /**
     * @brief 切换飞行状态
     *
     * 仅当 canFly 为 true 时才能切换。
     * 在飞行和非飞行状态之间切换。
     */
    void toggleFlying();

    // ========== 重写尺寸方法 ==========

    [[nodiscard]] f32 width() const override { return PLAYER_WIDTH; }
    /**
     * @brief 获取指定姿态下的玩家尺寸
     * @param pose 目标姿态
     * @return 对应姿态的尺寸信息
     */
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;
    [[nodiscard]] f32 height() const override;
    [[nodiscard]] f32 eyeHeight() const override;
    [[nodiscard]] f32 stepHeight() const override { return PLAYER_STEP_HEIGHT; }

    // ========== 水中物理和游泳 ==========

    /**
     * @brief 检查是否正在游泳
     *
     * 游泳条件：在水中且不站在地面上且向前移动
     */
    [[nodiscard]] bool isActualSwimming() const;

    /**
     * @brief 更新游泳状态
     *
     * 检测游泳条件并更新姿态
     */
    void updateSwimming();

    /**
     * @brief 自动更新姿态
     *
     * 参考 MC 1.16.5 PlayerEntity.updatePose()
     * 每帧根据当前状态自动判断正确姿态：
     * - 鞘翅飞行 -> FALL_FLYING
     * - 睡眠 -> SLEEPING
     * - 游泳 -> SWIMMING
     * - 三叉戟激流攻击 -> SPIN_ATTACK
     * - 潜行（非飞行） -> CROUCHING
     * - 默认 -> STANDING
     *
     * 如果目标姿态无法容纳，会尝试 CROUCHING 或 SWIMMING 作为后备。
     */
    void updatePose();

    /**
     * @brief 获取游泳动画进度
     * @return 0.0-1.0 之间的插值
     */
    [[nodiscard]] f32 swimAnimation() const { return m_swimAnimation; }
    [[nodiscard]] f32 prevSwimAnimation() const { return m_prevSwimAnimation; }

    /**
     * @brief 处理水中跳跃（向上游泳）
     */
    void swimUp();

    /**
     * @brief 获取深度守卫附魔等级
     * @return 深度守卫等级 (0-3)
     *
     * 检查玩家靴子上的深度守卫附魔等级。
     * 参考 MC 1.16.5 EnchantmentHelper.getDepthStriderModifier()
     */
    [[nodiscard]] i32 getDepthStriderLevel() const;

    /**
     * @brief 更新空气供应和溺水
     */
    void updateAirSupply();

    /**
     * @brief 更新移动距离（用于视野晃动和脚步声）
     */
    void updateMoveDistance();

    /**
     * @brief 播放脚步声
     *
     * 在行走距离累计超过阈值时触发。
     * 根据脚下方块类型选择不同的脚步声。
     */
    void playStepSound();

    /**
     * @brief 播放游泳声
     *
     * 在水中游泳时触发。
     * @param volume 音量（0.0-1.0）
     */
    void playSwimSound(f32 volume);

    /**
     * @brief 检查是否应该播放脚步声
     */
    [[nodiscard]] bool shouldPlayStepSound() const { return m_shouldPlayStepSound; }

    /**
     * @brief 检查是否应该播放游泳声
     */
    [[nodiscard]] bool shouldPlaySwimSound() const { return m_shouldPlaySwimSound; }

    /**
     * @brief 获取游泳声音量
     */
    [[nodiscard]] f32 swimSoundVolume() const { return m_swimSoundVolume; }

    /**
     * @brief 获取上一tick是否在水中（用于检测入水/出水）
     */
    [[nodiscard]] bool wasInWater() const { return m_wasInWater; }

    /**
     * @brief 获取脚步声位置
     */
    [[nodiscard]] BlockPos stepSoundPos() const { return m_stepSoundPos; }

    /**
     * @brief 获取空气供应量
     */
    [[nodiscard]] i32 airSupply() const { return m_airSupply; }

    /**
     * @brief 设置空气供应量
     */
    void setAirSupply(i32 air) { m_airSupply = air; }

    // ========== 受伤/死亡 ==========

    /**
     * @brief 获取受伤时间（无敌帧）
     */
    [[nodiscard]] i32 hurtTime() const { return m_hurtTime; }

    /**
     * @brief 获取死亡时间
     */
    [[nodiscard]] i32 deathTime() const { return m_deathTime; }

    // ========== 视野晃动 ==========

    /**
     * @brief 获取行走距离累计（用于视野晃动）
     */
    [[nodiscard]] f32 moveDistanceWalked() const { return m_moveDistanceWalked; }

    /**
     * @brief 获取游泳距离累计
     */
    [[nodiscard]] f32 moveDistanceSwam() const { return m_moveDistanceSwam; }

    /**
     * @brief 获取上一tick的行走距离
     */
    [[nodiscard]] f32 prevMoveDistanceWalked() const { return m_prevMoveDistanceWalked; }

    /**
     * @brief 获取上一tick的游泳距离
     */
    [[nodiscard]] f32 prevMoveDistanceSwam() const { return m_prevMoveDistanceSwam; }

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
     * @brief 获取当前打开的容器菜单
     * @return 当前打开的菜单指针，如果没有打开容器则返回 nullptr
     */
    [[nodiscard]] AbstractContainerMenu* openContainerMenu() { return m_openContainerMenu; }
    [[nodiscard]] const AbstractContainerMenu* openContainerMenu() const { return m_openContainerMenu; }

    /**
     * @brief 设置当前打开的容器菜单
     * @param menu 容器菜单指针
     */
    void setOpenContainerMenu(AbstractContainerMenu* menu) { m_openContainerMenu = menu; }

    /**
     * @brief 清空当前打开的容器菜单
     */
    void clearOpenContainerMenu() { m_openContainerMenu = nullptr; }

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
     * @brief 处理水中移动
     *
     * 参考MC LivingEntity.travel() 水中分支
     */
    void handleWaterMovement(f32 forward, f32 strafe, bool jumping, bool sneaking);

    /**
     * @brief 处理岩浆中移动
     */
    void handleLavaMovement(f32 forward, f32 strafe, bool jumping, bool sneaking);

    /**
     * @brief 应用移动速度修正
     */
    void applyMovementSpeed(f32& speed, bool sneaking) const;

    /**
     * @brief 重置过小的速度为零
     * 参考MC: if (Math.abs(motion) < 0.003) motion = 0
     */
    void clampMotion();

    /**
     * @brief 检查玩家是否能以指定姿态容纳在当前位置
     * @param pose 目标姿态
     * @return 如果当前位置没有阻挡则返回 true
     */
    [[nodiscard]] bool canFitPose(EntityPose pose) const;

    String m_username;
    PlayerId m_playerId = 0;
    GameMode m_gameMode = GameMode::Survival;
    ChatVisibility m_chatVisibility = ChatVisibility::Full;
    u8 m_playerModelParts = PLAYER_MODEL_PARTS_ALL_MASK;

    f32 m_health = 20.0f;
    f32 m_maxHealth = 20.0f;
    f32 m_absorptionAmount = 0.0f;

    FoodStats m_foodStats;
    PlayerAbilities m_abilities;
    PlayerInventory m_inventory{this};  // 玩家背包
    AbstractContainerMenu* m_openContainerMenu = nullptr;

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
    i32 m_hurtTime = 0;
    i32 m_deathTime = 0;

    // 睡眠位置（当前睡眠的床位）
    std::optional<BlockPos> m_sleepingPosition;

    // 重生点（床或重生锚设置的位置）
    std::optional<GlobalPos> m_spawnPoint;
    bool m_spawnForced = false;      // 是否强制重生点

    // 自动跳跃系统
    entity::movement::AutoJump m_autoJump;

    // 效果列表（Player 不继承 LivingEntity，独立管理效果）
    std::vector<entity::effect::EffectInstance> m_effects;

    // 游泳动画
    f32 m_swimAnimation = 0.0f;
    f32 m_prevSwimAnimation = 0.0f;

    // 空气供应和溺水
    i32 m_airSupply = 300;              // 当前空气量（15秒）
    i32 m_drownDamageTimer = 0;         // 溺水伤害计时器
    bool m_wasInWater = false;          // 上一tick是否在水中（用于检测入水/出水）

    // 视野晃动
    Vector3 m_moveDistanceSamplePosition{0.0f, 0.0f, 0.0f}; // 上次步距采样位置
    f32 m_moveDistanceWalked = 0.0f;    // 行走距离累计（用于视野晃动）
    f32 m_prevMoveDistanceWalked = 0.0f; // 上一帧行走距离
    f32 m_moveDistanceSwam = 0.0f;      // 游泳距离累计
    f32 m_prevMoveDistanceSwam = 0.0f;  // 上一帧游泳距离

    // 脚步声触发
    f32 m_distanceWalkedOnStep = 0.0f;  // 用于触发脚步声的行走距离
    f32 m_nextStepDistance = 1.0f;      // 下一次脚步声触发的距离阈值

    // 脚步声/游泳声状态（供客户端读取）
    bool m_shouldPlayStepSound = false;    // 是否应该播放脚步声
    bool m_shouldPlaySwimSound = false;    // 是否应该播放游泳声
    f32 m_swimSoundVolume = 0.0f;          // 游泳声音量
    BlockPos m_stepSoundPos;               // 脚步声位置
};

} // namespace mc
