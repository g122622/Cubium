#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace command {
class CommandRegistry;
}

namespace function {

/**
 * @brief 命令函数 - 表示从 .mcfunction 文件解析出的命令序列
 *
 * 每个 CommandFunction 包含一个函数 ID 和一组按行解析的命令字符串。
 * 命令在执行时通过 CommandRegistry 逐行解析和执行。
 *
 * TODO: 当前不支持宏函数（MacroFunction，$variable 语法），MC Java 的宏函数
 * 需要 CompoundTag 参数实例化（将 $(var) 替换为 NBT 值），这超出了当前命令系统的范畴。
 * 宏函数行（以 $ 开头的行）在 FunctionLoader::parseFunctionContent 中被跳过并记录警告。
 * 完整实现需要：1) MacroFunction 类 2) 参数实例化机制 3) CommandFunction 与 MacroFunction 的统一接口
 */
class CommandFunction {
public:
    /**
     * @brief 构造命令函数
     * @param id 函数的资源位置 ID（如 minecraft:foo/bar）
     * @param commands 按行解析的命令字符串列表（不含 / 前缀和注释行）
     */
    CommandFunction(ResourceLocation id, std::vector<std::string> commands);

    ~CommandFunction() = default;

    CommandFunction(const CommandFunction&) = default;
    CommandFunction& operator=(const CommandFunction&) = default;
    CommandFunction(CommandFunction&&) = default;
    CommandFunction& operator=(CommandFunction&&) = default;

    /**
     * @brief 获取函数 ID
     */
    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }

    /**
     * @brief 获取命令列表
     */
    [[nodiscard]] const std::vector<std::string>& commands() const noexcept { return m_commands; }

    /**
     * @brief 检查函数是否为空（无命令）
     */
    [[nodiscard]] bool isEmpty() const noexcept { return m_commands.empty(); }

    /**
     * @brief 获取命令数量
     */
    [[nodiscard]] Size commandCount() const noexcept { return m_commands.size(); }

private:
    ResourceLocation m_id;
    std::vector<std::string> m_commands;
};

} // namespace function
} // namespace mc
