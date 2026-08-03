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

#include "common/command/CommandTreeSnapshot.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mc::client::command {

/**
 * @brief 客户端命令管理器
 *
 * 负责接收服务端同步的命令树快照，并在本地生成聊天框补全建议。
 */
class ClientCommandManager {
public:
    using CandidateProvider = std::function<std::vector<std::string>()>;

    /**
     * @brief 构造命令管理器
     */
    ClientCommandManager();

    /**
     * @brief 清空当前命令树
     */
    void clear();

    /**
     * @brief 应用命令树 JSON
     * @param jsonText 服务端同步的 JSON 文本。会在收到服务端相关数据包时被调用
     * @return 处理结果
     */
    [[nodiscard]] Result<void> applyCommandTreeJson(std::string_view jsonText);

    /**
     * @brief 检查是否已接收命令树
     */
    [[nodiscard]] bool hasCommandTree() const noexcept;

    /**
     * @brief 获取当前命令名称列表
     */
    [[nodiscard]] std::vector<std::string> getCommandNames() const;

    /**
     * @brief 获取补全建议
     * @param input 当前输入
     * @param cursor 当前光标位置
     * @return 建议列表
     */
    [[nodiscard]] mc::command::Suggestions getSuggestions(std::string_view input, i32 cursor) const;

    /**
     * @brief 设置玩家名候选提供器
     */
    void setPlayerNameProvider(CandidateProvider provider);

    /**
     * @brief 设置实体名候选提供器
     */
    void setEntityNameProvider(CandidateProvider provider);

    /**
     * @brief 设置物品名候选提供器
     */
    void setItemNameProvider(CandidateProvider provider);

private:
    /**
     * @brief 获取指定节点
     */
    [[nodiscard]] const mc::command::CommandTreeNodeSnapshot* _getNode(u32 nodeId) const;

    /**
     * @brief 收集某个节点的候选建议
     */
    [[nodiscard]] mc::command::Suggestions _collectSuggestions(const mc::command::CommandTreeNodeSnapshot& node,
        std::string_view fullInput,
        i32 start,
        i32 end,
        std::string_view tokenPrefix) const;

    /**
     * @brief 获取节点候选项
     */
    [[nodiscard]] std::vector<std::string> _getCandidates(const mc::command::CommandTreeNodeSnapshot& node) const;

    /**
     * @brief 判断是否应当将 token 视为固定候选
     */
    [[nodiscard]] bool _matchesFixedCandidate(
        const mc::command::CommandTreeNodeSnapshot& node, std::string_view token) const;

    /**
     * @brief 判断是否为命令输入
     */
    [[nodiscard]] static bool _isCommandInput(std::string_view input);

    /**
     * @brief 转小写
     */
    [[nodiscard]] static std::string _toLower(std::string_view input);

    /**
     * @brief 不区分大小写的前缀匹配
     */
    [[nodiscard]] static bool _startsWithIgnoreCase(std::string_view value, std::string_view prefix);

    mc::command::CommandTreeSnapshot m_snapshot;
    CandidateProvider m_playerNameProvider;
    CandidateProvider m_entityNameProvider;
    CandidateProvider m_itemNameProvider;
};

} // namespace mc::client::command
