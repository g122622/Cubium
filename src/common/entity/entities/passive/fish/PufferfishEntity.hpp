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

#include "AbstractFishEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/sound/SoundEvents.hpp"
#include <memory>

namespace mc {

// Forward declaration
namespace entity::ai::goal {
class PuffGoal;
}

/**
 * @brief 河豚实体
 *
 * 有毒的海洋鱼类。
 *
 * 特性：
 * - 膨胀：玩家或敌对生物靠近时会膨胀
 * - 中毒：膨胀状态下接触会导致中毒
 * - 掉落：河豚、骨头
 *
 * 音效：
 * - ENTITY_PUFFER_FISH_AMBIENT: 水中环境音
 * - ENTITY_PUFFER_FISH_BLOW_UP: 膨胀音效
 * - ENTITY_PUFFER_FISH_BLOW_OUT: 收缩音效
 * - ENTITY_PUFFER_FISH_DEATH: 死亡音效
 * - ENTITY_PUFFER_FISH_FLOP: 陆地扑腾音效
 * - ENTITY_PUFFER_FISH_HURT: 受伤音效
 * - ENTITY_PUFFER_FISH_STING: 刺击音效
 */
class PufferfishEntity : public AbstractFishEntity {
public:
    /**
     * @brief 河豚膨胀状态
     */
    enum class PuffState : u8 {
        Deflated = 0,   // 未膨胀
        SemiPuffed = 1, // 半膨胀
        FullyPuffed = 2 // 完全膨胀
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    PufferfishEntity(EntityInstanceId id);
    ~PufferfishEntity() override = default;

    // 禁止拷贝
    PufferfishEntity(const PufferfishEntity&) = delete;
    PufferfishEntity& operator=(const PufferfishEntity&) = delete;

    // 允许移动
    PufferfishEntity(PufferfishEntity&&) = delete;
    PufferfishEntity& operator=(PufferfishEntity&&) = delete;

    /**
     * @brief 创建河豚实体
     * @param world 世界实例
     * @return 新的河豚实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 膨胀状态 ==========

    /**
     * @brief 获取膨胀状态
     *
     * 优先从 DataParameter 读取以获取同步值。
     */
    [[nodiscard]] PuffState getPuffState() const;

    /**
     * @brief 设置膨胀状态
     *
     * 通过 DataParameter 同步到客户端。
     * 当状态变化时会自动播放膨胀/收缩音效并刷新碰撞箱。
     */
    void setPuffState(PuffState state);

    /**
     * @brief 获取膨胀状态 DataParameter ID（客户端同步用）
     */
    [[nodiscard]] static u16 getPuffStateParamId() { return DATA_PUFF_STATE_PARAM.id(); }

    /**
     * @brief 获取膨胀尺寸缩放因子
     *
     * - Deflated: 0.5 (碰撞箱 0.35 x 0.35)
     * - SemiPuffed: 0.7 (碰撞箱 0.49 x 0.49)
     * - FullyPuffed: 1.0 (碰撞箱 0.7 x 0.7)
     *
     * @return 缩放因子
     */
    [[nodiscard]] f32 getPuffSize() const;

    /**
     * @brief 是否完全膨胀
     */
    [[nodiscard]] bool isFullyPuffed() const { return m_puffState == PuffState::FullyPuffed; }

    // ========== 中毒 ==========

    /**
     * @brief 是否会使接触者中毒
     *
     * 在膨胀状态（非 Deflated）时会使接触者中毒。
     */
    [[nodiscard]] bool canPoison() const { return m_puffState != PuffState::Deflated; }

    // ========== 膨胀计时器 ==========

    /**
     * @brief 开始膨胀计时
     *
     * 由 PuffGoal 调用，设置 puffTimer = 1 并重置 deflateTimer = 0。
     */
    void startPuffTimer();

    /**
     * @brief 重置膨胀计时器
     *
     * 由 PuffGoal 调用，设置 puffTimer = 0。
     */
    void resetPuffTimer();

    /**
     * @brief 获取膨胀计时器值
     */
    [[nodiscard]] i32 puffTimer() const { return m_puffTimer; }

    /**
     * @brief 获取收缩计时器值
     */
    [[nodiscard]] i32 deflateTimer() const { return m_deflateTimer; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.15f; }

    /**
     * @brief 获取动态尺寸
     *
     * 根据膨胀状态动态计算碰撞箱尺寸。
     * 基础尺寸 0.7 x 0.7，乘以 getPuffSize() 缩放因子。
     */
    [[nodiscard]] entity::EntitySize getDimensions(EntityPose pose) const override;

    // ========== 音效 ==========

    /**
     * @brief 获取扑腾声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getFlopSound() const override;

    /**
     * @brief 获取环境音效
     * 在水中和陆地播放不同的音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 数据参数注册 ==========
    void registerData() override;

private:
    PuffState m_puffState = PuffState::Deflated;
    i32 m_puffTimer = 0;
    i32 m_deflateTimer = 0;

    /**
     * @brief 膨胀状态 DataParameter（客户端同步）
     */
    static entity::DataParameter<i32> DATA_PUFF_STATE_PARAM;

    // 膨胀状态切换阈值（单位：ticks）
    static constexpr i32 PUFF_SEMI_THRESHOLD = 40;      // 膨胀到半膨胀的阈值
    static constexpr i32 DEFLATE_SEMI_TO_DEFLATE = 100; // 半膨胀到未膨胀的延迟
    static constexpr i32 DEFLATE_FULL_TO_SEMI = 60;     // 完全膨胀到半膨胀的延迟

    /**
     * @brief 检测并攻击附近敌人
     *
     * 在膨胀状态时检测碰撞箱扩展 0.3 格范围内的 MobEntity。
     */
    void _attackNearbyEnemies();

    // PuffGoal 需要访问私有成员
    friend class entity::ai::goal::PuffGoal;
};

} // namespace mc
