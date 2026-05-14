#include "MonsterVariantRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::monster {

void registerMonsterVariantRenderers(EntityRendererManager& manager)
{
    manager.registerRenderer("minecraft:zombie_villager",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ZombieVillagerRenderer>(); });

    manager.registerRenderer("minecraft:drowned",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<DrownedRenderer>(); });

    manager.registerRenderer(
        "minecraft:husk", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<HuskRenderer>(); });

    manager.registerRenderer(
        "minecraft:stray", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<StrayRenderer>(); });

    manager.registerRenderer("minecraft:cave_spider",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<CaveSpiderRenderer>(); });

    manager.registerRenderer(
        "minecraft:giant", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<GiantRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::monster
