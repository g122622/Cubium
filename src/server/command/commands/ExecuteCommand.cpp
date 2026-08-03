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

#include "ExecuteCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/DimensionArgument.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <cstddef>
#include <memory>
#include <string>

namespace mc {
namespace command {

namespace {

/**
 * @brief 解析方块ID
 */
Block* parseBlockId(const std::string& input)
{
    auto& registry = BlockRegistry::instance();

    std::string namespace_;
    std::string path;
    size_t colonPos = input.find(':');
    if (colonPos != std::string::npos) {
        namespace_ = input.substr(0, colonPos);
        path = input.substr(colonPos + 1);
    } else {
        namespace_ = "minecraft";
        path = input;
    }

    // 去除状态属性部分
    size_t bracketPos = path.find('[');
    if (bracketPos != std::string::npos) {
        path = path.substr(0, bracketPos);
    }

    ResourceLocation location(namespace_, path);
    return registry.getBlock(location);
}

} // namespace

// ========== 嵌套命令执行 ==========

i32 ExecuteCommand::_executeNestedCommand(ServerCommandSource& source, const std::string& command)
{
    if (command.empty()) {
        source.sendError("commands.execute.failed.emptyCommand");
        return 0;
    }

    // 确保命令以 / 开头
    std::string cmd = command;
    if (cmd[0] != '/') {
        cmd = "/" + cmd;
    }

    // 通过 CommandRegistry 执行嵌套命令
    auto& registry = CommandRegistry::getGlobal();
    auto result = registry.execute(cmd, source);

    if (result.failed()) {
        source.sendError(result.error().message());
        return 0;
    }

    return result.value();
}

// ========== 命令注册 ==========

void ExecuteCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto executeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("execute");
    executeNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(executeNode,
        support::makeMetadata("Executes a command.", "/execute as|at|in|positioned|run|if|unless ...", 2, {}, false));

    // ========== run <command> ==========
    // /execute run <command> - 直接执行命令
    auto runNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto runCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    runCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executeRun(ctx); });
    runNode->addChild(runCommandArg);
    executeNode->addChild(runNode);

    // ========== as <entity> run <command> ==========
    // /execute as <entity> run <command> - 以指定实体身份执行命令
    // 使用 entities() 支持所有实体类型（@e/@p/@a/@r/@s）
    auto asNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("as");
    auto asEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "entity", EntityArgumentType::entities());
    auto asRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto asCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    asCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executeAs(ctx); });
    asRunNode->addChild(asCommandArg);
    asEntityArg->addChild(asRunNode);
    asNode->addChild(asEntityArg);
    executeNode->addChild(asNode);

    // ========== at <entity> run <command> ==========
    // /execute at <entity> run <command> - 在指定实体位置执行命令
    // 使用 entities() 支持所有实体类型
    auto atNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("at");
    auto atEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "entity", EntityArgumentType::entities());
    auto atRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto atCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    atCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executeAt(ctx); });
    atRunNode->addChild(atCommandArg);
    atEntityArg->addChild(atRunNode);
    atNode->addChild(atEntityArg);
    executeNode->addChild(atNode);

    // ========== positioned <pos> run <command> ==========
    // /execute positioned <pos> run <command> - 在指定位置执行命令
    auto positionedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("positioned");
    auto posArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3());
    auto posRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto posCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    posCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executePositioned(ctx); });
    posRunNode->addChild(posCommandArg);
    posArg->addChild(posRunNode);
    positionedNode->addChild(posArg);
    executeNode->addChild(positionedNode);

    // ========== in <dimension> run <command> ==========
    // /execute in <dimension> run <command> - 在指定维度执行命令
    auto inNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("in");
    auto inDimArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, DimensionId>>(
        "dimension", DimensionArgumentType::dimension());
    inDimArg->setCustomSuggestions(std::make_shared<DimensionSuggestionProvider<ServerCommandSource>>());
    auto inRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto inCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    inCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executeIn(ctx); });
    inRunNode->addChild(inCommandArg);
    inDimArg->addChild(inRunNode);
    inNode->addChild(inDimArg);
    executeNode->addChild(inNode);

    // ========== if block <pos> <block> run <command> ==========
    // /execute if block <pos> <block> run <command> - 如果指定位置是指定方块则执行
    auto ifNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("if");
    auto ifBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto ifBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto ifBlockArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("block", StringArgumentType::string());
    auto ifBlockRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto ifBlockCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    ifBlockCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _executeIfBlock(ctx); });
    ifBlockRunNode->addChild(ifBlockCommandArg);
    ifBlockArg->addChild(ifBlockRunNode);
    ifBlockNode->addChild(ifBlockArg);
    ifBlockPosArg->addChild(ifBlockNode);
    ifNode->addChild(ifBlockPosArg);
    executeNode->addChild(ifNode);

    // ========== unless block <pos> <block> run <command> ==========
    // /execute unless block <pos> <block> run <command> - 如果指定位置不是指定方块则执行
    auto unlessNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("unless");
    auto unlessBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto unlessBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto unlessBlockArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("block", StringArgumentType::string());
    auto unlessBlockRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto unlessBlockCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    unlessBlockCommandArg->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _executeUnlessBlock(ctx); });
    unlessBlockRunNode->addChild(unlessBlockCommandArg);
    unlessBlockArg->addChild(unlessBlockRunNode);
    unlessBlockNode->addChild(unlessBlockArg);
    unlessBlockPosArg->addChild(unlessBlockNode);
    unlessNode->addChild(unlessBlockPosArg);
    executeNode->addChild(unlessNode);

    dispatcher.registerCommand(executeNode);
}

// ========== 子命令实现 ==========

i32 ExecuteCommand::_executeRun(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string command = context.getArgument<std::string>("command");

    return _executeNestedCommand(source, command);
}

i32 ExecuteCommand::_executeAs(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("entity");
    std::string command = context.getArgument<std::string>("command");

    // execute as <entity> 使用 EntityArgument.entities() 支持所有实体类型
    // 使用 EntityResolver 解析实体选择器（支持 @e/@p/@a/@r/@s）
    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendError("commands.execute.failed.noEntity");
        return 0;
    }

    // 以每个目标实体身份执行
    i32 totalResult = 0;
    for (Entity* entity : entities) {
        if (entity == nullptr) continue;

        // 创建修改后的命令源（以该实体身份执行）
        // withEntity(entity) 只替换实体和名称，
        // 保留位置、旋转、维度不变
        ServerCommandSource modifiedSource = source.withEntity(*entity);

        // 执行嵌套命令
        totalResult += _executeNestedCommand(modifiedSource, command);
    }

    return totalResult;
}

i32 ExecuteCommand::_executeAt(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("entity");
    std::string command = context.getArgument<std::string>("command");

    // execute at <entity> 使用 EntityArgument.entities() 支持所有实体类型
    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendError("commands.execute.failed.noEntity");
        return 0;
    }

    // 在每个目标实体位置执行
    // at 子命令修改位置+旋转+维度，但不改变执行者实体
    i32 totalResult = 0;
    for (Entity* entity : entities) {
        if (entity == nullptr) continue;

        // 修改位置、旋转和维度到目标实体
        // at 子命令同时设置位置和旋转
        ServerCommandSource modifiedSource = source.withPosition(Vector3d(static_cast<f64>(entity->position().x),
            static_cast<f64>(entity->position().y),
            static_cast<f64>(entity->position().z)));
        modifiedSource = modifiedSource.withRotation(Vector2f(entity->yaw(), entity->pitch()));
        modifiedSource = modifiedSource.withDimension(entity->dimension());

        // 执行嵌套命令
        totalResult += _executeNestedCommand(modifiedSource, command);
    }

    return totalResult;
}

i32 ExecuteCommand::_executePositioned(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Vector3d position = Vec3ArgumentType::getVec3(context, "pos", source);
    std::string command = context.getArgument<std::string>("command");

    // 创建修改位置的命令源
    ServerCommandSource modifiedSource = source.withPosition(position);

    return _executeNestedCommand(modifiedSource, command);
}

i32 ExecuteCommand::_executeIn(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    DimensionId targetDimId = context.getArgument<DimensionId>("dimension");
    std::string command = context.getArgument<std::string>("command");

    // 验证目标维度是否存在
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.execute.failed.noServer");
        return 0;
    }

    if (!server->dimensionManager().hasDimension(targetDimId)) {
        source.sendError("commands.execute.in.invalidDimension");
        return 0;
    }

    // 切换维度到目标维度
    ServerCommandSource modifiedSource = source.withDimension(targetDimId);

    // 如果维度发生变化，需要对坐标进行缩放
    // MC Java 的 withLevel 会同时进行坐标缩放（如下界 x/z ÷ 8），
    // 本项目的 withDimension 仅切换维度 ID，坐标缩放需要手动处理
    DimensionId sourceDimId = source.dimensionId();
    if (sourceDimId != targetDimId) {
        DimensionType sourceDimType = DimensionType::fromId(sourceDimId);
        DimensionType targetDimType = DimensionType::fromId(targetDimId);

        if (sourceDimType.coordinateScale() != targetDimType.coordinateScale()) {
            Vector3d scaledPos = DimensionType::transformPosition(source.position(), sourceDimType, targetDimType);
            modifiedSource = modifiedSource.withPosition(scaledPos);
        }
    }

    return _executeNestedCommand(modifiedSource, command);
}

i32 ExecuteCommand::_executeIfBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Vector3i position = BlockPosArgumentType::getBlockPos(context, "pos", source);
    std::string blockInput = context.getArgument<std::string>("block");
    std::string command = context.getArgument<std::string>("command");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.execute.failed.noWorld");
        return 0;
    }

    // 解析目标方块
    Block* targetBlock = parseBlockId(blockInput);
    if (targetBlock == nullptr) {
        source.sendError("commands.execute.failed.invalidBlock");
        return 0;
    }

    // 检查方块
    const BlockState* currentState = world->getBlockState(position.x, position.y, position.z);
    bool matches = false;
    if (currentState != nullptr) {
        matches = currentState->getBlock().blockId() == targetBlock->blockId();
    }

    if (!matches) {
        // 条件不满足，不执行命令
        return 0;
    }

    // 条件满足，执行嵌套命令
    return _executeNestedCommand(source, command);
}

i32 ExecuteCommand::_executeUnlessBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Vector3i position = BlockPosArgumentType::getBlockPos(context, "pos", source);
    std::string blockInput = context.getArgument<std::string>("block");
    std::string command = context.getArgument<std::string>("command");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.execute.failed.noWorld");
        return 0;
    }

    // 解析目标方块
    Block* targetBlock = parseBlockId(blockInput);
    if (targetBlock == nullptr) {
        source.sendError("commands.execute.failed.invalidBlock");
        return 0;
    }

    // 检查方块
    const BlockState* currentState = world->getBlockState(position.x, position.y, position.z);
    bool matches = false;
    if (currentState != nullptr) {
        matches = currentState->getBlock().blockId() == targetBlock->blockId();
    }

    if (matches) {
        // 条件满足，不执行命令
        return 0;
    }

    // 条件不满足，执行嵌套命令
    return _executeNestedCommand(source, command);
}

} // namespace command
} // namespace mc
