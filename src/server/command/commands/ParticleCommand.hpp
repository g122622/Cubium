#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace client::renderer::trident::particle {
enum class ParticleTypeId : u16;
}

namespace command {

/**
 * @brief ParticleCommand - 显示粒子效果
 *
 * 用法: /particle <name> [<pos>]
 * 权限: 2 (游戏管理员)
 */
class ParticleCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 spawnParticle(CommandContext<ServerCommandSource>& context);
    static std::optional<client::renderer::trident::particle::ParticleTypeId>
    parseParticleType(const std::string& name) noexcept;
};

} // namespace command
} // namespace mc
