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
#include "../MonsterEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class IWorld;
class DamageSource;
class LivingEntity;

/**
 * @brief 史莱姆实体
 *
 * 弹跳的绿色果冻状怪物。
 *
 * 特性：
 * - 分裂：被杀死时分裂成小史莱姆
 * - 弹跳：持续弹跳移动
 * - 尺寸：有4种尺寸（微小、小、中、大）
 * - 掉落：粘液球（仅小尺寸）
 * - 生成：只在特定区块
 */
class SlimeEntity : public MonsterEntity {
public:
    using Entity::onCollideWithPlayer;

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    SlimeEntity(EntityInstanceId id);
    ~SlimeEntity() override = default;

    // 禁止拷贝
    SlimeEntity(const SlimeEntity&) = delete;
    SlimeEntity& operator=(const SlimeEntity&) = delete;

    // 允许移动
    SlimeEntity(SlimeEntity&&) = delete;
    SlimeEntity& operator=(SlimeEntity&&) = delete;

    /**
     * @brief 创建史莱姆实体
     * @param world 世界实例
     * @return 新的史莱姆实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override
    {
        return std::nullopt; // 史莱姆无环境音效
    }

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取挤压声音
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getSquishSound() const;

    /**
     * @brief 获取跳跃声音
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getJumpSound() const;

    // ========== 粒子 ==========

    /**
     * @brief 获取着地粒子类型
     * 史莱姆返回 ITEM_SLIME，岩浆怪返回 FLAME
     */
    [[nodiscard]] virtual particle::ParticleTypeId getSquishParticle() const;

    // ========== 尺寸系统 ==========

    /**
     * @brief 获取史莱姆尺寸
     * 尺寸范围 1-4，1=微小，2=小，4=大
     */
    [[nodiscard]] i32 getSlimeSize() const { return m_size; }

    /**
     * @brief 设置史莱姆尺寸
     * @param size 尺寸（1-4）
     * @param resetHealth 是否重置生命值
     */
    virtual void setSlimeSize(i32 size, bool resetHealth = true);

    /**
     * @brief 是否是小史莱姆
     */
    [[nodiscard]] bool isSmallSlime() const { return m_size <= 1; }

    /**
     * @brief 是否可以对玩家造成伤害
     */
    [[nodiscard]] virtual bool canDamagePlayer() const;

    // ========== 挤压动画 ==========

    /**
     * @brief 获取挤压量
     */
    [[nodiscard]] f32 squishAmount() const { return m_squishAmount; }

    /**
     * @brief 设置挤压量
     * @brief 供子类使用（如岩浆怪）
     */
    void setSquishAmount(f32 amount) { m_squishAmount = amount; }

    /**
     * @brief 获取挤压因子
     */
    [[nodiscard]] f32 squishFactor() const { return m_squishFactor; }

    /**
     * @brief 获取上一帧挤压因子
     */
    [[nodiscard]] f32 prevSquishFactor() const { return m_prevSquishFactor; }

    // ========== 弹跳 ==========

    /**
     * @brief 获取跳跃延迟
     * 返回 10-29 tick（0.5-1.45秒）
     */
    [[nodiscard]] virtual i32 getJumpDelay() const;

    /**
     * @brief 跳跃时是否发出声音
     */
    [[nodiscard]] bool makesSoundOnJump() const { return m_size > 0; }

    // ========== 分裂 ==========

    /**
     * @brief 分裂成小史莱姆
     * @deprecated 使用 performSplit() 替代
     */
    void split();

    /**
     * @brief 执行分裂逻辑
     * 在实体被移除时生成 2-4 个小史莱姆。
     */
    void performSplit();

    /**
     * @brief 检查是否可以分裂
     */
    [[nodiscard]] bool canSplit() const { return m_size > 1; }

    // ========== 攻击 ==========

    /**
     * @brief 对目标造成伤害
     */
    void dealDamage(LivingEntity& target);

    // ========== 碰撞 ==========

    /**
     * @brief 玩家碰撞处理
     */
    void onCollideWithPlayer(LivingEntity& player);

    // ========== 阳光燃烧 ==========

    /**
     * @brief 史莱姆不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override;

    /**
     * @brief 获取实体尺寸
     */
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;

    /**
     * @brief 获取声音音量
     */
    [[nodiscard]] f32 getSoundVolume() const override { return 0.4f * static_cast<f32>(m_size); }

    /**
     * @brief 获取垂直面部旋转速度
     * 史莱姆不会抬头低头
     */
    [[nodiscard]] f32 getVerticalFaceSpeed() const override { return 0.0f; }

    // ========== 经验值 ==========

    /**
     * @brief 死亡时掉落经验
     * 经验值等于尺寸
     */
    void dropExperience() override;

    // ========== 生命周期 ==========

    /**
     * @brief 移除实体
     * 重写以实现史莱姆分裂逻辑。
     */
    void remove() override;

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 根据尺寸更新属性
     */
    void updateSizeAttributes();

    /**
     * @brief 更新挤压量
     * 子类可重写以改变衰减速率
     */
    virtual void alterSquishAmount();

private:
    // 尺寸
    i32 m_size = 1;

    // 挤压动画
    f32 m_squishAmount = 0.0f;
    f32 m_squishFactor = 0.0f;
    f32 m_prevSquishFactor = 0.0f;

    // 地面状态追踪
    bool m_wasOnGround = false;

    // 尺寸缩放因子
    static constexpr f32 SIZE_SCALE = 0.255f;
    // 眼睛高度因子
    static constexpr f32 EYE_HEIGHT_FACTOR = 0.625f;
    // 分裂最小数量
    static constexpr i32 SPLIT_COUNT_MIN = 2;
    // 分裂最大数量
    static constexpr i32 SPLIT_COUNT_MAX = 4;
};

} // namespace mc
