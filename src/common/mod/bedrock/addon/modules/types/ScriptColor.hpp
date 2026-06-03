#pragma once

#include "common/core/Types.hpp"

#include <optional>

namespace mc::mod::bedrock::addon {

class IScriptBindingContext;

/**
 * @brief 脚本层Color类型
 *
 * RGBA颜色，每个通道0.0-1.0范围，
 * 与Bedrock版@minecraft/server的Color接口一致。
 */
struct ScriptColor {
    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    f32 a = 1.0f;

    ScriptColor() = default;
    ScriptColor(f32 r_, f32 g_, f32 b_, f32 a_ = 1.0f)
        : r(r_)
        , g(g_)
        , b(b_)
        , a(a_)
    {}

    /// 从0-255整数构建
    static ScriptColor fromRGBA(u8 ri, u8 gi, u8 bi, u8 ai = 255)
    {
        return ScriptColor(ri / 255.0f, gi / 255.0f, bi / 255.0f, ai / 255.0f);
    }

    /// 从脚本对象读取Color
    static std::optional<ScriptColor> fromJs(IScriptBindingContext& ctx, void* obj);

    /// 转换为脚本对象（返回的句柄拥有引用所有权，调用者负责releaseValue）
    [[nodiscard]] void* toJs(IScriptBindingContext& ctx) const;
};

} // namespace mc::mod::bedrock::addon
