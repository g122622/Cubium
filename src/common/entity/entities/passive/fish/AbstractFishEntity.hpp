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

#include "../water/WaterMobEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>

namespace mc {

/**
 * @brief 鱼类实体基类
 *
 * 只保留所有鱼共享的游泳、离水扑腾与基础空气供应语义。
 * 群游逻辑由 AbstractGroupFishEntity 承载。
 *
 * AI 目标:
 * - 优先级 0: PanicGoal(1.25) - 恐慌逃跑
 * - 优先级 2: AvoidEntityGoal(Player, 8.0F, 1.6, 1.4) - 避开玩家
 * - 优先级 4: SwimGoal(1.0, 40) - 随机游泳
 */
class AbstractFishEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造鱼类实体
     * @param id 实体 ID
     * @param registry 实体注册表（ECS）
     */
    AbstractFishEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~AbstractFishEntity() override = default;

    /// 本类继承链标识（parent = WaterMobEntity::classInfo()）。见 Entity::classInfo()。
    // vanilla 1.21.11 AbstractFish 在 Mob(id15) 之后注册 FROM_BUCKET(Boolean,id16)。
    // 本项目保留此层并补齐 registerData+ClassRegisterGuard 对齐 vanilla 字段 id，见
    // entity-sync-alignment-decisions-2026-07。
    static const entity::EntityClassInfo& classInfo();

    AbstractFishEntity(const AbstractFishEntity&) = delete;
    AbstractFishEntity& operator=(const AbstractFishEntity&) = delete;
    AbstractFishEntity(AbstractFishEntity&&) noexcept = delete;
    AbstractFishEntity& operator=(AbstractFishEntity&&) noexcept = delete;

    /**
     * @brief 鱼默认只能在水中生成
     */
    [[nodiscard]] bool canSpawnInWater() const override { return true; }

    /**
     * @brief 默认鱼类不具备群游语义
     */
    [[nodiscard]] virtual bool canSchool() const { return false; }

    /**
     * @brief 当前是否处于游泳状态
     */
    [[nodiscard]] bool isSwimming() const { return m_swimming; }

    /**
     * @brief 设置游泳状态
     */
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 当前游泳朝向
     */
    [[nodiscard]] f32 getSwimAngle() const { return m_swimAngle; }

    /**
     * @brief 设置游泳朝向
     */
    void setSwimAngle(f32 angle) { m_swimAngle = angle; }

    /**
     * @brief 当前是否处于离水扑腾状态
     */
    [[nodiscard]] bool isFlopping() const { return m_flopping; }

    /**
     * @brief 设置离水扑腾状态
     */
    void setFlopping(bool flopping) { m_flopping = flopping; }

    // ========== 桶装鱼支持 ==========

    /**
     * @brief 检查是否来自桶
     *
     * 从桶放出的鱼不会消失。
     *
     * @return 如果是从桶放出的鱼返回 true
     */
    [[nodiscard]] bool isFromBucket() const { return m_fromBucket; }

    /**
     * @brief 设置是否来自桶
     *
     * 当从鱼桶放出鱼时调用此方法设置为 true。
     *
     * @param fromBucket 是否来自桶
     */
    void setFromBucket(bool fromBucket)
    {
        m_fromBucket = fromBucket;
        // 同步到数据管理器（vanilla FROM_BUCKET，业务权威源仍为 m_fromBucket）
        m_dataManager.set(FROM_BUCKET_PARAM, fromBucket);
    }

    // ========== 同步字段 id 访问器（测试/诊断用，抗字段 id 偏移） ==========

    [[nodiscard]] static u16 getFromBucketParamId() { return FROM_BUCKET_PARAM.id(); }

    /**
     * @brief 检查是否应阻止消失
     *
     * 从桶放出的鱼永远不会消失。
     *
     * @return 如果来自桶或正在被骑乘返回 true
     */
    [[nodiscard]] bool preventDespawn() const override { return WaterMobEntity::preventDespawn() || m_fromBucket; }

    /**
     * @brief 检查是否可以消失
     *
     * 从桶放出的鱼或有自定义名称的鱼不会消失。
     *
     * @param distanceToClosestPlayer 到最近玩家的距离
     * @return 如果可以消失返回 true
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const override
    {
        (void)distanceToClosestPlayer;
        return !m_fromBucket && !hasCustomName();
    }

    /**
     * @brief 获取最大空气供应量
     * 鱼类有更长的最大空气时间（480 ticks = 24秒）
     */
    [[nodiscard]] i32 maxAir() const override { return MAX_AIR_SUPPLY; }

    /**
     * @brief 是否可以随机游泳
     *
     * 用于 SwimGoal 的 shouldExecute 检查。
     * 基类默认返回 true，群游鱼类重写为 !hasGroupLeader()
     *
     * @return 如果可以随机游泳返回 true
     */
    [[nodiscard]] virtual bool canRandomSwim() const { return true; }

    /**
     * @brief 获取扑腾声音
     *
     * 子类应重写此方法返回对应的扑腾声音事件。
     *
     * @return 扑腾声音事件，默认返回空
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getFlopSound() const { return std::nullopt; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 注册同步数据参数
     *
     * 重写以注册 vanilla 1.21.11 AbstractFish.FROM_BUCKET(Boolean,id16)。
     * 派生类构造函数必须显式调用 registerData()，参考 MobEntity/WolfEntity 模式。
     */
    void registerData() override;

    /**
     * @brief 更新游泳状态
     */
    void updateSwimming();

    /**
     * @brief 更新离水扑腾状态
     *
     * 触发条件：不在水中 && 在地面 && 垂直碰撞
     * 行为：添加随机水平速度 + 向上速度 0.4
     * 音效：播放扑腾声音
     */
    void updateFlopping();

private:
    bool m_swimming = false;
    f32 m_swimAngle = 0.0f;
    bool m_flopping = false;
    i32 m_flopTimer = 0;
    bool m_fromBucket = false; // 是否来自桶（从桶放出的鱼不会消失）

    // ========== 同步数据参数（vanilla 1.21.11 AbstractFish.FROM_BUCKET，见 registerData） ==========
    static entity::DataParameter<bool> FROM_BUCKET_PARAM; // id16，桶装鱼标志同步镜像

    static constexpr i32 MAX_AIR_SUPPLY = 480;
};

} // namespace mc
