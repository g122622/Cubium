// Copyright (c) 2024 CubiumMC. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.

#pragma once

#include "common/command/suggestions/Suggestions.hpp"
#include <future>
#include <string>
#include <utility>

namespace mc {
namespace command {

/**
 * @brief 无候选的建议提供器（仅用于声明「该参数由服务端补全」）
 *
 * 原版命令树在序列化参数节点时会写一个建议提供器标识：用
 * `SuggestionProviders.getName(provider)` 得到，只有经 `SuggestionProviders.register`
 * 包装过的提供器会返回自己的名字（如 `minecraft:summonable_entities`），其余一律
 * 回退成 `minecraft:ask_server`（见原版 SuggestionProviders.getName）。
 *
 * 客户端收到 `minecraft:ask_server` 后会把补全请求转发给服务端，服务端用
 * `customSuggestion(context)` 计算候选。因此在命令树里「声明了建议提供器」本身就是
 * 协议的一部分：没有声明，客户端在 Tab 补全时不会向服务端请求，表现为这些参数
 * 完全没有补全。
 *
 * 这个类用于那些候选由执行期数据决定的参数（玩家名、计分板项、队伍名、bossbar id
 * 等）：它自身不产生候选，只负责让命令树带上 `ask_server` 标记，把补全请求正确地
 * 转给服务端。
 *
 * TODO: 真正的候选计算需要在 ServerSuggestionProvider 之外接入各子系统的注册表
 *   （Scoreboard / PlayerList / BossBarManager / TeamManager），
 *   目前只保证协议层的建议提供器标识与补全请求转发与原版一致。
 */
template <typename S>
class ServerSuggestionProvider : public ISuggestionProvider<S> {
public:
    std::future<Suggestions> getSuggestions(CommandContext<S>& /*context*/, SuggestionsBuilder& builder) override
    {
        return builder.buildFuture();
    }
};

/**
 * @brief 客户端按注册名解析的建议提供器
 */
template <typename S>
class RegisteredSuggestionProvider : public ServerSuggestionProvider<S> {
public:
    explicit RegisteredSuggestionProvider(std::string id)
        : m_id(std::move(id))
    {}

    [[nodiscard]] std::string_view getProviderId() const noexcept override
    {
        return m_id;
    }

private:
    std::string m_id;
};

} // namespace command
} // namespace mc
