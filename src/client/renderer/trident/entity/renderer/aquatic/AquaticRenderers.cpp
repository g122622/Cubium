#include "AquaticRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::aquatic {

void registerAquaticRenderers(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:cod", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<CodRenderer>();
    });

    manager.registerRenderer("minecraft:salmon", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<SalmonRenderer>();
    });

    manager.registerRenderer("minecraft:dolphin", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<DolphinRenderer>();
    });

    manager.registerRenderer("minecraft:turtle", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<TurtleRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::aquatic
