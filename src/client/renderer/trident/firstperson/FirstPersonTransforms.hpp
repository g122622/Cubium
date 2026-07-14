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

#include "MatrixStack.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"

// 前向声明，避免头文件引入重依赖
namespace mc {
class Player;
} // namespace mc

namespace mc::client::renderer::trident::firstperson {

// 导入手侧类型（HandSide 定义在 mc 命名空间，与 FirstPersonRenderer 共用）
using mc::HandSide;

/// 手臂在屏幕侧边的偏移（applyItemArmTransform 的常量）。
static constexpr f32 ITEM_POS_X = 0.56f;
static constexpr f32 ITEM_POS_Y = -0.52f;
static constexpr f32 ITEM_POS_Z = -0.72f;
static constexpr f32 ITEM_HEIGHT_SCALE = -0.6f;

/// 取手侧符号：右手 +1，左手 -1。
[[nodiscard]] inline f32 handSign(HandSide side)
{
    return side == HandSide::Right ? 1.0f : -1.0f;
}

// ============================================================================
// 物品-手臂基础定位
// ============================================================================

/**
 * @brief 应用物品-手臂基础定位变换
 *
 * translate(i*0.56, -0.52 + equip*-0.6, -0.72)
 *
 * @param stack 矩阵栈（右乘语义：current = current * T）
 * @param side 手侧
 * @param equipProgress 装备进度（1 = 完全可见，0 = 完全隐藏）
 */
void applyItemArmTransform(MatrixStack& stack, HandSide side, f32 equipProgress);

/**
 * @brief 应用物品挥动平移 + 攻击旋转
 *
 * X = -0.4*sin(sqrt(swing)*PI)，Y = 0.2*sin(sqrt(swing)*2PI)，Z = -0.2*sin(swing*PI)，
 * 随后调用 applyItemArmAttackTransform。
 */
void swingArm(MatrixStack& stack, HandSide side, f32 swingProgress);

/**
 * @brief 应用物品挥动的旋转部分
 *
 * YP (45 + sin(swing²·PI)·-20) → ZP (sign·sin(sqrt(swing)·PI)·-20)
 * → XP (sin(sqrt(swing)·PI)·-80) → YP (sign·-45)
 */
void applyItemArmAttackTransform(MatrixStack& stack, HandSide side, f32 swingProgress);

/**
 * @brief 空手手臂变换
 *
 * 完整复刻 renderPlayerArm 的平移/旋转序列，参数 equipProgress 与 swingProgress
 * 含义同 applyItemArmTransform / swingArm。网格绘制由调用方负责。
 */
void renderPlayerArmTransform(MatrixStack& stack, HandSide side, f32 equipProgress, f32 swingProgress);

// ============================================================================
// 使用物品动作变换
// ============================================================================

/**
 * @brief 进食/饮用变换
 *
 * @param useDuration 物品总使用时长（getUseDuration）
 * @param useCount 剩余使用 tick 数
 */
void applyEatTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useDuration, i32 useCount);

/**
 * @brief 刷子变换
 *
 * @param useItemRemainingTicks 玩家剩余使用 tick 数（getUseItemRemainingTicks）
 */
void applyBrushTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useItemRemainingTicks);

/**
 * @brief 弓蓄力变换（含基座 translate+rotate）
 *
 * @param useDuration 物品总使用时长
 * @param useCount 剩余使用 tick 数
 */
void applyBowTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useDuration, i32 useCount);

/**
 * @brief 三叉戟蓄力变换（含基座 translate+rotate）
 *
 * @param useDuration 物品总使用时长
 * @param useCount 剩余使用 tick 数
 */
void applyTridentTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useDuration, i32 useCount);

/**
 * @brief 弩变换
 *
 * 装填中：基座 translate+rotate + 蓄力；已装填空闲：swingArm + i*-0.641864 + YP i*10。
 *
 * @param isCharging 是否正在装填（isUsingItem 且 useItemRemainingTicks>0 且 !isCharged）
 * @param isCharged 是否已装填
 * @param isMainHand 是否主手（仅主手已装填空闲态施加额外偏移）
 * @param chargeDuration 装填总时长（CrossbowItem::getChargeTime）
 */
void applyCrossbowTransform(MatrixStack& stack,
    f32 partialTicks,
    HandSide side,
    i32 useDuration,
    i32 useCount,
    bool isCharging,
    bool isCharged,
    bool isMainHand,
    i32 chargeDuration,
    f32 swingProgress);

/**
 * @brief 非盾格挡变换
 *
 * translate(j*-0.14142136, 0.08, 0.14142136) + rot(XP -102.25, YP j*13.365, ZP j*78.05)
 */
void applyBlockTransform(MatrixStack& stack, HandSide side);

/**
 * @brief 三叉戟冲刺（auto-spin attack）变换
 *
 * applyItemArmTransform 后 translate(j*-0.4, 0.8, 0.3) + rot(YP j*65, ZP j*-85)
 */
void applyAutoSpinTransform(MatrixStack& stack, HandSide side, f32 equipProgress);

// ============================================================================
// 地图变换
// ============================================================================

/**
 * @brief 计算地图倾斜角
 *
 * f = 1 - pitch/45 + 0.1，clamp[0,1]，返回 -cos(f*PI)*0.5 + 0.5
 */
[[nodiscard]] f32 calculateMapTilt(f32 pitch);

/**
 * @brief 双手持地图变换（renderTwoHandedMap 的矩阵部分，不含地图内容绘制）
 */
void applyTwoHandedMapTransform(MatrixStack& stack, f32 pitch, f32 equipProgress, f32 swingProgress);

/**
 * @brief 单手持地图变换（renderOneHandedMap 的矩阵部分，不含地图内容绘制）
 *
 * @param equipProgress 装备进度
 * @param swingProgress 挥动进度
 */
void applyOneHandedMapTransform(MatrixStack& stack, HandSide side, f32 equipProgress, f32 swingProgress);

/**
 * @brief 计算装备进度
 *
 * equipProgress = swapAnimationScale * (1 - lerp(partialTick, oldHeight, height))。
 * height=1 表示完全可见，equipProgress=0 表示完全可见；返回值是“隐藏度”，
 * 供 applyItemArmTransform/renderPlayerArm 的 equip*-0.6 下落使用。
 *
 * @param oldHeight 上一 tick 的高度
 * @param height 当前 tick 的高度
 * @param partialTick 插值进度
 * @param swapAnimationScale per-item 切换动画缩放（默认 1.0，项目无该数据组件）
 */
[[nodiscard]] inline f32 computeEquipProgress(f32 oldHeight, f32 height, f32 partialTick, f32 swapAnimationScale)
{
    // swapAnimationScale * (1 - lerp(partial, oldHeight, height))
    // 项目 math::lerp(a,b,t) 签名，故 lerp(oldHeight, height, partialTick)。
    return swapAnimationScale * (1.0f - math::lerp(oldHeight, height, partialTick));
}

// ============================================================================
// 相机层抖动（bobHurt / bobView）
// ============================================================================

/**
 * @brief 视野摇晃变换
 *
 * 平移 (sin(walk*PI)*bob*0.5, -|cos(walk*PI)*bob|, 0)，
 * rot Z sin(walk*PI)*bob*3，rot X |cos(walk*PI-0.2)*bob|*5。
 *
 * @param backwardsWalkDistance 玩家后退插值行走距离（getBackwardsInterpolatedWalkDistance）
 * @param interpolatedBob 插值后的晃动幅度（getInterpolatedBob）
 */
void computeViewBobbing(MatrixStack& stack, f32 backwardsWalkDistance, f32 interpolatedBob);

/**
 * @brief 伤害倾斜变换
 *
 * hurtTime 四次方 sin 包络乘 -14*damageTiltStrength 做 Z 旋转，由 hurtDir 做 Y 包裹；
 * 死亡时额外 Z 旋转 40 - 8000/(deathTime+200)。
 *
 * @param hurtTime 当前受伤计时（已扣 partialTick 前的整数值）
 * @param hurtDuration 受伤持续时间（受击时设为 10）
 * @param hurtDir 受伤方向角（度，相对玩家朝向）
 * @param damageTiltStrength 玩家设置的倾斜强度（0-1）
 * @param deathTime 死亡计时（0 表示未死亡）
 * @param isDeadOrDying 是否濒死/已死
 */
void computeDamageTilt(MatrixStack& stack,
    f32 partialTick,
    f32 hurtTime,
    f32 hurtDuration,
    f32 hurtDir,
    f32 damageTiltStrength,
    f32 deathTime,
    bool isDeadOrDying);

// ============================================================================
// 手部渲染选择（evaluateWhichHandsToRender）
// ============================================================================

/// 第一人称双手渲染选择。
/// 决定本帧渲染主手/副手/双手，由持弓弩状态与使用状态共同决定。
enum class HandRenderSelection {
    RenderBothHands,    // 双手都渲染
    RenderMainHandOnly, // 仅主手
    RenderOffHandOnly   // 仅副手
};

/// 取某只手是否需要渲染。
[[nodiscard]] inline bool shouldRenderHand(HandRenderSelection selection, Hand hand)
{
    switch (selection) {
        case HandRenderSelection::RenderBothHands:
            return true;
        case HandRenderSelection::RenderMainHandOnly:
            return hand == Hand::MainHand;
        case HandRenderSelection::RenderOffHandOnly:
            return hand == Hand::OffHand;
    }
    return false;
}

/**
 * @brief 评估本帧需要渲染的手
 *
 * 规则：
 * - 主副手都不是弓/弩 → 双手；
 * - 持有弓/弩且正在使用物品 → 转入使用中的弓弩判定：
 *   - 使用物品本身是弓/弩 → 仅渲染使用手；
 *   - 否则若使用手是主手且副手是已装填弩 → 仅主手；
 *   - 否则双手；
 * - 持有弓/弩且未使用物品 → 主手是已装填弩则仅主手，否则双手。
 */
[[nodiscard]] HandRenderSelection evaluateWhichHandsToRender(const mc::Player& player);

} // namespace mc::client::renderer::trident::firstperson
