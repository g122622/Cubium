/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the following conditions:
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

/**
 * @file RendererRegistration.cpp
 * @brief 统一注册所有实体渲染器到 RendererFactory
 *
 * 此文件集中注册所有实体渲染器，使用工厂注册表模式替代在 EntityRendererManager 中直接注册。
 * 与 ModelRegistration.cpp 设计保持一致。
 */

#include "RendererRegistration.hpp"
#include "client/renderer/trident/entity/core/RendererFactory.hpp"
#include <memory>
#include <spdlog/spdlog.h>

// 动物渲染器
#include "animal/AnimalRenderers.hpp"
#include "animal/CatRenderer.hpp"
#include "animal/HorseRenderer.hpp"
#include "animal/LlamaRenderer.hpp"
#include "animal/OcelotRenderer.hpp"
#include "animal/VillagerRenderer.hpp"
#include "animal/WolfRenderer.hpp"

// 水生生物渲染器
#include "aquatic/AquaticRenderers.hpp"

// 怪物渲染器
#include "monster/MonsterRenderers.hpp"
#include "monster/MonsterVariantRenderers.hpp"
#include "monster/SpecialMonsterRenderers.hpp"

// 下界生物渲染器
#include "nether/NetherRenderers.hpp"

// 玩家渲染器
#include "player/PlayerRenderer.hpp"

// 投掷物渲染器
#include "projectile/BillboardRenderers.hpp"
#include "projectile/ExperienceOrbRenderer.hpp"
#include "projectile/FireballRenderers.hpp"
#include "projectile/FishingBobberRenderer.hpp"
#include "projectile/ItemEntityRenderer.hpp"
#include "projectile/ProjectileRenderers.hpp"

// 特殊实体渲染器
#include "special/SpecialEntityRenderers.hpp"

// 载具渲染器
#include "vehicle/VehicleRenderers.hpp"

// 实体类型常量
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "common/entity/core/EntityRegistry.hpp"

namespace mc::client::renderer::entity::renderer {

namespace ET = ::mc::entity::EntityTypeKeys;
namespace factory = core;

void initializeRendererRegistration()
{
    if (factory::RendererFactory::isInitialized()) {
        spdlog::warn("RendererFactory already initialized, skipping renderer registration");
        return;
    }

    auto& f = factory::RendererFactory::instance();

    // ==================== 基础动物渲染器 ====================
    f.registerRenderer(
        ET::PIG, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::PigRenderer>(); });
    f.registerRenderer(
        ET::COW, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::CowRenderer>(); });
    f.registerRenderer(
        ET::SHEEP, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::SheepRenderer>(); });
    f.registerRenderer(ET::CHICKEN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::ChickenRenderer>(); });
    f.registerRenderer(ET::RABBIT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::RabbitRenderer>(); });
    f.registerRenderer(ET::MOOSHROOM,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::MooshroomRenderer>(); });

    // ==================== 可驯服动物渲染器 ====================
    f.registerRenderer(
        ET::WOLF, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::WolfRenderer>(); });
    f.registerRenderer(ET::OCELOT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::OcelotRenderer>(); });
    f.registerRenderer(
        ET::CAT, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::CatRenderer>(); });
    f.registerRenderer(ET::PARROT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::ParrotRenderer>(); });
    f.registerRenderer(ET::PHANTOM,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::PhantomRenderer>(); });

    // ==================== 马类型渲染器 ====================
    f.registerRenderer(
        ET::HORSE, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::HorseRenderer>(); });
    f.registerRenderer(ET::DONKEY, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<animal::HorseRenderer>(); // 复用 HorseRenderer
    });
    f.registerRenderer(ET::MULE, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<animal::HorseRenderer>(); // 复用 HorseRenderer
    });
    f.registerRenderer(
        ET::LLAMA, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::LlamaRenderer>(); });
    f.registerRenderer(ET::SKELETON_HORSE, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<animal::HorseRenderer>(); // 复用 HorseRenderer
    });
    f.registerRenderer(ET::ZOMBIE_HORSE, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<animal::HorseRenderer>(); // 复用 HorseRenderer
    });
    f.registerRenderer(ET::TRADER_LLAMA, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<animal::LlamaRenderer>(); // 复用 LlamaRenderer
    });

    // ==================== 特殊动物渲染器 ====================
    f.registerRenderer(
        ET::FOX, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::FoxRenderer>(); });
    f.registerRenderer(ET::PANDA,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::PandaRenderer>(); });
    f.registerRenderer(ET::POLAR_BEAR,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::PolarBearRenderer>(); });
    f.registerRenderer(ET::TURTLE,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<aquatic::TurtleRenderer>(); });
    f.registerRenderer(
        ET::BEE, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::BeeRenderer>(); });
    f.registerRenderer(ET::STRIDER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<nether::StriderRenderer>(); });

    // ==================== 水生生物渲染器 ====================
    f.registerRenderer(
        ET::COD, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<aquatic::CodRenderer>(); });
    f.registerRenderer(ET::SALMON,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<aquatic::SalmonRenderer>(); });
    f.registerRenderer(ET::DOLPHIN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<aquatic::DolphinRenderer>(); });
    f.registerRenderer(ET::AXOLOTL,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<aquatic::AxolotlRenderer>(); });
    f.registerRenderer(ET::TROPICAL_FISH,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<aquatic::TropicalFishARenderer>(); });
    f.registerRenderer(ET::PUFFERFISH,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<aquatic::PufferfishRenderer>(); });

    // ==================== 环境生物/傀儡渲染器 ====================
    f.registerRenderer(
        ET::BAT, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::BatRenderer>(); });
    f.registerRenderer(ET::IRON_GOLEM,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::IronGolemRenderer>(); });
    f.registerRenderer(ET::SNOW_GOLEM,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::SnowGolemRenderer>(); });
    f.registerRenderer(ET::COPPER_GOLEM,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::CopperGolemRenderer>(); });

    // ==================== 村民渲染器 ====================
    f.registerRenderer(ET::VILLAGER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::VillagerRenderer>(); });
    f.registerRenderer(ET::WANDERING_TRADER, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<animal::VillagerRenderer>(); // 复用 VillagerRenderer
    });

    // ==================== 鱿鱼 ====================
    f.registerRenderer(
        ET::SQUID, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::SquidRenderer>(); });

    // ==================== 发光鱿鱼 ====================
    f.registerRenderer(ET::GLOW_SQUID,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<animal::GlowSquidRenderer>(); });

    // ==================== 基础怪物渲染器 ====================
    f.registerRenderer(ET::ZOMBIE,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::ZombieRenderer>(); });
    f.registerRenderer(ET::SKELETON,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::SkeletonRenderer>(); });
    f.registerRenderer(ET::CREEPER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::CreeperRenderer>(); });
    f.registerRenderer(ET::SPIDER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::SpiderRenderer>(); });
    f.registerRenderer(ET::ENDERMAN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::EndermanRenderer>(); });
    f.registerRenderer(ET::BLAZE,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::BlazeRenderer>(); });

    // ==================== 怪物变体渲染器 ====================
    f.registerRenderer(ET::ZOMBIE_VILLAGER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::ZombieVillagerRenderer>(); });
    f.registerRenderer(ET::DROWNED,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::DrownedRenderer>(); });
    f.registerRenderer(
        ET::HUSK, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::HuskRenderer>(); });
    f.registerRenderer(ET::STRAY,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::StrayRenderer>(); });
    f.registerRenderer(ET::CAVE_SPIDER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::CaveSpiderRenderer>(); });
    f.registerRenderer(ET::GIANT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::GiantRenderer>(); });

    // ==================== 特殊怪物渲染器 ====================
    f.registerRenderer(ET::WITHER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::WitherRenderer>(); });
    f.registerRenderer(ET::SLIME,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::SlimeRenderer>(); });
    f.registerRenderer(ET::GUARDIAN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::GuardianRenderer>(); });
    f.registerRenderer(ET::ELDER_GUARDIAN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::ElderGuardianRenderer>(); });
    f.registerRenderer(ET::SHULKER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::ShulkerRenderer>(); });
    f.registerRenderer(ET::SILVERFISH,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::SilverfishRenderer>(); });
    f.registerRenderer(ET::ENDERMITE,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::EndermiteRenderer>(); });

    // ==================== 灾厄村民渲染器 ====================
    f.registerRenderer(
        ET::VEX, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::VexRenderer>(); });
    f.registerRenderer(ET::VINDICATOR,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::VindicatorRenderer>(); });
    f.registerRenderer(ET::EVOKER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::EvokerRenderer>(); });
    f.registerRenderer(ET::PILLAGER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::PillagerRenderer>(); });
    f.registerRenderer(ET::RAVAGER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::RavagerRenderer>(); });
    f.registerRenderer(ET::WITCH,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::WitchRenderer>(); });
    f.registerRenderer(ET::WITHER_SKELETON,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::WitherSkeletonRenderer>(); });
    f.registerRenderer(ET::ILLUSIONER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<monster::IllusionerRenderer>(); });

    // ==================== 下界生物渲染器 ====================
    f.registerRenderer(
        ET::GHAST, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<nether::GhastRenderer>(); });
    f.registerRenderer(ET::MAGMA_CUBE,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<nether::MagmaCubeRenderer>(); });
    f.registerRenderer(ET::PIGLIN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<nether::PiglinRenderer>(); });
    f.registerRenderer(ET::PIGLIN_BRUTE,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<nether::PiglinBruteRenderer>(); });
    f.registerRenderer(ET::HOGLIN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<nether::HoglinRenderer>(); });
    f.registerRenderer(ET::ZOGLIN,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<nether::ZoglinRenderer>(); });
    f.registerRenderer(ET::ZOMBIFIED_PIGLIN, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<nether::PiglinRenderer>(); // 复用 PiglinRenderer
    });

    // ==================== 玩家渲染器 ====================
    f.registerRenderer(ET::PLAYER, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<player::PlayerRenderer>(false); // 标准手臂
    });

    // ==================== 投掷物渲染器 ====================
    f.registerRenderer(ET::ARROW,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::ArrowRenderer>(); });
    f.registerRenderer(ET::SPECTRAL_ARROW, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<projectile::SpectralArrowRenderer>();
    });
    f.registerRenderer(ET::TRIDENT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::TridentRenderer>(); });

    // ==================== 特殊实体渲染器 ====================
    f.registerRenderer(ET::END_CRYSTAL,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::EnderCrystalRenderer>(); });
    f.registerRenderer(ET::SHULKER_BULLET,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::ShulkerBulletRenderer>(); });
    f.registerRenderer(ET::LLAMA_SPIT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::LlamaSpitRenderer>(); });
    f.registerRenderer(ET::WITHER_SKULL,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::WitherSkullRenderer>(); });
    f.registerRenderer(ET::DRAGON_FIREBALL,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::DragonFireballRenderer>(); });
    f.registerRenderer(ET::EVOKER_FANGS,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::EvokerFangsRenderer>(); });
    f.registerRenderer(ET::LIGHTNING_BOLT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::LightningBoltRenderer>(); });
    f.registerRenderer(ET::AREA_EFFECT_CLOUD,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::AreaEffectCloudRenderer>(); });
    f.registerRenderer(ET::FALLING_BLOCK,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::FallingBlockRenderer>(); });
    f.registerRenderer(ET::ITEM_FRAME,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::ItemFrameRenderer>(); });
    f.registerRenderer(ET::PAINTING,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::PaintingRenderer>(); });
    f.registerRenderer(ET::LEASH_KNOT,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::LeashKnotRenderer>(); });
    f.registerRenderer(ET::ARMOR_STAND,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::ArmorStandRenderer>(); });
    f.registerRenderer(
        ET::TNT, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::TNTRenderer>(); });
    f.registerRenderer(ET::FIREWORK_ROCKET,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<special::FireworkRocketRenderer>(); });

    // ==================== 载具渲染器 ====================
    f.registerRenderer(ET::BOAT, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Oak);
    });
    f.registerRenderer("minecraft:spruce_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Spruce);
    });
    f.registerRenderer("minecraft:birch_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Birch);
    });
    f.registerRenderer("minecraft:jungle_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Jungle);
    });
    f.registerRenderer("minecraft:acacia_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Acacia);
    });
    f.registerRenderer("minecraft:dark_oak_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::DarkOak);
    });
    f.registerRenderer("minecraft:mangrove_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Mangrove);
    });
    f.registerRenderer("minecraft:cherry_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Cherry);
    });
    f.registerRenderer("minecraft:pale_oak_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::PaleOak);
    });
    f.registerRenderer("minecraft:bamboo_raft", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<vehicle::BoatRenderer>(vehicle::BoatType::Bamboo);
    });
    f.registerRenderer(ET::MINECART,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<vehicle::MinecartRenderer>(); });
    f.registerRenderer(ET::CHEST_MINECART,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<vehicle::MinecartRenderer>(); });
    f.registerRenderer(ET::FURNACE_MINECART,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<vehicle::MinecartRenderer>(); });
    f.registerRenderer(ET::HOPPER_MINECART,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<vehicle::MinecartRenderer>(); });
    f.registerRenderer(ET::TNT_MINECART,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<vehicle::MinecartRenderer>(); });

    // ==================== ItemEntity 渲染器 ====================
    // 注意：ItemEntityRenderer 需要 itemTextureAtlas，在 EntityRendererManager 中单独处理
    f.registerRenderer(ET::ITEM,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::ItemEntityRenderer>(); });

    // ==================== ExperienceOrb 渲染器 ====================
    f.registerRenderer(ET::EXPERIENCE_ORB, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<projectile::ExperienceOrbRenderer>();
    });

    // ==================== 投掷物渲染器 ====================
    f.registerRenderer(ET::SNOWBALL,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::SnowballRenderer>(); });
    f.registerRenderer(
        ET::EGG, []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::EggRenderer>(); });
    f.registerRenderer(ET::ENDER_PEARL,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::EnderPearlRenderer>(); });
    f.registerRenderer(ET::POTION,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::PotionRenderer>(); });
    f.registerRenderer(ET::EXPERIENCE_BOTTLE, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<projectile::ExperienceBottleRenderer>();
    });
    f.registerRenderer(ET::EYE_OF_ENDER,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::EyeOfEnderRenderer>(); });
    f.registerRenderer(ET::FIREBALL,
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<projectile::FireballRenderer>(); });
    f.registerRenderer(ET::SMALL_FIREBALL, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<projectile::SmallFireballRenderer>();
    });
    f.registerRenderer(ET::FISHING_BOBBER, []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<projectile::FishingBobberRenderer>();
    });

    factory::RendererFactory::markInitialized();
    spdlog::info("RendererFactory: Registered {} renderer types", f.size());
}

} // namespace mc::client::renderer::entity::renderer
