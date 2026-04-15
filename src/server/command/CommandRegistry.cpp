#include "CommandRegistry.hpp"
#include "commands/GameModeCommand.hpp"
#include "commands/TimeCommand.hpp"
#include "commands/KillCommand.hpp"
#include "commands/ListCommand.hpp"
#include "commands/HelpCommand.hpp"
#include "commands/SeedCommand.hpp"
#include "commands/TeleportCommand.hpp"
#include "commands/GiveCommand.hpp"
#include "commands/ClearCommand.hpp"
#include "commands/WeatherCommand.hpp"
#include "commands/ExperienceCommand.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc {
namespace command {

CommandRegistry::CommandRegistry()
    : m_dispatcher()
{
    registerDefaults();
}

Result<i32> CommandRegistry::execute(const String& input, ServerCommandSource& source) {
    auto result = m_dispatcher.execute(input, source);
    if (result.success()) {
        return result.value().result();
    }
    return result.error();
}

std::future<Suggestions> CommandRegistry::getSuggestions(const String& input, ServerCommandSource& source) {
    return m_dispatcher.getSuggestions(input, source);
}

CommandTreeSnapshot CommandRegistry::getCommandTreeSnapshot() const {
    return buildCommandTreeSnapshot(m_dispatcher);
}

String CommandRegistry::getCommandTreeJson() const {
    return getCommandTreeSnapshot().toJsonString();
}

void CommandRegistry::registerDefaults() {
    if (m_defaultsRegistered) {
        return;
    }

    // 注册核心命令
    GameModeCommand::registerTo(m_dispatcher);
    TimeCommand::registerTo(m_dispatcher);
    KillCommand::registerTo(m_dispatcher);
    ListCommand::registerTo(m_dispatcher);
    HelpCommand::registerTo(m_dispatcher);
    SeedCommand::registerTo(m_dispatcher);
    TeleportCommand::registerTo(m_dispatcher);
    GiveCommand::registerTo(m_dispatcher);
    ClearCommand::registerTo(m_dispatcher);
    WeatherCommand::registerTo(m_dispatcher);
    ExperienceCommand::registerTo(m_dispatcher);
    m_defaultsRegistered = true;

    spdlog::info("[CommandRegistry] Registered {} default commands", getCommandNames().size());
}

std::vector<String> CommandRegistry::getCommandNames() const {
    std::vector<String> names;
    const auto& children = m_dispatcher.getRoot()->getChildren();
    names.reserve(children.size());

    for (const auto& [name, child] : children) {
        (void)child;
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    return names;
}

bool CommandRegistry::hasCommand(const String& name) const {
    return m_dispatcher.getRoot()->getChild(name) != nullptr;
}

CommandRegistry& CommandRegistry::getGlobal() {
    static CommandRegistry registry;
    return registry;
}

} // namespace command
} // namespace mc
