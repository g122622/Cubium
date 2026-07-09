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
#include "../../../core/EntityDataManager.hpp"
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
 */
class EndermanEntity : public MonsterEntity, public entity::IAngerable {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    EndermanEntity(EntityId id);
    ~EndermanEntity() override = default;

    // 禁止拷贝
    EndermanEntity(const EndermanEntity&) = delete;
    EndermanEntity& operator=(const EndermanEntity&) = delete;

    // 禁止移动
    EndermanEntity(EndermanEntity&&) = delete;
    EndermanEntity& operator=(EndermanEntity&&) = delete;

    /**
     * @brief 创建末影人实体
     * @param world 世界实例
     * @return 新的末影人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 声音 ==========

    /**
     * @brief 获取环境音效（愤怒时返回 ambient，被注视时返回 scream）
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
     * @brief 获取 stare sound（被注视时的声音）
     */
    [[nodiscard]] std::optional<ResourceLocation> getStareSound() const;

    /**
     * @brief 获取瞬移声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getTeleportSound() const;

    // ========== IAngerable接口实现 ==========

    /**
     * @brief 设置攻击目标 (IAngerable接口实现)
     */
    void setAttackTarget(LivingEntity* target) override { MobEntity::setAttackTarget(target); }

    /**
     * @brief 获取攻击目标 (IAngerable接口实现)
     */
    [[nodiscard]] LivingEntity* getAttackTarget() const override
    {
        return const_cast<EndermanEntity*>(this)->MobEntity::attackTarget();
    }

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
     *
     * 读取 DATA_SCREAMING_PARAM（网络同步参数）。
     */
    [[nodiscard]] bool isScreaming() const { return m_dataManager.get<bool>(DATA_SCREAMING_PARAM); }

    /**
     * @brief 设置注视状态
     *
     * 写入 DATA_SCREAMING_PARAM，由 EntityTracker 自动广播到客户端。
     */
    void setScreaming(bool screaming) { m_dataManager.set(DATA_SCREAMING_PARAM, screaming); }

    // ========== 瞬移系统 ==========

    /**
     * @brief 尝试随机瞬移
     * @return 是否成功瞬移
     */
    bool teleport();

    /**
     * @brief 尝试瞬移到目标附近
     */
    bool teleportToTarget();

    /**
     * @brief 尝试瞬移避开水
     */
    bool teleportAwayFromWater();

    // ========== 搬方块系统 ==========

    /**
     * @brief 是否拿着方块
     *
     * 通过 DATA_CARRIED_BLOCK_STATE_ID_PARAM 判断：stateId > 0 表示持有方块。
     */
    [[nodiscard]] bool isHoldingBlock() const { return m_dataManager.get<i32>(DATA_CARRIED_BLOCK_STATE_ID_PARAM) > 0; }

    /**
     * @brief 获取拿着的方块状态
     *
     * 从 DATA_CARRIED_BLOCK_STATE_ID_PARAM 读取 stateId，
     * 通过 BlockRegistry 解析为 BlockState 指针。
     * 返回 nullptr 表示未持有方块。
     */
    [[nodiscard]] const BlockState* getHeldBlockState() const;

    /**
     * @brief 设置拿着的方块状态
     *
     * 将 BlockState 的 stateId 写入 DATA_CARRIED_BLOCK_STATE_ID_PARAM，
     * 由 EntityTracker 自动广播到客户端。传入 nullptr 清除持有方块。
     */
    void setHeldBlockState(const BlockState* state);

    /**
     * @brief 获取搬方块状态参数 ID（供客户端 ClientEntity 读取）
     */
    [[nodiscard]] static u16 getCarriedBlockStateIdParamId() { return DATA_CARRIED_BLOCK_STATE_ID_PARAM.id(); }

    /**
     * @brief 获取注视状态参数 ID（供客户端 ClientEntity 读取）
     */
    [[nodiscard]] static u16 getScreamingParamId() { return DATA_SCREAMING_PARAM.id(); }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 末影人不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 寻路权重 ==========

    /**
     * @brief 获取路径权重
     *
     * 末影人返回0.0f（无偏好），不依赖光照。
     * 对应 MC EnderMan.getWalkTargetValue 返回 0.0F。
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 水敏感检测 ==========

    /**
     * @brief 检查是否在水中或雨中
     * 对于末影人，气泡柱不会造成伤害，所以只检查水和雨。
     * @return 如果在水中或雨中返回 true
     */
    [[nodiscard]] bool isInWaterOrRain() const;

    // ========== 注视检测 ==========

    /**
     * @brief 检查玩家是否正在注视末影人（应被激怒）
     *
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

    /**
     * @brief 每tick更新
     */
    void tick() override;

    /**
     * @brief 受到伤害时的处理（攻击后瞬移）
     */
    bool hurt(DamageSource& source, f32 amount) override;

    // ========== 常量 ==========

    static constexpr i32 TELEPORT_COOLDOWN = 50;            // 瞬移冷却 (ticks)
    static constexpr i32 ANGER_DURATION = 600;              // 愤怒持续时间 (ticks)
    static constexpr f32 TELEPORT_RANGE = 64.0f;            // 瞬移范围
    static constexpr f32 WATER_DAMAGE = 1.0f;               // 水伤害
    static constexpr i32 WATER_DAMAGE_INTERVAL = 10;        // 水伤害间隔
    static constexpr i32 TELEPORT_PROJECTILE_ATTEMPTS = 64; // 投射物伤害时瞬移尝试次数

protected:
    // ========== AI 目标注册 ==========

    /**
     * @brief 注册 AI 目标
     */
    void registerGoals() override;

    // ========== 属性注册 ==========

    /**
     * @brief 注册实体属性
     */
    void registerAttributes() override;

    // ========== 数据参数注册 ==========

    /**
     * @brief 注册网络同步数据参数
     *
     * 注册 DATA_CARRIED_BLOCK_STATE_ID_PARAM 和 DATA_SCREAMING_PARAM 到
     * EntityDataManager，由 EntityTracker 自动广播到客户端。
     *
     * 必须在构造函数中显式调用（参考 WolfEntity 模式），因为基类构造函数
     * 中的虚函数调用不会派发到派生类。
     */
    void registerData() override;

private:
    // IAngerable接口（m_attackTarget 使用 MobEntity::m_attackTarget，不重复声明）
    std::optional<u64> m_revengeTargetId;
    i32 m_revengeTimer = 0;

    // 愤怒状态
    bool m_angry = false;
    i32 m_angerTime = 0;

    // 瞬移冷却
    i32 m_teleportCooldown = 0;

    // ========== 网络同步数据参数 ==========

    /**
     * @brief 搬方块状态同步参数
     *
     * 对应 MC 1.21.11 EnderMan.DATA_CARRY_STATE。
     * 存储 BlockState 的 stateId（i32），0 表示未持有方块。
     * 由 setHeldBlockState 写入，由 EntityTracker 自动广播。
     * 客户端 ClientEntity::syncMetadataFromDataManager 读取后通过
     * BlockRegistry::getBlockState 解析为 BlockState* 并缓存到镜像字段。
     */
    static entity::DataParameter<i32> DATA_CARRIED_BLOCK_STATE_ID_PARAM;

    /**
     * @brief 注视状态同步参数
     *
     * 对应 MC 1.21.11 EnderMan.DATA_CREEPY（被注视/尖叫状态）。
     * 由 setScreaming 写入，由 EntityTracker 自动广播。
     * 客户端 ClientEntity 读取后镜像到 endermanScreaming()。
     */
    static entity::DataParameter<bool> DATA_SCREAMING_PARAM;
};

} // namespace mc
