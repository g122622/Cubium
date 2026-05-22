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

#include "SignCommandHelper.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace server {

bool SignCommandHelper::executeSignCommands(blockentity::SignEntity& signEntity, mc::ServerPlayer& player)
{

    // MC 1.16.5: 参考 SignTileEntity.executeCommand()
    // 遍历所有行文本，检查并执行点击事件
    bool executedAny = false;

    // 获取告示牌位置
    const BlockPos& signPos = signEntity.getPos();

    // 遍历所有行
    for (i32 lineIdx = 0; lineIdx < blockentity::SignEntity::LINE_COUNT; ++lineIdx) {
        const text::ITextComponent* line = signEntity.getLine(lineIdx);
        if (!line) {
            continue;
        }

        // 获取点击事件
        const text::Style& style = line->getStyle();
        const text::ClickEvent* clickEvent = style.getClickEvent();

        if (clickEvent && clickEvent->isValid()) {
            switch (clickEvent->getAction()) {
                case text::ClickAction::RunCommand: {
                    // MC 1.16.5: 服务端执行命令
                    std::string command = clickEvent->getValue();
                    if (executeCommand(command, player, signPos)) {
                        executedAny = true;
                    }
                    break;
                }
                case text::ClickAction::SuggestCommand: {
                    // MC 1.16.5: 客户端功能 - 将命令填入聊天输入框
                    // 服务端不处理，由客户端实现
                    break;
                }
                case text::ClickAction::OpenUrl: {
                    // MC 1.16.5: 客户端功能 - 打开 URL
                    // 服务端不处理，由客户端实现
                    break;
                }
                case text::ClickAction::CopyToClipboard: {
                    // MC 1.16.5: 客户端功能 - 复制到剪贴板
                    // 服务端不处理，由客户端实现
                    break;
                }
                case text::ClickAction::OpenFile: {
                    // MC 1.16.5: 出于安全原因，不自动执行 OpenFile
                    break;
                }
            }
        }

        // 递归检查子组件
        for (const auto& sibling : line->getSiblings()) {
            if (sibling) {
                const text::Style& siblingStyle = sibling->getStyle();
                const text::ClickEvent* siblingClick = siblingStyle.getClickEvent();
                if (siblingClick && siblingClick->isValid() &&
                    siblingClick->getAction() == text::ClickAction::RunCommand) {
                    // 执行子组件中的命令
                    std::string command = siblingClick->getValue();
                    if (executeCommand(command, player, signPos)) {
                        executedAny = true;
                    }
                }
            }
        }
    }

    return executedAny;
}

bool SignCommandHelper::executeCommand(const std::string& command, mc::ServerPlayer& player, const BlockPos& signPos)
{

    // 检查服务器引用
    if (player.getServer() == nullptr) {
        spdlog::warn("SignCommandHelper: player {} has no server reference", player.username());
        return false;
    }

    // 准备命令字符串
    std::string cmd = command;
    if (!cmd.empty() && cmd[0] != '/') {
        cmd = "/" + cmd;
    }

    // 创建命令源
    // MC 1.16.5: 告示牌命令源的权限级别为 2，位置为告示牌位置
    command::ServerCommandSource source(player.getServer(),
        &player,
        player.dimension(),
        Vector3d(
            static_cast<f64>(signPos.x) + 0.5, static_cast<f64>(signPos.y) + 0.5, static_cast<f64>(signPos.z) + 0.5),
        Vector2f(0.0f, 0.0f),
        2, // 权限级别 2（相当于 OP 级别）
        player.playerId(),
        player.username());

    // 执行命令
    auto result = player.getServer()->commandRegistry().execute(cmd, source);

    if (result.failed()) {
        // 命令执行失败，发送错误消息给玩家
        player.sendSystemMessage("§c" + result.error().message());
        return false;
    }

    return true;
}

} // namespace server
} // namespace mc
