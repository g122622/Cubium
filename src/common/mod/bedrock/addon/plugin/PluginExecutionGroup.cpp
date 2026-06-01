#include "common/mod/bedrock/addon/plugin/PluginExecutionGroup.hpp"

namespace mc::mod::bedrock::addon {

const char* pluginExecutionGroupName(PluginExecutionGroup group)
{
    switch (group) {
        case PluginExecutionGroup::PrePackLoad:
            return "PrePackLoad";
        case PluginExecutionGroup::ServerStart:
            return "ServerStart";
        case PluginExecutionGroup::ClientLevel:
            return "ClientLevel";
    }
    return "Unknown";
}

} // namespace mc::mod::bedrock::addon
