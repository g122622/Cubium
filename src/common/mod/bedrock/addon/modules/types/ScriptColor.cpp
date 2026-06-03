#include "common/mod/bedrock/addon/modules/types/ScriptColor.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

namespace mc::mod::bedrock::addon {

std::optional<ScriptColor> ScriptColor::fromJs(IScriptBindingContext& ctx, void* obj)
{
    if (!ctx.isObject(obj)) {
        return std::nullopt;
    }

    auto rv = ctx.getPropertyFloat(obj, "red");
    auto gv = ctx.getPropertyFloat(obj, "green");
    auto bv = ctx.getPropertyFloat(obj, "blue");
    auto av = ctx.getPropertyFloat(obj, "alpha");

    if (!rv || !gv || !bv) {
        return std::nullopt;
    }

    return ScriptColor(
        static_cast<f32>(*rv), static_cast<f32>(*gv), static_cast<f32>(*bv), av ? static_cast<f32>(*av) : 1.0f);
}

void* ScriptColor::toJs(IScriptBindingContext& ctx) const
{
    void* obj = ctx.createObject();
    ctx.setPropertyFloat(obj, "red", static_cast<f64>(r));
    ctx.setPropertyFloat(obj, "green", static_cast<f64>(g));
    ctx.setPropertyFloat(obj, "blue", static_cast<f64>(b));
    ctx.setPropertyFloat(obj, "alpha", static_cast<f64>(a));
    return obj;
}

} // namespace mc::mod::bedrock::addon
