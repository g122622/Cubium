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
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class DamageSource;

/**
 * @brief 苦力怕实体
 *
 * 会爆炸的敌对生物。
 *
 * 特性：
 * - 爆炸：靠近玩家时会爆炸
 * - 闪烁：爆炸前会闪烁
 * - 害怕猫：会被猫吓跑
 * - 雷击：被雷击中变成高压苦力怕
 * - 打火石：可用打火石点燃
 */
class CreeperEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    CreeperEntity(EntityId id);
    ~CreeperEntity() noexcept override = default;

    // 禁止拷贝
    CreeperEntity(const CreeperEntity&) = delete;
    CreeperEntity& operator=(const CreeperEntity&) = delete;

    // 允许移动
    CreeperEntity(CreeperEntity&&) = delete;
    CreeperEntity& operator=(CreeperEntity&&) = delete;

    /**
     * @brief 创建苦力怕实体
     * @param world 世界实例
     * @return 新的苦力怕实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    // ========== 爆炸系统 ==========

    /**
     * @brief 获取苦力怕状态
     * @return -1 = idle, 1 = igniting/fusing
     */
    [[nodiscard]] i32 getCreeperState() const;

    /**
     * @brief 设置苦力怕状态
     * @param state -1 = idle, 1 = igniting/fusing
     */
    void setCreeperState(i32 state);

    /**
     * @brief 获取点燃时间（已点燃的持续时间）
     */
    [[nodiscard]] i32 getTimeSinceIgnited() const { return m_timeSinceIgnited; }

    /**
     * @brief 获取上一次点燃时间（用于渲染插值）
     */
    [[nodiscard]] i32 getLastActiveTime() const { return m_lastActiveTime; }

    /**
     * @brief 获取点燃时间配置
     */
    [[nodiscard]] i32 getFuseTime() const { return m_fuseTime; }

    /**
     * @brief 设置点燃时间配置
     */
    void setFuseTime(i32 time) { m_fuseTime = time; }

    /**
     * @brief 是否已被点燃
     */
    [[nodiscard]] bool hasIgnited() const { return m_ignited; }

    /**
     * @brief 点燃苦力怕
     */
    void ignite();

    // ========== 高压 ==========

    /**
     * @brief 是否是高压苦力怕
     */
    [[nodiscard]] bool isPowered() const { return m_powered; }

    /**
     * @brief 设置高压状态
     */
    void setPowered(bool powered) { m_powered = powered; }

    // ========== 爆炸 ==========

    /**
     * @brief 引爆炸药
     */
    void explode();

    /**
     * @brief 获取爆炸威力
     */
    [[nodiscard]] f32 getExplosionPower() const { return m_powered ? POWERED_EXPLOSION_POWER : NORMAL_EXPLOSION_POWER; }

    /**
     * @brief 获取爆炸半径
     */
    [[nodiscard]] i32 getExplosionRadius() const { return m_explosionRadius; }

    /**
     * @brief 设置爆炸半径
     */
    void setExplosionRadius(i32 radius) { m_explosionRadius = radius; }

    // ========== 头颅掉落 ==========

    /**
     * @brief 是否能够导致头颅掉落
     */
    [[nodiscard]] bool ableToCauseSkullDrop() const;

    /**
     * @brief 增加已掉落的头颅数量
     */
    void incrementDroppedSkulls() { ++m_droppedSkulls; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 苦力怕不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.54f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 1.7f; }

    // ========== 生命周期 ==========

    void tick() override;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 获取环境音效
     *
     * 苦力怕无环境音，对齐原版 Creeper（不 override getAmbientSound → Mob 默认 null），
     * 避免默认拼接出不存在的 entity.creeper.ambient（仅有 primed/hurt/death）。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

private:
    // 状态变量
    i32 m_fuseTime = DEFAULT_FUSE_TIME;               // 点燃时间配置（可修改）
    i32 m_explosionRadius = DEFAULT_EXPLOSION_RADIUS; // 爆炸半径（可修改）
    i32 m_timeSinceIgnited = 0;                       // 已点燃时间
    i32 m_lastActiveTime = 0;                         // 上一次点燃时间（渲染插值）
    bool m_ignited = false;                           // 是否被点燃
    bool m_powered = false;                           // 是否是高压苦力怕
    i32 m_droppedSkulls = 0;                          // 已掉落的头颅数量

    // 常量
    static constexpr i32 DEFAULT_FUSE_TIME = 30;         // 默认点燃时间 (1.5秒)
    static constexpr i32 DEFAULT_EXPLOSION_RADIUS = 3;   // 默认爆炸半径
    static constexpr f32 NORMAL_EXPLOSION_POWER = 3.0f;  // 普通爆炸威力
    static constexpr f32 POWERED_EXPLOSION_POWER = 6.0f; // 高压爆炸威力
    static constexpr f32 DETONATE_DISTANCE = 3.0f;       // 触发爆炸距离

    /**
     * @brief 生成滞留药水云（如果有效果）
     */
    void _spawnLingeringCloud();
};

} // namespace mc
