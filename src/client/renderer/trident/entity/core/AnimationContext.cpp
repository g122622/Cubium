#include "AnimationContext.hpp"
#include <functional>

namespace mc::client::renderer::entity::core {

void AnimationContext::computeHash() {
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

    // 组合布尔状态（转换为 0.0 或 1.0）
    hash = hashCombine(hash, isSitting ? 1.0 : 0.0);
    hash = hashCombine(hash, isChild ? 1.0 : 0.0);
    hash = hashCombine(hash, isSneaking ? 1.0 : 0.0);
    hash = hashCombine(hash, isSwimming ? 1.0 : 0.0);
    hash = hashCombine(hash, isRiding ? 1.0 : 0.0);

    stateHash = hash;
}

bool AnimationContext::hasSignificantChange(
    const AnimationContext& other,
    f64 threshold
) const {
    // 快速检查：哈希相等则无变化
    if (stateHash == other.stateHash) {
        return false;
    }

    // 检查每个动画参数是否超过阈值
    auto checkDiff = [threshold](f64 a, f64 b) {
        return std::abs(a - b) > threshold;
    };

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

    // 布尔状态变化（立即更新）
    if (isSitting != other.isSitting) return true;
    if (isChild != other.isChild) return true;
    if (isSneaking != other.isSneaking) return true;
    if (isSwimming != other.isSwimming) return true;
    if (isRiding != other.isRiding) return true;

    return false;
}

} // namespace mc::client::renderer::entity::core
