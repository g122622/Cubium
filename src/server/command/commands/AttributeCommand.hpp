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

    static String normalizeAttributeName(const String& name);
    static bool isKnownAttribute(const String& name) noexcept;
    static f64 getAttributeDefaultValue(const String& name) noexcept;
    static std::pair<f64, f64> getAttributeRange(const String& name) noexcept;
};

} // namespace command
} // namespace mc
