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

#include "../../../../core/Types.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../basic/AnimalEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// Forward declarations
class Player;
class ItemStack;
class DamageSource;
class ItemEntity;

/**
 * @brief 狐狸实体
 *
 * 具有特殊行为的被动动物。
 *
 * 特性：
 * - 信任机制：可喂食建立信任，但不可驯服
 * - 叼物品：会叼起地上的物品
 * - 狩猎：会攻击鸡、兔子等小动物
 * - 跳跃攻击：跳起来攻击（扑击）
 * - 睡觉：白天睡觉，晚上活动
 * - 躲避玩家：野生狐狸会躲避玩家
 * - 多种皮肤：红色、白色（雪地变种）
 * - 幼体：小狐狸
 * - 信任玩家：幼狐信任喂养者
 *
 * 状态标志位：
 * - bit 1 (0x01): 坐下 (sitting)
 * - bit 2 (0x04): 蹲伏 (crouching)
 * - bit 3 (0x08): 感兴趣 (interested) - 盯着目标
 * - bit 4 (0x10): 扑击准备 (pounceReady)
 * - bit 5 (0x20): 睡眠 (sleeping)
 * - bit 6 (0x40): 卡住 (stuck) - 卡在雪中
 * - bit 7 (0x80): 激怒 (foxAggroed) - 攻击状态
 */
class FoxEntity : public AnimalEntity {
public:
    /**
     * @brief 狐狸皮肤类型
     */
    enum class FoxType : u8 {
        Red = 0, // 红色狐狸
        Snow = 1 // 白色狐狸（雪地变种）
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    FoxEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~FoxEntity() noexcept override = default;

    // 禁止拷贝
    FoxEntity(const FoxEntity&) = delete;
    FoxEntity& operator=(const FoxEntity&) = delete;

    // 允许移动
    FoxEntity(FoxEntity&&) = delete;
    FoxEntity& operator=(FoxEntity&&) = delete;

    /**
     * @brief 创建狐狸实体
     * @param world 世界实例
     * @return 新的狐狸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 皮肤类型 ==========

    /**
     * @brief 获取皮肤类型
     */
    [[nodiscard]] FoxType getFoxType() const { return m_foxType; }

    /**
     * @brief 设置皮肤类型
     */
    void setFoxType(FoxType type) { m_foxType = type; }

    // ========== 信任系统 ==========

    /**
     * @brief 是否信任某个玩家
     * @param playerId 玩家ID
     * @return 如果信任返回true
     */
    [[nodiscard]] bool trusts(u64 playerId) const;

    /**
     * @brief 添加信任
     * @param playerId 玩家ID
     */
    void addTrustedPlayer(u64 playerId);

    /**
     * @brief 移除信任
     * @param playerId 玩家ID
     */
    void removeTrustedPlayer(u64 playerId);

    /**
     * @brief 获取第一个信任的玩家
     * @return 信任的玩家ID
     */
    [[nodiscard]] std::optional<u64> getFirstTrustedPlayer() const;

    /**
     * @brief 获取所有信任的玩家列表
     * @return 信任玩家ID列表的常量引用
     */
    [[nodiscard]] const std::vector<u64>& getTrustedPlayers() const { return m_trustedPlayers; }

    // ========== 状态标志位 ==========

    /**
     * @brief 是否正在坐下
     */
    [[nodiscard]] bool isSitting() const;

    /**
     * @brief 设置坐下状态
     */
    void setSitting(bool sitting);

    /**
     * @brief 是否正在蹲伏
     */
    [[nodiscard]] bool isCrouching() const;

    /**
     * @brief 设置蹲伏状态
     */
    void setCrouching(bool crouching);

    /**
     * @brief 获取蹲伏进度量 (0.0 - 3.0)
     */
    [[nodiscard]] f32 crouchAmount() const { return m_crouchAmount; }

    /**
     * @brief 设置蹲伏进度量
     */
    void setCrouchAmount(f32 amount) { m_crouchAmount = amount; }

    /**
     * @brief 是否完全蹲伏 (crouchAmount >= 3.0)
     */
    [[nodiscard]] bool isFullyCrouched() const { return m_crouchAmount >= 3.0f; }

    /**
     * @brief 是否"感兴趣"（盯着目标）
     */
    [[nodiscard]] bool isInterested() const;

    /**
     * @brief 设置"感兴趣"状态
     */
    void setInterested(bool interested);

    /**
     * @brief 是否处于扑击准备状态
     */
    [[nodiscard]] bool isPounceReady() const;

    /**
     * @brief 设置扑击准备状态
     */
    void setPounceReady(bool ready);

    /**
     * @brief 是否正在睡眠
     */
    [[nodiscard]] bool isSleeping() const { return (m_stateFlags & FLAG_SLEEPING) != 0; }

    /**
     * @brief 设置睡眠状态
     */
    void setSleeping(bool sleeping);

    /**
     * @brief 是否卡住（卡在雪中）
     */
    [[nodiscard]] bool isStuck() const;

    /**
     * @brief 设置卡住状态
     */
    void setStuck(bool stuck);

    /**
     * @brief 是否激怒状态
     */
    [[nodiscard]] bool isFoxAggroed() const;

    /**
     * @brief 设置激怒状态
     */
    void setFoxAggroed(bool aggroed);

    // ========== 叼物品 ==========

    /**
     * @brief 是否叼着物品
     */
    [[nodiscard]] bool isHoldingItem() const;

    /**
     * @brief 获取叼着的物品（主手）
     */
    [[nodiscard]] const ItemStack* getHeldItem() const { return m_heldItem.get(); }

    /**
     * @brief 设置叼着的物品
     */
    void setHeldItem(std::unique_ptr<ItemStack> item);

    /**
     * @brief 获取手持物品（实现 ItemHolder 接口）
     * @param hand 手
     * @return 物品堆
     */
    [[nodiscard]] ItemStack getHeldItem(Hand hand) const;

    /**
     * @brief 设置手持物品
     * @param hand 手
     * @param stack 物品堆
     */
    void setHeldItem(Hand hand, ItemStack stack);

    /**
     * @brief 丢弃叼着的物品
     */
    void dropHeldItem();

    /**
     * @brief 吐出物品（沿视线方向前方生成物品实体）
     *
     * 与 dropHeldItem 不同，spitOutItem 会在视线方向前方生成物品，
     * 并带有 40 tick 拾取延迟，同时播放吐出音效。
     *
     * @param stack 要吐出的物品堆
     */
    void spitOutItem(const ItemStack& stack);

    /**
     * @brief 判断是否可以持握指定物品
     *
     * 主手为空时可拾取任何物品；当正在进食时（ticksSinceEaten > 0），
     * 只有新物品是食物而当前物品不是食物时才替换。
     *
     * @param stack 要检查的物品堆
     * @return 是否可以持握
     */
    [[nodiscard]] bool canHoldItem(const ItemStack& stack) const override;

    /**
     * @brief 判断物品是否是可食用的食物
     *
     * 判断物品是否同时满足食物条件（用于狐狸食用逻辑）。
     * 与 isBreedingItem 不同，isBreedingItem 只检查甜浆果和发光浆果，
     * 而 isConsumableFood 检查更通用的食物属性。
     *
     * @param stack 物品堆
     * @return 是否是可食用食物
     */
    [[nodiscard]] bool isConsumableFood(const ItemStack& stack) const;

    /**
     * @brief 判断当前是否可以吃食物
     *
     * 条件：物品是可食用食物、没有攻击目标、在地面上、不在睡觉
     */
    [[nodiscard]] bool canEat() const;

    /**
     * @brief 拾取物品实体
     *
     * 狐狸拾取地上的物品。如果物品堆叠数 > 1，
     * 多余的丢到地上，只拿1个放入主手。
     * 拾取前会先吐出当前手持物品。
     *
     * @param itemEntity 要拾取的物品实体
     */
    void pickUpItem(class ItemEntity& itemEntity) override;

    // ========== 行为辅助方法 ==========

    /**
     * @brief 是否可以行动
     * 条件：非坐下、非蹲伏、非睡眠、非卡住、非激怒
     */
    [[nodiscard]] bool canAct() const;

    /**
     * @brief 重置所有状态
     */
    void resetAllStates();

    /**
     * @brief 唤醒（停止睡眠、坐下等）
     */
    void wakeUp();

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
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
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.2f : 0.4f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 白狐使用不同的叫声
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 播放睡觉音效
     */
    void playSleepSound();

    /**
     * @brief 播放嗅探音效
     */
    void playSniffSound();

    /**
     * @brief 播放咬音效
     */
    void playBiteSound();

    /**
     * @brief 播放进食音效
     */
    void playEatSound();

    /**
     * @brief 播放吐出物品音效
     */
    void playSpitSound();

    /**
     * @brief 播放尖叫音效（白狐专用）
     */
    void playScreechSound();

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 刻更新 ==========
    void tick() override;

private:
    // ========== 状态更新 ==========
    void _updateCrouchAmount();

    // ========== 数据成员 ==========

    // 皮肤类型
    FoxType m_foxType = FoxType::Red;

    // 信任的玩家（最多2个）
    std::vector<u64> m_trustedPlayers;

    // 状态标志位（使用位标志存储）
    u8 m_stateFlags = 0;

    // 蹲伏进度量 (0.0 - 3.0)
    f32 m_crouchAmount = 0.0f;
    f32 m_prevCrouchAmount = 0.0f;

    // 睡眠计时器
    i32 m_sleepTimer = 0;

    // 叼着的物品
    std::unique_ptr<ItemStack> m_heldItem;

    // 拾取食物后的进食计时器（tick）
    // 拾取时重置为 0，每 tick 递增，超过 600 时食用完成
    i32 m_ticksSinceEaten = 0;

    // 常量
    static constexpr size_t MAX_TRUSTED_PLAYERS = 2;

    // 拾取到食用完成所需的最少 tick 数
    static constexpr i32 MIN_TICKS_BEFORE_EAT = 600;

    // 开始播放进食动画的 tick 数（560 ~ 600 期间播放吃音效）
    static constexpr i32 EAT_ANIMATION_START_TICKS = 560;

    // 状态标志位定义
    static constexpr u8 FLAG_SITTING = 0x01;
    static constexpr u8 FLAG_CROUCHING = 0x04;
    static constexpr u8 FLAG_INTERESTED = 0x08;
    static constexpr u8 FLAG_POUNCE_READY = 0x10;
    static constexpr u8 FLAG_SLEEPING = 0x20;
    static constexpr u8 FLAG_STUCK = 0x40;
    static constexpr u8 FLAG_FOX_AGGROED = 0x80;
};

} // namespace mc
