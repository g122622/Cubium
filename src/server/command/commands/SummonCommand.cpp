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

#include "SummonCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <cmath>
#include <memory>
#include <sstream>
#include <utility>

namespace mc {
namespace command {

void SummonCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto summonNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("summon");
    summonNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        summonNode, support::makeMetadata("Summons an entity.", "/summon <entity> [<pos>]", 2, {}, false));

    // /summon <entity> - 在执行者位置生成
    auto entityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "entity", ResourceLocationArgumentType::resourceLocation());
    entityArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _summonEntity(ctx); });

    // /summon <entity> <pos> - 在指定位置生成
    auto posArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    posArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _summonEntityAtPosition(ctx); });

    entityArg->addChild(posArg);
    summonNode->addChild(entityArg);

    dispatcher.registerCommand(summonNode);
}

i32 SummonCommand::_summonEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    ResourceLocation entityId = context.getArgument<ResourceLocation>("entity");

    // 在执行者位置生成
    Vector3d position = source.position();

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.summon.failed.noWorld");
        return 0;
    }

    // 边界校验（对齐 Java SummonCommand.createEntity 首行守卫 SummonCommand.java:83-85：
    // !Level.isInSpawnableBounds(BlockPos.containing(pos)) → 抛 INVALID_POSITION）。
    // 拦截越界坐标（Y 超出 ±20,000,000 或 X/Z 超出 ±30,000,000 世界边界），防止
    // setPosition/spawnEntity 处理非法坐标时崩溃或产生越界实体。此守卫须在 peaceful
    // 守卫之前（vanilla 顺序：边界 → peaceful → 创建）。
    const BlockPos containing(static_cast<i32>(std::floor(position.x)),
        static_cast<i32>(position.y),
        static_cast<i32>(std::floor(position.z)));
    if (!world::isInSpawnableBounds(containing.x, containing.y, containing.z)) {
        source.sendError("commands.summon.invalidPosition");
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

    // 和平难度检查（对齐 Java SummonCommand.createEntity：difficulty==PEACEFUL 且 !isAllowedInPeaceful
    // 时拒绝召唤）。用 EntityType::isAllowedInPeaceful()（NotInPeaceful 标志位，对齐 vanilla 逐实体
    // notInPeaceful 标注）而非 classification 派生——少数 Monster 类实体（Shulker/Hoglin/Piglin 等）
    // vanilla 未标 notInPeaceful 故和平可召。同款守卫见 SpawnEggItem::spawnEntity。
    if (world->difficulty() == Difficulty::Peaceful && !entityType->isAllowedInPeaceful()) {
        source.sendError("commands.summon.failed.peaceful");
        return 0;
    }

    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* ecsRegistry = world->entityRegistry();
    if (ecsRegistry == nullptr) {
        source.sendError("commands.summon.failed.noRegistry");
        return 0;
    }

    // 创建实体
    std::unique_ptr<Entity> entity = entityType->create(world, *ecsRegistry);
    if (entity == nullptr) {
        source.sendError("commands.summon.failed.createFailed");
        return 0;
    }

    // 设置位置
    entity->setPosition(
        Vector3(static_cast<f32>(position.x), static_cast<f32>(position.y), static_cast<f32>(position.z)));

    // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
    auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
    if (mobEntity != nullptr) {
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*world,
            BlockPos(static_cast<i32>(std::floor(position.x)),
                static_cast<i32>(position.y),
                static_cast<i32>(std::floor(position.z))));
        mobEntity->finalizeSpawn(*world, difficultyInstance, world::spawn::SpawnReason::Command);
    }

    // 生成实体
    EntityInstanceId spawnedId = world->spawnEntity(std::move(entity));
    if (spawnedId == 0) {
        source.sendError("commands.summon.failed.spawnFailed");
        return 0;
    }

    // 触发进度检测：召唤实体
    // 参考 MC: CriteriaTriggers.SUMMONED_ENTITY.trigger(serverplayer, entity)
    // /summon 命令由玩家执行，触发 SummonedEntityTrigger
    if (source.isPlayer()) {
        Entity* spawnedEntity = world->getEntity(spawnedId);
        if (spawnedEntity != nullptr) {
            world->onSummonedEntity(source.playerId(), spawnedEntity);
        }
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Summoned " << entityId.toString();
    source.sendMessage(ss.str());

    return 1;
}

i32 SummonCommand::_summonEntityAtPosition(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    ResourceLocation entityId = context.getArgument<ResourceLocation>("entity");
    Vector3d position = Vec3ArgumentType::getVec3(context, "pos", source);

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.summon.failed.noWorld");
        return 0;
    }

    // 边界校验（对齐 Java SummonCommand.createEntity 首行守卫 SummonCommand.java:83-85：
    // !Level.isInSpawnableBounds(BlockPos.containing(pos)) → 抛 INVALID_POSITION）。
    // 拦截越界坐标（Y 超出 ±20,000,000 或 X/Z 超出 ±30,000,000 世界边界），防止
    // setPosition/spawnEntity 处理非法坐标时崩溃或产生越界实体。此守卫须在 peaceful
    // 守卫之前（vanilla 顺序：边界 → peaceful → 创建）。同款守卫见 _summonEntity。
    const BlockPos containing(static_cast<i32>(std::floor(position.x)),
        static_cast<i32>(position.y),
        static_cast<i32>(std::floor(position.z)));
    if (!world::isInSpawnableBounds(containing.x, containing.y, containing.z)) {
        source.sendError("commands.summon.invalidPosition");
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

    // 和平难度检查（对齐 Java SummonCommand.createEntity：difficulty==PEACEFUL 且 !isAllowedInPeaceful
    // 时拒绝召唤）。用 EntityType::isAllowedInPeaceful()（NotInPeaceful 标志位，对齐 vanilla 逐实体
    // notInPeaceful 标注）而非 classification 派生——少数 Monster 类实体（Shulker/Hoglin/Piglin 等）
    // vanilla 未标 notInPeaceful 故和平可召。同款守卫见 SpawnEggItem::spawnEntity。
    if (world->difficulty() == Difficulty::Peaceful && !entityType->isAllowedInPeaceful()) {
        source.sendError("commands.summon.failed.peaceful");
        return 0;
    }

    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* ecsRegistry = world->entityRegistry();
    if (ecsRegistry == nullptr) {
        source.sendError("commands.summon.failed.noRegistry");
        return 0;
    }

    // 创建实体
    std::unique_ptr<Entity> entity = entityType->create(world, *ecsRegistry);
    if (entity == nullptr) {
        source.sendError("commands.summon.failed.createFailed");
        return 0;
    }

    // 设置位置
    entity->setPosition(
        Vector3(static_cast<f32>(position.x), static_cast<f32>(position.y), static_cast<f32>(position.z)));

    // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化
    auto* mobEntity2 = dynamic_cast<MobEntity*>(entity.get());
    if (mobEntity2 != nullptr) {
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*world,
            BlockPos(static_cast<i32>(std::floor(position.x)),
                static_cast<i32>(position.y),
                static_cast<i32>(std::floor(position.z))));
        mobEntity2->finalizeSpawn(*world, difficultyInstance, world::spawn::SpawnReason::Command);
    }

    // 生成实体
    EntityInstanceId spawnedId = world->spawnEntity(std::move(entity));
    if (spawnedId == 0) {
        source.sendError("commands.summon.failed.spawnFailed");
        return 0;
    }

    // 触发进度检测：召唤实体
    // 参考 MC: CriteriaTriggers.SUMMONED_ENTITY.trigger(serverplayer, entity)
    // /summon 命令由玩家执行，触发 SummonedEntityTrigger
    if (source.isPlayer()) {
        Entity* spawnedEntity = world->getEntity(spawnedId);
        if (spawnedEntity != nullptr) {
            world->onSummonedEntity(source.playerId(), spawnedEntity);
        }
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Summoned " << entityId.toString() << " at " << static_cast<i32>(position.x) << ", "
       << static_cast<i32>(position.y) << ", " << static_cast<i32>(position.z);
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
