#include "CommandRegistry.hpp"
#include "commands/GameModeCommand.hpp"
#include "commands/DifficultyCommand.hpp"
#include "commands/DefaultGameModeCommand.hpp"
#include "commands/TimeCommand.hpp"
#include "commands/KickCommand.hpp"
#include "commands/KillCommand.hpp"
#include "commands/ListCommand.hpp"
#include "commands/HelpCommand.hpp"
#include "commands/SeedCommand.hpp"
#include "commands/TeleportCommand.hpp"
#include "commands/GiveCommand.hpp"
#include "commands/ClearCommand.hpp"
#include "commands/WeatherCommand.hpp"
#include "commands/ExperienceCommand.hpp"
#include "commands/SayCommand.hpp"
#include "commands/StopCommand.hpp"
#include "commands/SetIdleTimeoutCommand.hpp"
#include "commands/SummonCommand.hpp"
#include "commands/SetBlockCommand.hpp"
#include "commands/FillCommand.hpp"
#include "commands/ExecuteCommand.hpp"
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

    GameModeCommand::registerTo(m_dispatcher);
    DifficultyCommand::registerTo(m_dispatcher);
    DefaultGameModeCommand::registerTo(m_dispatcher);
    TimeCommand::registerTo(m_dispatcher);
    KickCommand::registerTo(m_dispatcher);
    KillCommand::registerTo(m_dispatcher);
    ListCommand::registerTo(m_dispatcher);
    HelpCommand::registerTo(m_dispatcher);
    SeedCommand::registerTo(m_dispatcher);
    TeleportCommand::registerTo(m_dispatcher);
    GiveCommand::registerTo(m_dispatcher);
    ClearCommand::registerTo(m_dispatcher);
    WeatherCommand::registerTo(m_dispatcher);
    ExperienceCommand::registerTo(m_dispatcher);
    SayCommand::registerTo(m_dispatcher);
    StopCommand::registerTo(m_dispatcher);
    SetIdleTimeoutCommand::registerTo(m_dispatcher);
    SummonCommand::registerTo(m_dispatcher);
    SetBlockCommand::registerTo(m_dispatcher);
    FillCommand::registerTo(m_dispatcher);
    ExecuteCommand::registerTo(m_dispatcher);
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
