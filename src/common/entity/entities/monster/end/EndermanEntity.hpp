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
#include "../../../interfaces/IAngerable.hpp"
#include "../MonsterEntity.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class BlockState;
class DamageSource;

/**
 * @brief 末影人实体
 *
 * 可以瞬移的中立型怪物。
 *
 * 特性：
 * - 瞬移：被攻击或看眼睛时会瞬移
 * - 搬方块：可以搬起和放置方块
 * - 中立：通常中立，被激怒后攻击
 * - 怕水：接触水会瞬移并受到伤害
 * - 怕雨：在雨中会瞬移并受到伤害
 *
 * 参考 MC 1.16.5 EndermanEntity
 */
class EndermanEntity : public MonsterEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    EndermanEntity(LegacyEntityType type, EntityId id);
    ~EndermanEntity() override = default;

    // 禁止拷贝
    EndermanEntity(const EndermanEntity&) = delete;
    EndermanEntity& operator=(const EndermanEntity&) = delete;

    // 允许移动
    EndermanEntity(EndermanEntity&&) = default;
    EndermanEntity& operator=(EndermanEntity&&) = default;

    /**
     * @brief 创建末影人实体
     * @param world 世界实例
     * @return 新的末影人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效
     * MC 1.16.5: entity.enderman.ambient (愤怒时), entity.enderman.scream (被注视时)
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤声音
     * MC 1.16.5: entity.enderman.hurt
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     * MC 1.16.5: entity.enderman.death
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取 stare sound（被注视时的声音）
     * MC 1.16.5: entity.enderman.stare
     */
    [[nodiscard]] std::optional<ResourceLocation> getStareSound() const;

    /**
     * @brief 获取瞬移声音
     * MC 1.16.5: entity.enderman.teleport
     */
    [[nodiscard]] std::optional<ResourceLocation> getTeleportSound() const;

    // ========== IAngerable接口实现 ==========

    /**
     * @brief 设置攻击目标 (IAngerable接口实现)
     */
    void setAttackTarget(LivingEntity* target) override { m_attackTarget = target; }

    /**
     * @brief 获取攻击目标 (IAngerable接口实现)
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const override { return m_attackTarget; }

    /**
     * @brief 设置复仇目标 (IAngerable接口实现)
     */
    void setRevengeTarget(LivingEntity* target) override;

    /**
     * @brief 获取复仇目标 (IAngerable接口实现)
     */
    [[nodiscard]] LivingEntity* getRevengeTarget() const override;

    /**
     * @brief 获取复仇计时器 (IAngerable接口实现)
     */
    [[nodiscard]] i32 getRevengeTimer() const override { return m_revengeTimer; }

    /**
     * @brief 是否愤怒 (IAngerable接口实现)
     */
    [[nodiscard]] bool isAngry() const override { return m_angry || m_angerTime > 0; }

    /**
     * @brief 设置愤怒状态 (IAngerable接口实现)
     */
    void setAngry(bool angry) override;

    /**
     * @brief 获取愤怒时间 (IAngerable接口实现)
     */
    [[nodiscard]] i32 getAngerTime() const override { return m_angerTime; }

    /**
     * @brief 设置愤怒时间 (IAngerable接口实现)
     */
    void setAngerTime(i32 time) override { m_angerTime = time; }

    // ========== 被注视检测 ==========

    /**
     * @brief 是否正在被玩家注视
     * MC 1.16.5: isScreaming()
     */
    [[nodiscard]] bool isScreaming() const { return m_screaming; }

    /**
     * @brief 设置注视状态
     */
    void setScreaming(bool screaming) { m_screaming = screaming; }

    // ========== 瞬移系统 ==========

    /**
     * @brief 尝试随机瞬移
     * MC 1.16.5: teleport()
     * @return 是否成功瞬移
     */
    bool teleport();

    /**
     * @brief 尝试瞬移到目标附近
     * MC 1.16.5: teleportTowards(Entity)
     */
    bool teleportToTarget();

    /**
     * @brief 尝试瞬移避开水
     */
    bool teleportAwayFromWater();

    // ========== 搬方块系统 ==========

    /**
     * @brief 是否拿着方块
     * MC 1.16.5: hasBlock()
     */
    [[nodiscard]] bool isHoldingBlock() const { return m_holdingBlock; }

    /**
     * @brief 获取拿着的方块状态
     * MC 1.16.5: getHeldBlockState()
     */
    [[nodiscard]] const BlockState* getHeldBlockState() const { return m_heldBlockState; }

    /**
     * @brief 设置拿着的方块状态
     * MC 1.16.5: setHeldBlockState()
     */
    void setHeldBlockState(const BlockState* state);

    /**
     * @brief 放下拿着的方块
     */
    void placeHeldBlock();

    /**
     * @brief 拿起方块
     */
    void pickUpBlock();

    // ========== 阳光燃烧 ==========

    /**
     * @brief 末影人不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 水敏感检测 ==========

    /**
     * @brief 检查是否在水中或雨中
     *
     * MC 1.16.5: Entity.isInWaterOrRainOrBubbleColumn()
     * 对于末影人，气泡柱不会造成伤害，所以只检查水和雨。
     *
     * @return 如果在水中或雨中返回 true
     */
    [[nodiscard]] bool isInWaterOrRain() const;

    // ========== 注视检测 ==========

    /**
     * @brief 检查玩家是否正在注视末影人（应被激怒）
     *
     * MC 1.16.5: EndermanEntity.shouldAttackPlayer()
     * 检查玩家是否正在注视末影人的眼睛：
     * 1. 检查玩家是否戴着南瓜头（南瓜头可避免激怒）
     * 2. 计算玩家视线方向与玩家到末影人向量的点积
     * 3. 根据距离调整阈值
     * 4. 检查视线是否被方块阻挡
     *
     * @param player 目标玩家
     * @return 如果玩家正在注视末影人返回 true
     */
    [[nodiscard]] bool shouldAttackPlayer(const Player& player) const;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     * MC 1.16.5: 2.55f * scale
     */
    [[nodiscard]] f32 eyeHeight() const override { return 2.55f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 2.9f; }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 受到伤害时的处理
     * MC 1.16.5: 攻击后瞬移
     */
    bool hurt(DamageSource& source, f32 amount) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // IAngerable接口
    LivingEntity* m_attackTarget = nullptr;
    std::optional<u64> m_revengeTargetId;
    i32 m_revengeTimer = 0;

    // 愤怒状态
    bool m_angry = false;
    bool m_screaming = false; // 被注视状态
    i32 m_angerTime = 0;

    // 搬方块
    bool m_holdingBlock = false;
    const BlockState* m_heldBlockState = nullptr;

    // 瞬移冷却
    i32 m_teleportCooldown = 0;

    // MC 1.16.5 常量
    static constexpr i32 TELEPORT_COOLDOWN = 50;     // 瞬移冷却 (ticks)
    static constexpr i32 ANGER_DURATION = 600;       // 愤怒持续时间 (ticks)
    static constexpr f32 TELEPORT_RANGE = 64.0f;     // 瞬移范围
    static constexpr f32 WATER_DAMAGE = 1.0f;        // 水伤害
    static constexpr i32 WATER_DAMAGE_INTERVAL = 10; // 水伤害间隔
};

} // namespace mc
