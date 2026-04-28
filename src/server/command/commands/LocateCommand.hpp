#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief LocateCommand - 定位结构
 *
 * 用法: /locate <structure>
 * 权限: 0 (所有玩家)
 */
class LocateCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 locateStructure(CommandContext<ServerCommandSource>& context);
    static String normalizeStructureName(const String& name);
};

} // namespace command
} // namespace mc
