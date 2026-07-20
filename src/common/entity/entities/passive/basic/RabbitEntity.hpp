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
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include <memory>
#include <random>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 兔子实体
 *
 * 小型被动动物，有多种变种。
 *
 * 特性：
 * - 8种皮肤：棕色、白色、黑白斑点、黑色、金色、椒盐色、杀手兔、吐司兔
 * - 快速移动和跳跃
 * - 繁殖：胡萝卜、金胡萝卜、蒲公英
 * - 幼体：小兔子
 * - 杀手兔：敌对变种（彩蛋）
 * - 吐司兔：特殊命名彩蛋
 */
class RabbitEntity : public AnimalEntity {
public:
    /**
     * @brief 兔子皮肤类型
     */
    enum class RabbitType : u8 {
        Brown = 0,         // 棕色兔子
        White = 1,         // 白色兔子
        Black = 2,         // 黑色兔子
        WhiteSpotted = 3,  // 黑白斑点兔子
        Gold = 4,          // 金色兔子
        SaltAndPepper = 5, // 椒盐色兔子
        Toast = 6,         // 吐司兔（命名彩蛋）
        Killer = 99        // 杀手兔（敌对）
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    RabbitEntity(EntityInstanceId id);
    ~RabbitEntity() override = default;

    // 禁止拷贝
    RabbitEntity(const RabbitEntity&) = delete;
    RabbitEntity& operator=(const RabbitEntity&) = delete;

    // 允许移动
    RabbitEntity(RabbitEntity&&) = delete;
    RabbitEntity& operator=(RabbitEntity&&) = delete;

    /**
     * @brief 创建兔子实体
     * @param world 世界实例
     * @return 新的兔子实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 皮肤类型 ==========

    /**
     * @brief 获取皮肤类型
     */
    [[nodiscard]] RabbitType getRabbitType() const { return m_rabbitType; }

    /**
     * @brief 设置皮肤类型
     *
     * 对应 MC 1.21.11 Rabbit.setVariant()：应用变种特定的属性和 AI 目标
     * （杀手兔变种会添加 ARMOR、ATTACK_DAMAGE 修改器和攻击目标）。
     */
    void setRabbitType(RabbitType type);

    /**
     * @brief 随机设置皮肤类型（基于群系）
     */
    void setRandomRabbitType();

    /**
     * @brief 根据群系获取默认的兔子类型
     *
     * 参考 MC 1.21.11 Rabbit.getRandomRabbitVariant：
     * - 雪地群系：80% 白色，20% 白色斑点
     * - 沙漠群系：100% 金色
     * - 其他群系：50% 棕色，40% 椒盐色，10% 黑色
     *
     * @return 基于当前位置群系的兔子类型
     */
    [[nodiscard]] RabbitType getDefaultRabbitTypeForBiome() const;

    /**
     * @brief 是否是杀手兔
     */
    [[nodiscard]] bool isKillerRabbit() const { return m_rabbitType == RabbitType::Killer; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 设置跳跃状态
     *
     * 重写 LivingEntity::setJumping：
     * - jumping=true 时播放跳跃音效（参考 MC 1.21.11 Rabbit.setJumping）
     * - jumping=false 时不做额外处理（跳跃动画的结束由 aiStep() 中 jumpTicks 推进逻辑负责）
     */
    void setJumping(bool jumping) override;

    /**
     * @brief 获取声音类别
     */
    [[nodiscard]] sound::SoundCategory getSoundCategory() const override;

    /**
     * @brief 播放攻击声音
     */
    void playAttackSound(LivingEntity& target) override;

    // ========== 跳跃动画状态机 ==========

    /**
     * @brief 开始一次跳跃动画
     *
     * 对应 MC 1.21.11 Rabbit.startJumping()：
     *   setJumping(true); jumpDuration = 10; jumpTicks = 0;
     *
     * 由 RabbitJumpControl.tick() 在希望跳跃时调用。
     * 幂等保护：若 m_rabbitJumpDuration != 0（动画进行中），则跳过。
     */
    void startJumping();

    /**
     * @brief 获取跳跃动画完成度（0.0 ~ 1.0+）
     *
     * 对应 MC 1.21.11 Rabbit.getJumpCompletion(float partialTick)：
     *   jumpDuration == 0 ? 0.0F : (jumpTicks + partialTick) / jumpDuration
     *
     * 用于客户端模型计算 jumpRotation = sin(completion * PI)。
     *
     * @param partialTick 渲染部分 tick（0.0 ~ 1.0）
     * @return 跳跃动画完成度；若未在跳跃中（jumpDuration==0）返回 0
     */
    [[nodiscard]] f32 getJumpCompletion(f32 partialTick) const;

    /**
     * @brief 当前跳跃动画的 tick 进度（服务端权威值）
     *
     * 对应 MC 1.21.11 Rabbit.jumpTicks 字段。在 aiStep() 中每 tick 递增，
     * 直到等于 jumpDuration 后归零并结束本次跳跃。
     *
     * 注意：与 LivingEntity::m_jumpTicks（跳跃冷却，不同语义）不同，
     * 此处使用独立的 m_rabbitJumpTicks 字段以避免语义冲突。
     */
    [[nodiscard]] i32 rabbitJumpTicks() const { return m_rabbitJumpTicks; }

    /**
     * @brief 当前跳跃动画的总持续 tick 数
     *
     * 对应 MC 1.21.11 Rabbit.jumpDuration 字段。startJumping() 时设为 10，
     * 跳跃结束后归零。客户端收到 RabbitJump 状态时也设为 10。
     */
    [[nodiscard]] i32 rabbitJumpDuration() const { return m_rabbitJumpDuration; }

    // ========== 着陆延迟 / 跳跃控制 ==========

    /**
     * @brief 当前着陆延迟剩余 tick 数
     *
     * 对应 MC 1.21.11 Rabbit.jumpDelayTicks 字段。
     * 着陆后设置为 10（慢速）或 1（快速），每 tick 递减。
     * 为 0 时允许下一次跳跃。
     */
    [[nodiscard]] i32 jumpDelayTicks() const { return m_jumpDelayTicks; }

    /**
     * @brief 上一 tick 是否在地面
     *
     * 对应 MC 1.21.11 Rabbit.wasOnGround 字段。用于检测着陆瞬间。
     */
    [[nodiscard]] bool wasOnGround() const { return m_wasOnGround; }

    /**
     * @brief moreCarrotTicks（啃食胡萝卜冷却）
     *
     * 对应 MC 1.21.11 Rabbit.moreCarrotTicks 字段。
     * RaidGardenGoal 啃食胡萝卜后设为 40，customServerAiStep 中随机递减。
     * 为 0 时表示兔子想要更多食物（wantsMoreFood() 返回 true）。
     */
    [[nodiscard]] i32 moreCarrotTicks() const { return m_moreCarrotTicks; }

    /**
     * @brief 设置 moreCarrotTicks（供 RaidGardenGoal 使用）
     */
    void setMoreCarrotTicks(i32 ticks) { m_moreCarrotTicks = ticks; }

    /**
     * @brief 是否想要更多食物
     *
     * 对应 MC 1.21.11 Rabbit.wantsMoreFood()：moreCarrotTicks <= 0
     */
    [[nodiscard]] bool wantsMoreFood() const { return m_moreCarrotTicks <= 0; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.2f : 0.35f; }

    /**
     * @brief AI 步进重写
     *
     * 对应 MC 1.21.11 Rabbit.aiStep()：
     *   super.aiStep();
     *   if (jumpTicks != jumpDuration) jumpTicks++;
     *   else if (jumpDuration != 0) { jumpTicks = 0; jumpDuration = 0; setJumping(false); }
     *
     * 推进跳跃动画计时器；动画结束时清除跳跃状态。
     */
    void aiStep() override;

    /**
     * @brief 服务端 AI 主循环（对应 MC customServerAiStep）
     *
     * 对应 MC 1.21.11 Rabbit.customServerAiStep(ServerLevel)：
     * - 递减 jumpDelayTicks
     * - 随机递减 moreCarrotTicks
     * - 着陆检测：从空中到地面的过渡时清除跳跃状态并设置着陆延迟
     * - 杀手兔的主动跳跃攻击
     * - 普通兔子的跳跃触发（有移动目标且 jumpDelayTicks==0 时 startJumping）
     * - 跳跃控制器的 enable/disable 管理
     *
     * 项目架构中 MobEntity::updateAITasks() 是空的虚方法，作为 customServerAiStep
     * 的等价入口点（在 MobEntity::tick() 中于 goalSelector/navigator 之后、控制器之前调用）。
     */
    void updateAITasks() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 尺寸 ==========

    [[nodiscard]] f32 getBaseWidth() const override { return 0.4f; }
    [[nodiscard]] f32 getBaseHeight() const override { return 0.5f; }

private:
    // 皮肤类型
    RabbitType m_rabbitType = RabbitType::Brown;

    // 跳跃动画状态（对应 MC 1.21.11 Rabbit 的 jumpTicks / jumpDuration）
    // 注意：不与 LivingEntity::m_jumpTicks（跳跃冷却）复用，语义不同
    i32 m_rabbitJumpTicks = 0;    // 当前跳跃已持续的 tick
    i32 m_rabbitJumpDuration = 0; // 当前跳跃总持续 tick；为 0 表示未在跳跃中

    // 着陆延迟 / 跳跃控制状态（对应 MC 1.21.11 Rabbit 的 jumpDelayTicks / wasOnGround）
    i32 m_jumpDelayTicks = 0;   // 着陆后禁止跳跃的剩余 tick
    bool m_wasOnGround = false; // 上一 tick 是否在地面

    // 啃食胡萝卜冷却（对应 MC 1.21.11 Rabbit.moreCarrotTicks）
    // RaidGardenGoal 啃食后设为 40，updateAITasks() 中随机递减
    i32 m_moreCarrotTicks = 0;

    // ========== 私有辅助方法 ==========

    /**
     * @brief 启用跳跃控制器（对应 MC Rabbit.enableJumpControl()）
     */
    void enableJumpControl();

    /**
     * @brief 禁用跳跃控制器（对应 MC Rabbit.disableJumpControl()）
     */
    void disableJumpControl();

    /**
     * @brief 设置着陆延迟（对应 MC Rabbit.setLandingDelay()）
     *
     * 移动速度倍率 < 2.2 时延迟 10 tick，否则延迟 1 tick。
     */
    void setLandingDelay();

    /**
     * @brief 检查并应用着陆延迟（对应 MC Rabbit.checkLandingDelay()）
     *
     * 调用 setLandingDelay() 并禁用跳跃控制器。
     */
    void checkLandingDelay();

    /**
     * @brief 朝向指定坐标（对应 MC Rabbit.facePoint()）
     * @param targetX 目标 X
     * @param targetZ 目标 Z
     */
    void facePoint(f64 targetX, f64 targetZ);

    /**
     * @brief 应用兔子变种（对应 MC Rabbit.setVariant()）
     *
     * 杀手兔（EVIL）变种：设置 ARMOR=8、添加 ATTACK_DAMAGE 修改器 +5、
     * 注册近战攻击目标和反击目标。非杀手兔变种：移除 EVIL 修改器。
     *
     * 注意：项目当前由 setRabbitType() 调用，因为 m_rabbitType 是项目原生枚举。
     */
    void applyRabbitType(RabbitType newType);

    /// 杀手兔 ATTACK_DAMAGE 修改器 ID（对应 MC EVIL_ATTACK_POWER_MODIFIER）
    static constexpr const char* EVIL_ATTACK_POWER_MODIFIER_ID = "rabbit_evil_attack_power";

    /// moreCarrotTicks 重置值（对应 MC MORE_CARROTS_DELAY = 40）
    static constexpr i32 MORE_CARROTS_DELAY = 40;
};

} // namespace mc
