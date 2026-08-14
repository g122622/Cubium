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
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"

#include <memory>
#include <optional>
#include <random>

namespace mc {

// Forward declarations
class Player;
class ItemStack;
class DamageSource;

/**
 * @brief 熊猫实体
 *
 * 具有独特性格基因的被动动物。
 *
 * 特性：
 * - 7种性格：普通、懒惰、忧愁、顽皮、好斗、棕色、虚弱
 * - 吃竹子：繁殖食物
 * - 打喷嚏：幼体可能打喷嚏喷出粘液球
 * - 打滚：顽皮熊猫会打滚
 * - 救助：好斗熊猫会保护其他熊猫
 * - 懒惰：懒惰熊猫经常躺着
 * - 忧愁：忧愁熊猫下雨时吃竹子
 * - 棕色：稀有棕色变种
 * - 虚弱：虚弱熊猫生病且生命值低
 */
class PandaEntity : public AnimalEntity {
public:
    /**
     * @brief 熊猫性格基因
     */
    enum class Personality : u8 {
        Normal = 0,        // 普通
        Lazy = 1,          // 懒惰
        Worried = 2,       // 忧愁
        Playful = 3,       // 顽皮
        Aggressive = 4,    // 好斗
        Weak = 5,          // 虚弱
        Brown = 6,         // 棕色（普通性格）
        AggressiveLazy = 7 // 好斗懒惰（隐藏）
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    PandaEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~PandaEntity() override = default;

    // 禁止拷贝
    PandaEntity(const PandaEntity&) = delete;
    PandaEntity& operator=(const PandaEntity&) = delete;

    // 允许移动
    PandaEntity(PandaEntity&&) noexcept = delete;
    PandaEntity& operator=(PandaEntity&&) noexcept = delete;

    /**
     * @brief 创建熊猫实体
     * @param world 世界实例
     * @return 新的熊猫实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 性格 ==========

    /**
     * @brief 获取性格
     */
    [[nodiscard]] Personality getPersonality() const { return m_personality; }

    /**
     * @brief 设置性格
     */
    void setPersonality(Personality personality) { m_personality = personality; }

    /**
     * @brief 随机生成性格
     */
    void randomizePersonality();

    /**
     * @brief 是否是懒惰性格
     */
    [[nodiscard]] bool isLazy() const { return m_personality == Personality::Lazy; }

    /**
     * @brief 是否是好斗性格
     */
    [[nodiscard]] bool isAggressive() const { return m_personality == Personality::Aggressive; }

    /**
     * @brief 是否是顽皮性格
     */
    [[nodiscard]] bool isPlayful() const { return m_personality == Personality::Playful; }

    /**
     * @brief 是否是忧愁性格
     */
    [[nodiscard]] bool isWorried() const { return m_personality == Personality::Worried; }

    /**
     * @brief 是否是虚弱性格
     */
    [[nodiscard]] bool isWeak() const { return m_personality == Personality::Weak; }

    /**
     * @brief 是否是棕色变种
     */
    [[nodiscard]] bool isBrown() const { return m_personality == Personality::Brown; }

    // ========== 基因系统 ==========

    /**
     * @brief 获取主基因
     */
    [[nodiscard]] u8 getMainGene() const { return m_mainGene; }

    /**
     * @brief 设置主基因
     */
    void setMainGene(u8 gene) { m_mainGene = gene; }

    /**
     * @brief 获取隐藏基因
     */
    [[nodiscard]] u8 getHiddenGene() const { return m_hiddenGene; }

    /**
     * @brief 设置隐藏基因
     */
    void setHiddenGene(u8 gene) { m_hiddenGene = gene; }

    /**
     * @brief 根据主基因和隐藏基因计算表达的性格
     */
    [[nodiscard]] Personality calculateExpressedPersonality() const;

    /**
     * @brief 随机获取主基因或隐藏基因中的一个
     */
    [[nodiscard]] u8 getOneOfGenesRandomly(math::Random& rng) const;

    /**
     * @brief 从父母遗传基因
     * @param father 父本
     * @param mother 母本（可能为 nullptr）
     */
    void inheritGenesFromParents(PandaEntity* father, PandaEntity* mother);

    /**
     * @brief 根据基因计算并设置性格
     */
    void updatePersonalityFromGenes();

    // ========== 行为状态 ==========

    /**
     * @brief 是否正在打滚
     */
    [[nodiscard]] bool isRolling() const { return m_rolling; }

    /**
     * @brief 设置打滚状态
     */
    void setRolling(bool rolling) { m_rolling = rolling; }

    /**
     * @brief 获取打滚计时器
     */
    [[nodiscard]] i32 getRollTimer() const { return m_rollTimer; }

    /**
     * @brief 设置打滚计时器
     * @param timer 计时器值（ticks）
     */
    void setRollTimer(i32 timer) { m_rollTimer = timer; }

    /**
     * @brief 检查是否可以执行动作
     *
     * 检查熊猫是否不在任何阻止动作的状态
     */
    [[nodiscard]] bool canPerformAction() const { return !isSneezing() && !isEating() && !isLying() && !isRolling(); }

    /**
     * @brief 是否正在打喷嚏
     */
    [[nodiscard]] bool isSneezing() const { return m_sneezing; }

    /**
     * @brief 设置打喷嚏状态
     */
    void setSneezing(bool sneezing) { m_sneezing = sneezing; }

    /**
     * @brief 获取打喷嚏计时器
     */
    [[nodiscard]] i32 getSneezeTimer() const { return m_sneezeTimer; }

    /**
     * @brief 设置打喷嚏计时器
     */
    void setSneezeTimer(i32 timer) { m_sneezeTimer = timer; }

    /**
     * @brief 是否正在吃东西
     */
    [[nodiscard]] bool isEating() const { return m_eating; }

    /**
     * @brief 设置吃东西状态
     */
    void setEating(bool eating) { m_eating = eating; }

    /**
     * @brief 是否正在躺着
     */
    [[nodiscard]] bool isLying() const { return m_lying; }

    /**
     * @brief 设置躺着状态
     */
    void setLying(bool lying) { m_lying = lying; }

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
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.6f : 1.2f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 根据性格返回不同音效
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
     * @brief 播放进食音效
     */
    void playEatSound();

    /**
     * @brief 播放打喷嚏音效
     */
    void playSneezeSound();

    /**
     * @brief 播放预喷嚏音效
     */
    void playPreSneezeSound();

    /**
     * @brief 播放咬音效
     */
    void playBiteSound();

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 刻更新 ==========
    void tick() override;

    /**
     * @brief 打喷嚏完成时调用
     *
     * 执行以下操作：
     * - 播放喷嚏音效
     * - 生成喷嚏粒子
     * - 让周围成年熊猫跳跃
     * - 1/700 概率掉落粘液球（需 doMobLoot 游戏规则）
     */
    void _onSneezeComplete();

    /**
     * @brief 更新打滚物理
     *
     * 处理打滚时的移动和跳跃逻辑：
     * - 第1帧：初始化速度向量
     * - 第7、15、23帧：执行小跳
     * - 其他帧：维持水平移动
     */
    void _updateRoll();

private:
    // 性格
    Personality m_personality = Personality::Normal;

    // 行为状态
    bool m_rolling = false;
    bool m_sneezing = false;
    bool m_eating = false;
    bool m_lying = false;

    // 计时器
    i32 m_rollTimer = 0;
    i32 m_sneezeTimer = 0;
    i32 m_eatTimer = 0;
    i32 m_lyingTimer = 0;

    // 打滚速度向量
    Vector3 m_rollVelocity{0.0, 0.0, 0.0};

    // 基因（用于遗传）
    u8 m_mainGene = 0;
    u8 m_hiddenGene = 0;

    // 打滚持续时间常量
    static constexpr i32 ROLL_DURATION = 32;         // 打滚总持续时间（ticks）
    static constexpr f32 ROLL_SPEED_ADULT = 0.2f;    // 成年熊猫打滚速度
    static constexpr f32 ROLL_SPEED_CHILD = 0.1f;    // 幼年熊猫打滚速度（减半）
    static constexpr f32 ROLL_JUMP_VELOCITY = 0.27f; // 打滚跳跃速度
};

} // namespace mc
