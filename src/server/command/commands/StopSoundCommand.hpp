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
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <optional>

namespace mc {
namespace command {

/**
 * @brief /stopsound 命令
 *
 * 停止播放声音效果。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /stopsound <player>
 * - /stopsound <player> * [<sound>]
 * - /stopsound <player> <source>
 * - /stopsound <player> <source> <sound>
 *
 * 参考: net.minecraft.server.commands.StopSoundCommand
 */
class StopSoundCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 停止目标玩家的所有声音
     *
     * /stopsound <player>
     */
    static i32 _stopAllSounds(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 停止目标玩家指定类别的声音（可指定具体声音ID）
     *
     * /stopsound <player> <source> [<sound>]
     * /stopsound <player> * [<sound>]
     *
     * @param category 声音类别（nullopt 表示所有类别）
     * @param soundId 要停止的特定声音ID（nullopt 表示该类别下所有声音）
     */
    static i32 _stopSounds(CommandContext<ServerCommandSource>& context,
        std::optional<sound::SoundCategory> category,
        std::optional<ResourceLocation> soundId);
};

} // namespace command
} // namespace mc
