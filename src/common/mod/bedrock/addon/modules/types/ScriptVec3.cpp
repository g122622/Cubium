#include "common/mod/bedrock/addon/modules/types/ScriptVec3.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

namespace mc::mod::bedrock::addon {

std::optional<ScriptVec3> ScriptVec3::fromJs(IScriptBindingContext& ctx, void* obj)
{
    if (!ctx.isObject(obj)) {
        return std::nullopt;
    }

    auto xv = ctx.getPropertyFloat(obj, "x");
    auto yv = ctx.getPropertyFloat(obj, "y");
    auto zv = ctx.getPropertyFloat(obj, "z");

    if (!xv || !yv || !zv) {
        return std::nullopt;
    }

    return ScriptVec3(*xv, *yv, *zv);
}

void* ScriptVec3::toJs(IScriptBindingContext& ctx) const
{
    void* obj = ctx.createObject();
    ctx.setPropertyFloat(obj, "x", x);
    ctx.setPropertyFloat(obj, "y", y);
    ctx.setPropertyFloat(obj, "z", z);
    return obj;
}

} // namespace mc::mod::bedrock::addon
