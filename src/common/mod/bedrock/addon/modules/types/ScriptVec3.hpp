#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"

#include <optional>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本层Vector3类型
 *
 * 与Bedrock版@minecraft/server的Vector3接口一致。
 * 使用f64（双精度浮点数）以匹配JS的number类型。
 * 同时用于表示BlockPos（整数值的Vector3）。
 */
struct ScriptVec3 {
    f64 x = 0.0;
    f64 y = 0.0;
    f64 z = 0.0;

    ScriptVec3() = default;
    ScriptVec3(f64 x_, f64 y_, f64 z_)
        : x(x_)
        , y(y_)
        , z(z_)
    {}

    /// 从游戏Vector3f转换
    explicit ScriptVec3(const math::Vector3f& v)
        : x(static_cast<f64>(v.x))
        , y(static_cast<f64>(v.y))
        , z(static_cast<f64>(v.z))
    {}

    /// 从游戏Vector3d转换
    explicit ScriptVec3(const math::Vector3d& v)
        : x(v.x)
        , y(v.y)
        , z(v.z)
    {}

    /// 从BlockPos转换（整数值）
    explicit ScriptVec3(const BlockPos& p)
        : x(static_cast<f64>(p.x))
        , y(static_cast<f64>(p.y))
        , z(static_cast<f64>(p.z))
    {}

    /// 转换为Vector3f
    [[nodiscard]] math::Vector3f toVec3f() const
    {
        return {static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)};
    }

    /// 转换为Vector3d
    [[nodiscard]] math::Vector3d toVec3d() const { return {x, y, z}; }

    /// 转换为BlockPos
    [[nodiscard]] BlockPos toBlockPos() const
    {
        return {static_cast<BlockCoord>(x), static_cast<BlockCoord>(y), static_cast<BlockCoord>(z)};
    }

    /// 从脚本对象读取Vector3
    static std::optional<ScriptVec3> fromJs(IScriptBindingContext& ctx, void* obj);

    /// 转换为脚本对象（返回的句柄拥有引用所有权，调用者负责releaseValue）
    [[nodiscard]] void* toJs(IScriptBindingContext& ctx) const;
};

} // namespace mc::mod::bedrock::addon
