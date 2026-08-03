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

#include "FirstPersonTransforms.hpp"
#include "client/renderer/trident/firstperson/MatrixStack.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>

namespace mc::client::renderer::trident::firstperson {

using namespace mc::math;

// ============================================================================
// 物品-手臂基础定位
// ============================================================================

void applyItemArmTransform(MatrixStack& stack, HandSide side, f32 equipProgress)
{
    const f32 i = handSign(side);
    stack.translate(i * ITEM_POS_X, ITEM_POS_Y + equipProgress * ITEM_HEIGHT_SCALE, ITEM_POS_Z);
}

void swingArm(MatrixStack& stack, HandSide side, f32 swingProgress)
{
    const f32 i = handSign(side);
    const f32 sqrtSwing = std::sqrt(swingProgress);
    const f32 offsetX = -0.4f * std::sin(sqrtSwing * PI);
    const f32 offsetY = 0.2f * std::sin(sqrtSwing * TWO_PI);
    const f32 offsetZ = -0.2f * std::sin(swingProgress * PI);
    stack.translate(i * offsetX, offsetY, offsetZ);
    applyItemArmAttackTransform(stack, side, swingProgress);
}

void applyItemArmAttackTransform(MatrixStack& stack, HandSide side, f32 swingProgress)
{
    const f32 i = handSign(side);
    const f32 swing = std::sin(swingProgress * swingProgress * PI);
    const f32 swingSqrt = std::sin(std::sqrt(swingProgress) * PI);
    stack.rotateY(i * (45.0f + swing * -20.0f));
    stack.rotateZ(i * swingSqrt * -20.0f);
    stack.rotateX(swingSqrt * -80.0f);
    stack.rotateY(i * -45.0f);
}

void renderPlayerArmTransform(MatrixStack& stack, HandSide side, f32 equipProgress, f32 swingProgress)
{
    const f32 f = handSign(side);

    const f32 sqrtSwing = std::sqrt(swingProgress);
    const f32 swingSin = std::sin(sqrtSwing * PI);

    const f32 translateX = f * (-0.3f * swingSin + 0.64000005f);
    const f32 translateY = 0.4f * std::sin(sqrtSwing * TWO_PI) - 0.6f + equipProgress * -0.6f;
    const f32 translateZ = -0.4f * std::sin(swingProgress * PI) - 0.71999997f;
    stack.translate(translateX, translateY, translateZ);

    stack.rotateY(f * 45.0f);

    const f32 swingSquareSin = std::sin(swingProgress * swingProgress * PI);
    stack.rotateY(f * swingSin * 70.0f);
    stack.rotateZ(f * swingSquareSin * -20.0f);

    stack.translate(f * -1.0f, 3.6f, 3.5f);
    stack.rotateZ(f * 120.0f);
    stack.rotateX(200.0f);
    stack.rotateY(f * -135.0f);
    stack.translate(f * 5.6f, 0.0f, 0.0f);
}

// ============================================================================
// 使用物品动作变换
// ============================================================================

void applyEatTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useDuration, i32 useCount)
{
    if (useDuration <= 0) {
        return;
    }

    // f = useDuration - useCount + partialTicks + 1.0（getUseItemRemainingTicks - partial + 1 的反向）
    const f32 f = static_cast<f32>(useDuration - useCount) + partialTicks + 1.0f;
    const f32 f1 = f / static_cast<f32>(useDuration);

    if (f1 < 0.8f) {
        const f32 f2 = std::abs(std::cos(f / 4.0f * PI) * 0.1f);
        stack.translate(0.0f, f2, 0.0f);
    }

    const f32 f3 = 1.0f - static_cast<f32>(std::pow(static_cast<f64>(f1), 27.0));
    const f32 i = handSign(side);

    stack.translate(i * f3 * 0.6f, f3 * -0.5f, f3 * 0.0f);
    stack.rotateY(i * f3 * 90.0f);
    stack.rotateX(f3 * 10.0f);
    stack.rotateZ(i * f3 * 30.0f);
}

void applyBrushTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useItemRemainingTicks)
{
    const f32 f = static_cast<f32>(useItemRemainingTicks % 10);
    const f32 f1 = f - partialTicks + 1.0f;
    const f32 f2 = 1.0f - f1 / 10.0f;
    const f32 f7 = -15.0f + 75.0f * std::cos(f2 * 2.0f * PI);

    if (side != HandSide::Right) {
        stack.translate(0.1f, 0.83f, 0.35f);
        stack.rotateX(-80.0f);
        stack.rotateY(-90.0f);
        stack.rotateX(f7);
        stack.translate(-0.3f, 0.22f, 0.35f);
    } else {
        stack.translate(-0.25f, 0.22f, 0.35f);
        stack.rotateX(-80.0f);
        stack.rotateY(90.0f);
        stack.rotateX(f7);
    }
}

void applyBowTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useDuration, i32 useCount)
{
    const f32 j = handSign(side);

    // 基座变换
    stack.translate(j * -0.2785682f, 0.18344387f, 0.15731531f);
    stack.rotateX(-13.935f);
    stack.rotateY(j * 35.3f);
    stack.rotateZ(j * -9.785f);

    // 蓄力进度
    const f32 f7 = static_cast<f32>(useDuration - useCount) + partialTicks + 1.0f;
    f32 f9 = f7 / 20.0f;
    f9 = clamp((f9 * f9 + f9 * 2.0f) / 3.0f, 0.0f, 1.0f);

    if (f9 > 0.1f) {
        const f32 f11 = std::sin((f7 - 0.1f) * 1.3f);
        const f32 f13 = f9 - 0.1f;
        const f32 f15 = f11 * f13;
        stack.translate(f15 * 0.0f, f15 * 0.004f, f15 * 0.0f);
    }

    stack.translate(f9 * 0.0f, f9 * 0.0f, f9 * 0.04f);
    stack.scale(1.0f, 1.0f, 1.0f + f9 * 0.2f);
    stack.rotateY(-j * 45.0f);
}

void applyTridentTransform(MatrixStack& stack, f32 partialTicks, HandSide side, i32 useDuration, i32 useCount)
{
    const f32 j = handSign(side);

    // 基座变换
    stack.translate(j * -0.5f, 0.7f, 0.1f);
    stack.rotateX(-55.0f);
    stack.rotateY(j * 35.3f);
    stack.rotateZ(j * -9.785f);

    // 蓄力进度
    const f32 f6 = static_cast<f32>(useDuration - useCount) + partialTicks + 1.0f;
    const f32 f8 = clamp(f6 / 10.0f, 0.0f, 1.0f);

    if (f8 > 0.1f) {
        const f32 f10 = std::sin((f6 - 0.1f) * 1.3f);
        const f32 f12 = f8 - 0.1f;
        const f32 f14 = f10 * f12;
        stack.translate(f14 * 0.0f, f14 * 0.004f, f14 * 0.0f);
    }

    stack.translate(0.0f, 0.0f, f8 * 0.2f);
    stack.scale(1.0f, 1.0f, 1.0f + f8 * 0.2f);
    stack.rotateY(-j * 45.0f);
}

void applyCrossbowTransform(MatrixStack& stack,
    f32 partialTicks,
    HandSide side,
    i32 useDuration,
    i32 useCount,
    bool isCharging,
    bool isCharged,
    bool isMainHand,
    i32 chargeDuration,
    f32 swingProgress)
{
    const f32 i = handSign(side);

    // CROSSBOW 分支先调 applyItemArmTransform（基座），由调用方在分派前完成。
    if (isCharging) {
        // 装填基座
        stack.translate(i * -0.4785682f, -0.094387f, 0.05731531f);
        stack.rotateX(-11.935f);
        stack.rotateY(i * 65.3f);
        stack.rotateZ(i * -9.785f);

        f32 f = static_cast<f32>(useDuration - useCount) + partialTicks + 1.0f;
        const f32 f1 = (chargeDuration > 0) ? clamp(f / static_cast<f32>(chargeDuration), 0.0f, 1.0f) : 0.0f;

        if (f1 > 0.1f) {
            const f32 f2 = std::sin((f - 0.1f) * 1.3f);
            const f32 f3 = f1 - 0.1f;
            const f32 f4 = f2 * f3;
            stack.translate(f4 * 0.0f, f4 * 0.004f, f4 * 0.0f);
        }

        stack.translate(f1 * 0.0f, f1 * 0.0f, f1 * 0.04f);
        stack.scale(1.0f, 1.0f, 1.0f + f1 * 0.2f);
        stack.rotateY(-i * 45.0f);
    } else {
        // 已装填空闲态：先挥动，再施加已装填偏移
        swingArm(stack, side, swingProgress);
        if (isCharged && swingProgress < 0.001f && isMainHand) {
            stack.translate(i * -0.641864f, 0.0f, 0.0f);
            stack.rotateY(i * 10.0f);
        }
    }
}

void applyBlockTransform(MatrixStack& stack, HandSide side)
{
    const f32 j = handSign(side);
    stack.translate(j * -0.14142136f, 0.08f, 0.14142136f);
    stack.rotateX(-102.25f);
    stack.rotateY(j * 13.365f);
    stack.rotateZ(j * 78.05f);
}

void applyAutoSpinTransform(MatrixStack& stack, HandSide side, f32 equipProgress)
{
    // 先 applyItemArmTransform 再施加冲刺变换。
    applyItemArmTransform(stack, side, equipProgress);
    const f32 j = handSign(side);
    stack.translate(j * -0.4f, 0.8f, 0.3f);
    stack.rotateY(j * 65.0f);
    stack.rotateZ(j * -85.0f);
}

// ============================================================================
// 地图变换
// ============================================================================

f32 calculateMapTilt(f32 pitch)
{
    f32 f = 1.0f - pitch / 45.0f + 0.1f;
    f = clamp(f, 0.0f, 1.0f);
    return -std::cos(f * PI) * 0.5f + 0.5f;
}

void applyTwoHandedMapTransform(MatrixStack& stack, f32 pitch, f32 equipProgress, f32 swingProgress)
{
    const f32 f = std::sqrt(swingProgress);
    const f32 f1 = -0.2f * std::sin(swingProgress * PI);
    const f32 f2 = -0.4f * std::sin(f * PI);
    stack.translate(0.0f, -f1 / 2.0f, f2);

    const f32 f3 = calculateMapTilt(pitch);
    stack.translate(0.0f, 0.04f + equipProgress * -1.2f + f3 * -0.5f, -0.72f);
    stack.rotateX(f3 * -85.0f);

    // 双手举起手臂网格由调用方在 _ensureArmMesh + 手臂管线负责绘制，此处仅施加地图板变换。
    const f32 f4 = std::sin(f * PI);
    stack.rotateX(f4 * 20.0f);
    stack.scale(2.0f, 2.0f, 2.0f);
    // 地图内容绘制由调用方负责。
}

void applyOneHandedMapTransform(MatrixStack& stack, HandSide side, f32 equipProgress, f32 swingProgress)
{
    const f32 f = handSign(side);
    stack.translate(f * 0.125f, -0.125f, 0.0f);

    // renderPlayerArm 由调用方在 push/pop 中完成，此处仅保留地图板变换。
    stack.push();
    stack.translate(f * 0.51f, -0.08f + equipProgress * -1.2f, -0.75f);
    const f32 f1 = std::sqrt(swingProgress);
    const f32 f2 = std::sin(f1 * PI);
    const f32 f3 = -0.5f * f2;
    const f32 f4 = 0.4f * std::sin(f1 * TWO_PI);
    const f32 f5 = -0.3f * std::sin(swingProgress * PI);
    stack.translate(f * f3, f4 - 0.3f * f2, f5);
    stack.rotateX(f2 * -45.0f);
    stack.rotateY(f * f2 * -30.0f);
    // 地图内容绘制由调用方负责。
    stack.pop();
}

// ============================================================================
// 相机层抖动
// ============================================================================

void computeViewBobbing(MatrixStack& stack, f32 backwardsWalkDistance, f32 interpolatedBob)
{
    const f32 sinWalk = std::sin(backwardsWalkDistance * PI);
    const f32 cosWalk = std::cos(backwardsWalkDistance * PI);

    stack.translate(sinWalk * interpolatedBob * 0.5f, -std::abs(cosWalk * interpolatedBob), 0.0f);
    stack.rotateZ(sinWalk * interpolatedBob * 3.0f);
    stack.rotateX(std::abs(std::cos(backwardsWalkDistance * PI - 0.2f) * interpolatedBob) * 5.0f);
}

void computeDamageTilt(MatrixStack& stack,
    f32 partialTick,
    f32 hurtTime,
    f32 hurtDuration,
    f32 hurtDir,
    f32 damageTiltStrength,
    f32 deathTime,
    bool isDeadOrDying)
{
    if (isDeadOrDying) {
        const f32 f = std::min(deathTime + partialTick, 20.0f);
        stack.rotateZ(40.0f - 8000.0f / (f + 200.0f));
    }

    f32 f2 = hurtTime - partialTick;
    if (f2 < 0.0f) {
        return;
    }

    f2 /= (hurtDuration > 0.0f) ? hurtDuration : 1.0f;
    f2 = std::sin(f2 * f2 * f2 * f2 * PI);
    stack.rotateY(-hurtDir);
    const f32 f1 = -f2 * 14.0f * damageTiltStrength;
    stack.rotateZ(f1);
    stack.rotateY(hurtDir);
}

// ============================================================================
// 手部渲染选择
// ============================================================================

namespace {

/// 主手或副手物品是弓/弩之一（itemstack.is(Items.BOW/CROSSBOW)）。
[[nodiscard]] bool isBowLike(const Item* item)
{
    return item == Items::BOW || item == Items::CROSSBOW;
}

/// 物品是已装填弩（isChargedCrossbow）。
[[nodiscard]] bool isChargedCrossbow(const ItemStack& stack)
{
    return !stack.isEmpty() && stack.getItem() == Items::CROSSBOW && item::CrossbowItem::isCharged(stack);
}

/// 持有弓/弩且正在使用物品时的渲染选择（selectionUsingItemWhileHoldingBowLike）。
[[nodiscard]] HandRenderSelection selectionUsingItemWhileHoldingBowLike(const Player& player)
{
    const ItemStack& useItem = player.getActiveItem();
    const Item* useItemPtr = useItem.isEmpty() ? nullptr : useItem.getItem();
    if (!isBowLike(useItemPtr)) {
        // 使用物品本身不是弓/弩：仅当主手使用 + 副手是已装填弩时只渲染主手，否则双手。
        if (player.getActiveHand() == Hand::MainHand && isChargedCrossbow(player.getOffHandItem())) {
            return HandRenderSelection::RenderMainHandOnly;
        }
        return HandRenderSelection::RenderBothHands;
    }
    // 使用弓/弩：只渲染使用手。
    return player.getActiveHand() == Hand::MainHand ? HandRenderSelection::RenderMainHandOnly
                                                    : HandRenderSelection::RenderOffHandOnly;
}

} // namespace

HandRenderSelection evaluateWhichHandsToRender(const Player& player)
{
    const ItemStack& mainHand = player.getMainHandItem();
    const ItemStack& offHand = player.getOffHandItem();
    const Item* mainItem = mainHand.isEmpty() ? nullptr : mainHand.getItem();
    const Item* offItem = offHand.isEmpty() ? nullptr : offHand.getItem();

    const bool holdsBow = isBowLike(mainItem) || isBowLike(offItem);
    if (!holdsBow) {
        return HandRenderSelection::RenderBothHands;
    }
    if (player.isUsingItem()) {
        return selectionUsingItemWhileHoldingBowLike(player);
    }
    return isChargedCrossbow(mainHand) ? HandRenderSelection::RenderMainHandOnly : HandRenderSelection::RenderBothHands;
}

} // namespace mc::client::renderer::trident::firstperson
