#include "SummonCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/Entity.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/application/IServer.hpp"

#include <sstream>

namespace mc {
namespace command {

void SummonCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto summonNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("summon");
    summonNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        summonNode,
        support::makeMetadata(
            "Summons an entity.",
            "/summon <entity> [<pos>]",
            2,
            {},
            false));

    // /summon <entity> - 在执行者位置生成
    auto entityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "entity",
        ResourceLocationArgumentType::resourceLocation()
    );
    entityArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return summonEntity(ctx);
    });

    // /summon <entity> <pos> - 在指定位置生成
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3()
    );
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return summonEntityAtPosition(ctx);
    });

    entityArg->addChild(posArg);
    summonNode->addChild(entityArg);

    dispatcher.registerCommand(summonNode);
}

i32 SummonCommand::summonEntity(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    ResourceLocation entityId = context.getArgument<ResourceLocation>("entity");

    // 在执行者位置生成
    Vector3d position = source.position();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendMessage("commands.summon.failed.noWorld");
        return 0;
    }

    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entityId.toString());
    if (entityType == nullptr) {
        std::ostringstream ss;
        ss << "commands.summon.failed.invalidEntity: " << entityId.toString();
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查是否可召唤
    if (!entityType->canSummon()) {
        std::ostringstream ss;
        ss << "commands.summon.failed.notSummonable: " << entityId.toString();
        source.sendMessage(ss.str());
        return 0;
    }

    // 创建实体
    std::unique_ptr<Entity> entity = entityType->create(world);
    if (entity == nullptr) {
        source.sendMessage("commands.summon.failed.createFailed");
        return 0;
    }

    // 设置位置
    entity->setPosition(Vector3(
        static_cast<f32>(position.x),
        static_cast<f32>(position.y),
        static_cast<f32>(position.z)
    ));

    // 生成实体
    EntityId spawnedId = world->spawnEntity(std::move(entity));
    if (spawnedId == 0) {
        source.sendMessage("commands.summon.failed.spawnFailed");
        return 0;
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Summoned " << entityId.toString();
    source.sendMessage(ss.str());

    return 1;
}

i32 SummonCommand::summonEntityAtPosition(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    ResourceLocation entityId = context.getArgument<ResourceLocation>("entity");
    Vector3d position = context.getArgument<Vector3d>("pos");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendMessage("commands.summon.failed.noWorld");
        return 0;
    }

    // 获取实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entityId.toString());
    if (entityType == nullptr) {
        std::ostringstream ss;
        ss << "commands.summon.failed.invalidEntity: " << entityId.toString();
        source.sendMessage(ss.str());
        return 0;
    }

    // 检查是否可召唤
    if (!entityType->canSummon()) {
        std::ostringstream ss;
        ss << "commands.summon.failed.notSummonable: " << entityId.toString();
        source.sendMessage(ss.str());
        return 0;
    }

    // 创建实体
    std::unique_ptr<Entity> entity = entityType->create(world);
    if (entity == nullptr) {
        source.sendMessage("commands.summon.failed.createFailed");
        return 0;
    }

    // 设置位置
    entity->setPosition(Vector3(
        static_cast<f32>(position.x),
        static_cast<f32>(position.y),
        static_cast<f32>(position.z)
    ));

    // 生成实体
    EntityId spawnedId = world->spawnEntity(std::move(entity));
    if (spawnedId == 0) {
        source.sendMessage("commands.summon.failed.spawnFailed");
        return 0;
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Summoned " << entityId.toString() << " at "
       << static_cast<i32>(position.x) << ", "
       << static_cast<i32>(position.y) << ", "
       << static_cast<i32>(position.z);
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
