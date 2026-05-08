#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief AttributeCommand - 修改实体属性
 *
 * 用法: /attribute <target> <attribute> (get|set|base|modifier)
 * 权限: 2 (游戏管理员)
 */
class AttributeCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 getAttribute(CommandContext<ServerCommandSource>& context);
    static i32 setAttributeBase(CommandContext<ServerCommandSource>& context);

    static std::string normalizeAttributeName(const std::string& name);
    static bool isKnownAttribute(const std::string& name) noexcept;
    static f64 getAttributeDefaultValue(const std::string& name) noexcept;
    static std::pair<f64, f64> getAttributeRange(const std::string& name) noexcept;
};

} // namespace command
} // namespace mc
