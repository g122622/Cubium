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

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/network/PacketBuilders.hpp"
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

    auto posArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
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
        auto coords = context.getArgument<Coordinates::Ptr>("pos");
        Vector3d anchorPos = (source.anchor() == EntityAnchorType::Eyes && source.entity() != nullptr)
            ? Vector3d(source.position().x,
                  source.position().y + static_cast<f64>(source.entity()->eyeHeight()),
                  source.position().z)
            : source.position();
        pos = coords->getPosition(anchorPos, source.rotation());
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

    // 广播粒子效果（批5b：经 buildLevelParticlesIr + connectionManager.broadcast 投递，
    // 原 IServer 弱类型 broadcastParticleInRange 纯虚已删。/particle 为管理员命令，
    // 全在线玩家广播（原 range=256 距离过滤由 connectionManager.broadcast 替代）。
    // 默认速度为 0，数量为 1，偏移为 0。
    server->connectionManager().broadcast(mc::server::net::buildLevelParticlesIr(
        particleType.value(), Vector3(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z)), 1));

    std::ostringstream ss;
    ss << "Displayed particle '" << name << "' at " << pos.x << ", " << pos.y << ", " << pos.z;
    source.sendMessage(ss.str());

    return 1;
}

std::optional<particle::ParticleTypeId> ParticleCommand::_parseParticleType(const std::string& name) noexcept
{
    using namespace particle;

    // 支持简化名称（不带 minecraft: 前缀）
    std::string normalizedName = name;
    if (normalizedName.find("minecraft:") == 0) {
        normalizedName = normalizedName.substr(10);
    }

    // 粒子名称映射（与 MC Java 1.21.11 协议 ID 0~114 对齐，115~123 为项目内部扩展）
    static const std::unordered_map<std::string, ParticleTypeId> particleMap = {
        // 方块类粒子 (0-2)
        {"angry_villager", ParticleTypeId::AngryVillager},
        {"block", ParticleTypeId::Block},
        {"block_marker", ParticleTypeId::BlockMarker},

        // 环境类粒子 (3-9)
        {"bubble", ParticleTypeId::Bubble},
        {"cloud", ParticleTypeId::Cloud},
        {"copper_fire_flame", ParticleTypeId::CopperFireFlame},
        {"crit", ParticleTypeId::Crit},
        {"damage_indicator", ParticleTypeId::DamageIndicator},
        {"dragon_breath", ParticleTypeId::DragonBreath},

        // 液体滴落类粒子 (9-13)
        {"dripping_lava", ParticleTypeId::DrippingLava},
        {"falling_lava", ParticleTypeId::FallingLava},
        {"landing_lava", ParticleTypeId::LandingLava},
        {"dripping_water", ParticleTypeId::DrippingWater},
        {"falling_water", ParticleTypeId::FallingWater},

        // 染色粒子 (14-15)
        {"dust", ParticleTypeId::Dust},
        {"dust_color_transition", ParticleTypeId::DustColorTransition},

        // 效果类粒子 (16-28)
        {"effect", ParticleTypeId::Spell},
        {"elder_guardian", ParticleTypeId::ElderGuardian},
        {"enchanted_hit", ParticleTypeId::EnchantedHit},
        {"enchant", ParticleTypeId::Enchant},
        {"end_rod", ParticleTypeId::EndRod},
        {"entity_effect", ParticleTypeId::EntityEffect},
        {"explosion_emitter", ParticleTypeId::HugeExplosion},
        {"explosion", ParticleTypeId::Explosion},
        {"gust", ParticleTypeId::Gust},
        {"small_gust", ParticleTypeId::SmallGust},
        {"gust_emitter_large", ParticleTypeId::GustEmitterLarge},
        {"gust_emitter_small", ParticleTypeId::GustEmitterSmall},
        {"sonic_boom", ParticleTypeId::SonicBoom},

        // 方块/物品/烟花粒子 (29-31)
        {"falling_dust", ParticleTypeId::FallingDust},
        {"firework", ParticleTypeId::Firework},
        {"fishing", ParticleTypeId::Fishing},

        // 火焰/效果粒子 (32-52)
        {"flame", ParticleTypeId::Flame},
        {"infested", ParticleTypeId::Infested},
        {"cherry_leaves", ParticleTypeId::CherryLeaves},
        {"pale_oak_leaves", ParticleTypeId::PaleOakLeaves},
        {"tinted_leaves", ParticleTypeId::TintedLeaves},
        {"sculk_soul", ParticleTypeId::SculkSoul},
        {"sculk_charge", ParticleTypeId::SculkCharge},
        {"sculk_charge_pop", ParticleTypeId::SculkChargePop},
        {"soul_fire_flame", ParticleTypeId::SoulFireFlame},
        {"soul", ParticleTypeId::Soul},
        {"flash", ParticleTypeId::Flash},
        {"happy_villager", ParticleTypeId::HappyVillager},
        {"composter", ParticleTypeId::Composter},
        {"heart", ParticleTypeId::Heart},
        {"instant_effect", ParticleTypeId::InstantSpell},
        {"item", ParticleTypeId::Item},
        {"vibration", ParticleTypeId::Vibration},
        {"trail", ParticleTypeId::Trail},
        {"item_slime", ParticleTypeId::ItemSlime},
        {"item_cobweb", ParticleTypeId::ItemCobweb},
        {"item_snowball", ParticleTypeId::ItemSnowball},

        // 烟雾/天气/生物粒子 (53-69)
        {"large_smoke", ParticleTypeId::LargeSmoke},
        {"lava", ParticleTypeId::Lava},
        {"mycelium", ParticleTypeId::Mycelium},
        {"note", ParticleTypeId::Note},
        {"poof", ParticleTypeId::Poof},
        {"portal", ParticleTypeId::Portal},
        {"rain", ParticleTypeId::Rain},
        {"smoke", ParticleTypeId::Smoke},
        {"white_smoke", ParticleTypeId::WhiteSmoke},
        {"sneeze", ParticleTypeId::Sneeze},
        {"spit", ParticleTypeId::Spit},
        {"squid_ink", ParticleTypeId::SquidInk},
        {"sweep_attack", ParticleTypeId::SweepAttack},
        {"totem_of_undying", ParticleTypeId::TotemOfUndying},
        {"underwater", ParticleTypeId::Underwater},
        {"splash", ParticleTypeId::Splash},
        {"witch", ParticleTypeId::Witch},

        // 水下/营地/蜂蜜粒子 (70-79)
        {"bubble_pop", ParticleTypeId::BubblePop},
        {"current_down", ParticleTypeId::CurrentDown},
        {"bubble_column_up", ParticleTypeId::BubbleColumnUp},
        {"nautilus", ParticleTypeId::Nautilus},
        {"dolphin", ParticleTypeId::Dolphin},
        {"campfire_cosy_smoke", ParticleTypeId::CampfireCozy},
        {"campfire_signal_smoke", ParticleTypeId::CampfireSignal},
        {"dripping_honey", ParticleTypeId::DrippingHoney},
        {"falling_honey", ParticleTypeId::FallingHoney},
        {"landing_honey", ParticleTypeId::LandingHoney},

        // 花蜜/孢子/下界粒子 (80-98)
        {"falling_nectar", ParticleTypeId::FallingNectar},
        {"falling_spore_blossom", ParticleTypeId::FallingSporeBlossom},
        {"ash", ParticleTypeId::Ash},
        {"crimson_spore", ParticleTypeId::CrimsonSpore},
        {"warped_spore", ParticleTypeId::WarpedSpore},
        {"spore_blossom_air", ParticleTypeId::SporeBlossomAir},
        {"dripping_obsidian_tear", ParticleTypeId::DrippingObsidianTear},
        {"falling_obsidian_tear", ParticleTypeId::FallingObsidianTear},
        {"landing_obsidian_tear", ParticleTypeId::LandingObsidianTear},
        {"reverse_portal", ParticleTypeId::ReversePortal},
        {"white_ash", ParticleTypeId::WhiteAsh},
        {"small_flame", ParticleTypeId::SmallFlame},
        {"snowflake", ParticleTypeId::Snowflake},
        {"dripping_dripstone_lava", ParticleTypeId::DrippingDripstoneLava},
        {"falling_dripstone_lava", ParticleTypeId::FallingDripstoneLava},
        {"dripping_dripstone_water", ParticleTypeId::DrippingDripstoneWater},
        {"falling_dripstone_water", ParticleTypeId::FallingDripstoneWater},
        {"glow_squid_ink", ParticleTypeId::GlowSquidInk},
        {"glow", ParticleTypeId::Glow},

        // 铜蚀/幽匿/试炼/不祥粒子 (99-114)
        {"wax_on", ParticleTypeId::WaxOn},
        {"wax_off", ParticleTypeId::WaxOff},
        {"electric_spark", ParticleTypeId::ElectricSpark},
        {"scrape", ParticleTypeId::Scrape},
        {"shriek", ParticleTypeId::Shriek},
        {"egg_crack", ParticleTypeId::EggCrack},
        {"dust_plume", ParticleTypeId::DustPlume},
        {"trial_spawner_detection", ParticleTypeId::TrialSpawnerDetection},
        {"trial_spawner_detection_ominous", ParticleTypeId::TrialSpawnerDetectionOminous},
        {"vault_connection", ParticleTypeId::VaultConnection},
        {"dust_pillar", ParticleTypeId::DustPillar},
        {"ominous_spawning", ParticleTypeId::OminousSpawning},
        {"raid_omen", ParticleTypeId::RaidOmen},
        {"trial_omen", ParticleTypeId::TrialOmen},
        {"block_crumble", ParticleTypeId::BlockCrumble},
        {"firefly", ParticleTypeId::Firefly},

        // 项目内部扩展粒子（不在 MC 协议中，用于内部渲染等）
        {"breaking", ParticleTypeId::Breaking},
        {"barrier", ParticleTypeId::Barrier},
        {"light", ParticleTypeId::Light},
        {"redstone", ParticleTypeId::Redstone},
        {"large_explosion", ParticleTypeId::LargeExplosion},
        {"item_pickup", ParticleTypeId::ItemPickup},
        {"dripping_cherry_leaves", ParticleTypeId::DrippingCherryLeaves},
        {"falling_cherry_leaves", ParticleTypeId::FallingCherryLeaves},
        {"landing_cherry_leaves", ParticleTypeId::LandingCherryLeaves},
    };

    auto it = particleMap.find(normalizedName);
    if (it != particleMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace command
} // namespace mc
