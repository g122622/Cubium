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

#include "FunctionCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/FunctionArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/NbtPath.hpp"
#include "common/command/arguments/NbtPathArgumentType.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/application/IServer.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/command/support/FunctionSuggestionProvider.hpp"
#include "server/function/FunctionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <exception>
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {

// ========== 注册命令 ==========

void FunctionCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto functionNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("function");
    functionNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(functionNode,
        support::makeMetadata(
            "Runs a function from a data pack.", "/function <name> [<arguments>|with <source> [path]]", 2, {}, true));

    // /function <name>（无参数）
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, FunctionArgumentResult>>(
        "name", FunctionArgumentType::functions());
    nameArg->setCustomSuggestions(std::make_shared<FunctionSuggestionProvider>());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runFunction(ctx); });

    // /function <name> <arguments>
    auto argumentsArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::compound_tag>>>(
            "arguments", NbtCompoundArgumentType::nbtCompound());
    argumentsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runFunctionWithArguments(ctx); });

    // /function <name> with <entity|block|storage> [path]
    auto withNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("with");

    // with entity <target> [path]
    auto withEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto withEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto withEntityPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    withEntityPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runWithEntity(ctx); });
    withEntityTargetArg->addChild(withEntityPathArg);
    withEntityTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runWithEntity(ctx); });
    withEntityNode->addChild(withEntityTargetArg);

    // with block <pos> [path]
    auto withBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto withBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto withBlockPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    withBlockPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runWithBlock(ctx); });
    withBlockPosArg->addChild(withBlockPathArg);
    withBlockPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runWithBlock(ctx); });
    withBlockNode->addChild(withBlockPosArg);

    // with storage <id> [path]
    auto withStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto withStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", ResourceLocationArgumentType::resourceLocation());
    auto withStoragePathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    withStoragePathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runWithStorage(ctx); });
    withStorageIdArg->addChild(withStoragePathArg);
    withStorageIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _runWithStorage(ctx); });
    withStorageNode->addChild(withStorageIdArg);

    withNode->addChild(withEntityNode);
    withNode->addChild(withBlockNode);
    withNode->addChild(withStorageNode);

    // 命令树挂接顺序：name → arguments / with
    nameArg->addChild(argumentsArg);
    nameArg->addChild(withNode);

    functionNode->addChild(nameArg);
    dispatcher.registerCommand(functionNode);
}

// ========== 子命令处理器 ==========

i32 FunctionCommand::_runFunction(CommandContext<ServerCommandSource>& context)
{
    return _executeFunctions(context, nullptr);
}

i32 FunctionCommand::_runFunctionWithArguments(CommandContext<ServerCommandSource>& context)
{
    auto arguments = NbtCompoundArgumentType::getNbt(context, "arguments");
    if (arguments == nullptr) {
        context.getSource().sendError("Invalid arguments");
        return 0;
    }
    return _executeFunctions(context, arguments.get());
}

i32 FunctionCommand::_runWithEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("target");
    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendError("No entity matched the selector");
        return 0;
    }

    Entity* entity = entities.front();
    EntityDataAccessor accessor(entity);

    try {
        auto data = accessor.getData();
        if (data == nullptr) {
            source.sendError("Failed to get entity data");
            return 0;
        }

        // 检查是否有路径参数
        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");
            auto results = path.get(*data);
            if (results.empty()) {
                source.sendError("Path not found in entity data");
                return 0;
            }
            if (results.size() != 1) {
                source.sendError("Path matched multiple values; expected a single compound tag");
                return 0;
            }
            const nbt::tags::tag* result = results[0];
            if (result->id() != nbt::TagId::Compound) {
                source.sendError("Path did not resolve to a compound tag");
                return 0;
            }
            const auto* compound = static_cast<const nbt::tags::compound_tag*>(result);
            return _executeFunctions(context, compound);
        }

        return _executeFunctions(context, data.get());
    }
    catch (const std::exception& e) {
        source.sendError(std::string("Failed to read entity data: ") + e.what());
        return 0;
    }
}

i32 FunctionCommand::_runWithBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();
    if (world == nullptr) {
        source.sendError("No world available");
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "pos", source);
    BlockDataAccessor accessor(world, BlockPos(pos.x, pos.y, pos.z));

    if (!accessor.isValid()) {
        source.sendError("No block entity at the given position");
        return 0;
    }

    try {
        auto data = accessor.getData();
        if (data == nullptr) {
            source.sendError("Failed to get block data");
            return 0;
        }

        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");
            auto results = path.get(*data);
            if (results.empty()) {
                source.sendError("Path not found in block data");
                return 0;
            }
            if (results.size() != 1) {
                source.sendError("Path matched multiple values; expected a single compound tag");
                return 0;
            }
            const nbt::tags::tag* result = results[0];
            if (result->id() != nbt::TagId::Compound) {
                source.sendError("Path did not resolve to a compound tag");
                return 0;
            }
            const auto* compound = static_cast<const nbt::tags::compound_tag*>(result);
            return _executeFunctions(context, compound);
        }

        return _executeFunctions(context, data.get());
    }
    catch (const std::exception& e) {
        source.sendError(std::string("Failed to read block data: ") + e.what());
        return 0;
    }
}

i32 FunctionCommand::_runWithStorage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("No server available");
        return 0;
    }

    ResourceLocation id = context.getArgument<ResourceLocation>("id");
    CommandStorage& storage = server->commandStorage();
    StorageDataAccessor accessor(&storage, id);

    try {
        auto data = accessor.getData();
        if (data == nullptr) {
            source.sendError("Failed to get storage data");
            return 0;
        }

        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");
            auto results = path.get(*data);
            if (results.empty()) {
                source.sendError("Path not found in storage data");
                return 0;
            }
            if (results.size() != 1) {
                source.sendError("Path matched multiple values; expected a single compound tag");
                return 0;
            }
            const nbt::tags::tag* result = results[0];
            if (result->id() != nbt::TagId::Compound) {
                source.sendError("Path did not resolve to a compound tag");
                return 0;
            }
            const auto* compound = static_cast<const nbt::tags::compound_tag*>(result);
            return _executeFunctions(context, compound);
        }

        return _executeFunctions(context, data.get());
    }
    catch (const std::exception& e) {
        source.sendError(std::string("Failed to read storage data: ") + e.what());
        return 0;
    }
}

// ========== 辅助 ==========

i32 FunctionCommand::_executeFunctions(
    CommandContext<ServerCommandSource>& context, const nbt::tags::compound_tag* arguments)
{
    auto& source = context.getSource();
    auto result = FunctionArgumentType::getFunctionResult(context, "name");

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("Function command requires a server instance");
        return 0;
    }

    auto& functionManager = server->functionManager();

    if (result.isTag()) {
        // 标签引用: #namespace:path
        const auto& tagId = result.id();

        if (!functionManager.hasTag(tagId)) {
            std::ostringstream ss;
            ss << "Unknown function tag '" << tagId.toString() << "'";
            source.sendError(ss.str());
            return 0;
        }

        // 执行标签中的所有函数
        const auto& functionIds = functionManager.getTag(tagId);
        i32 totalSuccess = 0;
        i32 totalFailure = 0;
        Size executedCount = 0;

        for (const auto& funcId : functionIds) {
            auto execResult = functionManager.execute(funcId, source, arguments);
            totalSuccess += execResult.successCount;
            totalFailure += execResult.failureCount;
            ++executedCount;
        }

        std::ostringstream ss;
        ss << "Executed " << executedCount << " functions from tag '" << tagId.toString() << "' (" << totalSuccess
           << " commands succeeded";
        if (totalFailure > 0) {
            ss << ", " << totalFailure << " failed";
        }
        ss << ")";
        source.sendMessage(ss.str());

        return totalSuccess;
    }

    // 普通函数引用
    const auto& functionId = result.id();

    // 检查函数是否存在
    if (!functionManager.hasFunction(functionId)) {
        std::ostringstream ss;
        ss << "Unknown function '" << functionId.toString() << "'";
        source.sendError(ss.str());
        return 0;
    }

    // 执行函数
    auto execResult = functionManager.execute(functionId, source, arguments);

    // 反馈执行结果
    std::ostringstream ss;
    ss << "Executed function '" << functionId.toString() << "' (" << execResult.successCount << " commands succeeded";
    if (execResult.failureCount > 0) {
        ss << ", " << execResult.failureCount << " failed";
    }
    ss << ")";
    source.sendMessage(ss.str());

    return execResult.successCount;
}

} // namespace command
} // namespace mc
