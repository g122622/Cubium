#include "SpecialMonsterRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::monster {

void registerSpecialMonsterRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer("minecraft:wither",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<WitherRenderer>(); });

    manager.registerRenderer(
        "minecraft:slime", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SlimeRenderer>(); });

    manager.registerRenderer("minecraft:guardian",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<GuardianRenderer>(); });

    manager.registerRenderer("minecraft:elder_guardian",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ElderGuardianRenderer>(); });

    manager.registerRenderer("minecraft:shulker",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ShulkerRenderer>(); });

    manager.registerRenderer("minecraft:silverfish",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SilverfishRenderer>(); });

    manager.registerRenderer("minecraft:endermite",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<EndermiteRenderer>(); });
}

void registerIllagerRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer(
        "minecraft:vex", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<VexRenderer>(); });

    manager.registerRenderer("minecraft:vindicator",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<VindicatorRenderer>(); });

    manager.registerRenderer("minecraft:evoker",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<EvokerRenderer>(); });

    manager.registerRenderer("minecraft:pillager",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PillagerRenderer>(); });

    manager.registerRenderer("minecraft:ravager",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<RavagerRenderer>(); });

    manager.registerRenderer(
        "minecraft:witch", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<WitchRenderer>(); });
}

void registerPassiveMobRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer("minecraft:iron_golem",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<IronGolemRenderer>(); });

    manager.registerRenderer("minecraft:snow_golem",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SnowGolemRenderer>(); });

    manager.registerRenderer(
        "minecraft:bee", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<BeeRenderer>(); });

    manager.registerRenderer(
        "minecraft:fox", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<FoxRenderer>(); });

    manager.registerRenderer(
        "minecraft:panda", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PandaRenderer>(); });

    manager.registerRenderer("minecraft:polar_bear",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PolarBearRenderer>(); });

    manager.registerRenderer("minecraft:parrot",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ParrotRenderer>(); });

    manager.registerRenderer("minecraft:phantom",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PhantomRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::monster
