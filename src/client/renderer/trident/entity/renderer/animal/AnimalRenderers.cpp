#include "AnimalRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"
#include "BatModel.hpp"
#include "RabbitModel.hpp"
#include "SquidModel.hpp"

namespace mc::client::renderer::entity::renderer::animal {

void registerAnimalRenderers(EntityRendererManager& manager)
{
    // 猪
    manager.registerRenderer(
        "minecraft:pig", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<PigRenderer>(); });

    // 牛
    manager.registerRenderer(
        "minecraft:cow", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<CowRenderer>(); });

    // 羊
    manager.registerRenderer(
        "minecraft:sheep", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SheepRenderer>(); });

    // 哞菇
    manager.registerRenderer("minecraft:mooshroom",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<MooshroomRenderer>(); });

    // 鸡
    manager.registerRenderer("minecraft:chicken",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ChickenRenderer>(); });

    // 兔子
    manager.registerRenderer("minecraft:rabbit",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<RabbitRenderer>(); });

    // 蝙蝠
    manager.registerRenderer(
        "minecraft:bat", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<BatRenderer>(); });

    // 鱿鱼
    manager.registerRenderer(
        "minecraft:squid", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SquidRenderer>(); });

    // 已有的动物（狼、猫、豹猫、马、村民）
    // 这些在单独的文件中注册
}

} // namespace mc::client::renderer::entity::renderer::animal
