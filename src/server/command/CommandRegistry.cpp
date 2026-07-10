/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CommandRegistry.hpp"
#include "commands/AdvancementCommand.hpp"
#include "commands/AttributeCommand.hpp"
#include "commands/BanCommand.hpp"
#include "commands/BanIpCommand.hpp"
#include "commands/BanListCommand.hpp"
#include "commands/BossBarCommand.hpp"
#include "commands/ClearCommand.hpp"
#include "commands/CloneCommand.hpp"
#include "commands/DataCommand.hpp"
#include "commands/DataPackCommand.hpp"
#include "commands/DeOpCommand.hpp"
#include "commands/DefaultGameModeCommand.hpp"
#include "commands/DifficultyCommand.hpp"
#include "commands/EffectCommand.hpp"
#include "commands/EnchantCommand.hpp"
#include "commands/ExecuteCommand.hpp"
#include "commands/ExperienceCommand.hpp"
#include "commands/FillCommand.hpp"
#include "commands/ForceLoadCommand.hpp"
#include "commands/FunctionCommand.hpp"
#include "commands/GameModeCommand.hpp"
#include "commands/GameRuleCommand.hpp"
#include "commands/GiveCommand.hpp"
#include "commands/HelpCommand.hpp"
#include "commands/KickCommand.hpp"
#include "commands/KillCommand.hpp"
#include "commands/ListCommand.hpp"
#include "commands/LocateBiomeCommand.hpp"
#include "commands/LocateCommand.hpp"
#include "commands/LootCommand.hpp"
#include "commands/MeCommand.hpp"
#include "commands/MessageCommand.hpp"
#include "commands/OpCommand.hpp"
#include "commands/PardonCommand.hpp"
#include "commands/PardonIpCommand.hpp"
#include "commands/ParticleCommand.hpp"
#include "commands/PlaySoundCommand.hpp"
#include "commands/PublishCommand.hpp"
#include "commands/RecipeCommand.hpp"
#include "commands/ReloadCommand.hpp"
#include "commands/ReplaceItemCommand.hpp"
#include "commands/SaveAllCommand.hpp"
#include "commands/SaveOffCommand.hpp"
#include "commands/SaveOnCommand.hpp"
#include "commands/SayCommand.hpp"
#include "commands/ScheduleCommand.hpp"
#include "commands/ScoreboardCommand.hpp"
#include "commands/SeedCommand.hpp"
#include "commands/SetBlockCommand.hpp"
#include "commands/SetIdleTimeoutCommand.hpp"
#include "commands/SetWorldSpawnCommand.hpp"
#include "commands/SpawnPointCommand.hpp"
#include "commands/SpectateCommand.hpp"
#include "commands/SpreadPlayersCommand.hpp"
#include "commands/StopCommand.hpp"
#include "commands/StopSoundCommand.hpp"
#include "commands/SummonCommand.hpp"
#include "commands/TagCommand.hpp"
#include "commands/TeamCommand.hpp"
#include "commands/TeleportCommand.hpp"
#include "commands/TellRawCommand.hpp"
#include "commands/TimeCommand.hpp"
#include "commands/TitleCommand.hpp"
#include "commands/TriggerCommand.hpp"
#include "commands/WardenSpawnTrackerCommand.hpp"
#include "commands/WeatherCommand.hpp"
#include "commands/WhitelistCommand.hpp"
#include "commands/WorldBorderCommand.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc {
namespace command {

CommandRegistry::CommandRegistry()
    : m_dispatcher()
{
    registerDefaults();
}

Result<i32> CommandRegistry::execute(const std::string& input, ServerCommandSource& source)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "CommandRegistry::execute", "input", input);

    auto result = m_dispatcher.execute(input, source);
    if (result.success()) {
        return result.value().result();
    }
    return result.error();
}

std::future<Suggestions> CommandRegistry::getSuggestions(const std::string& input, ServerCommandSource& source)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "CommandRegistry::getSuggestions", "input", input);
    return m_dispatcher.getSuggestions(input, source);
}

CommandTreeSnapshot CommandRegistry::getCommandTreeSnapshot() const
{
    return buildCommandTreeSnapshot(m_dispatcher);
}

std::string CommandRegistry::getCommandTreeJson() const
{
    return getCommandTreeSnapshot().toJsonString();
}

void CommandRegistry::registerDefaults()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "CommandRegistry::registerDefaults");

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
    OpCommand::registerTo(m_dispatcher);
    DeOpCommand::registerTo(m_dispatcher);
    BanCommand::registerTo(m_dispatcher);
    BanIpCommand::registerTo(m_dispatcher);
    PardonCommand::registerTo(m_dispatcher);
    PardonIpCommand::registerTo(m_dispatcher);
    BanListCommand::registerTo(m_dispatcher);
    WhitelistCommand::registerTo(m_dispatcher);
    SaveAllCommand::registerTo(m_dispatcher);
    SaveOnCommand::registerTo(m_dispatcher);
    SaveOffCommand::registerTo(m_dispatcher);
    SpawnPointCommand::registerTo(m_dispatcher);
    SetWorldSpawnCommand::registerTo(m_dispatcher);
    MessageCommand::registerTo(m_dispatcher);
    TellRawCommand::registerTo(m_dispatcher);
    TitleCommand::registerTo(m_dispatcher);
    PlaySoundCommand::registerTo(m_dispatcher);
    StopSoundCommand::registerTo(m_dispatcher);
    EffectCommand::registerTo(m_dispatcher);
    EnchantCommand::registerTo(m_dispatcher);
    MeCommand::registerTo(m_dispatcher);
    ParticleCommand::registerTo(m_dispatcher);
    LocateCommand::registerTo(m_dispatcher);
    LocateBiomeCommand::registerTo(m_dispatcher);
    AttributeCommand::registerTo(m_dispatcher);
    CloneCommand::registerTo(m_dispatcher);
    DataCommand::registerTo(m_dispatcher);
    FunctionCommand::registerTo(m_dispatcher);
    ScheduleCommand::registerTo(m_dispatcher);
    SpreadPlayersCommand::registerTo(m_dispatcher);
    WorldBorderCommand::registerTo(m_dispatcher);
    ScoreboardCommand::registerTo(m_dispatcher);
    BossBarCommand::registerTo(m_dispatcher);
    TagCommand::registerTo(m_dispatcher);
    TeamCommand::registerTo(m_dispatcher);
    AdvancementCommand::registerTo(m_dispatcher);
    DataPackCommand::registerTo(m_dispatcher);
    ForceLoadCommand::registerTo(m_dispatcher);
    LootCommand::registerTo(m_dispatcher);
    PublishCommand::registerTo(m_dispatcher);
    RecipeCommand::registerTo(m_dispatcher);
    ReloadCommand::registerTo(m_dispatcher);
    ReplaceItemCommand::registerTo(m_dispatcher);
    SpectateCommand::registerTo(m_dispatcher);
    TriggerCommand::registerTo(m_dispatcher);
    GameRuleCommand::registerTo(m_dispatcher);
    WardenSpawnTrackerCommand::registerTo(m_dispatcher);
    m_defaultsRegistered = true;

    spdlog::info("[CommandRegistry] Registered {} default commands", getCommandNames().size());
}

std::vector<std::string> CommandRegistry::getCommandNames() const
{
    std::vector<std::string> names;
    const auto& children = m_dispatcher.getRoot()->getChildren();
    names.reserve(children.size());

    for (const auto& [name, child] : children) {
        (void)child;
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    return names;
}

bool CommandRegistry::hasCommand(const std::string& name) const noexcept
{
    return m_dispatcher.getRoot()->getChild(name) != nullptr;
}

CommandRegistry& CommandRegistry::getGlobal()
{
    static CommandRegistry registry;
    return registry;
}

} // namespace command
} // namespace mc
