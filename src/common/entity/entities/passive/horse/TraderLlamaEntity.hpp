/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "LlamaEntity.hpp"

#include <memory>

namespace mc {

// 前向声明
class Player;

namespace nbt::tags {
struct compound_tag;
}

/**
 * @brief 商队羊驼实体
 *
 * 商队羊驼是跟随流浪商人的特殊羊驼，具有以下特性：
 * - 与流浪商人绑定：被拴绳拴在流浪商人身上，保卫流浪商人
 * - 消失机制：与流浪商人的消失倒计时同步，或独立倒计时消失
 * - 防御目标：当流浪商人受到攻击时，商队羊驼会反击攻击者
 * - 特殊骑乘限制：被拴在流浪商人身上时不允许玩家骑乘
 * - 目标选择：攻击僵尸（除僵尸猪灵）和灾厄村民
 */
class TraderLlamaEntity : public LlamaEntity {
public:
    static constexpr i32 DEFAULT_DESPAWN_DELAY = 47999;

    /**
     * @brief 构造商队羊驼
     * @param id 实体 ID
     */
    TraderLlamaEntity(EntityInstanceId id);

    ~TraderLlamaEntity() override = default;

    TraderLlamaEntity(const TraderLlamaEntity&) = delete;
    TraderLlamaEntity& operator=(const TraderLlamaEntity&) = delete;
    TraderLlamaEntity(TraderLlamaEntity&&) = delete;
    TraderLlamaEntity& operator=(TraderLlamaEntity&&) = delete;

    /**
     * @brief 创建商队羊驼
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 标识 ==========

    /**
     * @brief 当前是否为商队羊驼
     */
    [[nodiscard]] bool isTraderLlama() const { return true; }

    // ========== 消失倒计时 ==========

    /**
     * @brief 获取消失倒计时
     */
    [[nodiscard]] i32 getDespawnDelay() const { return m_despawnDelay; }

    /**
     * @brief 设置消失倒计时
     */
    void setDespawnDelay(i32 despawnDelay) { m_despawnDelay = despawnDelay; }

    /**
     * @brief 以流浪商人倒计时同步自身倒计时
     * @param traderDespawnDelay 流浪商人的消失倒计时
     */
    void syncDespawnDelayFromTrader(i32 traderDespawnDelay) { m_despawnDelay = traderDespawnDelay - 1; }

    // ========== 消失逻辑 ==========

    /**
     * @brief 检查是否可以消失
     *
     * 商队羊驼在以下情况下不会消失：
     * 1. 已被驯服
     * 2. 被拴住（任何拴绳持有者，包括流浪商人）
     * 3. 正好有一名玩家乘客
     *
     * 注意：被拴住时不应消失，因为拴绳状态意味着实体受玩家/商人控制。
     * 流浪商人自身的消失机制通过 maybeDespawn() 管理，
     * 不应被 DespawnManager 的距离判断干扰。
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const noexcept override;

    // ========== 交互 ==========

    /**
     * @brief 玩家与商队羊驼交互
     *
     * 当商队羊驼被拴在流浪商人身上时，不允许玩家骑乘。
     * 行为由 tests/entity/TraderLlamaIntegrationTest.cpp 覆盖。
     */
    [[nodiscard]] ActionResultType interactMob(Player& player, Hand hand) override;

    /**
     * @brief 获取拴绳持有者实体（通过 UUID 查找）
     * @return 拴绳持有者实体指针，未找到或未拴住返回 nullptr
     *
     * 此方法为 public 以供 AI 目标类访问。
     * 使用 IWorld::getEntityByUuid() 进行 O(1) UUID 查找。
     */
    [[nodiscard]] Entity* getLeashHolderEntity() const;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 完成商队羊驼的生成初始化
     *
     * 重写 MobEntity::finalizeSpawn() 以确保消失倒计时正确设置。
     * 当商队羊驼自然生成时（非由流浪商人生成），需要初始化消失倒计时。
     *
     * @param world 世界引用
     * @param difficulty 区域难度实例
     * @param spawnReason 生成原因
     */
    void finalizeSpawn(IWorld& world,
        const entity::combat::DifficultyInstance& difficulty,
        world::spawn::SpawnReason spawnReason) override;

protected:
    /**
     * @brief 注册商队羊驼的 AI 目标
     *
     * 在羊驼目标基础上添加：
     * - PanicGoal（优先级 1）
     * - TraderLlamaDefendWanderingTraderGoal（目标优先级 1）
     * - NearestAttackableTargetGoal<ZombieEntity>（目标优先级 2，排除僵尸猪灵）
     * - NearestAttackableTargetGoal<AbstractIllagerEntity>（目标优先级 2）
     */
    void registerGoals() override;

    /**
     * @brief 获取环境音效
     *
     * 商队羊驼复用普通羊驼的环境音，对齐原版 TraderLlama（继承 Llama.getAmbientSound）。
     * sounds.json 中无 entity.trader_llama.*（商队羊驼共享 llama.* 音效），
     * 故不能走默认 makeSoundEventId("ambient")（会拼接出 trader_llama.ambient）。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief NBT 序列化：写入商队羊驼特有数据
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief NBT 反序列化：读取商队羊驼特有数据
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /**
     * @brief 每 tick 执行消失倒计时逻辑
     *
     * 内部消失判定（对应 MC 1.21.11 TraderLlama 的私有 canDespawn()）：
     *   !isTame() && !isLeashedToSomethingOtherThanTheWanderingTrader() && !hasExactlyOnePlayerPassenger()
     * 与 MobEntity::canDespawn(double) 不同：后者供 DespawnManager 距离判断使用，
     * 对任何拴绳状态均返回 false；本方法允许"拴在流浪商人身上"的羊驼继续消失，
     * 以便与流浪商人的消失倒计时同步。
     *
     * - 拴在流浪商人身上时，同步流浪商人的消失倒计时（trader.despawnDelay - 1）
     * - 否则自行递减消失倒计时
     * - 当倒计时 <= 0 时：解除拴绳并丢弃实体
     *
     * 行为由 tests/entity/TraderLlamaIntegrationTest.cpp 覆盖。
     */
    void maybeDespawn();

    /**
     * @brief 检查是否被拴在流浪商人身上
     *
     * 行为由 tests/entity/TraderLlamaIntegrationTest.cpp 覆盖。
     */
    [[nodiscard]] bool isLeashedToWanderingTrader() const;

    /**
     * @brief 检查是否正好有一名玩家乘客
     *
     * 行为由 tests/entity/TraderLlamaIntegrationTest.cpp 覆盖。
     */
    [[nodiscard]] bool hasExactlyOnePlayerPassenger() const;

    i32 m_despawnDelay = DEFAULT_DESPAWN_DELAY;
};

} // namespace mc
