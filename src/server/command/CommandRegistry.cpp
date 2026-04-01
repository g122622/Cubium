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

    // 记录命令名称
    m_commandNames = {
        "gamemode", "time", "kill", "list", "help", "seed", "tp", "give", "clear", "weather",
        "experience", "xp"
    };
    m_commandNameSet = std::unordered_set<String>(m_commandNames.begin(), m_commandNames.end());
    m_defaultsRegistered = true;

    spdlog::info("[CommandRegistry] Registered {} default commands", m_commandNames.size());
}

std::vector<String> CommandRegistry::getCommandNames() const {
    return m_commandNames;
}

bool CommandRegistry::hasCommand(const String& name) const {
    return m_commandNameSet.find(name) != m_commandNameSet.end();
}

CommandRegistry& CommandRegistry::getGlobal() {
    static CommandRegistry registry;
    return registry;
}

} // namespace command
} // namespace mc
