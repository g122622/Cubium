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

#include "../../../item/core/ActionResult.hpp"
#include "../../../physics/PhysicsConstants.hpp"
#include "../../../resource/ResourceLocation.hpp"
#include "../../../world/GlobalPos.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../effect/EffectInstance.hpp"
#include "../../experience/ExperienceManager.hpp"
#include "../../food/FoodStats.hpp"
#include "../../inventory/PlayerEnderChestInventory.hpp"
#include "../../inventory/PlayerInventory.hpp"
#include "../../movement/AutoJump.hpp"
#include "../../player/CooldownTracker.hpp"
#include "../../player/SleepResult.hpp"
#include "../misc/MiscEntities.hpp"
#include "ChatVisibility.hpp"
#include "GameModeUtils.hpp"
#include "PlayerModelPart.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/ecs/components/PlayerScoreComponent.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/UuidUtils.hpp" // for mc::Uuid + util::uuidFromString
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace mc {

// Forward declaration
class ItemStack;
class INamedContainerProvider;

class AbstractContainerMenu;
class ItemEntity;
class DamageSource;

enum class Direction : u8;

namespace scoreboard {
class Scoreboard;
}

// ============================================================================
// 玩家能力标志
// ============================================================================

struct PlayerAbilities {
    bool invulnerable = false; // 无敌
    bool flying = false;       // 正在飞行
    bool canFly = false;       // 允许飞行
    bool creativeMode = false; // 创造模式
    bool allowEdit = true;     // 允许编辑方块
    f32 flySpeed = 0.05f;      // 飞行速度
    f32 walkSpeed = 0.1f;      // 行走速度
};

// FoodStats 类已移至 food/FoodStats.hpp

// ============================================================================
// 玩家类
// ============================================================================

/**
 * @brief 玩家实体类
 *
 * 继承自 LivingEntity，添加玩家特有的属性和能力：
 * - 玩家尺寸常量（宽度、高度、眼睛高度）
 * - 游戏模式、饥饿值
 * - 经验系统
 * - 能力标志（飞行、无敌等）
 * - 物理移动支持（步进、跳跃）
 */
class Player : public LivingEntity {
public:
    // 玩家尺寸常量
    static constexpr f32 PLAYER_WIDTH = 0.6f;
    static constexpr f32 PLAYER_HEIGHT = 1.8f;
    static constexpr f32 PLAYER_EYE_HEIGHT = 1.62f;
    static constexpr f32 PLAYER_CROUCH_HEIGHT = 1.5f;
    static constexpr f32 PLAYER_SWIM_HEIGHT = 0.6f;

    // 物理常量
    // 注意：PLAYER_STEP_HEIGHT 已移至 physics::STEP_HEIGHT (PhysicsConstants.hpp)
    // 注意：MOTION_THRESHOLD 已移至 physics::MOTION_THRESHOLD (PhysicsConstants.hpp)
    static constexpr i32 JUMP_COOLDOWN = 10;          // 跳跃冷却(ticks)
    static constexpr f32 SNEAK_EDGE_DISTANCE = 0.05f; // 潜行边缘检测距离

    Player(EntityInstanceId id, const std::string& username, ecs::EntityRegistry& registry);
    ~Player() override;

    // 同步数据参数注册（对齐 vanilla 1.21.11 Player/Avatar.defineId）。
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类，Player 构造函数必须显式调用，
    // 参考 MobEntity 模式。
    void registerData() override;

    // 禁止拷贝
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // 禁止移动（基类 Entity 不可移动）
    Player(Player&&) = delete;
    Player& operator=(Player&&) = delete;

    // ========== 玩家特有属性 ==========

    [[nodiscard]] const std::string& username() const { return m_username; }
    [[nodiscard]] PlayerId playerId() const { return m_playerId; }
    void setPlayerId(PlayerId id) { m_playerId = id; }

    // 玩家的 16 字节 profile UUID（用于 TameableEntity::getOwner 按 UUID 匹配主人，
    // 对齐 vanilla TamableAnimal.getOwnerReference().resolve()）。
    // 由 Entity::uuid()（string）解析得到；uuid() 在 createPlayerForConnection 回填为
    // 离线 profile UUID（util::generateOfflineUuid），故此处即玩家稳定身份标识。
    [[nodiscard]] Uuid uuidBytes() const { return util::uuidFromString(uuid()); }

    [[nodiscard]] ChatVisibility chatVisibility() const { return m_chatVisibility; }
    void setChatVisibility(ChatVisibility visibility) { m_chatVisibility = visibility; }

    [[nodiscard]] u8 playerModelParts() const { return m_playerModelParts; }
    void setPlayerModelParts(u8 modelPartsMask)
    {
        m_playerModelParts = modelPartsMask;
        // 同步到 vanilla DATA_PLAYER_MODE_CUSTOMISATION（set_entity_data 经此下发客户端）
        m_dataManager.set(DATA_PLAYER_MODE_CUSTOMISATION_PARAM, static_cast<i8>(modelPartsMask));
    }

    [[nodiscard]] bool isWearing(PlayerModelPart part) const
    {
        return (m_playerModelParts & getPlayerModelPartMask(part)) != 0;
    }

    void setModelPartEnabled(PlayerModelPart part, bool enabled)
    {
        if (enabled) {
            m_playerModelParts = static_cast<u8>(m_playerModelParts | getPlayerModelPartMask(part));
        } else {
            m_playerModelParts = static_cast<u8>(m_playerModelParts & ~getPlayerModelPartMask(part));
        }
        // 同步到 vanilla DATA_PLAYER_MODE_CUSTOMISATION
        m_dataManager.set(DATA_PLAYER_MODE_CUSTOMISATION_PARAM, static_cast<i8>(m_playerModelParts));
    }

    // 游戏模式
    [[nodiscard]] GameMode gameMode() const { return m_gameMode; }
    void setGameMode(GameMode mode);

    /**
     * @brief 检查是否是创造模式
     */
    [[nodiscard]] bool isCreative() const { return entity::GameModeUtils::isCreative(m_gameMode); }

    /**
     * @brief 检查是否是观察者模式
     *
     * 重写 Entity::isSpectator()，返回实际的旁观者模式状态。
     */
    [[nodiscard]] bool isSpectator() const override { return entity::GameModeUtils::isSpectator(m_gameMode); }

    /**
     * @brief 弹射物是否可命中此玩家
     *
     * 对应 MC Java Player.canBeHitByProjectile()。
     * 旁观者模式的玩家不可被弹射物命中。
     */
    [[nodiscard]] bool canBeHitByProjectile() const override
    {
        return !isSpectator() && Entity::canBeHitByProjectile();
    }

    // ========== 旁观者跟踪系统 ==========

    /**
     * @brief 获取当前旁观目标的实体ID
     *
     * 返回当前玩家正在旁观（摄像机跟踪）的实体ID。
     * 如果没有旁观目标（即正常视角），返回 std::nullopt。
     *
     * @return 旁观目标的实体ID，或 std::nullopt 表示正常视角
     */
    [[nodiscard]] std::optional<EntityInstanceId> getCameraEntityId() const { return m_cameraEntityId; }

    /**
     * @brief 检查当前是否正在旁观某个实体
     * @return 如果有旁观目标返回 true
     */
    [[nodiscard]] bool isSpectating() const { return m_cameraEntityId.has_value(); }

    /**
     * @brief 设置旁观目标实体ID
     *
     * 设置玩家的摄像机跟踪目标。当设置后，玩家的视角将跟随目标实体。
     * 传入 std::nullopt 表示恢复正常视角（摄像机跟踪自身）。
     *
     * 当摄像机目标实际发生变化时，会调用虚方法 onCameraEntityChanged()，
     * ServerPlayer 重写该方法以发送 SetCameraPacket 给客户端并执行传送等操作。
     *
     * @param entityId 旁观目标的实体ID，或 std::nullopt 恢复正常视角
     */
    void setCameraEntityId(std::optional<EntityInstanceId> entityId);

    /**
     * @brief 摄像机目标变更通知
     *
     * 当 setCameraEntityId() 导致摄像机目标实际发生变化时调用。
     * 基类版本为空操作，ServerPlayer 重写以发送 SetCameraPacket 给客户端
     * 并执行传送等网络同步操作。
     *
     * @param oldCameraId 变更前的摄像机目标实体ID（std::nullopt 表示正常视角）
     * @param newCameraId 变更后的摄像机目标实体ID（std::nullopt 表示恢复正常视角）
     */
    virtual void onCameraEntityChanged(
        std::optional<EntityInstanceId> oldCameraId, std::optional<EntityInstanceId> newCameraId)
    {}

    // ========== 幽匿尖啸体警告追踪 ==========

    /**
     * @brief 获取监守者警告效果
     *
     * 用于追踪玩家在深暗之域中被幽匿尖啸体警告的等级和冷却。
     */
    [[nodiscard]] entity::WardenWarningEffect& wardenWarningEffect() { return m_wardenWarningEffect; }
    [[nodiscard]] const entity::WardenWarningEffect& wardenWarningEffect() const { return m_wardenWarningEffect; }

    /**
     * @brief 检查是否是生存模式
     */
    [[nodiscard]] bool isSurvival() const { return m_gameMode == GameMode::Survival; }

    /**
     * @brief 检查是否是冒险模式
     */
    [[nodiscard]] bool isAdventure() const { return m_gameMode == GameMode::Adventure; }

    /**
     * @brief 检查玩家是否可以在指定位置与方块交互
     *
     * 重写 Entity::mayInteract。参考 MC Java 的 Player.mayUseItemAt() 冒险模式逻辑。
     *
     * 行为：
     * - 旁观模式：禁止交互
     * - 冒险模式：检查主手和副手物品的 CanPlaceOn 标签，
     *   如果任一只手的物品可以在目标方块上放置则允许交互
     * - 生存/创造模式：允许交互
     *
     * @param world 世界引用
     * @param pos 目标方块位置
     * @return 如果允许交互返回 true
     */
    [[nodiscard]] bool mayInteract(IWorld& world, const BlockPos& pos) const override;

    // ========== 权限等级 ==========

    /**
     * @brief 获取玩家权限等级
     *
     * 权限等级对应 MC 的 OP 等级系统：
     * - 0: 普通玩家
     * - 1: 版主（可绕过重生点保护）
     * - 2: 游戏管理员（可使用游戏管理命令）
     * - 3: 服务器管理员
     * - 4: 服务器所有者（控制台级别）
     *
     * @return 权限等级 (0-4)
     */
    [[nodiscard]] i32 permissionLevel() const { return m_permissionLevel; }

    /**
     * @brief 设置玩家权限等级
     * @param level 权限等级 (0-4)
     */
    void setPermissionLevel(i32 level) { m_permissionLevel = level; }

    /**
     * @brief 检查玩家是否拥有指定权限等级
     * @param level 所需的最低权限等级
     * @return 如果玩家权限等级 >= level 则返回 true
     */
    [[nodiscard]] bool hasPermission(i32 level) const { return m_permissionLevel >= level; }

    /**
     * @brief 检查玩家是否可以使用游戏管理员方块
     *
     * 游戏管理员方块包括命令方块、结构方块、拼图方块等，
     * 只有创造模式且拥有游戏管理员权限的玩家才能使用。
     *
     * 条件：creativeMode == true 且 permissionLevel >= 2 (GameMaster)
     *
     * @return 如果玩家可以使用游戏管理员方块则返回 true
     */
    [[nodiscard]] bool canUseGameMasterBlocks() const { return m_abilities.creativeMode && m_permissionLevel >= 2; }

    /**
     * @brief 检查玩家是否有建造权限
     *
     * 对应 MC Java 的 Player.mayBuild()。返回 abilities.allowEdit 标志。
     * 生存模式和创造模式下为 true，冒险模式和旁观者模式下为 false。
     * 用于控制玩家是否可以执行建造相关操作（如空手熄灭蜡烛、编辑告示牌、切换红石元件状态等）。
     *
     * @return 如果玩家有建造权限返回 true
     */
    [[nodiscard]] bool mayBuild() const { return m_abilities.allowEdit; }

    /**
     * @brief 检查玩家是否可以在指定位置使用物品
     *
     * 对应 MC Java 的 Player.mayUseItemAt(BlockPos, Direction, ItemStack)。
     * 如果玩家有建造权限则直接允许；否则检查物品的 CanPlaceOn 标签。
     *
     * @param world 世界引用
     * @param pos 目标方块位置
     * @param facing 交互方向
     * @param itemStack 要使用的物品
     * @return 如果允许使用物品返回 true
     */
    [[nodiscard]] bool mayUseItemAt(
        IWorld& world, const BlockPos& pos, Direction facing, const ItemStack& itemStack) const;

    /**
     * @brief 检查玩家对方块的操作是否受限制
     *
     * 对应 MC Java 的 Player.blockActionRestricted(Level, BlockPos, GameType)。
     * 在冒险模式和旁观者模式下，玩家的方块操作受限。
     * 如果玩家有建造权限（mayBuild），则操作不受限。
     * 冒险模式下还需检查主手物品的 CanDestroy 标签。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return 如果方块操作受限返回 true
     */
    [[nodiscard]] bool blockActionRestricted(IWorld& world, const BlockPos& pos) const;

    // 维度
    [[nodiscard]] DimensionId dimension() const { return m_dimension; }
    void setDimension(DimensionId dim) { m_dimension = dim; }

    [[nodiscard]] sound::SoundCategory getSoundCategory() const override { return sound::SoundCategory::Players; }

    // ========== 传送门 ==========

    /**
     * @brief 获取在传送门中停留所需的最大时间
     *
     * 玩家需要 80 tick (4秒) 在传送门中才能传送。
     * 创造模式（无敌状态）只需要 1 tick。
     *
     * @return 创造模式返回 1，其他返回 80
     */
    [[nodiscard]] i32 getMaxInPortalTime() const override { return m_abilities.invulnerable ? 1 : 80; }

    /**
     * @brief 获取传送冷却时间
     *
     * 玩家的传送冷却时间为 10 tick，而非默认的 300 tick。
     *
     * @return 10 tick
     */
    [[nodiscard]] i32 getPortalCooldown() const override { return 10; }

    // ========== 火焰系统 ==========

    /**
     * @brief 获取玩家火焰免疫期时长
     *
     * MC Java 中 Player 重写此方法返回 20（1 秒免疫期），
     * 普通实体返回 0（无免疫期）。这使得玩家在火焰熄灭后有 1 秒
     * 的时间不会被立即重新点燃。
     *
     * @return 20 tick
     */
    [[nodiscard]] i32 getFireImmuneTicks() const override { return 20; }

    /**
     * @brief 强制设置火焰计时器（创造模式限制）
     *
     * MC Java 中 Player 重写此方法，当创造模式（无敌状态）时
     * 将火焰计时器限制为最多 1 tick，防止创造模式玩家燃烧。
     *
     * @param ticks 火焰计时器值
     */
    void forceFireTicks(i32 ticks) override
    {
        Entity::forceFireTicks(m_abilities.invulnerable ? std::min(ticks, 1) : ticks);
    }

    /**
     * @brief 设置位置并重置步距采样
     */
    void setPosition(f32 x, f32 y, f32 z);
    void setPosition(const Vector3& pos) { setPosition(pos.x, pos.y, pos.z); }

    // 生命值和饥饿
    void setHealth(f32 health);
    void heal(f32 amount);

    [[nodiscard]] const FoodStats& foodStats() const { return m_foodStats; }
    FoodStats& foodStats() { return m_foodStats; }

    /**
     * @brief 检查玩家是否可以进食
     *
     * - 创造模式或观察者模式: 返回 false
     * - ignoreHunger 为 true: 返回 true（如金苹果等特殊食物）
     * - 否则: 返回 hunger < 20
     *
     * @param ignoreHunger 是否忽略饥饿值检查
     * @return 是否可以进食
     */
    [[nodiscard]] bool canEat(bool ignoreHunger = false) const;

    // ========== 饥饿消耗 ==========

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
    [[nodiscard]] const entity::experience::ExperienceManager& experienceManager() const
    {
        return *m_experienceManager;
    }
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
    virtual void addExperienceLevels(i32 levels);

    /**
     * @brief 消耗经验值
     * @param amount 要消耗的经验值
     * @return 是否成功消耗
     */
    [[nodiscard]] virtual bool consumeExperience(i32 amount);

    /**
     * @brief 消耗经验等级（用于附魔）
     * @param levels 要消耗的等级数
     * @return 是否成功消耗
     */
    [[nodiscard]] virtual bool consumeExperienceLevels(i32 levels);

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
    virtual void setExperience(i32 level, f32 progress, i32 totalExperience);

    // ========== 消息发送 ==========

    /**
     * @brief 发送状态消息给玩家
     *
     * 在 Player 基类中默认为空操作。
     * ServerPlayer 重写此方法以通过网络发送消息到客户端。
     * 客户端 Player 可以直接显示在聊天界面。
     *
     * @param message 消息内容（通常是翻译键或格式化文本）
     * @param actionBar 是否显示在 Action Bar（物品栏上方的提示区域）
     *                  当前实现中此参数可能被忽略，消息始终发送到聊天区域
     */
    virtual void sendStatusMessage(const std::string& message, bool actionBar = false);

    /**
     * @brief 检查玩家是否能接收消息
     *
     * ServerPlayer 重写此方法，在有有效网络连接时返回 true。
     *
     * @return 如果玩家能接收消息返回 true
     */
    [[nodiscard]] virtual bool canReceiveMessages() const { return false; }

    // ========== 统计系统 ==========

    /**
     * @brief 增加物品使用统计
     *
     * 当玩家使用物品时调用（如打火石点燃TNT、使用火焰弹等）。
     * ServerPlayer 重写此方法以实际更新统计。
     *
     * @param itemId 物品资源位置
     * @param count 使用次数
     */
    virtual void awardUsedStat(const ResourceLocation& itemId, i32 count)
    {
        MC_UNUSED(itemId);
        MC_UNUSED(count);
        // 基类默认空实现
    }

    /**
     * @brief 增加物品合成统计
     *
     * 当玩家合成物品时调用，更新统计数据。
     * ServerPlayer 重写此方法以实际更新统计。
     *
     * @param itemId 物品资源位置
     * @param count 合成数量
     */
    virtual void awardCraftedStat(const ResourceLocation& itemId, i32 count)
    {
        MC_UNUSED(itemId);
        MC_UNUSED(count);
        // 基类默认空实现
    }

    /**
     * @brief 物品合成完成时调用
     *
     * 当物品通过合成或熔炼创建时调用，用于触发成就等。
     * ServerPlayer 重写此方法以触发成就系统。
     *
     * @param stack 合成的物品堆
     * @param amount 合成数量
     */
    virtual void onItemCrafted(ItemStack& stack, i32 amount)
    {
        MC_UNUSED(stack);
        MC_UNUSED(amount);
        // 基类默认空实现
    }

    /**
     * @brief 增加自定义统计
     *
     * 当玩家触发自定义统计事件时调用（如打开容器、与方块交互等）。
     * ServerPlayer 重写此方法以实际更新统计。
     *
     * @param statId 自定义统计的资源位置（使用 mc::stats 命名空间中的常量）
     * @param count 增量值
     */
    virtual void awardCustomStat(const ResourceLocation& statId, i32 count)
    {
        MC_UNUSED(statId);
        MC_UNUSED(count);
        // 基类默认空实现
    }

    /**
     * @brief 解锁配方
     *
     * 当玩家首次合成某配方时调用，用于解锁配方书和触发成就。
     * ServerPlayer 重写此方法以触发 RecipeUnlockedTrigger。
     *
     * @param recipeId 配方资源位置
     */
    virtual void unlockRecipe(const ResourceLocation& recipeId)
    {
        MC_UNUSED(recipeId);
        // 基类默认空实现
    }

    // ========== 类型转换 ==========

    /**
     * @brief 转换为 ServerPlayer 指针
     *
     * 只有 ServerPlayer 会返回有效的指针，其他实现返回 nullptr。
     * 用于需要访问 ServerPlayer 特有功能（如命令执行、服务器引用）的场景。
     *
     * @return ServerPlayer 指针，如果不是 ServerPlayer 返回 nullptr
     */
    [[nodiscard]] virtual class ServerPlayer* asServerPlayer() { return nullptr; }
    [[nodiscard]] virtual const ServerPlayer* asServerPlayer() const { return nullptr; }

    /**
     * @brief 向玩家客户端发送速度同步包
     *
     * 将实体当前的速度通过网络包发送给此玩家的客户端。
     * 基类版本返回 false（未发送），ServerPlayer 重写以实际发送网络包并返回 true。
     * 用于 causeExtraKnockback() 中对 ServerPlayer 目标立即发送速度包，
     * 避免 EntityTracker::tick() 重复发送导致速度重复应用。
     *
     * @return true 如果成功发送了速度包，false 如果未发送（非 ServerPlayer）
     */
    [[nodiscard]] virtual bool sendVelocityPacket() { return false; }

    /**
     * @brief 获取玩家所在的记分板
     *
     * 只有 ServerPlayer 会返回有效的指针，客户端实现返回 nullptr。
     * 用于战利品条件等需要访问记分板的场景。
     *
     * @return 记分板指针，如果不可用返回 nullptr
     */
    [[nodiscard]] virtual scoreboard::Scoreboard* getScoreboard() { return nullptr; }
    [[nodiscard]] virtual const scoreboard::Scoreboard* getScoreboard() const { return nullptr; }

    /**
     * @brief 掉落经验（死亡时调用）
     *
     * 玩家死亡时掉落 min(level * 7, 100) 点经验。
     */
    void dropExperience() override;

    /**
     * @brief 死亡时掉落库存物品（在 shouldDropLoot 守卫之外，不受 doMobLoot 影响）
     *
     * 重写 LivingEntity::dropEquipment()，对齐 MC Java 1.21.11 Player.dropEquipment
     * （Player.java:556-565）。非 keepInventory 时：
     *   1. destroyVanishingCursedItems()：销毁带消失诅咒（PREVENT_EQUIPMENT_DROP component）
     *      的库存物品（从库存移除，不掉落）。
     *   2. inventory.dropAll()：把库存所有物品以 ItemEntity 形式掉落到死亡位置。
     * keepInventory=true 时保留库存不掉落。
     *
     * 注：vanilla Player.die 不直接调 dropEquipment，而是经 LivingEntity.die → dropAllDeathLoot
     * → dropEquipment（守卫外）。Cubium 同链路：Player::die 调 LivingEntity::die → dropAllDeathLoot
     * → dropEquipment（Player override）。
     */
    void dropEquipment() override;

    /**
     * @brief 销毁库存中带消失诅咒的物品（对齐 vanilla Player.destroyVanishingCursedItems）
     *
     * 遍历库存所有槽位，带消失诅咒（hasVanishingCurse，等价 PREVENT_EQUIPMENT_DROP component）
     * 的物品直接从库存移除（removeItemNoUpdate，不掉落、不同步）。在 dropEquipment 中
     * 非 keepInventory 时于 dropAll 之前调用，使消失诅咒物品死亡时销毁而非掉落。
     */
    void destroyVanishingCursedItems();

    /**
     * @brief 丢弃物品
     *
     * 在玩家位置生成物品实体。
     *
     * @param stack 要丢弃的物品堆
     * @param dropAround 是否向四周散射（Q键丢弃 vs Ctrl+Q丢弃）
     * @param traceItem 是否追踪物品（设置 thrower UUID）
     * @return 生成的物品实体，如果物品为空则返回 nullptr
     */
    ItemEntity* dropItem(ItemStack& stack, bool dropAround, bool traceItem = true);

    /**
     * @brief 丢弃物品（简化版本）
     *
     * 在玩家位置生成物品实体，使用默认参数。
     *
     * @param stack 要丢弃的物品堆
     * @param unused 未使用参数（用于签名匹配）
     * @return 生成的物品实体
     */
    ItemEntity* dropItem(ItemStack& stack, bool unused = false);

    /**
     * @brief 从主手选中槽位丢弃物品
     *
     * 对齐 Java Player.drop(boolean dropAll)：从当前选中的主手槽位丢一个
     * （dropAll=false）或整组（dropAll=true），在玩家位置生成 ItemEntity。
     * 创造/旁观模式不丢弃（旁观无物品栏语义，创造由客户端处理）。
     *
     * @param dropAll true=丢弃整组物品，false=仅丢弃一个
     * @return 生成的物品实体，失败返回 nullptr
     */
    ItemEntity* drop(bool dropAll);

    /**
     * @brief 受伤时损坏护甲
     *
     * 重写 LivingEntity::damageArmor()，委托给 PlayerInventory::damageArmor()。
     *
     * @param source 伤害来源
     * @param amount 伤害量
     */
    void damageArmor(DamageSource& source, f32 amount) override;

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
    [[nodiscard]] bool isOnGround() const { return m_builtIn.physicsState->m_onGround; }
    [[nodiscard]] bool isSprinting() const { return m_isSprinting; }
    [[nodiscard]] bool isSneaking() const override { return m_isSneaking; }
    [[nodiscard]] bool isSwimming() const { return m_isSwimming; }
    [[nodiscard]] bool isSleeping() const { return m_isSleeping; }

    /**
     * @brief 获取玩家是否正在输入潜行
     *
     * 与 isSneaking() 不同，此方法返回的是玩家是否按住了潜行键，
     * 而非实际的潜行状态（旁观者模式下输入潜行用于退出旁观跟踪）。
     */
    [[nodiscard]] bool isInputSneaking() const { return m_inputSneaking; }

    /**
     * @brief 获取玩家前进移动输入
     * @return 前进移动值 (-1到1，负为后退)
     */
    [[nodiscard]] f32 moveForward() const { return m_inputForward; }

    /**
     * @brief 获取玩家横向移动输入
     * @return 横向移动值 (-1到1，负为左)
     */
    [[nodiscard]] f32 moveStrafing() const { return m_inputStrafe; }

    void setSprinting(bool sprinting);
    void setSneaking(bool sneaking);
    void setSwimming(bool swimming);
    void setSleeping(bool sleeping);

    /**
     * @brief 玩家最近一次客户端输入位掩码（服务端权威缓存）。
     *
     * 对齐 Java Player.setLastClientInput：服务端 handlePlayerInput 收到
     * ServerboundPlayerInput 后写入此字段，骑乘载具的 travel() 在 tick 中
     * 据此驱动移动。位定义同 ir::play::PlayerInput：
     * bit0=forward bit1=backward bit2=left bit3=right
     * bit4=jump bit5=shift bit6=sprint。
     */
    void setLastClientInput(u8 input) { m_lastClientInput = input; }
    [[nodiscard]] u8 lastClientInput() const { return m_lastClientInput; }

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
    [[nodiscard]] bool isPlayerFullyAsleep() const { return m_isSleeping && m_sleepTimer >= 100; }

    /**
     * @brief 获取睡眠计时器
     * @return 睡眠计时器值 (0-100 完全入睡后保持)
     */
    [[nodiscard]] i32 getSleepTimer() const { return m_sleepTimer; }

    /**
     * @brief 设置睡眠计时器
     * @param value 计时器值
     */
    void setSleepTimer(i32 value) { m_sleepTimer = value; }

    /**
     * @brief 开始睡眠（基础状态管理）
     *
     * 设置睡眠状态和位置，切换到睡眠姿态，重置睡眠计时器。
     *
     * 注意：这是低级 API，仅设置状态。
     * - 客户端：用于接收服务端睡眠状态更新
     * - 服务端：应使用 tryStartSleeping() 进行完整验证流程
     *
     * @param pos 床位位置
     */
    void startSleeping(const BlockPos& pos);

    /**
     * @brief 尝试开始睡眠（带验证）
     *
     * 执行完整的睡眠验证流程，包括距离检查、阻挡检查、时间检查、怪物检查等。
     * 基类实现为简单成功（直接调用 startSleeping）。
     * ServerPlayer 重写此方法进行完整验证。
     *
     * @param bedPos 床头位置
     * @return 睡眠结果
     */
    virtual entity::SleepResult tryStartSleeping(const BlockPos& bedPos);

    /**
     * @brief 停止睡眠（基础状态管理）
     *
     * 清除睡眠状态和位置，切换到站立姿态。
     *
     * 注意：这是低级 API，仅清除状态。
     * - 客户端：用于接收服务端唤醒状态更新
     * - 服务端：应使用 ServerPlayer::stopSleepInBed() 或 wakeUp() 进行完整唤醒流程
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

    // ========== 下界进度追踪 ==========

    /**
     * @brief 获取进入下界时的位置
     *
     * 用于 nether_travel 进度触发器。
     * 当玩家从主世界进入下界时记录，从下界返回主世界时清除。
     *
     * @return 进入下界时的位置，如果未记录则返回空
     */
    [[nodiscard]] std::optional<Vector3d> getEnteredNetherPosition() const { return m_enteredNetherPosition; }

    /**
     * @brief 设置进入下界时的位置
     * @param pos 位置
     */
    void setEnteredNetherPosition(const Vector3d& pos) { m_enteredNetherPosition = pos; }

    /**
     * @brief 清除进入下界时的位置
     */
    void clearEnteredNetherPosition() { m_enteredNetherPosition = std::nullopt; }

    // ========== 死亡位置追踪 ==========

    /**
     * @brief 获取上次死亡位置
     * @return 死亡位置（维度+方块位置），如果未记录则返回空
     */
    [[nodiscard]] std::optional<GlobalPos> getLastDeathLocation() const { return m_lastDeathLocation; }

    /**
     * @brief 设置上次死亡位置
     * @param location 死亡位置（维度+方块位置），传入 nullopt 清除
     */
    void setLastDeathLocation(std::optional<GlobalPos> location) { m_lastDeathLocation = std::move(location); }

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
    [[nodiscard]] f32 stepHeight() const override { return physics::STEP_HEIGHT; }

    /**
     * @brief 获取乘客Y偏移
     * @return -0.35
     *
     * 当玩家作为乘客时，相对于载具骑乘点的 Y 偏移。
     * 这个负值使玩家稍微下沉到载具上，使骑乘动画看起来更自然。
     */
    [[nodiscard]] f64 getYOffset() const override { return -0.35; }

    /**
     * @brief 检查是否是本地玩家
     * @return 如果是本地玩家返回true
     *
     * 客户端：返回 true 表示这是本地玩家控制的实体
     * 服务端：总是返回 false
     *
     * 用于 canPassengerSteer() 判断控制权。
     */
    [[nodiscard]] virtual bool isLocalPlayer() const { return false; }

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

    // ========== 鞘翅飞行（Elytra Glide） ==========

    /**
     * @brief 重写 canGlide：创造/旁观飞行模式下禁止滑翔
     *
     * 对应 MC 1.21.11 Player.canGlide()：
     *   return !this.abilities.flying && super.canGlide();
     * 当玩家通过创造模式双击空格进入飞行状态（abilities.flying=true）时，
     * 即使穿戴鞘翅也不能滑翔，避免两种飞行模式冲突。
     *
     * @return 如果玩家未在创造飞行且基类判定可滑翔返回 true
     */
    [[nodiscard]] bool canGlide() const override;

    /**
     * @brief 重写 tryToStartFallFlying：玩家专属的开始滑翔逻辑
     *
     * 对应 MC 1.21.11 Player.tryToStartFallFlying()。
     * Player 不沿用 LivingEntity 基类的通用实现，而是显式重写：
     * - 仅当未在飞行、canGlide() 返回 true、不在水中时调用 startFallFlying()
     * - 由 MinecraftServer::handlePlayerCommandPacket 在收到 ir::play::PlayerCommand
     *   （action=START_FALL_FLYING）时调用
     * - 若返回 false，该方法会调用 stopFallFlying() 强制收起鞘翅
     *
     * @return 如果成功开始滑翔返回 true
     */
    bool tryToStartFallFlying() override;

    /**
     * @brief 玩家专属的开始鞘翅飞行
     *
     * 对应 MC 1.21.11 Player.startFallFlying()。
     * 设置 EntityFlags::FallFlying 标志位，触发数据参数同步给客户端。
     */
    void startFallFlying();

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
     */
    [[nodiscard]] i32 getDepthStriderLevel() const;

    /**
     * @brief 更新空气供应和溺水
     * 玩家有特殊的水下呼吸附魔处理
     */
    void updateAirSupply() override;

    /**
     * @brief 更新移动距离（用于视野晃动和脚步声）
     */
    void updateMoveDistance();

    /**
     * @brief 检测与附近实体的碰撞
     *
     * 检测玩家碰撞箱扩展范围内的实体，并调用它们的 onCollideWithPlayer 方法。
     * 用于处理物品拾取、箭矢拾取、经验球吸收等。
     */
    void checkEntityCollisions();

    /**
     * @brief 播放脚步声
     *
     * 在行走距离累计超过阈值时触发。
     * 根据脚下方块类型选择不同的脚步声。
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;

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

    // ========== 受伤/死亡 ==========

    /**
     * @brief 受伤处理
     *
     * 覆盖 LivingEntity::hurt()，添加创造模式无敌检查。
     */
    bool hurt(DamageSource& source, f32 amount) override;

    /**
     * @brief 盾牌格挡判定（对齐 MC Java 1.21.11 Player.canBlockDamageSource /
     *        LivingEntity.getItemBlockingWith + BlocksAttacks.bypassedBy）
     *
     * 基类 LivingEntity::canBlockDamageSource 恒返回 false（占位）。玩家在使用盾牌（主/副手
     * 处于使用状态且物品是 ShieldItem）且伤害来源不绕过盾牌（DamageTypeTags::BYPASSES_SHIELD）
     * 时，可以格挡该伤害。对齐 Java applyItemBlocking 的判定逻辑（ItemStack.getItemBlockingWith
     * → BlocksAttacks.bypassedBy().map(source::is)）。
     *
     * @param source 伤害来源
     * @return 是否可以格挡
     */
    [[nodiscard]] bool canBlockDamageSource(DamageSource& source) const override;

    /**
     * @brief 格挡成功后消耗盾牌耐久（对齐 MC Java 1.21.11 BlocksAttacks.hurtBlockingItem）
     *
     * 基类 LivingEntity::damageShield 空实现。玩家格挡成功时消耗正在使用的盾牌的耐久度
     * （对齐 Java hurtBlockingItem：damageItem(amount, this, usedItemHand)），并播放
     * ITEM_SHIELD_BLOCK 音效。盾牌耐久耗尽时由 damageItem 内部触发破坏。
     *
     * @param amount 被格挡的伤害量
     */
    void damageShield(f32 amount) override;

    /**
     * @brief 攻击者手持斧头时破盾秒数（对齐 MC Java 1.21.11
     *        LivingEntity.getSecondsToDisableBlocking + Weapon.disableBlockingForSeconds）
     *
     * vanilla 攻击者主手武器带 WEAPON 组件且 disableBlockingForSeconds>0 时返回该值
     * （斧头 5.0F）。Cubium 暂无 WEAPON 组件体系，改为检测主手是否为 AxeItem（斧头
     * 攻击破盾是 wiki 明确行为：用斧攻击盾牌可使盾牌失效 5 秒）。
     *
     * @return 主手持斧返回 5.0F，否则 0.0F（不破盾）
     */
    [[nodiscard]] f32 getSecondsToDisableBlocking() const noexcept override;

    /**
     * @brief 受害者盾牌被破盾时禁用盾牌（对齐 MC Java 1.21.11 Player.blockUsingItem
     *        破盾分支 → BlocksAttacks.disable）
     *
     * 玩家举盾格挡时，若攻击者 getSecondsToDisableBlocking > 0，则对自身活跃盾牌
     * setItemCooldown(shield, round(seconds*20))（斧头 5.0 秒 = 100 tick）+ stopActiveHand
     * （停止举盾）+ 播放 ITEM_SHIELD_BREAK 破盾音效（对齐 vanilla disableSound）。
     *
     * @param attacker 攻击者（提供 getSecondsToDisableBlocking 破盾秒数）
     */
    void onShieldDisabled(LivingEntity& attacker) override;

    /**
     * @brief 检查玩家是否对指定伤害类型免疫
     *
     * 在基类检查的基础上，额外检查玩家专属游戏规则：
     * - 溺水伤害 + DROWNING_DAMAGE 关闭 → 免疫
     * - 摔落伤害 + FALL_DAMAGE 关闭 → 免疫
     * - 火焰伤害 + FIRE_DAMAGE 关闭 → 免疫
     * - 冰冻伤害 + FREEZE_DAMAGE 关闭 → 免疫
     */
    [[nodiscard]] bool isInvulnerableTo(DamageSource& source) const override;

    /**
     * @brief 判断本玩家是否可以对目标玩家造成伤害
     *
     * 基础实现检查队伍友伤规则：
     * - 如果攻击者没有队伍，可以伤害
     * - 如果两个队伍不是盟友关系，可以伤害
     * - 如果两个队伍是盟友关系，取决于队伍是否允许友伤
     *
     * ServerPlayer 会重写此方法，额外检查 PvP 游戏规则。
     *
     * @param target 目标玩家
     * @return 如果可以造成伤害返回 true
     */
    [[nodiscard]] virtual bool canHarmPlayer(const Player& target) const;

    /**
     * @brief 死亡处理
     *
     * 覆盖 LivingEntity::die()，添加玩家特有死亡逻辑（掉落经验等）。
     */
    void die(DamageSource& cause) override;

    /**
     * @brief 处理摔落伤害
     *
     * 覆盖 LivingEntity::handleFallDamage()，添加玩家特有摔落音效。
     */
    void handleFallDamage(f32 distance, f32 damageMultiplier) override;

    /**
     * @brief 使用自定义伤害来源处理摔落伤害
     *
     * 覆盖 LivingEntity::causeFallDamage()，实现冲量坠落伤害减免逻辑。
     * 当玩家处于冲量免疫状态（如重锤砸地攻击或风弹爆炸后），
     * 仅计算冲量冲击点以下部分的坠落伤害，冲量冲击点以上的部分不计伤害。
     */
    void causeFallDamage(f32 distance, f32 damageMultiplier, const DamageSource& source) override;

    // ========== 冲量坠落伤害免疫 ==========

    /**
     * @brief 设置是否忽略当前冲量造成的坠落伤害
     *
     * 当设置为 true 时，同时启动 40 tick 的宽限期计时器。
     * 当设置为 false 时，立即清除宽限期计时器。
     *
     * 由重锤砸地攻击和风弹爆炸触发。
     *
     * @param ignore 是否忽略坠落伤害
     */
    void setIgnoreFallDamageFromCurrentImpulse(bool ignore);

    /**
     * @brief 检查是否忽略当前冲量造成的坠落伤害
     * @return 如果忽略则返回 true
     */
    [[nodiscard]] bool isIgnoringFallDamageFromCurrentImpulse() const { return m_ignoreFallDamageFromCurrentImpulse; }

    /**
     * @brief 应用冲量后宽限期
     *
     * 设置宽限期计时器为当前值和新值中的较大者，不会缩短已有的宽限期。
     * 风爆附魔使用 10 tick 宽限期。
     *
     * @param graceTime 宽限期 tick 数
     */
    void applyPostImpulseGraceTime(i32 graceTime);

    /**
     * @brief 检查是否处于冲量后宽限期
     * @return 如果宽限期计时器 > 0 则返回 true
     */
    [[nodiscard]] bool isInPostImpulseGraceTime() const { return m_currentImpulseContextResetGraceTime > 0; }

    /**
     * @brief 获取冲量上下文重置宽限期剩余 tick 数
     * @return 剩余宽限期 tick 数，0 表示无宽限期
     */
    [[nodiscard]] i32 currentImpulseContextResetGraceTime() const { return m_currentImpulseContextResetGraceTime; }

    /**
     * @brief 尝试重置冲量上下文
     *
     * 仅当宽限期计时器为 0 时才重置。宽限期期间此方法为空操作。
     */
    void tryResetCurrentImpulseContext();

    /**
     * @brief 完全重置冲量上下文
     *
     * 清除所有冲量状态：宽限期计时器、爆炸原因、冲击位置、忽略坠落伤害标志。
     */
    void resetCurrentImpulseContext();

    /**
     * @brief 获取当前冲量冲击位置
     * @return 冲击位置，如果没有活跃冲量则返回空
     */
    [[nodiscard]] const std::optional<Vector3>& currentImpulseImpactPos() const { return m_currentImpulseImpactPos; }

    /**
     * @brief 设置当前冲量冲击位置
     * @param pos 冲击位置
     */
    void setCurrentImpulseImpactPos(const Vector3& pos) { m_currentImpulseImpactPos = pos; }

    /**
     * @brief 获取当前爆炸原因实体ID
     * @return 实体ID，0 表示无
     */
    [[nodiscard]] EntityInstanceId currentExplosionCause() const { return m_currentExplosionCause; }

    /**
     * @brief 设置当前爆炸原因实体ID
     * @param entityId 实体ID
     */
    void setCurrentExplosionCause(EntityInstanceId entityId) { m_currentExplosionCause = entityId; }

    /**
     * @brief 计算重锤砸地攻击的冲击位置
     *
     * 如果玩家已有活跃冲量且其冲击位置不高于当前位置，保留原有冲击位置
     * （防止连续砸地攻击时"双重获利"）。否则使用玩家当前位置。
     *
     * @return 冲击位置
     */
    [[nodiscard]] Vector3 calculateMaceImpactPosition() const;

    /**
     * @brief 被爆炸击中时调用
     *
     * 设置冲量冲击位置和爆炸原因，如果爆炸由风弹引起则启用坠落伤害免疫。
     * 子类可重写此方法以添加额外逻辑（如服务端进度触发）。
     *
     * @param cause 引起爆炸的实体，可能为 nullptr
     */
    virtual void onExplosionHit(Entity* cause) override;

protected:
    /**
     * @brief 移除肩部实体（对齐 vanilla Player.removeEntitiesOnShoulder()，空实现）
     *
     * vanilla Player 基类中为空实现，ServerPlayer 重写以将肩部鹦鹉生成回世界。
     * Cubium 中由 ServerPlayer::removeEntitiesOnShoulder() override 承载实际逻辑。
     */
    virtual void removeEntitiesOnShoulder() {}

    /**
     * @brief 获取受伤声音
     *
     * 覆盖 LivingEntity::getHurtSound()，返回玩家特殊受伤音效。
     * 根据伤害类型返回不同音效：
     * - 火焰伤害: ENTITY_PLAYER_HURT_ON_FIRE
     * - 溺水伤害: ENTITY_PLAYER_HURT_DROWN
     * - 甜浆果丛伤害: ENTITY_PLAYER_HURT_SWEET_BERRY_BUSH
     * - 其他: ENTITY_PLAYER_HURT
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     *
     * 覆盖 LivingEntity::getDeathSound()，返回玩家死亡音效。
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取摔落声音
     *
     * @param fallHeight 摔落高度（格数）
     * @return 摔落音效，高空摔落返回 ENTITY_PLAYER_BIG_FALL，否则 ENTITY_PLAYER_SMALL_FALL
     */
    [[nodiscard]] std::optional<ResourceLocation> getFallSound(i32 fallHeight) const override;

public:
    // ========== 水花溅射效果 ==========

    /**
     * @brief 获取溅水声音
     *
     * 玩家使用特定的溅水声音。
     *
     * @return ENTITY_PLAYER_SPLASH 声音事件
     */
    [[nodiscard]] ResourceLocation getSplashSound() const override;

    /**
     * @brief 获取高速溅水声音
     *
     * 玩家高速入水时使用特定的声音。
     *
     * @return ENTITY_PLAYER_SPLASH_HIGH_SPEED 声音事件
     */
    [[nodiscard]] ResourceLocation getHighspeedSplashSound() const override;

    /**
     * @brief 执行水花溅射效果
     *
     * 覆盖以检查观察者模式（观察者不产生水花效果）。
     */
    void doWaterSplashEffect() override;

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
     * @brief 获取当前视野晃动强度
     */
    [[nodiscard]] f32 cameraYaw() const { return m_cameraYaw; }

    /**
     * @brief 获取上一 tick 的视野晃动强度
     */
    [[nodiscard]] f32 prevCameraYaw() const { return m_prevCameraYaw; }

    /**
     * @brief 获取上一tick的游泳距离
     */
    [[nodiscard]] f32 prevMoveDistanceSwam() const { return m_prevMoveDistanceSwam; }

    // ========== 更新 ==========

    void tick() override;
    void update() override;

    // tickPortal() 已删除：传送门 tick 逻辑迁入 PortalTickSystem（PostEntityTick 阶段）。
    // 玩家 80 tick/创造 1 tick 的差异由 getMaxInPortalTime() override 承载，逻辑统一在 System。

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

    // ========== 背包 ==========

    /**
     * @brief 获取玩家背包
     */
    [[nodiscard]] const PlayerInventory& inventory() const { return m_inventory; }
    PlayerInventory& inventory() { return m_inventory; }

    /**
     * @brief 获取末影箱物品栏
     */
    [[nodiscard]] const PlayerEnderChestInventory& enderChestInventory() const { return m_enderChestInventory; }
    PlayerEnderChestInventory& enderChestInventory() { return m_enderChestInventory; }

    /**
     * @brief 获取玩家分数
     *
     * 分数是一个简单的整数计数器，在玩家死亡时增加。
     * 与计分板系统（Scoreboard）独立，MC Java 中通过实体数据同步。
     */
    [[nodiscard]] i32 getScore() const
    {
        const auto* c = m_entityContext->tryGetComponent<ecs::PlayerScoreComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        return c->m_score;
    }

    /**
     * @brief 设置玩家分数
     */
    void setScore(i32 score)
    {
        auto* c = m_entityContext->tryGetComponent<ecs::PlayerScoreComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        c->m_score = score;
        // 同步到 vanilla DATA_PLAYER_SCORE（set_entity_data 经此下发客户端）
        m_dataManager.set(DATA_PLAYER_SCORE_PARAM, c->m_score);
    }

    /**
     * @brief 增加玩家分数
     */
    void increaseScore(i32 amount)
    {
        auto* c = m_entityContext->tryGetComponent<ecs::PlayerScoreComponent>();
        MC_ASSERT_RELEASE(c != nullptr);
        c->m_score += amount;
        // 同步到 vanilla DATA_PLAYER_SCORE
        m_dataManager.set(DATA_PLAYER_SCORE_PARAM, c->m_score);
    }

    /**
     * @brief 重写吸收值设置：基类写 HurtStateComponent（真相源，含 clamp）后，
     * 额外下发 Player 专属 DATA_PLAYER_ABSORPTION_PARAM 到客户端。
     * 基类 LivingEntity 不持有该 DataParameter，故须在 Player 层补同步。
     */
    void setAbsorptionAmount(f32 amount) override
    {
        LivingEntity::setAbsorptionAmount(amount);
        // 取 clamp 后的真相源值下发镜像，确保客户端拿到与组件一致的值。
        m_dataManager.set(DATA_PLAYER_ABSORPTION_PARAM, absorptionAmount());
    }

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
     * @brief 打开实体容器
     *
     * 与实现 INamedContainerProvider 接口的实体交互时调用。
     * 例如：村民交易界面、箱子矿车等。
     *
     * @param provider 命名容器提供者
     * @return 如果成功打开返回 true
     */
    [[nodiscard]] bool openContainer(INamedContainerProvider& provider);

    /**
     * @brief 打开告示牌编辑器
     *
     * 当玩家右键点击可编辑的告示牌时调用。
     * 基类默认空实现（客户端 Player 无网络能力）。
     * ServerPlayer 重写以发送 OpenSignEditorPacket 给客户端。
     *
     * @param pos 告示牌方块位置
     * @param isFrontSide 是否编辑正面
     */
    virtual void openSignEditor(const BlockPos& pos, bool isFrontSide)
    {
        MC_UNUSED(pos);
        MC_UNUSED(isFrontSide);
    }

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
     * @brief 获取护甲值
     */
    [[nodiscard]] i32 armorValue() const;

    // ========== 装备重写 ==========

    /**
     * @brief 获取装备（重写 LivingEntity::getEquipment）
     *
     * 玩家的装备存储在 PlayerInventory 中，而不是 LivingEntity::m_equipment。
     * 此方法将 EquipmentSlot 映射到 PlayerInventory 的对应槽位。
     *
     * @param slot 装备槽位
     * @return 装备物品堆的引用
     */
    [[nodiscard]] const ItemStack& getEquipment(EquipmentSlot slot) const override;

    /**
     * @brief 获取装备可变引用（重写 LivingEntity::getMutableEquipment）
     *
     * 将 EquipmentSlot 映射到 PlayerInventory 的对应可变槽位引用。
     *
     * @param slot 装备槽位
     * @return 装备物品堆的可变引用
     */
    [[nodiscard]] ItemStack& getMutableEquipment(EquipmentSlot slot) override;

    /**
     * @brief 设置装备（重写 LivingEntity::setEquipment）
     *
     * 将装备设置到 PlayerInventory 的对应槽位。
     *
     * @param slot 装备槽位
     * @param stack 物品堆
     */
    void setEquipment(EquipmentSlot slot, const ItemStack& stack) override;

    /**
     * @brief 获取跳跃因子
     *
     * 跳跃因子影响玩家能跳多高。正常方块返回 1.0，
     * 蜂蜜块返回 0.5（降低跳跃高度）。
     *
     * @return 跳跃因子（0.0 - 1.0）
     */
    [[nodiscard]] virtual f32 getJumpFactor() const { return 1.0f; }

    // ========== 注视检测 ==========

    /**
     * @brief 获取玩家视线方向向量
     *
     * 根据玩家的 yaw 和 pitch 计算视线方向。
     *
     * @return 归一化的视线方向向量
     */
    [[nodiscard]] Vector3 getLookVector() const;

    /**
     * @brief 获取玩家眼睛位置
     *
     * 返回玩家眼睛在世界中的位置，用于射线检测等。
     *
     * @return 眼睛位置向量
     */
    [[nodiscard]] Vector3 getEyePosition() const;

    // ========== 交互范围 ==========

    /**
     * @brief 获取玩家方块交互距离
     *
     * 对应 MC 1.21.11 Player.blockInteractionRange()。
     * 返回 generic.block_interaction_range 属性的计算值。
     * 生存/冒险模式默认 4.5 格，创造模式通过修饰符 +0.5 达到 5.0 格。
     *
     * @return 方块交互距离（格）
     */
    [[nodiscard]] f64 blockInteractionRange() const;

    /**
     * @brief 获取玩家实体交互距离
     *
     * 对应 MC 1.21.11 Player.entityInteractionRange()。
     * 返回 generic.entity_interaction_range 属性的计算值。
     * 生存/冒险模式默认 3.0 格，创造模式通过修饰符 +2.0 达到 5.0 格。
     *
     * @return 实体交互距离（格）
     */
    [[nodiscard]] f64 entityInteractionRange() const;

    /**
     * @brief 检查玩家是否在方块交互范围内
     *
     * 对应 MC 1.21.11 Player.isWithinBlockInteractionRange(BlockPos, double)。
     * 计算玩家眼睛到方块 AABB 的距离平方，与 (blockInteractionRange + padding)² 比较。
     *
     * @param pos 方块位置
     * @param padding 额外容差（如 1.0 用于破坏/使用，4.0 用于容器菜单）
     * @return 玩家在该方块交互范围内返回 true
     */
    [[nodiscard]] bool isWithinBlockInteractionRange(const BlockPos& pos, f64 padding) const;

    /**
     * @brief 检查玩家是否在实体交互范围内
     *
     * 对应 MC 1.21.11 Player.isWithinEntityInteractionRange(Entity, double)。
     * 若实体已移除返回 false，否则委托到 AABB 重载。
     *
     * @param entity 目标实体
     * @param padding 额外容差（如 0.0 用于交互，3.0 用于捡选，4.0 用于坐骑容器）
     * @return 玩家在该实体交互范围内返回 true
     */
    [[nodiscard]] bool isWithinEntityInteractionRange(const Entity& entity, f64 padding) const;

    /**
     * @brief 检查玩家是否在指定 AABB 交互范围内
     *
     * 对应 MC 1.21.11 Player.isWithinEntityInteractionRange(AABB, double)。
     * 计算玩家眼睛到 AABB 的距离平方，与 (entityInteractionRange + padding)² 比较。
     *
     * @param aabb 目标 AABB
     * @param padding 额外容差
     * @return 玩家在该 AABB 交互范围内返回 true
     */
    [[nodiscard]] bool isWithinEntityInteractionRange(const AxisAlignedBB& aabb, f64 padding) const;

    /**
     * @brief 检查玩家是否戴着南瓜头
     *
     * 戴着南瓜头的玩家不会激怒末影人。
     * 南瓜头包括：雕刻南瓜（carved_pumpkin）和南瓜灯（jack_o_lantern）。
     *
     * @return 如果玩家戴着南瓜头返回 true
     */
    [[nodiscard]] bool isWearingPumpkin() const;

    /**
     * @brief 检查玩家是否正在注视目标实体
     *
     * 计算玩家视线方向与玩家到目标向量的点积，
     * 判断玩家是否正在看向目标。
     *
     * @param target 目标实体
     * @return 如果玩家正在注视目标返回 true
     */
    [[nodiscard]] bool isLookingAt(const Entity& target) const;

    /**
     * @brief 检查玩家是否穿戴金装备
     *
     * 猪灵会对未穿戴金装备的玩家产生敌意。
     * 检查玩家的四个盔甲槽位是否有金制盔甲。
     *
     * @return 如果玩家穿戴任何金装备返回 true
     */
    [[nodiscard]] bool isWearingGoldArmor() const;

    /**
     * @brief 获取自动跳跃系统
     */
    [[nodiscard]] entity::movement::AutoJump& autoJump() { return m_autoJump; }
    [[nodiscard]] const entity::movement::AutoJump& autoJump() const { return m_autoJump; }

    // ========== 攻击系统 ==========

    /**
     * @brief 获取攻击冷却进度
     *
     * 计算当前攻击冷却进度（0-1）。
     * 冷却进度 = min(ticksSinceLastAttack + adjustTicks, cooldownPeriod) / cooldownPeriod
     *
     * @param adjustTicks 调整的 tick 数（用于部分冷却补偿）
     * @return 冷却进度（0-1，1 表示完全冷却）
     */
    [[nodiscard]] f32 getCooledAttackStrength(f32 adjustTicks = 0.0f) const;

    /**
     * @brief 重置攻击冷却
     *
     * 在攻击后调用，重置攻击冷却计时器。
     */
    void resetCooldown();

    /**
     * @brief 获取物品切换缩放进度（0-1）
     *
     * Player.getItemSwapScale：基于独立的 itemSwapTicker，
     * 该计时器仅在主手物品种类切换时重置（不同于攻击冷却）。
     * 第一人称装备动画（mainHandHeight 的 target = f^3）使用此值，
     * 使切换物品后的“举起”动画与攻击挥动解耦。
     *
     * @param adjustTicks 调整的 tick 数（部分 tick 补偿）
     * @return 切换缩放进度（0-1，1 表示已稳定）
     */
    [[nodiscard]] f32 getItemSwapScale(f32 adjustTicks) const;

    /**
     * @brief 获取上次攻击后的 tick 数
     */
    [[nodiscard]] i32 ticksSinceLastAttack() const { return m_ticksSinceLastAttack; }

    // ========== 钓鱼系统 ==========

    /**
     * @brief 获取钓鱼浮标实体ID
     * @return 钓鱼浮标实体ID，0表示未投掷
     */
    [[nodiscard]] EntityInstanceId fishingBobber() const { return m_fishingBobber; }

    /**
     * @brief 设置钓鱼浮标实体ID
     * @param bobberId 浮标实体ID，0表示清除
     */
    void setFishingBobber(EntityInstanceId bobberId) { m_fishingBobber = bobberId; }

    /**
     * @brief 检查是否正在钓鱼
     * @return 如果有投掷的浮标返回 true
     */
    [[nodiscard]] bool isFishing() const { return m_fishingBobber != 0; }

    // ========== 物品冷却系统 ==========

    /**
     * @brief 获取物品冷却追踪器
     * @return 冷却追踪器的常量引用
     */
    [[nodiscard]] const CooldownTracker& cooldownTracker() const { return m_cooldownTracker; }
    CooldownTracker& cooldownTracker() { return m_cooldownTracker; }

    /**
     * @brief 获取物品冷却进度
     *
     * 便捷方法，直接查询指定物品的冷却进度。
     * 冷却进度 0 表示可用，1 表示刚开始冷却。
     *
     * @param item 物品指针
     * @param partialTicks 部分帧时间
     * @return 冷却进度（0-1）
     */
    [[nodiscard]] f32 getItemCooldown(const Item* item, f32 partialTicks = 0.0f) const
    {
        return m_cooldownTracker.getCooldownProgress(item, partialTicks);
    }

    /**
     * @brief 检查物品是否在冷却中
     * @param item 物品指针
     * @return 如果物品在冷却中返回 true
     */
    [[nodiscard]] bool hasItemCooldown(const Item* item) const { return m_cooldownTracker.hasCooldown(item); }

    /**
     * @brief 设置物品冷却
     * @param item 物品指针
     * @param ticks 冷却时间（tick）
     */
    void setItemCooldown(const Item* item, i32 ticks) { m_cooldownTracker.setCooldown(item, ticks); }

    // ========== 挖掘系统 ==========

    /**
     * @brief 获取玩家挖掘速度
     *
     * 计算玩家对指定方块的挖掘速度，考虑以下因素：
     * 1. 工具基础挖掘速度
     * 2. 效率附魔加成（仅当工具有效时）
     * 3. 急迫效果和潮涌能量加成
     * 4. 挖掘疲劳惩罚
     * 5. 水下挖掘惩罚（无水下速掘附魔时）
     * 6. 空中挖掘惩罚（不在地面时）
     *
     * @param state 目标方块状态
     * @param pos 方块位置（用于流体检测，可选）
     * @return 挖掘速度倍率
     */
    [[nodiscard]] f32 getDigSpeed(const BlockState& state, const BlockPos& pos = BlockPos(0, 0, 0)) const;

    /**
     * @brief 检查玩家是否能采集方块
     *
     * 判断玩家使用当前手持工具是否能采集指定方块。
     *
     * 采集条件：
     * 1. 方块不需要工具（requiresTool() == false）-> 可采集
     * 2. 手持物品的工具类型匹配且等级足够 -> 可采集
     * 3. 其他情况 -> 不可采集
     *
     * @param state 目标方块状态
     * @return 如果可以采集返回 true
     */
    [[nodiscard]] bool canHarvestBlock(const BlockState& state) const;

    /**
     * @brief 攻击目标实体
     *
     * 玩家使用当前手持物品攻击目标实体。
     *
     * 包含完整的攻击逻辑：
     * - 攻击冷却伤害衰减
     * - 暴击判定
     * - 击退计算
     * - 横扫攻击
     * - 火焰附加
     * - 饱食度消耗
     *
     * @param target 目标实体
     */
    virtual void attack(Entity& target);

    /**
     * @brief 应用额外击退（冲刺击退/攻击击退）
     *
     * 重写 LivingEntity::causeExtraKnockback()，添加 ServerPlayer 目标的特殊处理：
     * 当目标是 ServerPlayer 且 hurtMarked 为 true 时，立即发送 EntityVelocityPacket
     * 并重置 hurtMarked，然后恢复 preHurtVelocity，避免疾跑击退导致速度重复应用。
     *
     * @param target 击退目标实体
     * @param strength 额外击退强度（包含冲刺加成和附魔击退）
     * @param preHurtVelocity 目标在 hurt() 调用之前的速度（用于 ServerPlayer 速度修正）
     */
    void causeExtraKnockback(Entity& target, f32 strength, const Vector3& preHurtVelocity) override;

    /**
     * @brief 与实体交互
     *
     * 玩家右键点击实体时调用，处理实体交互和物品交互。
     *
     * 交互流程：
     * 1. 旁观者模式：只能打开命名容器
     * 2. 先调用实体的 processInitialInteract() 方法
     * 3. 如果实体不处理，尝试物品的 interactWithEntity()
     *
     * @param target 目标实体
     * @param hand 使用的手
     * @return 交互结果类型
     */
    ActionResultType interactOn(Entity& target, Hand hand);

    // ========== 重生 ==========

    /**
     * @brief 重生时的状态重置
     *
     * 重置玩家状态到初始值：
     * - 生命值恢复到最大
     * - 饥饿值恢复
     * - 清除睡眠状态
     * - 重置姿态为站立
     * - 重置经验
     *
     * 注意：此方法不处理重生位置确定。
     * 服务端重生流程：
     * 1. 调用 determineRespawnPosition() 确定位置
     * 2. 调用 respawn() 重置状态
     * 3. 传送到重生位置
     */
    void respawn();

    // ========== 物理/移动 ==========

    /**
     * @brief 移动物理处理
     *
     * 覆盖 LivingEntity::travel()，添加飞行和游泳处理。
     */
    void travel(f32 strafing, f32 vertical, f32 forward) override;

    /**
     * @brief AI步进更新
     *
     * 覆盖 LivingEntity::aiStep()，替换为玩家输入处理。
     */
    void aiStep() override;

    /**
     * @brief 注册属性
     *
     * 覆盖 LivingEntity::registerAttributes()，注册玩家特有属性。
     */
    void registerAttributes() override;

    /**
     * @brief 序列化玩家额外数据到 NBT
     *
     * 写入玩家特有字段：游戏模式、食物数据、经验、能力、背包、
     * 冲量上下文等。调用 LivingEntity 基类实现后追加自身数据。
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 从 NBT 反序列化玩家额外数据
     *
     * 读取玩家特有字段，调用 LivingEntity 基类实现后读取自身数据。
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /**
     * @brief 更新原版视野晃动强度
     */
    void _updateCameraYaw();

    /**
     * @brief 根据当前游戏模式刷新创造模式交互距离修饰符
     *
     * 对应 MC 1.21.11 ServerPlayer.updatePlayerAttributes()。
     * 创造模式时为 BLOCK_INTERACTION_RANGE 和 ENTITY_INTERACTION_RANGE 添加 +0.5/+2.0 的 Addition 修饰符；
     * 非创造模式时移除这两个修饰符。
     *
     * 该方法具有幂等性：内部先 removeModifier 再 addModifier，可安全重复调用。
     * 在 setGameMode() 中调用以响应模式切换；在 readAdditionalSaveData() 末尾调用以
     * 修正存档中可能残留的旧修饰符状态（属性 NBT 在基类 readAdditionalSaveData 中加载，
     * 此时若存档来自创造模式玩家，修饰符已被持久化到 NBT 中）。
     */
    void _applyCreativeInteractionRangeModifiers();

    /**
     * @brief 按当前缓存输入向速度添加玩家加速度
     *
     * 该方法只应由固定 20TPS 的物理更新调用，避免渲染帧率影响玩家速度。
     */
    void _applyCachedMovementInput(f32 groundSlipperiness);

    /**
     * @brief 处理水中移动
     */
    void _handleWaterMovement(f32 forward, f32 strafe, bool jumping, bool sneaking);

    /**
     * @brief 处理岩浆中移动
     */
    void _handleLavaMovement(f32 forward, f32 strafe, bool jumping, bool sneaking);

    /**
     * @brief 应用移动速度修正
     */
    void _applyMovementSpeed(f32& speed, bool sneaking) const;

    /**
     * @brief 获取当前脚下方块的滑度
     *
     * 用于复刻地面摩擦公式：脚下方块滑度乘以 0.91。
     * @return 脚下方块滑度；没有世界或方块数据时返回默认滑度 0.6
     */
    [[nodiscard]] f32 _groundSlipperiness() const;

    /**
     * @brief 重置过小的速度为零
     */
    void _clampMotion();

    /**
     * @brief 检查玩家是否能以指定姿态容纳在当前位置
     * @param pose 目标姿态
     * @return 如果当前位置没有阻挡则返回 true
     */
    [[nodiscard]] bool _canFitPose(EntityPose pose) const;

    /**
     * @brief 应用风爆附魔效果
     *
     * 在重锤砸地攻击命中后，根据风爆附魔等级创建风爆效果。
     * 风爆会对爆炸范围内的实体施加定向击退（含爆炸保护减免），
     * 并播放风爆音效和粒子效果。不造成伤害，不破坏方块。
     *
     * 对应 MC Java 的 ExplodeEffect + TRIGGER 爆炸模式。
     *
     * @param windBurstLevel 风爆附魔等级（1-3）
     */
    void _applyWindBurstEffect(i32 windBurstLevel);

    /**
     * @brief 计算风爆爆炸中心到实体碰撞箱的视线遮挡比例
     *
     * 在实体碰撞箱内均匀采样点，射线检测是否有方块遮挡爆炸中心。
     * 返回值 0.0 表示完全遮挡，1.0 表示完全可见。
     *
     * @param entityBox 实体碰撞箱
     * @param center 爆炸中心位置
     * @return 遮挡比例 (0.0 - 1.0)
     */
    [[nodiscard]] f32 _calculateWindBurstSeenPercent(const AxisAlignedBB& entityBox, const Vector3& center) const;

    std::string m_username;
    PlayerId m_playerId = 0;
    GameMode m_gameMode = GameMode::Survival;
    i32 m_permissionLevel = 0; ///< 权限等级 (0-4)，对应 OP 等级
    ChatVisibility m_chatVisibility = ChatVisibility::Full;
    u8 m_playerModelParts = PLAYER_MODEL_PARTS_ALL_MASK;

    FoodStats m_foodStats;
    PlayerAbilities m_abilities;
    PlayerInventory m_inventory{this};               // 玩家背包
    PlayerEnderChestInventory m_enderChestInventory; // 末影箱物品栏
    AbstractContainerMenu* m_openContainerMenu = nullptr;

    // m_score 已迁入 ecs::PlayerScoreComponent（真相源），DATA_PLAYER_SCORE_PARAM 退为同步镜像。
    // 经 getScore()/setScore()/increaseScore() 读写（见 m_entityContext->tryGetComponent）。

    // 经验管理器（唯一数据源）
    std::unique_ptr<entity::experience::ExperienceManager> m_experienceManager;

    // XP 冷却（拾取经验球的延迟）
    i32 m_xpCooldown = 0;

    // 物品冷却追踪器（紫颂果、末影珍珠、盾牌等）
    CooldownTracker m_cooldownTracker;

    bool m_isSprinting = false;
    bool m_isSneaking = false;
    bool m_isSwimming = false;
    bool m_isSleeping = false;

    f32 m_inputForward = 0.0f;
    f32 m_inputStrafe = 0.0f;
    bool m_inputJumping = false;
    bool m_inputSneaking = false;

    // 服务端权威的最近客户端输入位掩码（对齐 Java Player.lastClientInput）。
    u8 m_lastClientInput = 0;

    i32 m_sleepTimer = 0;

    // 睡眠位置（当前睡眠的床位）
    std::optional<BlockPos> m_sleepingPosition;

    // 重生点（床或重生锚设置的位置）
    std::optional<GlobalPos> m_spawnPoint;
    bool m_spawnForced = false; // 是否强制重生点

    // 进入下界时的位置（用于进度触发器 nether_travel）
    std::optional<Vector3d> m_enteredNetherPosition;

    // 上次死亡位置（用于追溯指南针和存档持久化）
    std::optional<GlobalPos> m_lastDeathLocation;

    // 自动跳跃系统
    entity::movement::AutoJump m_autoJump;

    // 游泳动画
    f32 m_swimAnimation = 0.0f;
    f32 m_prevSwimAnimation = 0.0f;

    // 入水/出水状态追踪（用于溅水声）
    bool m_wasInWater = false;

    // 视野晃动
    Vector3 m_moveDistanceSamplePosition{0.0f, 0.0f, 0.0f}; // 上次步距采样位置
    f32 m_moveDistanceWalked = 0.0f;                        // distanceWalkedModified 等价累计
    f32 m_prevMoveDistanceWalked = 0.0f;                    // 上一 tick distanceWalkedModified
    f32 m_moveDistanceSwam = 0.0f;                          // 游泳距离累计
    f32 m_prevMoveDistanceSwam = 0.0f;                      // 上一帧游泳距离
    f32 m_cameraYaw = 0.0f;                                 // 原版 cameraYaw
    f32 m_prevCameraYaw = 0.0f;                             // 原版 prevCameraYaw

    // 脚步声触发
    f32 m_distanceWalkedOnStep = 0.0f; // 用于触发脚步声的行走距离
    f32 m_nextStepDistance = 1.0f;     // 下一次脚步声触发的距离阈值

    // 脚步声/游泳声状态（供客户端读取）
    bool m_shouldPlayStepSound = false; // 是否应该播放脚步声
    bool m_shouldPlaySwimSound = false; // 是否应该播放游泳声
    f32 m_swimSoundVolume = 0.0f;       // 游泳声音量
    BlockPos m_stepSoundPos;            // 脚步声位置

    // 攻击冷却系统
    i32 m_ticksSinceLastAttack = 0;   // 上次攻击后的 tick 数
    f32 m_offHandAttackChance = 0.0f; // 副手攻击概率（双持武器用）

    // 物品切换缩放系统（对应 MC Player.itemSwapTicker + lastItemInMainHand）
    // itemSwapTicker 每 tick 递增，仅在主手物品种类切换时重置为 0；
    // getItemSwapScale 据此计算装备动画的“举起”进度，与攻击冷却解耦。
    i32 m_itemSwapTicker = 0;
    ItemStack m_lastItemInMainHand;

    // 钓鱼系统
    EntityInstanceId m_fishingBobber = 0; // 当前投掷的钓鱼浮标实体ID，0表示未投掷

    // 冲量坠落伤害免疫上下文
    // 当玩家执行重锤砸地攻击或被风弹爆炸击中时，这些字段记录冲量上下文，
    // 用于减免从冲量冲击位置以下的坠落伤害。
    // 冲量上下文字段已通过 addAdditionalSaveData/readAdditionalSaveData 和 PlayerSaveData 实现持久化。
    // 注意：m_currentExplosionCause 不序列化到 NBT（MC Java 中为运行时瞬时引用，不持久化）。
    std::optional<Vector3> m_currentImpulseImpactPos;  ///< 冲量冲击位置（砸地/爆炸位置）
    EntityInstanceId m_currentExplosionCause = 0;      ///< 引起冲量的实体ID（用于进度触发，运行时瞬时状态，不持久化）
    bool m_ignoreFallDamageFromCurrentImpulse = false; ///< 是否忽略当前冲量的坠落伤害
    i32 m_currentImpulseContextResetGraceTime = 0;     ///< 冲量上下文重置宽限期（tick）

    // 旁观者跟踪系统
    // 当前旁观目标实体ID。std::nullopt 表示正常视角（摄像机跟踪自身）。
    // 在旁观者模式下，玩家的视角将跟随目标实体的位置和旋转。
    std::optional<EntityInstanceId> m_cameraEntityId;

    // 幽匿尖啸体警告追踪系统
    // 记录玩家在深暗之域中被幽匿尖啸体警告的等级和冷却时间
    entity::WardenWarningEffect m_wardenWarningEffect;

    // ============================================================================
    // 同步数据参数（对齐 vanilla 1.21.11 Player/Avatar.defineId，id 15..20）
    // ============================================================================
    // 扁平化方案（同 BoatEntity）：项目无 vanilla Avatar 中间层，故在 Player::registerData
    // 内先注册 Avatar 两字段再注册 Player 四字段，classInfo parent 指 LivingEntity，继承链
    // 分配器据此续接 LivingEntity id14 之后，得到与 vanilla 逐字段一致的 wire id。
    //
    // Avatar(id15-16, vanilla net.minecraft.world.entity.player.Avatar):
    //   DATA_PLAYER_MAIN_HAND    HumanoidArm(38)  默认 RIGHT(1)
    //   DATA_PLAYER_MODE_CUSTOMISATION Byte(0)    默认 127（PLAYER_MODEL_PARTS_ALL_MASK）
    // Player(id17-20, vanilla net.minecraft.world.entity.player.Player):
    //   DATA_PLAYER_ABSORPTION       Float(3)     默认 0.0F
    //   DATA_PLAYER_SCORE            Int(1)       默认 0
    //   DATA_PLAYER_SHOULDER_PARROT_LEFT  OptionalUnsignedInt(19)  默认 absent
    //   DATA_PLAYER_SHOULDER_PARROT_RIGHT OptionalUnsignedInt(19)  默认 absent
    static entity::DataParameter<entity::HumanoidArmValue> DATA_PLAYER_MAIN_HAND_PARAM;
    static entity::DataParameter<i8> DATA_PLAYER_MODE_CUSTOMISATION_PARAM;
    static entity::DataParameter<f32> DATA_PLAYER_ABSORPTION_PARAM;
    static entity::DataParameter<i32> DATA_PLAYER_SCORE_PARAM;
    static entity::DataParameter<entity::OptionalUnsignedIntValue> DATA_PLAYER_SHOULDER_PARROT_LEFT_PARAM;
    static entity::DataParameter<entity::OptionalUnsignedIntValue> DATA_PLAYER_SHOULDER_PARROT_RIGHT_PARAM;

    /// 本类继承链标识（parent = LivingEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();
};

} // namespace mc
