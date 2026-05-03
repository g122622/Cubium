#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "common/sound/SoundCategory.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /playsound 命令
 *
 * 播放声音效果。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /playsound <sound> <source> <player>
 * - /playsound <sound> <source> <player> <pos>
 * - /playsound <sound> <source> <player> <pos> <volume>
 * - /playsound <sound> <source> <player> <pos> <volume> <pitch>
 * - /playsound <sound> <source> <player> <pos> <volume> <pitch> <minimumVolume>
 *
 * 参数:
 * - sound: 声音事件ID（如 minecraft:block.stone.break）
 * - source: 声音类别（master, music, record, weather, block, hostile, neutral, player, ambient, voice）
 * - player: 目标玩家选择器
 * - pos: 声音播放位置（可选，默认为玩家位置）
 * - volume: 音量（0.0-1000.0，可选，默认1.0）
 * - pitch: 音调（0.0-2.0，可选，默认1.0）
 * - minimumVolume: 最小音量（0.0-1.0，可选，默认0.0）
 *
 * 参考: net.minecraft.command.impl.PlaySoundCommand
 */
class PlaySoundCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 在玩家位置播放声音（默认参数）
     */
    static i32 playSoundDefault(CommandContext<ServerCommandSource>& context, sound::SoundCategory category);

    /**
     * @brief 在指定位置播放声音
     */
    static i32 playSoundAtPosition(CommandContext<ServerCommandSource>& context, sound::SoundCategory category);

    /**
     * @brief 在指定位置播放声音（带音量）
     */
    static i32 playSoundWithVolume(CommandContext<ServerCommandSource>& context, sound::SoundCategory category);

    /**
     * @brief 在指定位置播放声音（带音量和音调）
     */
    static i32 playSoundWithPitch(CommandContext<ServerCommandSource>& context, sound::SoundCategory category);

    /**
     * @brief 在指定位置播放声音（带所有参数，包括最小音量）
     */
    static i32 playSoundWithMinVolume(CommandContext<ServerCommandSource>& context, sound::SoundCategory category);
};

} // namespace command
} // namespace mc
