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

#include "ParticleCommand.hpp"

#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <sstream>
#include <unordered_map>

namespace mc {
namespace command {

void ParticleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto particleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("particle");
    particleNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(particleNode,
        support::makeMetadata("Creates particles at a position.", "/particle <name> [<pos>]", 2, {}, true));

    auto nameArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("name", StringArgumentType::string());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _spawnParticle(ctx); });

    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>("pos", Vec3ArgumentType::vec3());
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _spawnParticle(ctx); });

    nameArg->addChild(posArg);
    particleNode->addChild(nameArg);
    dispatcher.registerCommand(particleNode);
}

i32 ParticleCommand::_spawnParticle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const std::string name = context.getArgument<std::string>("name");
    Vector3d pos = source.position();

    // 如果提供了位置参数，使用它
    if (context.hasArgument("pos")) {
        pos = context.getArgument<Vector3d>("pos");
    }

    // 解析粒子类型
    auto particleType = _parseParticleType(name);
    if (!particleType.has_value()) {
        source.sendError("Unknown particle type: " + name);
        return 0;
    }

    // 获取服务器实例
    auto* server = source.server();
    if (!server) {
        source.sendError("Server not available");
        return 0;
    }

    // 广播粒子效果
    // 默认速度为 0，数量为 1，偏移为 0
    // 粒子广播范围为 256 格（与 ParticlePacket 默认范围一致）
    server->broadcastParticleInRange(static_cast<u32>(particleType.value()),
        pos.x,
        pos.y,
        pos.z,
        0.0f,
        0.0f,
        0.0f, // velocity
        0.0f,
        0.0f,
        0.0f,  // offset
        1,     // count
        256.0f // range
    );

    std::ostringstream ss;
    ss << "Displayed particle '" << name << "' at " << pos.x << ", " << pos.y << ", " << pos.z;
    source.sendMessage(ss.str());

    return 1;
}

std::optional<client::renderer::trident::particle::ParticleTypeId> ParticleCommand::_parseParticleType(
    const std::string& name) noexcept
{
    using namespace client::renderer::trident::particle;

    // 支持简化名称（不带 minecraft: 前缀）
    std::string normalizedName = name;
    if (normalizedName.find("minecraft:") == 0) {
        normalizedName = normalizedName.substr(10);
    }

    // 粒子名称映射
    static const std::unordered_map<std::string, ParticleTypeId> particleMap = {
        {"flame", ParticleTypeId::Flame},
        {"smoke", ParticleTypeId::Smoke},
        {"large_smoke", ParticleTypeId::LargeSmoke},
        {"lava", ParticleTypeId::Lava},
        {"portal", ParticleTypeId::Portal},
        {"reverse_portal", ParticleTypeId::ReversePortal},
        {"explosion", ParticleTypeId::Explosion},
        {"poof", ParticleTypeId::Poof},
        {"crit", ParticleTypeId::Crit},
        {"enchanted_hit", ParticleTypeId::EnchantedHit},
        {"spell", ParticleTypeId::Spell},
        {"instant_spell", ParticleTypeId::InstantSpell},
        {"entity_effect", ParticleTypeId::EntityEffect},
        {"redstone", ParticleTypeId::Redstone},
        {"enchant", ParticleTypeId::Enchant},
        {"sweep_attack", ParticleTypeId::SweepAttack},
        {"spit", ParticleTypeId::Spit},
        {"squid_ink", ParticleTypeId::SquidInk},
        {"dragon_breath", ParticleTypeId::DragonBreath},
        {"end_rod", ParticleTypeId::EndRod},
        {"campfire_cosy_smoke", ParticleTypeId::CampfireCozy},
        {"campfire_signal_smoke", ParticleTypeId::CampfireSignal},
        {"large_explosion", ParticleTypeId::LargeExplosion},
        {"huge_explosion", ParticleTypeId::HugeExplosion},
        {"dripping_water", ParticleTypeId::DrippingWater},
        {"falling_water", ParticleTypeId::FallingWater},
        {"dripping_lava", ParticleTypeId::DrippingLava},
        {"falling_lava", ParticleTypeId::FallingLava},
        {"landing_lava", ParticleTypeId::LandingLava},
        {"dripping_honey", ParticleTypeId::DrippingHoney},
        {"falling_honey", ParticleTypeId::FallingHoney},
        {"landing_honey", ParticleTypeId::LandingHoney},
        {"rain", ParticleTypeId::Rain},
        {"snowflake", ParticleTypeId::Snowflake},
        {"splash", ParticleTypeId::Splash},
        {"cloud", ParticleTypeId::Cloud},
        {"heart", ParticleTypeId::Heart},
        {"angry_villager", ParticleTypeId::AngryVillager},
        {"happy_villager", ParticleTypeId::HappyVillager},
        {"sneeze", ParticleTypeId::Sneeze},
        {"dolphin", ParticleTypeId::Dolphin},
        {"totem_of_undying", ParticleTypeId::TotemOfUndying},
        {"flash", ParticleTypeId::Flash},
        {"elder_guardian", ParticleTypeId::ElderGuardian},
        {"nautilus", ParticleTypeId::Nautilus},
        {"firework", ParticleTypeId::Firework},
        {"ash", ParticleTypeId::Ash},
        {"white_ash", ParticleTypeId::WhiteAsh},
        {"crimson_spore", ParticleTypeId::CrimsonSpore},
        {"warped_spore", ParticleTypeId::WarpedSpore},
        {"dust", ParticleTypeId::Dust},
        {"dust_color_transition", ParticleTypeId::DustColorTransition},
        {"glow_squid_ink", ParticleTypeId::GlowSquidInk},
        {"glow", ParticleTypeId::Glow},
        {"bubble", ParticleTypeId::Bubble},
        {"bubble_pop", ParticleTypeId::BubblePop},
        {"bubble_column_up", ParticleTypeId::BubbleColumnUp},
        {"current_down", ParticleTypeId::CurrentDown},
        {"underwater", ParticleTypeId::Underwater},
        {"barrier", ParticleTypeId::Barrier},
        {"light", ParticleTypeId::Light},
        {"soul_fire_flame", ParticleTypeId::SoulFireFlame},
        {"soul", ParticleTypeId::Soul},
        {"ambient_entity_effect", ParticleTypeId::AmbientEntityEffect},
    };

    auto it = particleMap.find(normalizedName);
    if (it != particleMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace command
} // namespace mc
