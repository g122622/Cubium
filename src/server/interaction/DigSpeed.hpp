#pragma once

#include "common/core/Types.hpp"

namespace mc::server::interaction {

/**
 * @brief 影响挖掘速度的全部输入
 *
 * 这些量对应原版 `Player.getDestroySpeed` 链上的每一个因子。把它们收进一个纯数据
 * 结构体，是为了让公式本身可以被单独验证——差分测试需要在没有完整世界的条件下
 * 覆盖这条链，而 `MiningManager` 的成员函数依赖 `ServerWorld` 与 `ServerPlayerData`。
 */
struct DigSpeedInputs {
    /// 手持物品对方块的破坏速度（原版 ItemStack.getDestroySpeed）
    f32 toolSpeed;

    /// mining_efficiency 属性值（效率附魔经修饰符折算而来；无附魔为 0）
    f32 miningEfficiency;

    /// 急迫等级的 amplifier（等级减一）；没有急迫时为 -1
    i32 hasteAmplifier;

    /// 潮涌能量等级的 amplifier；没有该效果时为 -1
    i32 conduitAmplifier;

    /// 挖掘疲劳等级的 amplifier；没有该效果时为 -1
    i32 miningFatigueAmplifier;

    /// block_break_speed 属性值（默认 1.0）
    f32 blockBreakSpeed;

    /// submerged_mining_speed 属性值（默认 0.2，水下速掘把它提到 1.0）
    f32 submergedMiningSpeed;

    /// 眼睛是否在水中
    bool eyesInWater;

    /// 是否站在地面上
    bool onGround;
};

/**
 * @brief 计算挖掘速度
 *
 * 逐环对应原版 `Player.getDestroySpeed`：
 *
 *   1. 基础速度取手持物品对方块的破坏速度；
 *   2. **仅当基础速度 > 1.0** 时叠加 mining_efficiency（空手挖泥土不吃效率附魔）；
 *   3. 有急迫或潮涌能量时乘 `1 + (max(amp) + 1) * 0.2`；
 *   4. 有挖掘疲劳时按等级乘 0.3 / 0.09 / 0.0027 / 8.1e-4；
 *   5. 乘 block_break_speed；
 *   6. 眼睛在水中时乘 submerged_mining_speed；
 *   7. 不在空中时除以 5（在空中则不变）。
 *
 * 第 2 环的 `> 1.0` 判断是整条链里最容易漏掉的一处：它意味着效率附魔对「手挖软方块」
 * 完全无效，而不是简单地加到速度上。
 *
 * @param in 全部输入
 * @return 挖掘速度（越大越快）
 */
[[nodiscard]] f32 digSpeed(const DigSpeedInputs& in);

} // namespace mc::server::interaction
