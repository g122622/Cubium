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

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "SpellcastingIllagerEntity.hpp"

#include <array>
#include <memory>

namespace mc {

/**
 * @brief 幻术师实体
 *
 * 幻术师是能够施放法术的灾厄村民，使用弓进行远程攻击。
 *
 * 特性：
 * - 远程攻击：使用弓发射箭矢
 * - 失明法术：对目标施加失明效果（仅困难难度）
 * - 镜像法术：使自己隐身60秒
 * - 手臂姿势：施法时显示施法姿势，攻击时显示弓箭姿势
 *
 * AI 目标：
 * - 优先级 0: 游泳
 * - 优先级 4: 镜像法术（隐身）
 * - 优先级 5: 失明法术
 * - 优先级 6: 弓箭远程攻击
 * - 优先级 8: 随机行走
 * - 优先级 9: 看向玩家
 * - 优先级 10: 看向生物
 *
 * 目标选择：
 * - 优先级 1: 被攻击后反击并呼叫支援
 * - 优先级 2: 攻击玩家
 * - 优先级 3: 攻击村民
 * - 优先级 3: 攻击铁傀儡
 */
class IllusionerEntity : public SpellcastingIllagerEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    IllusionerEntity(EntityInstanceId id);
    ~IllusionerEntity() noexcept override = default;

    // 禁止拷贝
    IllusionerEntity(const IllusionerEntity&) = delete;
    IllusionerEntity& operator=(const IllusionerEntity&) = delete;

    // 允许移动
    IllusionerEntity(IllusionerEntity&&) = delete;
    IllusionerEntity& operator=(IllusionerEntity&&) = delete;

    /// 本类继承链标识（parent = SpellcastingIllagerEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    /**
     * @brief 创建幻术师实体
     * @param world 世界实例
     * @return 新的幻术师实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== IRangedAttackMob 接口 ==========

    /**
     * @brief 对目标进行远程攻击（发射箭矢）
     *
     * 箭矢速度: 1.6，不精确度: 14 - difficulty * 4，弹道补偿: horizontalDist * 0.2
     *
     * @param target 目标实体
     * @param charge 蓄力程度
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    /**
     * @brief 获取攻击间隔时间
     * @return 20 ticks (1秒)
     */
    [[nodiscard]] i32 getAttackInterval() const override { return 20; }

    /**
     * @brief 检查是否可以进行远程攻击
     * @return 如果不在施法状态返回true
     */
    [[nodiscard]] bool canRangedAttack() const override { return !isSpellcasting(); }

    // ========== 状态查询 ==========

    /**
     * @brief 是否正在施法
     */
    [[nodiscard]] bool isCasting() const { return isSpellcasting(); }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    // ========== 镜像分身 ==========

    /// 镜像分身数量
    static constexpr i32 NUM_ILLUSIONS = 4;

    /**
     * @brief 获取镜像分身偏移量（用于客户端渲染）
     *
     * 返回4个分身相对于实体位置的偏移量，根据过渡动画插值计算。
     * 仅在客户端隐身状态下有效。
     *
     * TODO: 客户端渲染层需在 IllusionerRenderer 中调用此方法，
     * 获取4个分身偏移量并渲染4个半透明幻术师模型，实现镜像分身视觉效果。
     *
     * @param partialTick 部分tick时间（0~1），用于平滑动画
     * @return 包含4个偏移量的数组，每个元素为 (x, y, z) 偏移
     */
    [[nodiscard]] std::array<Vector3, NUM_ILLUSIONS> getIllusionOffsets(f32 partialTick) const;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    /**
     * @brief 获取施法音效ID
     */
    [[nodiscard]] const char* getSpellSoundId() const noexcept override { return "entity.illusioner.cast_spell"; }

private:
    /**
     * @brief 客户端镜像分身更新逻辑
     *
     * 当幻术师隐身时，每1200tick或受伤时重新生成分身偏移并播放云粒子和音效；
     * 受伤结束时将分身偏移归零。仅在客户端执行。
     *
     * TODO: 客户端渲染层需在 IllusionerRenderer::render() 中，
     * 当幻术师处于隐身状态时，调用 getIllusionOffsets() 获取分身偏移，
     * 渲染4个半透明幻术师模型以实现镜像分身视觉效果。
     * 分身模型应使用实体实际位置 + 偏移量作为渲染位置。
     */
    void _updateIllusionLogic();

    // ========== 冷却时间 ==========
    i32 m_blindnessCooldown = 0;
    i32 m_mirrorCooldown = 0;

    // ========== 镜像分身状态（客户端） ==========
    static constexpr i32 ILLUSION_TRANSITION_TICKS = 3; ///< 分身过渡动画持续时间（ticks）
    static constexpr i32 ILLUSION_SPREAD = 3;           ///< 分身散布范围

    i32 m_clientSideIllusionTicks = 0; ///< 分身过渡剩余ticks

    /// m_illusionOffsets[0] = 旧偏移（过渡起点），m_illusionOffsets[1] = 新偏移（过渡终点）
    std::array<std::array<Vector3, NUM_ILLUSIONS>, 2> m_illusionOffsets = {};

    // 常量
    static constexpr f32 ARROW_VELOCITY = 1.6f; ///< 箭矢速度
};

} // namespace mc
