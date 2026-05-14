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
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <sstream>

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

/**
 * @brief 获取玩家实体
 * @param source 命令源
 * @param playerId 玩家ID
 * @return 玩家实体指针，如果不存在返回 nullptr
 */
ServerPlayer* getPlayerEntity(ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    if (server == nullptr) {
        return nullptr;
    }

    auto* world = source.world();
    if (world == nullptr) {
        return nullptr;
    }

    // 通过 ServerPlayerEntityManager 获取玩家实体
    Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
    return static_cast<ServerPlayer*>(player);
}

} // namespace

// ========== 嵌套命令执行 ==========

i32 ExecuteCommand::executeNestedCommand(ServerCommandSource& source, const std::string& command)
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
        support::makeMetadata("Executes a command.", "/execute as|at|positioned|run|if|unless ...", 2, {}, false));

    // ========== run <command> ==========
    // /execute run <command> - 直接执行命令
    auto runNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto runCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    runCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return executeRun(ctx); });
    runNode->addChild(runCommandArg);
    executeNode->addChild(runNode);

    // ========== as <entity> run <command> ==========
    // /execute as <entity> run <command> - 以指定实体身份执行命令
    auto asNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("as");
    auto asEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "entity", EntityArgumentType::player());
    auto asRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto asCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    asCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return executeAs(ctx); });
    asRunNode->addChild(asCommandArg);
    asEntityArg->addChild(asRunNode);
    asNode->addChild(asEntityArg);
    executeNode->addChild(asNode);

    // ========== at <entity> run <command> ==========
    // /execute at <entity> run <command> - 在指定实体位置执行命令
    auto atNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("at");
    auto atEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "entity", EntityArgumentType::player());
    auto atRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto atCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    atCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return executeAt(ctx); });
    atRunNode->addChild(atCommandArg);
    atEntityArg->addChild(atRunNode);
    atNode->addChild(atEntityArg);
    executeNode->addChild(atNode);

    // ========== positioned <pos> run <command> ==========
    // /execute positioned <pos> run <command> - 在指定位置执行命令
    auto positionedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("positioned");
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos", Vec3ArgumentType::vec3());
    auto posRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto posCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    posCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return executePositioned(ctx); });
    posRunNode->addChild(posCommandArg);
    posArg->addChild(posRunNode);
    positionedNode->addChild(posArg);
    executeNode->addChild(positionedNode);

    // ========== if block <pos> <block> run <command> ==========
    // /execute if block <pos> <block> run <command> - 如果指定位置是指定方块则执行
    auto ifNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("if");
    auto ifBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto ifBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3i>>(
        "pos", BlockPosArgumentType::blockPos());
    auto ifBlockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "block", StringArgumentType::string());
    auto ifBlockRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto ifBlockCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    ifBlockCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return executeIfBlock(ctx); });
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
    auto unlessBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3i>>(
        "pos", BlockPosArgumentType::blockPos());
    auto unlessBlockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "block", StringArgumentType::string());
    auto unlessBlockRunNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("run");
    auto unlessBlockCommandArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "command", StringArgumentType::greedyString());
    unlessBlockCommandArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return executeUnlessBlock(ctx); });
    unlessBlockRunNode->addChild(unlessBlockCommandArg);
    unlessBlockArg->addChild(unlessBlockRunNode);
    unlessBlockNode->addChild(unlessBlockArg);
    unlessBlockPosArg->addChild(unlessBlockNode);
    unlessNode->addChild(unlessBlockPosArg);
    executeNode->addChild(unlessNode);

    dispatcher.registerCommand(executeNode);
}

// ========== 子命令实现 ==========

i32 ExecuteCommand::executeRun(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string command = context.getArgument<std::string>("command");

    return executeNestedCommand(source, command);
}

i32 ExecuteCommand::executeAs(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("entity");
    std::string command = context.getArgument<std::string>("command");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.execute.failed.noEntity");
        return 0;
    }

    // 以每个目标玩家身份执行
    i32 totalResult = 0;

    for (PlayerId playerId : playerIds) {
        // 获取玩家实体
        ServerPlayer* player = getPlayerEntity(source, playerId);
        if (player == nullptr) {
            continue;
        }

        // 创建修改后的命令源（以该玩家身份执行）
        ServerCommandSource modifiedSource = source.withPlayer(player);

        // 执行嵌套命令
        totalResult += executeNestedCommand(modifiedSource, command);
    }

    return totalResult;
}

i32 ExecuteCommand::executeAt(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("entity");
    std::string command = context.getArgument<std::string>("command");

    // 解析目标玩家
    std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("commands.execute.failed.noEntity");
        return 0;
    }

    // 在每个目标玩家位置执行
    i32 totalResult = 0;
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.execute.failed.noServer");
        return 0;
    }

    for (PlayerId playerId : playerIds) {
        // 获取玩家数据
        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            continue;
        }

        // 获取玩家所在的世界
        ServerPlayer* player = getPlayerEntity(source, playerId);
        IWorld* iworld = player != nullptr ? player->world() : nullptr;
        server::ServerWorld* world = iworld != nullptr ? static_cast<server::ServerWorld*>(iworld) : nullptr;

        // 创建修改位置和世界的命令源
        ServerCommandSource modifiedSource = source.withPosition(
            Vector3d(playerData->x, playerData->y, playerData->z));
        if (world != nullptr) {
            modifiedSource = modifiedSource.withWorld(world);
        }

        // 执行嵌套命令
        totalResult += executeNestedCommand(modifiedSource, command);
    }

    return totalResult;
}

i32 ExecuteCommand::executePositioned(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Vector3d position = context.getArgument<Vector3d>("pos");
    std::string command = context.getArgument<std::string>("command");

    // 创建修改位置的命令源
    ServerCommandSource modifiedSource = source.withPosition(position);

    return executeNestedCommand(modifiedSource, command);
}

i32 ExecuteCommand::executeIfBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Vector3i position = context.getArgument<Vector3i>("pos");
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
    return executeNestedCommand(source, command);
}

i32 ExecuteCommand::executeUnlessBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    Vector3i position = context.getArgument<Vector3i>("pos");
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
    return executeNestedCommand(source, command);
}

} // namespace command
} // namespace mc
