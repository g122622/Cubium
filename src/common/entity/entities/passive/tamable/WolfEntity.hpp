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

#include "TameableEntity.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 狼实体
 *
 * 可驯服的动物，驯服后成为狗。
 *
 * 特性：
 * - 驯服：用骨头驯服
 * - 愤怒：被攻击后反击
 * - 跟随主人：驯服后跟随
 * - 坐下/站起：右键切换
 * - 攻击目标：保护主人
 * - 尾巴角度：表示生命值和心情
 * - 颈圈颜色：驯服后可染色
 */
class WolfEntity : public TameableEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    WolfEntity(EntityId id);
    ~WolfEntity() override = default;

    // 禁止拷贝
    WolfEntity(const WolfEntity&) = delete;
    WolfEntity& operator=(const WolfEntity&) = delete;

    // 允许移动
    WolfEntity(WolfEntity&&) = delete;
    WolfEntity& operator=(WolfEntity&&) = delete;

    /**
     * @brief 创建狼实体
     * @param world 世界实例
     * @return 新的狼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 驯服系统 ==========

    // TODO: Wolf 尚未实现 interactMob() 交互驯服逻辑。
    // 参考鹦鹉 ParrotEntity::interactMob() 的实现模式：
    // 1. 检查 isTameItem()（骨头）
    // 2. 有 1/3 概率驯服成功
    // 3. 成功时调用 setTamed(true) + setOwnerId(player.playerId())
    // 4. 调用 m_world->onTameAnimal(player.playerId(), this) 触发 TameAnimalTrigger 进度检测
    // 5. 广播 TamingSucceeded/TamingFailed 实体状态

    /**
     * @brief 检查物品是否可用于驯服
     * @param itemStack 物品堆
     * @return 如果是骨头返回true
     */
    [[nodiscard]] bool isTameItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于繁殖
     * @param itemStack 物品堆
     * @return 如果是肉类返回true
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查物品是否可用于治疗
     * @param itemStack 物品堆
     * @return 如果是肉类返回true
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const;

    /**
     * @brief 判断此狼是否想要攻击指定目标
     *
     * 狼不会攻击以下目标：
     * - 苦力怕、恶魂、盔甲架
     * - 已驯服的其他驯服动物（包括已驯服的马）
     * - 与自己同主人的其他狼
     * - 主人不能伤害的玩家（PvP保护）
     *
     * @param target 待攻击的目标实体
     * @param owner 此狼的主人（可能为 nullptr）
     * @return true 如果此狼愿意攻击该目标
     */
    [[nodiscard]] bool wantsToAttack(const LivingEntity& target, const LivingEntity* owner) const override;

    // ========== 繁殖 ==========

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 刻更新
     */
    void tick() override;

    // ========== 愤怒系统 ==========

    /**
     * @brief 获取尾巴角度
     * @return 尾巴角度（弧度）
     *
     * 尾巴角度基于生命值：
     * - 满血时约40度
     * - 低血量时约-10度
     */
    [[nodiscard]] f32 getTailAngle() const;

    /**
     * @brief 检查是否感兴趣（乞求食物）
     * @return 如果正在乞求食物返回true
     */
    [[nodiscard]] bool isInterested() const { return m_interested; }

    /**
     * @brief 设置感兴趣状态
     * @param interested 是否感兴趣
     */
    void setInterested(bool interested) { m_interested = interested; }

    // ========== 颈圈颜色 ==========

    /**
     * @brief 获取颈圈颜色
     * @return 颜色ID（0-15）
     */
    [[nodiscard]] u8 getCollarColor() const { return m_collarColor; }

    /**
     * @brief 设置颈圈颜色
     * @param color 颜色ID（0-15）
     */
    void setCollarColor(u8 color) { m_collarColor = color; }

    // ========== 水域行为 ==========

    /**
     * @brief 检查是否在水中
     * 狼在水中会减速
     */
    [[nodiscard]] bool isInWater() const override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.4f : 0.8f; }

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.6f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.85f; }

    // ========== 驯服回调 ==========
    void onTamed(bool tamed) override;

    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取声音音量
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 0.4f; }

    /**
     * @brief 播放脚步声音
     */
    void playStepSound(const BlockPos& pos, const BlockState* blockState) override;
    void playStepSound();

    /**
     * @brief 播放甩水声音
     */
    void playShakingSound();

private:
    // 兴趣状态（乞求食物）
    bool m_interested = false;

    // 颈圈颜色（0-15，对应16种染色）
    u8 m_collarColor = 14; // 默认红色

    // 常量
    static constexpr f32 TAIL_ANGLE_HEALTHY = 0.698f;    // 健康时尾巴角度（弧度）
    static constexpr f32 TAIL_ANGLE_UNHEALTHY = -0.175f; // 不健康时尾巴角度（弧度）

    // 声音状态
    bool m_wasInWater = false;
    f32 m_stepSoundDistance = 0.0f;
    f32 m_nextStepSoundDistance = 1.0f;
};

} // namespace mc
