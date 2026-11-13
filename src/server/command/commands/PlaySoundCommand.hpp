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

#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "common/core/Types.hpp"
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
     * @brief 播放声音。
     *
     * 所有命令末端共用这一实现：未给出的可选参数取原版默认值（位置=命令源位置、
     * 音量=1、音调=1、最小音量=0）；未给出 targets 时目标是执行该命令的玩家。
     */
    static i32 _playSoundImpl(CommandContext<ServerCommandSource>& context, sound::SoundCategory category);
};

} // namespace command
} // namespace mc
