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
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/CommandSource.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/network/outbound/PacketBuilders.hpp"
#include "server/network/outbound/PacketSender.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include "server/player/runtime/ServerPlayerData.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace command {

namespace {

/** @brief 按粒子注册名解析粒子类型；未知名称返回空值。定义在文件末尾。 */
[[nodiscard]] std::optional<particle::ParticleTypeId> parseParticleTypeId(const std::string& name) noexcept;

/**
 * @brief `/particle` 的粒子名参数。
 *
 * 原版 ParticleArgument 在解析期查 `minecraft:particle_type` 注册表，未知名称属于语法
 * 错误（`argument.resource.not_found`）；本项目此前只在执行期报 Unknown particle type。
 */
class ParticleNameArgumentType final : public ArgumentType<ResourceLocation> {
public:
    [[nodiscard]] Result<ResourceLocation, CommandError> parse(StringReader& reader) override
    {
        auto parsed = readResourceLocationId(reader);
        if (parsed.failed()) {
            return parsed;
        }

        const ResourceLocation id = parsed.value();
        if (!parseParticleTypeId(id.toString()).has_value()) {
            auto message = std::make_shared<::mc::text::TranslationTextComponent>("argument.resource.not_found");
            message->addParam(::mc::text::literal(id.toString()));
            message->addParam(::mc::text::literal("minecraft:particle_type"));
            return CommandError(CommandErrorType::EntityNotFound,
                std::shared_ptr<const ::mc::text::ITextComponent>(std::move(message)),
                reader.getCursor());
        }
        return id;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "particle";
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"registry", "minecraft:particle_type"}};
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:flame", "flame", "minecraft:heart"};
    }
};

} // namespace

void ParticleCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto particleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("particle");
    particleNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(particleNode,
        support::makeMetadata("Creates particles at a position.", "/particle <name> [<pos>]", 2, {}, true));

    // 原版是 ParticleArgument（在 `minecraft:particle_type` 注册表中解析）。
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "name", std::make_shared<ParticleNameArgumentType>());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _spawnParticleImpl(ctx, false);
    });

    auto posArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _spawnParticleImpl(ctx, false);
    });

    // /particle <name> <pos> <delta> <speed> <count> [force|normal] [<viewers>]
    // delta 不做方块中心偏移（原版 Vec3Argument.vec3(false)）。
    auto deltaArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "delta", Vec3ArgumentType::vec3NoCenter());
    auto speedArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("speed", FloatArgumentType::floatArg(0.0f));
    auto countArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(0));
    auto forceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("force");
    auto normalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("normal");
    auto viewersArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "viewers", EntityArgumentType::players());
    // 原版把 viewers 挂在 speed / count / force / normal 之下（每个末端都能再接查看者）。
    forceNode->addChild(viewersArg);
    auto normalViewers = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "viewers", EntityArgumentType::players());
    normalViewers->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _spawnParticleImpl(ctx, false);
    });
    normalNode->addChild(normalViewers);
    forceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _spawnParticleImpl(ctx, true);
    });
    normalNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _spawnParticleImpl(ctx, false);
    });
    viewersArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _spawnParticleImpl(ctx, true);
    });
    countArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _spawnParticleImpl(ctx, false);
    });
    countArg->addChild(forceNode);
    countArg->addChild(normalNode);
    speedArg->addChild(countArg);
    deltaArg->addChild(speedArg);
    posArg->addChild(deltaArg);

    nameArg->addChild(posArg);
    particleNode->addChild(nameArg);
    dispatcher.registerCommand(particleNode);
}

i32 ParticleCommand::_spawnParticleImpl(CommandContext<ServerCommandSource>& context, bool force)
{
    auto& source = context.getSource();
    auto* server = source.server();
    MC_ASSERT_RELEASE(server != nullptr);

    const ResourceLocation nameId = context.getArgument<ResourceLocation>("name");
    const auto particleType = parseParticleTypeId(nameId.toString());
    // 解析期已按注册表校验，正常不会缺失；缺失说明解析器与注册表不一致。
    MC_ASSERT_RELEASE(particleType.has_value());

    Vector3d pos = source.position();
    if (context.hasArgument("pos")) {
        auto coords = context.getArgument<Coordinates::Ptr>("pos");
        Vector3d anchorPos = (source.anchor() == EntityAnchorType::Eyes && source.entity() != nullptr)
            ? Vector3d(source.position().x,
                  source.position().y + static_cast<f64>(source.entity()->eyeHeight()),
                  source.position().z)
            : source.position();
        pos = coords->getPosition(anchorPos, source.rotation());
    }

    Vector3d delta{0.0, 0.0, 0.0};
    if (context.hasArgument("delta")) {
        delta = Vec3ArgumentType::getVec3(context, "delta", source);
    }

    f32 speed = 0.0f;
    if (context.hasArgument("speed")) {
        speed = context.getArgument<f32>("speed");
    }

    i32 count = 0;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    const Vector3 position(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    const Vector3 offset(static_cast<f32>(delta.x), static_cast<f32>(delta.y), static_cast<f32>(delta.z));

    mc::network::ir::play::ParticleOptions options;
    options.type = particleType.value();
    const auto packet =
        mc::server::net::buildLevelParticlesIr(options, position, offset, static_cast<u32>(count), speed, force, false);

    // 查看者集合：默认全部在线玩家，也可由 viewers 参数限定。
    std::vector<PlayerId> viewerIds;
    if (context.hasArgument("viewers")) {
        viewerIds = support::resolvePlayerIds(source, context.getArgument<EntitySelector>("viewers"));
    } else {
        server->playerManager().forEachPlayer([&viewerIds](const server::ServerPlayerData& data) {
            if (data.loggedIn) {
                viewerIds.push_back(data.playerId);
            }
        });
    }

    // 原版只把粒子发给同维度、且在半径内的玩家：普通 32 格，force 512 格。
    const f64 range = force ? 512.0 : 32.0;
    const f64 rangeSq = range * range;
    i32 sentCount = 0;
    for (PlayerId viewerId : viewerIds) {
        const server::ServerPlayerData* data = server->playerManager().getPlayer(viewerId);
        if (data == nullptr || !data->loggedIn) {
            continue;
        }
        if (server->getPlayerWorld(viewerId) != source.world()) {
            continue;
        }

        // 以玩家所在方块的中心点到粒子位置的距离判定（原版 BlockPos.closerToCenterThan）。
        const f64 dx = std::floor(static_cast<f64>(data->x)) + 0.5 - pos.x;
        const f64 dy = std::floor(static_cast<f64>(data->y)) + 0.5 - pos.y;
        const f64 dz = std::floor(static_cast<f64>(data->z)) + 0.5 - pos.z;
        if (dx * dx + dy * dy + dz * dz > rangeSq) {
            continue;
        }

        if (server->packetSender().sendToPlayer(viewerId, packet)) {
            ++sentCount;
        }
    }

    if (sentCount == 0) {
        source.sendFailure(::mc::text::translatable("commands.particle.failed"));
        return 0;
    }

    source.sendSuccess(::mc::text::translatable("commands.particle.success", {nameId.toString()}), true);
    return sentCount;
}

namespace {

std::optional<particle::ParticleTypeId> parseParticleTypeId(const std::string& name) noexcept
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

} // namespace

} // namespace command
} // namespace mc
