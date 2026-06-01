#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"

namespace mc::mod::bedrock::addon {

ScriptPackPermissions::ScriptPackPermissions(const std::vector<std::string>& capabilities)
{
    for (const auto& cap : capabilities) {
        if (cap == "script_eval") {
            setPermission(ScriptPermission::AllowEval);
        }
    }
}

void ScriptPackPermissions::setPermission(ScriptPermission perm, bool enabled)
{
    if (enabled) {
        m_flags |= static_cast<u32>(perm);
    } else {
        m_flags &= ~static_cast<u32>(perm);
    }
}

bool ScriptPackPermissions::hasPermission(ScriptPermission perm) const
{
    return (m_flags & static_cast<u32>(perm)) != 0;
}

} // namespace mc::mod::bedrock::addon
