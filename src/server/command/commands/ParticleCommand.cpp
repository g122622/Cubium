#include "ParticleCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <sstream>
#include <unordered_map>

namespace mc {
namespace command {

void ParticleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto particleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("particle");
    particleNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        particleNode,
        support::makeMetadata(
            "Creates particles at a position.",
            "/particle <name> [<pos>]",
            2,
            {},
            true));

    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "name",
        StringArgumentType::string());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return spawnParticle(ctx);
    });

    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return spawnParticle(ctx);
    });

    nameArg->addChild(posArg);
    particleNode->addChild(nameArg);
    dispatcher.registerCommand(particleNode);
}

i32 ParticleCommand::spawnParticle(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const String name = context.getArgument<String>("name");
    Vector3d pos = source.position();

    // 如果提供了位置参数，使用它
    if (context.hasArgument("pos")) {
        pos = context.getArgument<Vector3d>("pos");
    }

    // 解析粒子类型
    auto particleType = parseParticleType(name);
    if (!particleType.has_value()) {
        source.sendError("Unknown particle type: " + name);
        return 0;
    }

    // TODO: 实现粒子效果广播
    // 需要 ServerWorld 或 MinecraftServer 的广播接口

    std::ostringstream ss;
    ss << "Displayed particle '" << name << "' at "
       << pos.x << ", " << pos.y << ", " << pos.z;
    source.sendMessage(ss.str());

    return 1;
}

std::optional<client::renderer::trident::particle::ParticleTypeId>
ParticleCommand::parseParticleType(const String& name) noexcept
{
    using namespace client::renderer::trident::particle;

    // 支持简化名称（不带 minecraft: 前缀）
    String normalizedName = name;
    if (normalizedName.find("minecraft:") == 0) {
        normalizedName = normalizedName.substr(10);
    }

    // 粒子名称映射
    static const std::unordered_map<String, ParticleTypeId> particleMap = {
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
