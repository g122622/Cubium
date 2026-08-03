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

#include "PardonIpCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/BannedIpList.hpp"

#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

namespace {

/**
 * @brief 检查字符串是否为有效的 IPv4 地址
 * @param str 字符串
 * @return true 如果是有效的 IPv4 地址
 */
bool isValidIpv4(const std::string& str)
{
    // IPv4 正则表达式
    static const std::regex ipv4Regex(
        R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(str, ipv4Regex);
}

} // anonymous namespace

void PardonIpCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto pardonIpNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("pardon-ip");
    pardonIpNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(pardonIpNode,
        support::makeMetadata("Removes an IP address from the ban list.", "/pardon-ip <target>", 3, {}, false));

    // /pardon-ip <target>
    auto targetArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("target", StringArgumentType::string());
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _pardonIp(ctx); });

    pardonIpNode->addChild(targetArg);
    dispatcher.registerCommand(pardonIpNode);
}

i32 PardonIpCommand::_pardonIp(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string target = context.getArgument<std::string>("target");

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.pardonip.failed.noServer");
        return 0;
    }

    // 验证 IP 格式
    if (!isValidIpv4(target)) {
        source.sendError("commands.pardonip.failed.invalidIp");
        return 0;
    }

    auto& banList = server->bannedIpList();

    // 检查 IP 是否被封禁
    if (!banList.isBanned(target)) {
        std::ostringstream ss;
        ss << "Could not unban IP " << target << " (not banned)";
        source.sendError(ss.str());
        return 0;
    }

    // 移除封禁
    if (!banList.removeEntry(target)) {
        std::ostringstream ss;
        ss << "Failed to remove IP " << target << " from ban list";
        source.sendError(ss.str());
        return 0;
    }

    // 保存封禁列表
    auto saveResult = banList.save();
    if (saveResult.failed()) {
        spdlog::error("Failed to save banned IPs list: {}", saveResult.error().message());
    }

    // 发送成功消息
    std::ostringstream ss;
    ss << "Unbanned IP " << target;
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
