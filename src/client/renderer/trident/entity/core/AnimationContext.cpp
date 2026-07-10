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

#include "AnimationContext.hpp"
#include <functional>

namespace mc::client::renderer::entity::core {

void AnimationContext::computeHash()
{
    // 使用简单但有效的哈希组合方法
    // 参考 boost::hash_combine 的实现
    auto hashCombine = [](u32 seed, f64 value) {
        auto hash = std::hash<f64>{}(value);
        return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    };

    u32 hash = 0;

    // 组合主要动画参数
    hash = hashCombine(hash, limbSwing);
    hash = hashCombine(hash, limbSwingAmount);
    hash = hashCombine(hash, ageInTicks);
    hash = hashCombine(hash, netHeadYaw);
    hash = hashCombine(hash, headPitch);
    hash = hashCombine(hash, scale);
    hash = hashCombine(hash, static_cast<f64>(swingProgress));
    hash = hashCombine(hash, static_cast<f64>(standingProgress));
    hash = hashCombine(hash, static_cast<f64>(puffState));
    hash = hashCombine(hash, static_cast<f64>(eatAnimationTimer));
    hash = hashCombine(hash, static_cast<f64>(attackAnimationTicks));
    hash = hashCombine(hash, static_cast<f64>(wolfShakeAnim));
    hash = hashCombine(hash, static_cast<f64>(wolfInterestedAngle));
    hash = hashCombine(hash, static_cast<f64>(wolfWetShade));
    hash = hashCombine(hash, static_cast<f64>(swimAmount));

    // 凋灵侧头朝向
    hash = hashCombine(hash, static_cast<f64>(witherSideHeadYaw[0]));
    hash = hashCombine(hash, static_cast<f64>(witherSideHeadPitch[0]));
    hash = hashCombine(hash, static_cast<f64>(witherSideHeadYaw[1]));
    hash = hashCombine(hash, static_cast<f64>(witherSideHeadPitch[1]));

    // 组合布尔状态（转换为 0.0 或 1.0）
    hash = hashCombine(hash, isSitting ? 1.0 : 0.0);
    hash = hashCombine(hash, isChild ? 1.0 : 0.0);
    hash = hashCombine(hash, isSneaking ? 1.0 : 0.0);
    hash = hashCombine(hash, isSwimming ? 1.0 : 0.0);
    hash = hashCombine(hash, isRiding ? 1.0 : 0.0);
    hash = hashCombine(hash, isAngry ? 1.0 : 0.0);

    stateHash = hash;
}

bool AnimationContext::hasSignificantChange(const AnimationContext& other, f64 threshold) const
{
    // 快速检查：哈希相等则无变化
    if (stateHash == other.stateHash) {
        return false;
    }

    // 检查每个动画参数是否超过阈值
    auto checkDiff = [threshold](f64 a, f64 b) { return std::abs(a - b) > threshold; };

    // 主要动画参数
    if (checkDiff(limbSwing, other.limbSwing)) return true;
    if (checkDiff(limbSwingAmount, other.limbSwingAmount)) return true;
    if (checkDiff(netHeadYaw, other.netHeadYaw)) return true;
    if (checkDiff(headPitch, other.headPitch)) return true;

    // 缩放变化
    if (checkDiff(scale, other.scale)) return true;

    // 挥手进度
    if (checkDiff(static_cast<f64>(swingProgress), static_cast<f64>(other.swingProgress))) {
        return true;
    }
    if (checkDiff(static_cast<f64>(standingProgress), static_cast<f64>(other.standingProgress))) {
        return true;
    }

    // 河豚膨胀状态变化（立即更新网格）
    if (puffState != other.puffState) return true;

    // 吃草动画计时器变化（立即更新网格）
    if (eatAnimationTimer != other.eatAnimationTimer) return true;

    // 撞飞攻击动画计时器变化（立即更新网格）
    if (attackAnimationTicks != other.attackAnimationTicks) return true;

    // 狼甩水动画进度变化（立即更新网格）
    if (checkDiff(static_cast<f64>(wolfShakeAnim), static_cast<f64>(other.wolfShakeAnim))) return true;

    // 狼湿润着色变化（立即更新网格）
    if (checkDiff(static_cast<f64>(wolfWetShade), static_cast<f64>(other.wolfWetShade))) return true;

    // 狼乞求角度变化
    if (checkDiff(static_cast<f64>(wolfInterestedAngle), static_cast<f64>(other.wolfInterestedAngle))) {
        return true;
    }

    // 游泳动画渐变量变化（驱动 DrownedModel 手臂/腿部覆盖，需立即更新网格）
    if (checkDiff(static_cast<f64>(swimAmount), static_cast<f64>(other.swimAmount))) {
        return true;
    }

    // 凋灵侧头朝向变化（立即更新网格）
    for (i32 i = 0; i < 2; ++i) {
        if (checkDiff(static_cast<f64>(witherSideHeadYaw[i]), static_cast<f64>(other.witherSideHeadYaw[i]))) {
            return true;
        }
        if (checkDiff(static_cast<f64>(witherSideHeadPitch[i]), static_cast<f64>(other.witherSideHeadPitch[i]))) {
            return true;
        }
    }

    // 布尔状态变化（立即更新）
    if (isSitting != other.isSitting) return true;
    if (isChild != other.isChild) return true;
    if (isSneaking != other.isSneaking) return true;
    if (isSwimming != other.isSwimming) return true;
    if (isRiding != other.isRiding) return true;
    if (isAngry != other.isAngry) return true;

    return false;
}

} // namespace mc::client::renderer::entity::core
