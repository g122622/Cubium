#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief System 分类标签位掩码
 *
 * 决定 system 走哪条 tick 路径：
 *   - Game 走主 tick；
 *   - Editor 仅编辑器 tick（项目无编辑器，首批不启用）；
 *   - Movement（=UsedInServerPlayerMovement | UsedInClientMovementCorrections）既走主 tick，
 *     也走 tickMovementCatchup / tickMovementCorrectionReplay 双专用路径（首批占位空实现，
 *     Cubium 客户端 ClientEntity 独立不跑物理，无消费方）。
 *
 * 一个 system 可同时挂多个 category（位或注册）。首批所有 system 都挂 Game（走主 tick），
 * 双 movement 路径暂无消费方。
 */
enum class SystemCategory : u32 {
    None = 0,

    /// 游戏运行时 system——进主 tick 路径（registerTickingSystem 默认 category）
    Game = 1 << 0,

    /// 编辑器专用 system（项目无编辑器，首批不启用）
    Editor = 1 << 1,

    /// 参与服务端玩家移动校正回放（tickMovementCorrectionReplay 路径）
    UsedInServerPlayerMovement = 1 << 2,

    /// 参与客户端移动预测追帧（tickMovementCatchup 路径）
    UsedInClientMovementCorrections = 1 << 3,

    /// 移动 system——同时挂服务端移动 + 客户端校正两标记
    Movement = UsedInServerPlayerMovement | UsedInClientMovementCorrections,
};

/// 位或运算（注册多 category）
[[nodiscard]] constexpr SystemCategory operator|(SystemCategory a, SystemCategory b) noexcept
{
    return static_cast<SystemCategory>(static_cast<u32>(a) | static_cast<u32>(b));
}

/// 位与运算（判定是否含某 category）
[[nodiscard]] constexpr SystemCategory operator&(SystemCategory a, SystemCategory b) noexcept
{
    return static_cast<SystemCategory>(static_cast<u32>(a) & static_cast<u32>(b));
}

/// 位反运算
[[nodiscard]] constexpr SystemCategory operator~(SystemCategory a) noexcept
{
    return static_cast<SystemCategory>(~static_cast<u32>(a));
}

/// 判定 flags 是否包含 category
[[nodiscard]] constexpr bool hasCategory(SystemCategory flags, SystemCategory category) noexcept
{
    return (flags & category) != SystemCategory::None;
}

} // namespace mc::ecs
