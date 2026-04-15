#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include <functional>
#include <vector>

namespace mc::client::command {

/**
 * @brief 客户端命令管理器
 *
 * 负责接收服务端同步的命令树快照，并在本地生成聊天框补全建议。
 */
class ClientCommandManager {
public:
    using CandidateProvider = std::function<std::vector<String>()>;

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
     * @param jsonText 服务端同步的 JSON 文本
     * @return 处理结果
     */
    [[nodiscard]] Result<void> applyCommandTreeJson(StringView jsonText);

    /**
     * @brief 检查是否已接收命令树
     */
    [[nodiscard]] bool hasCommandTree() const noexcept;

    /**
     * @brief 获取当前命令名称列表
     */
    [[nodiscard]] std::vector<String> getCommandNames() const;

    /**
     * @brief 获取补全建议
     * @param input 当前输入
     * @param cursor 当前光标位置
     * @return 建议列表
     */
    [[nodiscard]] mc::command::Suggestions getSuggestions(StringView input, i32 cursor) const;

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
    [[nodiscard]] const mc::command::CommandTreeNodeSnapshot* getNode(u32 nodeId) const;

    /**
     * @brief 收集某个节点的候选建议
     */
    [[nodiscard]] mc::command::Suggestions collectSuggestions(
        const mc::command::CommandTreeNodeSnapshot& node,
        StringView fullInput,
        i32 start,
        i32 end,
        StringView tokenPrefix) const;

    /**
     * @brief 获取节点候选项
     */
    [[nodiscard]] std::vector<String> getCandidates(
        const mc::command::CommandTreeNodeSnapshot& node) const;

    /**
     * @brief 判断是否应当将 token 视为固定候选
     */
    [[nodiscard]] bool matchesFixedCandidate(
        const mc::command::CommandTreeNodeSnapshot& node,
        StringView token) const;

    /**
     * @brief 判断是否为命令输入
     */
    [[nodiscard]] static bool isCommandInput(StringView input);

    /**
     * @brief 转小写
     */
    [[nodiscard]] static String toLower(StringView input);

    /**
     * @brief 不区分大小写的前缀匹配
     */
    [[nodiscard]] static bool startsWithIgnoreCase(StringView value, StringView prefix);

    mc::command::CommandTreeSnapshot m_snapshot;
    CandidateProvider m_playerNameProvider;
    CandidateProvider m_entityNameProvider;
    CandidateProvider m_itemNameProvider;
};

} // namespace mc::client::command
