#include "common/mod/bedrock/addon/modules/types/ScriptVec2.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

namespace mc::mod::bedrock::addon {

std::optional<ScriptVec2> ScriptVec2::fromJs(IScriptBindingContext& ctx, void* obj)
{
    if (!ctx.isObject(obj)) {
        return std::nullopt;
    }

    auto xv = ctx.getPropertyFloat(obj, "x");
    auto yv = ctx.getPropertyFloat(obj, "y");

    if (!xv || !yv) {
        return std::nullopt;
    }

    return ScriptVec2(*xv, *yv);
}

void* ScriptVec2::toJs(IScriptBindingContext& ctx) const
{
    void* obj = ctx.createObject();
    ctx.setPropertyFloat(obj, "x", x);
    ctx.setPropertyFloat(obj, "y", y);
    return obj;
}

} // namespace mc::mod::bedrock::addon
