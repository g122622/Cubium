#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"

#include <optional>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本层Vector2类型
 *
 * 与Bedrock版@minecraft/server的Vector2接口一致。
 * 使用f64以匹配JS的number类型。
 */
struct ScriptVec2 {
    f64 x = 0.0;
    f64 y = 0.0;

    ScriptVec2() = default;
    ScriptVec2(f64 x_, f64 y_)
        : x(x_)
        , y(y_)
    {}

    /// 从游戏Vector2f转换
    explicit ScriptVec2(const math::Vector2f& v)
        : x(static_cast<f64>(v.x))
        , y(static_cast<f64>(v.y))
    {}

    /// 从游戏Vector2d转换
    explicit ScriptVec2(const math::Vector2d& v)
        : x(v.x)
        , y(v.y)
    {}

    /// 转换为Vector2f
    [[nodiscard]] math::Vector2f toVec2f() const { return {static_cast<f32>(x), static_cast<f32>(y)}; }

    /// 从脚本对象读取Vector2
    static std::optional<ScriptVec2> fromJs(IScriptBindingContext& ctx, void* obj);

    /// 转换为脚本对象（返回的句柄拥有引用所有权，调用者负责releaseValue）
    [[nodiscard]] void* toJs(IScriptBindingContext& ctx) const;
};

} // namespace mc::mod::bedrock::addon
