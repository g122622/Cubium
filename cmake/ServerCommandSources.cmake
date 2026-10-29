# ===========================================================================
# 服务端命令实现源清单（相对 src/server）
#
# CommandRegistry::registerBuiltinCommands() 引用全部命令的 registerTo()，因此任何
# 链接 CommandRegistry 的目标都必须编译同一份命令实现清单：
#   - minecraft-server（生产服务端）
#   - mc_benchmark（ServerInitializeBenchmark 走同一条启动链路）
#
# 历史上两处各写一份，新增命令时只改一边，另一边在链接期报
# "undefined symbol: ...Command::registerTo"。新增命令时只改本文件即可，
# 两个 target 自动同步。
# ===========================================================================

set(MC_SERVER_COMMAND_SOURCES
    command/commands/GameModeCommand.cpp
    command/commands/DifficultyCommand.cpp
    command/commands/DefaultGameModeCommand.cpp
    command/commands/TimeCommand.cpp
    command/commands/KickCommand.cpp
    command/commands/KillCommand.cpp
    command/commands/ListCommand.cpp
    command/commands/HelpCommand.cpp
    command/commands/SeedCommand.cpp
    command/commands/TeleportCommand.cpp
    command/commands/GiveCommand.cpp
    command/commands/ClearCommand.cpp
    command/commands/WeatherCommand.cpp
    command/commands/ExperienceCommand.cpp
    command/commands/SayCommand.cpp
    command/commands/StopCommand.cpp
    command/commands/SetIdleTimeoutCommand.cpp
    command/commands/DebugConfigCommand.cpp
    command/commands/SummonCommand.cpp
    command/commands/SetBlockCommand.cpp
    command/commands/FillCommand.cpp
    command/commands/ExecuteCommand.cpp
    command/commands/OpCommand.cpp
    command/commands/DeOpCommand.cpp
    command/commands/BanCommand.cpp
    command/commands/BanIpCommand.cpp
    command/commands/BanListCommand.cpp
    command/commands/PardonCommand.cpp
    command/commands/PardonIpCommand.cpp
    command/commands/WhitelistCommand.cpp
    command/commands/SaveAllCommand.cpp
    command/commands/SaveOnCommand.cpp
    command/commands/SaveOffCommand.cpp
    command/commands/SpawnPointCommand.cpp
    command/commands/SetWorldSpawnCommand.cpp
    command/commands/MessageCommand.cpp
    command/commands/TellRawCommand.cpp
    command/commands/TitleCommand.cpp
    command/commands/DialogCommand.cpp
    command/commands/PlaySoundCommand.cpp
    command/commands/StopSoundCommand.cpp
    command/commands/EffectCommand.cpp
    command/commands/EnchantCommand.cpp
    command/commands/MeCommand.cpp
    command/commands/ParticleCommand.cpp
    command/commands/LocateCommand.cpp
    command/commands/LocateBiomeCommand.cpp
    command/commands/AttributeCommand.cpp
    command/commands/CloneCommand.cpp
    command/commands/DataCommand.cpp
    command/commands/FunctionCommand.cpp
    command/commands/ScheduleCommand.cpp
    command/commands/SpreadPlayersCommand.cpp
    command/commands/WorldBorderCommand.cpp
    command/commands/ScoreboardCommand.cpp
    command/commands/BossBarCommand.cpp
    command/commands/TagCommand.cpp
    command/commands/TeamCommand.cpp
    command/commands/AdvancementCommand.cpp
    command/commands/DataPackCommand.cpp
    command/commands/ForceLoadCommand.cpp
    command/commands/GameRuleCommand.cpp
    command/commands/VersionCommand.cpp
    command/commands/JfrCommand.cpp
    command/commands/PerfCommand.cpp
    command/commands/TeamMsgCommand.cpp
    command/commands/ReturnCommand.cpp
    command/commands/StopwatchCommand.cpp
    command/commands/RotateCommand.cpp
    command/commands/RideCommand.cpp
    command/commands/DamageCommand.cpp
    command/commands/DebugCommand.cpp
    command/commands/LootCommand.cpp
    command/commands/PublishCommand.cpp
    command/commands/RecipeCommand.cpp
    command/commands/ReloadCommand.cpp
    command/commands/ReplaceItemCommand.cpp
    command/commands/SpectateCommand.cpp
    command/commands/TriggerCommand.cpp
    # 交互式终端面板（非原版命令，只服务于 --interactive，不参与命令树差分）
    command/commands/PanelCommand.cpp
)

# 同一清单的绝对路径形式，供 benchmark 等不位于 src/server 目录下的 target 使用。
set(MC_SERVER_COMMAND_SOURCES_ABS "")
foreach(_mcCommandSource IN LISTS MC_SERVER_COMMAND_SOURCES)
    list(APPEND MC_SERVER_COMMAND_SOURCES_ABS "${CMAKE_SOURCE_DIR}/src/server/${_mcCommandSource}")
endforeach()
unset(_mcCommandSource)