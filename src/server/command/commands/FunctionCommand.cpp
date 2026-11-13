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
#include "common/util/text/ComponentFactory.hpp"
#include "common/util/text/ComponentUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "server/application/IServer.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
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
    functionNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(functionNode,
        support::makeMetadata(
            "Runs a function from a data pack.", "/function <name> [<arguments>|with <source> [path]]", 2, {}, true));

    // /function <name>（无参数）
    auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, FunctionArgumentResult>>(
        "name", FunctionArgumentType::functions());
    nameArg->setCustomSuggestions(std::make_shared<FunctionSuggestionProvider>());
    nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runFunction(ctx);
    });

    // /function <name> <arguments>
    auto argumentsArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::compound_tag>>>(
            "arguments", NbtCompoundArgumentType::nbtCompound());
    argumentsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runFunctionWithArguments(ctx);
    });

    // /function <name> with <entity|block|storage> [path]
    auto withNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("with");

    // with entity <target> [path]
    auto withEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    // 原版 with entity 的参数名是 source。
    auto withEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "source", EntityArgumentType::entity());
    auto withEntityPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    withEntityPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runWithEntity(ctx);
    });
    withEntityTargetArg->addChild(withEntityPathArg);
    withEntityTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runWithEntity(ctx);
    });
    withEntityNode->addChild(withEntityTargetArg);

    // with block <pos> [path]
    auto withBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    // 原版 with block 的参数名是 sourcePos。
    auto withBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "sourcePos", BlockPosArgumentType::blockPos());
    auto withBlockPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    withBlockPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runWithBlock(ctx);
    });
    withBlockPosArg->addChild(withBlockPathArg);
    withBlockPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runWithBlock(ctx);
    });
    withBlockNode->addChild(withBlockPosArg);

    // with storage <id> [path]
    auto withStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    // 原版 with storage 的参数名是 source。
    auto withStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "source", ResourceLocationArgumentType::resourceLocation());
    // 原版 with storage 的 source 挂了 suggestResource(STORAGE)。
    withStorageIdArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    auto withStoragePathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    withStoragePathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runWithStorage(ctx);
    });
    withStorageIdArg->addChild(withStoragePathArg);
    withStorageIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _runWithStorage(ctx);
    });
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
        context.getSource().sendFailure(::mc::text::literal("Invalid arguments"));
        return 0;
    }
    return _executeFunctions(context, arguments.get());
}

i32 FunctionCommand::_runWithEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("source");
    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendFailure(::mc::text::literal("No entity matched the selector"));
        return 0;
    }

    Entity* entity = entities.front();
    EntityDataAccessor accessor(entity);

    {
        auto dataResult = accessor.getData();
        if (dataResult.failed()) {
            source.sendFailure(::mc::text::literal("Failed to read entity data: " + dataResult.error().message()));
            return 0;
        }
        auto data = std::move(dataResult).value();

        // 检查是否有路径参数
        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");
            auto results = path.get(*data);
            if (results.empty()) {
                source.sendFailure(::mc::text::literal("Path not found in entity data"));
                return 0;
            }
            if (results.size() != 1) {
                source.sendFailure(::mc::text::literal("Path matched multiple values; expected a single compound tag"));
                return 0;
            }
            const nbt::tags::tag* result = results[0];
            if (result->id() != nbt::TagId::Compound) {
                source.sendFailure(::mc::text::literal("Path did not resolve to a compound tag"));
                return 0;
            }
            const auto* compound = static_cast<const nbt::tags::compound_tag*>(result);
            return _executeFunctions(context, compound);
        }

        return _executeFunctions(context, data.get());
    }
}

i32 FunctionCommand::_runWithBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();
    if (world == nullptr) {
        source.sendFailure(::mc::text::literal("No world available"));
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "sourcePos", source);
    BlockDataAccessor accessor(world, BlockPos(pos.x, pos.y, pos.z));

    if (!accessor.isValid()) {
        source.sendFailure(::mc::text::literal("No block entity at the given position"));
        return 0;
    }

    {
        auto dataResult = accessor.getData();
        if (dataResult.failed()) {
            source.sendFailure(::mc::text::literal("Failed to read data: " + dataResult.error().message()));
            return 0;
        }
        auto data = std::move(dataResult).value();

        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");
            auto results = path.get(*data);
            if (results.empty()) {
                source.sendFailure(::mc::text::literal("Path not found in block data"));
                return 0;
            }
            if (results.size() != 1) {
                source.sendFailure(::mc::text::literal("Path matched multiple values; expected a single compound tag"));
                return 0;
            }
            const nbt::tags::tag* result = results[0];
            if (result->id() != nbt::TagId::Compound) {
                source.sendFailure(::mc::text::literal("Path did not resolve to a compound tag"));
                return 0;
            }
            const auto* compound = static_cast<const nbt::tags::compound_tag*>(result);
            return _executeFunctions(context, compound);
        }

        return _executeFunctions(context, data.get());
    }
}

i32 FunctionCommand::_runWithStorage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendFailure(::mc::text::literal("No server available"));
        return 0;
    }

    ResourceLocation id = context.getArgument<ResourceLocation>("source");
    CommandStorage& storage = server->commandStorage();
    StorageDataAccessor accessor(&storage, id);

    {
        auto dataResult = accessor.getData();
        if (dataResult.failed()) {
            source.sendFailure(::mc::text::literal("Failed to read data: " + dataResult.error().message()));
            return 0;
        }
        auto data = std::move(dataResult).value();

        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");
            auto results = path.get(*data);
            if (results.empty()) {
                source.sendFailure(::mc::text::literal("Path not found in storage data"));
                return 0;
            }
            if (results.size() != 1) {
                source.sendFailure(::mc::text::literal("Path matched multiple values; expected a single compound tag"));
                return 0;
            }
            const nbt::tags::tag* result = results[0];
            if (result->id() != nbt::TagId::Compound) {
                source.sendFailure(::mc::text::literal("Path did not resolve to a compound tag"));
                return 0;
            }
            const auto* compound = static_cast<const nbt::tags::compound_tag*>(result);
            return _executeFunctions(context, compound);
        }

        return _executeFunctions(context, data.get());
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
        source.sendFailure(::mc::text::literal("Function command requires a server instance"));
        return 0;
    }

    auto& functionManager = server->functionManager();

    // 函数体以「被抑制输出」的源执行：原版 modifySenderForExecution 会 withSuppressedOutput，
    // 函数内命令的反馈不再直接进玩家聊天框，只有 /function 自己报「Running function ...」。
    auto executionSource = source.withSuppressedOutput();

    if (result.isTag()) {
        // 标签引用: #namespace:path
        const auto& tagId = result.id();

        if (!functionManager.hasTag(tagId)) {
            source.sendFailure(::mc::text::translatableEscape("arguments.function.tag.unknown", tagId.toString()));
            return 0;
        }

        const auto& functionIds = functionManager.getTag(tagId);
        std::vector<std::unique_ptr<::mc::text::ITextComponent>> ids;
        ids.reserve(functionIds.size());
        for (const auto& funcId : functionIds) {
            ids.push_back(::mc::text::literal(funcId.toString()));
        }

        source.sendSuccess(::mc::text::translatable("commands.function.scheduled.multiple",
                               ::mc::text::ComponentUtils::formatList(std::move(ids))),
            true);

        for (const auto& funcId : functionIds) {
            static_cast<void>(functionManager.execute(funcId, executionSource, arguments));
        }

        // 原版 /function 作为命令返回 0（执行数只用于内部统计）。
        return 0;
    }

    // 普通函数引用
    const auto& functionId = result.id();

    // 检查函数是否存在
    if (!functionManager.hasFunction(functionId)) {
        source.sendFailure(::mc::text::translatableEscape("arguments.function.unknown", functionId.toString()));
        return 0;
    }

    source.sendSuccess(
        ::mc::text::translatable("commands.function.scheduled.single", ::mc::text::literal(functionId.toString())),
        true);

    static_cast<void>(functionManager.execute(functionId, executionSource, arguments));
    return 0;
}

} // namespace command
} // namespace mc
