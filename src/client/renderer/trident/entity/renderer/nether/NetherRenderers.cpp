#include "NetherRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::nether {

void registerNetherRenderers(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:ghast", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<GhastRenderer>();
    });

    manager.registerRenderer("minecraft:magma_cube", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<MagmaCubeRenderer>();
    });

    manager.registerRenderer("minecraft:piglin", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<PiglinRenderer>();
    });

    manager.registerRenderer("minecraft:piglin_brute", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<PiglinBruteRenderer>();
    });

    manager.registerRenderer("minecraft:hoglin", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<HoglinRenderer>();
    });

    manager.registerRenderer("minecraft:zoglin", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<ZoglinRenderer>();
    });

    manager.registerRenderer("minecraft:strider", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<StriderRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::nether
