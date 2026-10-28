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
 * The copyright notice and this permission notice shall be included in all
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

#include "DataCommand.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/NbtPath.hpp"
#include "common/command/arguments/NbtPathArgumentType.hpp"
#include "common/command/arguments/ResourceArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <memory>
#include <string>

// Bring operator<< for nbt::tags::tag into scope for ADL
using mc::nbt::operator<<;

namespace mc {
namespace command {

namespace {

/// 原版 storage 参数（/data、/execute 的 storage 目标）注册时挂了 suggestResource(STORAGE)，
/// 命令树里带建议提供器位；这里给参数节点挂上自定义提供器以对齐。
void attachStorageSuggestions(const std::shared_ptr<CommandNode<ServerCommandSource>>& node)
{
    node->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
}

} // namespace

// ========== 注册命令 ==========

namespace {

/// `/data modify <target> <path>` 之后的五种写操作（原版 DataCommand 的 set/append/prepend/
/// merge/insert）。参数名、来源分支都与原版逐层对齐；真正的写入由 DataCommand::_modify 完成。
///
/// 可执行性逐层照抄原版：只有完整给出写入内容的分支才挂命令（`set <value>`、`from ...`、
/// `string ...` 及其子分支），中间的 set/append/from/string 字面量与 targetPath 本身都不可执行。
void makeModifyOperations(const std::shared_ptr<CommandNode<ServerCommandSource>>& pathArg)
{
    using Callback = std::function<i32(CommandContext<ServerCommandSource>&)>;
    const Callback setCallback = [](CommandContext<ServerCommandSource>& ctx) {
        return DataCommand::_modify(ctx, 0);
    };
    const Callback appendCallback = [](CommandContext<ServerCommandSource>& ctx) {
        return DataCommand::_modify(ctx, 1);
    };
    const Callback prependCallback = [](CommandContext<ServerCommandSource>& ctx) {
        return DataCommand::_modify(ctx, 2);
    };
    const Callback mergeCallback = [](CommandContext<ServerCommandSource>& ctx) {
        return DataCommand::_modify(ctx, 3);
    };
    const Callback insertCallback = [](CommandContext<ServerCommandSource>& ctx) {
        return DataCommand::_modify(ctx, 4);
    };

    // 来源参数：from block|entity|storage ... 与 string ...，叶子都落到同一个 _modify。
    auto addSources = [](const std::shared_ptr<CommandNode<ServerCommandSource>>& parent, const Callback& command) {
        // from block|entity|storage <...> <path>
        auto fromNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("from");
        {
            auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
            auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "sourcePos", BlockPosArgumentType::blockPos());
            posNode->setCommand(command);
            auto pathNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
                "sourcePath", NbtPathArgumentType::nbtPath());
            pathNode->setCommand(command);
            posNode->addChild(pathNode);
            blockNode->addChild(posNode);
            fromNode->addChild(blockNode);

            auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
            auto sourceNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
                "source", EntityArgumentType::entity());
            sourceNode->setCommand(command);
            auto entityPathNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
                "sourcePath", NbtPathArgumentType::nbtPath());
            entityPathNode->setCommand(command);
            sourceNode->addChild(entityPathNode);
            entityNode->addChild(sourceNode);
            fromNode->addChild(entityNode);

            auto storageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
            auto sourceStorageNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
                "source", std::make_shared<ResourceLocationArgumentType>());
            attachStorageSuggestions(sourceStorageNode);
            sourceStorageNode->setCommand(command);
            auto storagePathNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
                "sourcePath", NbtPathArgumentType::nbtPath());
            storagePathNode->setCommand(command);
            sourceStorageNode->addChild(storagePathNode);
            storageNode->addChild(sourceStorageNode);
            fromNode->addChild(storageNode);
        }
        parent->addChild(fromNode);

        // string ...：把源值转成字符串写入。原版 string 下先挂字面量 block|entity|storage，
        // 再挂一个 string 参数（原样文本），值来源之后还有 start|end 用来截取子串。
        auto stringNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("string");
        const Callback stringCommand = [command](CommandContext<ServerCommandSource>& ctx) {
            ctx.markParsed("stringValueArg");
            return command(ctx);
        };
        const auto addExtract = [&](const std::shared_ptr<CommandNode<ServerCommandSource>>& parent) {
            auto startNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
                "start", IntegerArgumentType::integer());
            startNode->setCommand(stringCommand);
            auto endNode =
                std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("end", IntegerArgumentType::integer());
            endNode->setCommand(stringCommand);
            startNode->addChild(endNode);
            parent->addChild(startNode);
        };
        {
            auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
            auto posNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
                "sourcePos", BlockPosArgumentType::blockPos());
            posNode->setCommand(stringCommand);
            auto pathNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
                "sourcePath", NbtPathArgumentType::nbtPath());
            pathNode->setCommand(stringCommand);
            addExtract(pathNode);
            posNode->addChild(pathNode);
            blockNode->addChild(posNode);
            stringNode->addChild(blockNode);

            auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
            auto sourceNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
                "source", EntityArgumentType::entity());
            sourceNode->setCommand(stringCommand);
            auto entityPathNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
                "sourcePath", NbtPathArgumentType::nbtPath());
            entityPathNode->setCommand(stringCommand);
            addExtract(entityPathNode);
            sourceNode->addChild(entityPathNode);
            entityNode->addChild(sourceNode);
            stringNode->addChild(entityNode);

            auto storageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
            auto sourceStorageNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
                "source", std::make_shared<ResourceLocationArgumentType>());
            attachStorageSuggestions(sourceStorageNode);
            sourceStorageNode->setCommand(stringCommand);
            auto storagePathNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
                "sourcePath", NbtPathArgumentType::nbtPath());
            storagePathNode->setCommand(stringCommand);
            addExtract(storagePathNode);
            sourceStorageNode->addChild(storagePathNode);
            storageNode->addChild(sourceStorageNode);
            stringNode->addChild(storageNode);
        }
        parent->addChild(stringNode);
    };

    // set <value> | from ... | string ...
    {
        auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
        auto valueLiteral = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
        auto valueNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::tag>>>(
            "value", NbtTagArgumentType::nbtTag());
        valueNode->setCommand(setCallback);
        valueLiteral->addChild(valueNode);
        setNode->addChild(valueLiteral);
        addSources(setNode, setCallback);
        pathArg->addChild(setNode);
    }

    for (const auto& op : {std::pair<const char*, Callback>{"append", appendCallback},
             std::pair<const char*, Callback>{"prepend", prependCallback}}) {
        auto opNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(op.first);
        // 原版 append/prepend 除了 from|string 也直接收 value <nbt_tag>，字面量也叫 value。
        auto valueLiteral = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
        auto valueNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::tag>>>(
            "value", NbtTagArgumentType::nbtTag());
        valueNode->setCommand(op.second);
        valueLiteral->addChild(valueNode);
        opNode->addChild(valueLiteral);
        addSources(opNode, op.second);
        pathArg->addChild(opNode);
    }

    // merge <value> | from ... | string ...
    {
        auto mergeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("merge");
        auto valueLiteral = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
        auto valueNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::tag>>>(
            "value", NbtTagArgumentType::nbtTag());
        valueNode->setCommand(mergeCallback);
        valueLiteral->addChild(valueNode);
        mergeNode->addChild(valueLiteral);
        addSources(mergeNode, mergeCallback);
        pathArg->addChild(mergeNode);
    }

    // insert <index> ...：原版的 index 是普通 integer，可正可负。
    {
        auto insertNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("insert");
        auto indexNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
            "index", std::make_shared<IntegerArgumentType>());
        auto valueLiteral = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");
        auto valueNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::tag>>>(
            "value", NbtTagArgumentType::nbtTag());
        valueNode->setCommand(insertCallback);
        valueLiteral->addChild(valueNode);
        indexNode->addChild(valueLiteral);
        insertNode->addChild(indexNode);
        addSources(indexNode, insertCallback);
        pathArg->addChild(insertNode);
    }
}

} // namespace

void DataCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 创建主命令节点
    auto dataNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("data");
    dataNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(dataNode,
        support::makeMetadata("Gets, merges, modifies, or removes block entity and entity NBT data.",
            "/data <get|set|merge|remove> <target> [<path>]",
            2,
            {},
            true));

    // ========== /data get 子命令 ==========
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");

    // /data get block <pos> [<path>] [<scale>]
    auto getBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto getBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "targetPos", BlockPosArgumentType::blockPos());
    auto getBlockPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto getBlockScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>("scale", DoubleArgumentType::doubleArg());

    getBlockScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getBlock(ctx);
    });
    getBlockPathArg->addChild(getBlockScaleArg);
    getBlockPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getBlock(ctx);
    });
    getBlockPosArg->addChild(getBlockPathArg);
    getBlockPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getBlock(ctx);
    });
    getBlockNode->addChild(getBlockPosArg);
    getNode->addChild(getBlockNode);

    // /data get entity <target> [<path>] [<scale>]
    auto getEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto getEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto getEntityPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto getEntityScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>("scale", DoubleArgumentType::doubleArg());

    getEntityScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getEntity(ctx);
    });
    getEntityPathArg->addChild(getEntityScaleArg);
    getEntityPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getEntity(ctx);
    });
    getEntityTargetArg->addChild(getEntityPathArg);
    getEntityTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getEntity(ctx);
    });
    getEntityNode->addChild(getEntityTargetArg);
    getNode->addChild(getEntityNode);

    // /data get storage <id> [<path>] [<scale>]
    auto getStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto getStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "target", std::make_shared<ResourceLocationArgumentType>());
    attachStorageSuggestions(getStorageIdArg);
    auto getStoragePathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto getStorageScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f64>>("scale", DoubleArgumentType::doubleArg());

    getStorageScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getStorage(ctx);
    });
    getStoragePathArg->addChild(getStorageScaleArg);
    getStoragePathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getStorage(ctx);
    });
    getStorageIdArg->addChild(getStoragePathArg);
    getStorageIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getStorage(ctx);
    });
    getStorageNode->addChild(getStorageIdArg);
    getNode->addChild(getStorageNode);

    dataNode->addChild(getNode);

    // ========== /data modify 子命令 ==========
    // 形状对齐原版 DataCommand.register：modify → {block <pos>|entity <target>|storage <id>} → <path>
    // → {set|append|prepend|merge|insert}，每种操作后再接 value/from/string 来源。
    {
        auto modifyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("modify");

        auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
        auto blockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
            "targetPos", BlockPosArgumentType::blockPos());
        auto blockPathArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
            "targetPath", NbtPathArgumentType::nbtPath());
        makeModifyOperations(blockPathArg);
        blockPosArg->addChild(blockPathArg);
        blockNode->addChild(blockPosArg);

        auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
        auto entityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "target", EntityArgumentType::entity());
        auto entityPathArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
            "targetPath", NbtPathArgumentType::nbtPath());
        makeModifyOperations(entityPathArg);
        entityTargetArg->addChild(entityPathArg);
        entityNode->addChild(entityTargetArg);

        auto storageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
        auto storageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
            "target", std::make_shared<ResourceLocationArgumentType>());
        attachStorageSuggestions(storageIdArg);
        auto storagePathArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>(
            "targetPath", NbtPathArgumentType::nbtPath());
        makeModifyOperations(storagePathArg);
        storageIdArg->addChild(storagePathArg);
        storageNode->addChild(storageIdArg);

        modifyNode->addChild(blockNode);
        modifyNode->addChild(entityNode);
        modifyNode->addChild(storageNode);
        dataNode->addChild(modifyNode);
    }

    // ========== /data merge 子命令 ==========
    auto mergeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("merge");

    // /data merge block <pos> <nbt>
    auto mergeBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto mergeBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "targetPos", BlockPosArgumentType::blockPos());
    auto mergeBlockNbtArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::compound_tag>>>(
            "nbt", NbtCompoundArgumentType::nbtCompound());

    mergeBlockNbtArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _mergeBlock(ctx);
    });
    mergeBlockPosArg->addChild(mergeBlockNbtArg);
    mergeBlockNode->addChild(mergeBlockPosArg);
    mergeNode->addChild(mergeBlockNode);

    // /data merge entity <target> <nbt>
    auto mergeEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto mergeEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto mergeEntityNbtArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::compound_tag>>>(
            "nbt", NbtCompoundArgumentType::nbtCompound());

    mergeEntityNbtArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _mergeEntity(ctx);
    });
    mergeEntityTargetArg->addChild(mergeEntityNbtArg);
    mergeEntityNode->addChild(mergeEntityTargetArg);
    mergeNode->addChild(mergeEntityNode);

    // /data merge storage <id> <nbt>
    auto mergeStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto mergeStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "target", std::make_shared<ResourceLocationArgumentType>());
    attachStorageSuggestions(mergeStorageIdArg);
    auto mergeStorageNbtArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::compound_tag>>>(
            "nbt", NbtCompoundArgumentType::nbtCompound());

    mergeStorageNbtArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _mergeStorage(ctx);
    });
    mergeStorageIdArg->addChild(mergeStorageNbtArg);
    mergeStorageNode->addChild(mergeStorageIdArg);
    mergeNode->addChild(mergeStorageNode);

    dataNode->addChild(mergeNode);

    // ========== /data remove 子命令 ==========
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");

    // /data remove block <pos> <path>
    auto removeBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto removeBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "targetPos", BlockPosArgumentType::blockPos());
    auto removeBlockPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());

    removeBlockPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeBlock(ctx);
    });
    removeBlockPosArg->addChild(removeBlockPathArg);
    removeBlockNode->addChild(removeBlockPosArg);
    removeNode->addChild(removeBlockNode);

    // /data remove entity <target> <path>
    auto removeEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto removeEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto removeEntityPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());

    removeEntityPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeEntity(ctx);
    });
    removeEntityTargetArg->addChild(removeEntityPathArg);
    removeEntityNode->addChild(removeEntityTargetArg);
    removeNode->addChild(removeEntityNode);

    // /data remove storage <id> <path>
    auto removeStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto removeStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "target", std::make_shared<ResourceLocationArgumentType>());
    attachStorageSuggestions(removeStorageIdArg);
    auto removeStoragePathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());

    removeStoragePathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeStorage(ctx);
    });
    removeStorageIdArg->addChild(removeStoragePathArg);
    removeStorageNode->addChild(removeStorageIdArg);
    removeNode->addChild(removeStorageNode);

    dataNode->addChild(removeNode);

    dispatcher.registerCommand(dataNode);
}

// ========== get 子命令实现 ==========

i32 DataCommand::_getBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();

    if (world == nullptr) {
        _sendError(source, "commands.data.block.failed.noWorld");
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "targetPos", source);
    BlockDataAccessor accessor(world, BlockPos(pos.x, pos.y, pos.z));

    if (!accessor.isValid()) {
        _sendError(source, "commands.data.block.failed.noBlockEntity");
        return 0;
    }

    try {
        auto data = accessor.getData();

        // 检查是否有路径参数
        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");

            try {
                auto results = path.get(*data);

                if (results.empty()) {
                    _sendError(source, "commands.data.get.pathNotFound");
                    return 0;
                }

                if (results.size() == 1) {
                    const nbt::tags::tag* result = results[0];

                    // 检查是否有缩放参数
                    if (context.hasArgument("scale")) {
                        const f64 scale = context.getArgument<f64>("scale");
                        i32 scaledValue = _scaleValue(*result, scale);
                        source.sendMessage(accessor.getGetMessage(path, scale, scaledValue));
                        return scaledValue;
                    }

                    i32 resultValue = _getSingleResult(*result);
                    source.sendMessage(accessor.getQueryMessage(*result));
                    return resultValue;
                } else {
                    // 多个结果
                    source.sendMessage("Found " + std::to_string(results.size()) + " matches");
                    return static_cast<i32>(results.size());
                }
            }
            catch (const CommandException& e) {
                _sendError(source, e.message());
                return 0;
            }
        } else {
            // 没有路径，返回整个 NBT
            source.sendMessage(accessor.getQueryMessage(*data));
            return 1;
        }
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

i32 DataCommand::_getEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    if (server == nullptr) {
        _sendError(source, "commands.data.entity.failed.noServer");
        return 0;
    }

    EntitySelector selector = context.getArgument<EntitySelector>("target");
    auto entities = support::EntityResolver::resolve(source, selector);

    if (entities.empty()) {
        _sendError(source, "commands.data.entity.failed.noEntity");
        return 0;
    }

    // 只处理第一个匹配的实体
    Entity* entity = entities.front();

    EntityDataAccessor accessor(entity);

    try {
        auto data = accessor.getData();

        // 检查是否有路径参数
        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");

            try {
                auto results = path.get(*data);

                if (results.empty()) {
                    _sendError(source, "commands.data.get.pathNotFound");
                    return 0;
                }

                if (results.size() == 1) {
                    const nbt::tags::tag* result = results[0];

                    // 检查是否有缩放参数
                    if (context.hasArgument("scale")) {
                        const f64 scale = context.getArgument<f64>("scale");
                        i32 scaledValue = _scaleValue(*result, scale);
                        source.sendMessage(accessor.getGetMessage(path, scale, scaledValue));
                        return scaledValue;
                    }

                    i32 resultValue = _getSingleResult(*result);
                    source.sendMessage(accessor.getQueryMessage(*result));
                    return resultValue;
                } else {
                    source.sendMessage("Found " + std::to_string(results.size()) + " matches");
                    return static_cast<i32>(results.size());
                }
            }
            catch (const CommandException& e) {
                _sendError(source, e.message());
                return 0;
            }
        } else {
            source.sendMessage(accessor.getQueryMessage(*data));
            return 1;
        }
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

i32 DataCommand::_getStorage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    if (server == nullptr) {
        _sendError(source, "commands.data.storage.failed.noServer");
        return 0;
    }

    ResourceLocation id = context.getArgument<ResourceLocation>("target");

    CommandStorage& storage = server->commandStorage();
    StorageDataAccessor accessor(&storage, id);

    try {
        auto data = accessor.getData();

        // 检查是否有路径参数
        if (context.hasArgument("path")) {
            NbtPath path = context.getArgument<NbtPath>("path");

            try {
                auto results = path.get(*data);

                if (results.empty()) {
                    _sendError(source, "commands.data.get.pathNotFound");
                    return 0;
                }

                if (results.size() == 1) {
                    const nbt::tags::tag* result = results[0];

                    if (context.hasArgument("scale")) {
                        const f64 scale = context.getArgument<f64>("scale");
                        i32 scaledValue = _scaleValue(*result, scale);
                        source.sendMessage(accessor.getGetMessage(path, scale, scaledValue));
                        return scaledValue;
                    }

                    i32 resultValue = _getSingleResult(*result);
                    source.sendMessage(accessor.getQueryMessage(*result));
                    return resultValue;
                } else {
                    source.sendMessage("Found " + std::to_string(results.size()) + " matches");
                    return static_cast<i32>(results.size());
                }
            }
            catch (const CommandException& e) {
                _sendError(source, e.message());
                return 0;
            }
        } else {
            source.sendMessage(accessor.getQueryMessage(*data));
            return 1;
        }
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

// ========== set 子命令实现 ==========

namespace {

/// 原版 NbtPathArgument 的节点字符串写法：字符串节点原样，其余按 SNBT 文本。
std::string stringifyNbtValue(const nbt::tags::tag& value)
{
    if (value.id() == nbt::TagId::String) {
        return static_cast<const nbt::tags::string_tag&>(value).value;
    }
    return std::to_string(value);
}

/// 原版 DataCommands.SourceDataAccessor 的「来源」：按路径文本读出一组值。
///
/// 每次调用都重新读取底层数据（原版也是每次都重新 resolve/读存储），因此来源与目标
/// 是同一个存储时也能拿到写入前的最新状态。
std::vector<std::unique_ptr<nbt::tags::tag>> readSourcePath(IDataAccessor& accessor, const NbtPath& path)
{
    auto data = accessor.getData();
    std::vector<std::unique_ptr<nbt::tags::tag>> values;
    for (const auto* tag : path.get(*data)) {
        values.push_back(tag->copy());
    }
    return values;
}

} // namespace

i32 DataCommand::_modify(CommandContext<ServerCommandSource>& context, i32 operation)
{
    auto& source = context.getSource();
    NbtPath targetPath = context.getArgument<NbtPath>("targetPath");

    // 原版：来源参数在解析时就被求值，后续对目标数据源的操作都以来源值为输入。
    std::vector<std::unique_ptr<nbt::tags::tag>> sourceValues;

    const bool stringify = context.hasParsed("stringValueArg");
    if (context.hasArgument("source")) {
        const std::any raw = context.getArgumentAny("source");
        if (raw.type() == typeid(std::shared_ptr<nbt::tags::tag>)) {
            // `value` / `from` / `string` 直接跟 NBT 值：单值。
            const auto value = std::any_cast<std::shared_ptr<nbt::tags::tag>>(raw);
            sourceValues.push_back(value->copy());
        } else {
            const NbtPath sourcePath = context.getArgument<NbtPath>("sourcePath");

            // `from block <pos> <path>`
            if (context.hasArgument("sourcePos")) {
                auto world = source.world();
                if (world == nullptr) {
                    _sendError(source, "commands.data.block.failed.noWorld");
                    return 0;
                }
                Vector3i pos = BlockPosArgumentType::getBlockPos(context, "sourcePos", source);
                BlockDataAccessor accessor(world, BlockPos(pos.x, pos.y, pos.z));
                if (!accessor.isValid()) {
                    _sendError(source, "commands.data.block.failed.noBlockEntity");
                    return 0;
                }
                sourceValues = readSourcePath(accessor, sourcePath);
            }
            // `from entity <source> <path>`
            else if (raw.type() == typeid(EntitySelector)) {
                auto entities = support::EntityResolver::resolve(source, std::any_cast<EntitySelector>(raw));
                if (entities.empty()) {
                    _sendError(source, "argument.entity.notfound.entity");
                    return 0;
                }
                Entity* sourceEntity = entities.front();
                EntityDataAccessor accessor(sourceEntity);
                sourceValues = readSourcePath(accessor, sourcePath);
            }
            // `from storage <source> <path>`
            else if (raw.type() == typeid(ResourceLocation)) {
                auto* server = source.server();
                if (server == nullptr) {
                    _sendError(source, "commands.data.storage.failed.noServer");
                    return 0;
                }
                StorageDataAccessor accessor(&server->commandStorage(), std::any_cast<ResourceLocation>(raw));
                sourceValues = readSourcePath(accessor, sourcePath);
            } else {
                _sendError(source, "commands.data.modify.invalidSource");
                return 0;
            }
        }
    }

    // `string ...`：原版把源值逐个字符串化再写入，可选的 start/end 截取子串。
    if (stringify) {
        const i32 start = context.hasArgument("start") ? context.getArgument<i32>("start") : 0;
        const i32 end = context.hasArgument("end") ? context.getArgument<i32>("end") : -1;
        for (auto& value : sourceValues) {
            std::string text = stringifyNbtValue(*value);
            if (start >= 0 && start < static_cast<i32>(text.size()) && (end < 0 || end > start) &&
                end <= static_cast<i32>(text.size())) {
                text = text.substr(static_cast<size_t>(start),
                    static_cast<size_t>(end < 0 ? text.size() : end) - static_cast<size_t>(start));
            }
            value = std::make_unique<nbt::tags::string_tag>(std::move(text));
        }
    }

    // 目标数据源：原版 modify 的三类目标共用同一条写入流程。
    std::unique_ptr<IDataAccessor> targetAccessor;
    bool targetValid = true;
    if (context.hasArgument("targetPos")) {
        auto world = source.world();
        if (world == nullptr) {
            _sendError(source, "commands.data.block.failed.noWorld");
            return 0;
        }
        Vector3i pos = BlockPosArgumentType::getBlockPos(context, "targetPos", source);
        auto accessor = std::make_unique<BlockDataAccessor>(world, BlockPos(pos.x, pos.y, pos.z));
        targetValid = accessor->isValid();
        targetAccessor = std::move(accessor);
    } else if (context.hasArgument("target")) {
        const std::any raw = context.getArgumentAny("target");
        if (raw.type() == typeid(ResourceLocation)) {
            auto* server = source.server();
            if (server == nullptr) {
                _sendError(source, "commands.data.storage.failed.noServer");
                return 0;
            }
            targetAccessor =
                std::make_unique<StorageDataAccessor>(&server->commandStorage(), std::any_cast<ResourceLocation>(raw));
        } else {
            auto entities = support::EntityResolver::resolve(source, std::any_cast<EntitySelector>(raw));
            if (entities.empty()) {
                _sendError(source, "commands.data.entity.failed.noEntity");
                return 0;
            }
            auto accessor = std::make_unique<EntityDataAccessor>(entities.front());
            targetValid = accessor->isValid();
            targetAccessor = std::move(accessor);
        }
    } else {
        _sendError(source, "commands.data.modify.invalidTarget");
        return 0;
    }

    if (!targetValid) {
        _sendError(source, "commands.data.block.failed.noBlockEntity");
        return 0;
    }

    try {
        auto data = targetAccessor->getData();
        i32 count = 0;
        switch (operation) {
            case 0: // set
            {
                const nbt::tags::tag* value = sourceValues.empty() ? nullptr : sourceValues.front().get();
                if (value == nullptr) {
                    _sendError(source, "commands.data.modify.missingValue");
                    return 0;
                }
                count = targetPath.set(*data, [value]() -> std::unique_ptr<nbt::tags::tag> {
                    return value->copy();
                });
                break;
            }
            case 1: // append
                count = targetPath.append(*data, sourceValues);
                break;
            case 2: // prepend
                count = targetPath.prepend(*data, sourceValues);
                break;
            case 3: // merge
                if (sourceValues.empty() || sourceValues.front()->id() != nbt::TagId::Compound) {
                    _sendError(source, "commands.data.modify.merge.expectedCompound");
                    return 0;
                }
                count =
                    targetPath.merge(*data, *static_cast<const nbt::tags::compound_tag*>(sourceValues.front().get()));
                break;
            case 4: // insert
            {
                const i32 index = context.getArgument<i32>("index");
                count = targetPath.insert(*data, index, sourceValues);
                break;
            }
            default:
                _sendError(source, "commands.data.modify.invalidOperation");
                return 0;
        }

        if (count == 0) {
            _sendError(source, "commands.data.modify.invalidPath");
            return 0;
        }

        targetAccessor->mergeData(*data);
        source.sendMessage(targetAccessor->getModifiedMessage());
        return count;
    }
    catch (const CommandException& e) {
        _sendCommandError(source, e);
        return 0;
    }
}

i32 DataCommand::_setBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();

    if (world == nullptr) {
        _sendError(source, "commands.data.block.failed.noWorld");
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "targetPos", source);
    NbtPath path = context.getArgument<NbtPath>("path");
    auto value = context.getArgument<std::shared_ptr<nbt::tags::tag>>("value");

    BlockDataAccessor accessor(world, BlockPos(pos.x, pos.y, pos.z));

    if (!accessor.isValid()) {
        _sendError(source, "commands.data.block.failed.noBlockEntity");
        return 0;
    }

    try {
        auto data = accessor.getData();

        // 设置值
        i32 count = path.set(*data, [&value]() {
            return value->copy();
        });

        if (count == 0) {
            _sendError(source, "commands.data.set.pathNotFound");
            return 0;
        }

        // 写回数据
        accessor.mergeData(*data);

        source.sendMessage(accessor.getModifiedMessage());
        return count;
    }
    catch (const CommandException& e) {
        _sendCommandError(source, e);
        return 0;
    }
}

i32 DataCommand::_setEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    if (server == nullptr) {
        _sendError(source, "commands.data.entity.failed.noServer");
        return 0;
    }

    EntitySelector selector = context.getArgument<EntitySelector>("target");
    NbtPath path = context.getArgument<NbtPath>("path");
    auto value = context.getArgument<std::shared_ptr<nbt::tags::tag>>("value");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        _sendError(source, "commands.data.entity.failed.noEntity");
        return 0;
    }

    Entity* entity = entities.front();

    EntityDataAccessor accessor(entity);

    if (accessor.isPlayer()) {
        _sendError(source, "commands.data.entity.failed.player");
        return 0;
    }

    try {
        auto data = accessor.getData();

        i32 count = path.set(*data, [&value]() {
            return value->copy();
        });

        if (count == 0) {
            _sendError(source, "commands.data.set.pathNotFound");
            return 0;
        }

        accessor.mergeData(*data);

        source.sendMessage(accessor.getModifiedMessage());
        return count;
    }
    catch (const CommandException& e) {
        _sendCommandError(source, e);
        return 0;
    }
}

i32 DataCommand::_setStorage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    ResourceLocation id = context.getArgument<ResourceLocation>("target");
    NbtPath path = context.getArgument<NbtPath>("path");
    auto value = context.getArgument<std::shared_ptr<nbt::tags::tag>>("value");

    auto* server = source.server();
    if (server == nullptr) {
        _sendError(source, "commands.data.storage.failed.noServer");
        return 0;
    }

    CommandStorage& storage = server->commandStorage();
    StorageDataAccessor accessor(&storage, id);

    try {
        auto data = accessor.getData();

        i32 count = path.set(*data, [&value]() {
            return value->copy();
        });

        if (count == 0) {
            _sendError(source, "commands.data.set.pathNotFound");
            return 0;
        }

        accessor.mergeData(*data);

        source.sendMessage(accessor.getModifiedMessage());
        return count;
    }
    catch (const CommandException& e) {
        _sendCommandError(source, e);
        return 0;
    }
}

// ========== merge 子命令实现 ==========

i32 DataCommand::_mergeBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();

    if (world == nullptr) {
        _sendError(source, "commands.data.block.failed.noWorld");
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "targetPos", source);
    auto nbt = context.getArgument<std::shared_ptr<nbt::tags::compound_tag>>("nbt");

    BlockDataAccessor accessor(world, BlockPos(pos.x, pos.y, pos.z));

    if (!accessor.isValid()) {
        _sendError(source, "commands.data.block.failed.noBlockEntity");
        return 0;
    }

    try {
        accessor.mergeData(*nbt);
        source.sendMessage(accessor.getModifiedMessage());
        return 1;
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

i32 DataCommand::_mergeEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    if (server == nullptr) {
        _sendError(source, "commands.data.entity.failed.noServer");
        return 0;
    }

    EntitySelector selector = context.getArgument<EntitySelector>("target");
    auto nbt = context.getArgument<std::shared_ptr<nbt::tags::compound_tag>>("nbt");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        _sendError(source, "commands.data.entity.failed.noEntity");
        return 0;
    }

    Entity* entity = entities.front();

    EntityDataAccessor accessor(entity);

    if (accessor.isPlayer()) {
        _sendError(source, "commands.data.entity.failed.player");
        return 0;
    }

    try {
        accessor.mergeData(*nbt);
        source.sendMessage(accessor.getModifiedMessage());
        return 1;
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

i32 DataCommand::_mergeStorage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    ResourceLocation id = context.getArgument<ResourceLocation>("target");
    auto nbt = context.getArgument<std::shared_ptr<nbt::tags::compound_tag>>("nbt");

    auto* server = source.server();
    if (server == nullptr) {
        _sendError(source, "commands.data.storage.failed.noServer");
        return 0;
    }

    CommandStorage& storage = server->commandStorage();
    StorageDataAccessor accessor(&storage, id);

    try {
        accessor.mergeData(*nbt);
        source.sendMessage(accessor.getModifiedMessage());
        return 1;
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

// ========== remove 子命令实现 ==========

i32 DataCommand::_removeBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();

    if (world == nullptr) {
        _sendError(source, "commands.data.block.failed.noWorld");
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "targetPos", source);
    NbtPath path = context.getArgument<NbtPath>("path");

    BlockDataAccessor accessor(world, BlockPos(pos.x, pos.y, pos.z));

    if (!accessor.isValid()) {
        _sendError(source, "commands.data.block.failed.noBlockEntity");
        return 0;
    }

    try {
        auto data = accessor.getData();

        i32 count = path.remove(*data);

        if (count == 0) {
            _sendError(source, "commands.data.remove.pathNotFound");
            return 0;
        }

        accessor.mergeData(*data);
        source.sendMessage(accessor.getModifiedMessage());
        return count;
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

i32 DataCommand::_removeEntity(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    if (server == nullptr) {
        _sendError(source, "commands.data.entity.failed.noServer");
        return 0;
    }

    EntitySelector selector = context.getArgument<EntitySelector>("target");
    NbtPath path = context.getArgument<NbtPath>("path");

    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        _sendError(source, "commands.data.entity.failed.noEntity");
        return 0;
    }

    Entity* entity = entities.front();

    EntityDataAccessor accessor(entity);

    if (accessor.isPlayer()) {
        _sendError(source, "commands.data.entity.failed.player");
        return 0;
    }

    try {
        auto data = accessor.getData();

        i32 count = path.remove(*data);

        if (count == 0) {
            _sendError(source, "commands.data.remove.pathNotFound");
            return 0;
        }

        accessor.mergeData(*data);
        source.sendMessage(accessor.getModifiedMessage());
        return count;
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

i32 DataCommand::_removeStorage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    ResourceLocation id = context.getArgument<ResourceLocation>("target");
    NbtPath path = context.getArgument<NbtPath>("path");

    auto* server = source.server();
    if (server == nullptr) {
        _sendError(source, "commands.data.storage.failed.noServer");
        return 0;
    }

    CommandStorage& storage = server->commandStorage();
    StorageDataAccessor accessor(&storage, id);

    try {
        auto data = accessor.getData();

        i32 count = path.remove(*data);

        if (count == 0) {
            _sendError(source, "commands.data.remove.pathNotFound");
            return 0;
        }

        accessor.mergeData(*data);
        source.sendMessage(accessor.getModifiedMessage());
        return count;
    }
    catch (const CommandException& e) {
        _sendError(source, e.message());
        return 0;
    }
}

// ========== 辅助函数实现 ==========

i32 DataCommand::_getSingleResult(const nbt::tags::tag& tag)
{
    switch (tag.id()) {
        case nbt::TagId::Byte:
            return dynamic_cast<const nbt::tags::byte_tag&>(tag).value;
        case nbt::TagId::Short:
            return dynamic_cast<const nbt::tags::short_tag&>(tag).value;
        case nbt::TagId::Int:
            return dynamic_cast<const nbt::tags::int_tag&>(tag).value;
        case nbt::TagId::Long:
            return static_cast<i32>(dynamic_cast<const nbt::tags::long_tag&>(tag).value);
        case nbt::TagId::Float:
            return static_cast<i32>(dynamic_cast<const nbt::tags::float_tag&>(tag).value);
        case nbt::TagId::Double:
            return static_cast<i32>(dynamic_cast<const nbt::tags::double_tag&>(tag).value);
        case nbt::TagId::ByteArray:
            return static_cast<i32>(dynamic_cast<const nbt::tags::bytearray_tag&>(tag).value.size());
        case nbt::TagId::String:
            return static_cast<i32>(dynamic_cast<const nbt::tags::string_tag&>(tag).value.size());
        case nbt::TagId::List:
            return static_cast<i32>(dynamic_cast<const nbt::tags::list_tag&>(tag).size());
        case nbt::TagId::Compound:
            return static_cast<i32>(dynamic_cast<const nbt::tags::compound_tag&>(tag).value.size());
        case nbt::TagId::IntArray:
            return static_cast<i32>(dynamic_cast<const nbt::tags::intarray_tag&>(tag).value.size());
        case nbt::TagId::LongArray:
            return static_cast<i32>(dynamic_cast<const nbt::tags::longarray_tag&>(tag).value.size());
        default:
            return 0;
    }
}

i32 DataCommand::_scaleValue(const nbt::tags::tag& tag, f64 scale)
{
    f64 value = 0.0;

    switch (tag.id()) {
        case nbt::TagId::Byte:
            value = dynamic_cast<const nbt::tags::byte_tag&>(tag).value;
            break;
        case nbt::TagId::Short:
            value = dynamic_cast<const nbt::tags::short_tag&>(tag).value;
            break;
        case nbt::TagId::Int:
            value = dynamic_cast<const nbt::tags::int_tag&>(tag).value;
            break;
        case nbt::TagId::Long:
            value = static_cast<f64>(dynamic_cast<const nbt::tags::long_tag&>(tag).value);
            break;
        case nbt::TagId::Float:
            value = dynamic_cast<const nbt::tags::float_tag&>(tag).value;
            break;
        case nbt::TagId::Double:
            value = dynamic_cast<const nbt::tags::double_tag&>(tag).value;
            break;
        default:
            return _getSingleResult(tag);
    }

    return static_cast<i32>(std::floor(value * scale));
}

void DataCommand::_sendError(ServerCommandSource& source, const std::string& message)
{
    source.sendMessage("§c" + message);
}

void DataCommand::_sendCommandError(ServerCommandSource& source, const CommandException& error)
{
    // 数据过深是写入前的整体拒绝，与"路径不存在"不同，需要给出明确的错误提示
    if (error.type() == CommandErrorType::NbtPathDataTooDeep) {
        _sendError(source, "commands.data.set.tooDeep");
        return;
    }
    _sendError(source, error.message());
}

} // namespace command
} // namespace mc
