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
#include "common/core/Types.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TextEvents.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <string>
#include <spdlog/spdlog.h>

namespace mc {
namespace server {

bool SignCommandHelper::executeSignCommands(blockentity::SignEntity& signEntity, mc::ServerPlayer& player)
{
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
                    // 服务端执行命令
                    std::string command = clickEvent->getValue();
                    if (_executeCommand(command, player, signPos)) {
                        executedAny = true;
                    }
                    break;
                }
                case text::ClickAction::SuggestCommand:
                case text::ClickAction::OpenUrl:
                case text::ClickAction::CopyToClipboard:
                case text::ClickAction::OpenFile:
                    // 客户端功能，服务端不处理
                    break;
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
                    if (_executeCommand(command, player, signPos)) {
                        executedAny = true;
                    }
                }
            }
        }
    }

    return executedAny;
}

bool SignCommandHelper::_executeCommand(const std::string& command, mc::ServerPlayer& player, const BlockPos& signPos)
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

    // 创建命令源（使用玩家自身的权限等级，位置为告示牌位置）
    // 对齐 MC Java: 告示牌执行命令时，CommandSourceStack.entity = 编辑告示牌的玩家
    i32 signPermissionLevel = std::min(player.permissionLevel(), 2);
    command::ServerCommandSource source(player.getServer(),
        &player,
        player.dimension(),
        Vector3d(
            static_cast<f64>(signPos.x) + 0.5, static_cast<f64>(signPos.y) + 0.5, static_cast<f64>(signPos.z) + 0.5),
        Vector2f(0.0f, 0.0f),
        signPermissionLevel,
        player.playerId(),
        player.username(),
        static_cast<Entity*>(&player));

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
