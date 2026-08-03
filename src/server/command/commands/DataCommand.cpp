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
#include "server/command/support/EntityResolver.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <memory>
#include <string>

// Bring operator<< for nbt::tags::tag into scope for ADL
using mc::nbt::operator<<;

namespace mc {
namespace command {

// ========== 注册命令 ==========

void DataCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 创建主命令节点
    auto dataNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("data");
    dataNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
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
        "pos", BlockPosArgumentType::blockPos());
    auto getBlockPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto getBlockScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("scale", FloatArgumentType::floatArg());

    getBlockScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getBlock(ctx); });
    getBlockPathArg->addChild(getBlockScaleArg);
    getBlockPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getBlock(ctx); });
    getBlockPosArg->addChild(getBlockPathArg);
    getBlockPosArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getBlock(ctx); });
    getBlockNode->addChild(getBlockPosArg);
    getNode->addChild(getBlockNode);

    // /data get entity <target> [<path>] [<scale>]
    auto getEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto getEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto getEntityPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto getEntityScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("scale", FloatArgumentType::floatArg());

    getEntityScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getEntity(ctx); });
    getEntityPathArg->addChild(getEntityScaleArg);
    getEntityPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getEntity(ctx); });
    getEntityTargetArg->addChild(getEntityPathArg);
    getEntityTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getEntity(ctx); });
    getEntityNode->addChild(getEntityTargetArg);
    getNode->addChild(getEntityNode);

    // /data get storage <id> [<path>] [<scale>]
    auto getStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto getStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", ResourceLocationArgumentType::resourceLocation());
    auto getStoragePathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto getStorageScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("scale", FloatArgumentType::floatArg());

    getStorageScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getStorage(ctx); });
    getStoragePathArg->addChild(getStorageScaleArg);
    getStoragePathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getStorage(ctx); });
    getStorageIdArg->addChild(getStoragePathArg);
    getStorageIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getStorage(ctx); });
    getStorageNode->addChild(getStorageIdArg);
    getNode->addChild(getStorageNode);

    dataNode->addChild(getNode);

    // ========== /data set 子命令 ==========
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");

    // /data set block <pos> <path> <value>
    auto setBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto setBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto setBlockPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto setBlockValueArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::tag>>>(
        "value", NbtTagArgumentType::nbtTag());

    setBlockValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setBlock(ctx); });
    setBlockPathArg->addChild(setBlockValueArg);
    setBlockPosArg->addChild(setBlockPathArg);
    setBlockNode->addChild(setBlockPosArg);
    setNode->addChild(setBlockNode);

    // /data set entity <target> <path> <value>
    auto setEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto setEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto setEntityPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto setEntityValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::tag>>>(
            "value", NbtTagArgumentType::nbtTag());

    setEntityValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setEntity(ctx); });
    setEntityPathArg->addChild(setEntityValueArg);
    setEntityTargetArg->addChild(setEntityPathArg);
    setEntityNode->addChild(setEntityTargetArg);
    setNode->addChild(setEntityNode);

    // /data set storage <id> <path> <value>
    auto setStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto setStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", ResourceLocationArgumentType::resourceLocation());
    auto setStoragePathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());
    auto setStorageValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::tag>>>(
            "value", NbtTagArgumentType::nbtTag());

    setStorageValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setStorage(ctx); });
    setStoragePathArg->addChild(setStorageValueArg);
    setStorageIdArg->addChild(setStoragePathArg);
    setStorageNode->addChild(setStorageIdArg);
    setNode->addChild(setStorageNode);

    dataNode->addChild(setNode);

    // ========== /data merge 子命令 ==========
    auto mergeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("merge");

    // /data merge block <pos> <nbt>
    auto mergeBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto mergeBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto mergeBlockNbtArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::compound_tag>>>(
            "nbt", NbtCompoundArgumentType::nbtCompound());

    mergeBlockNbtArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _mergeBlock(ctx); });
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

    mergeEntityNbtArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _mergeEntity(ctx); });
    mergeEntityTargetArg->addChild(mergeEntityNbtArg);
    mergeEntityNode->addChild(mergeEntityTargetArg);
    mergeNode->addChild(mergeEntityNode);

    // /data merge storage <id> <nbt>
    auto mergeStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto mergeStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", ResourceLocationArgumentType::resourceLocation());
    auto mergeStorageNbtArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::shared_ptr<nbt::tags::compound_tag>>>(
            "nbt", NbtCompoundArgumentType::nbtCompound());

    mergeStorageNbtArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _mergeStorage(ctx); });
    mergeStorageIdArg->addChild(mergeStorageNbtArg);
    mergeStorageNode->addChild(mergeStorageIdArg);
    mergeNode->addChild(mergeStorageNode);

    dataNode->addChild(mergeNode);

    // ========== /data remove 子命令 ==========
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");

    // /data remove block <pos> <path>
    auto removeBlockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto removeBlockPosArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto removeBlockPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());

    removeBlockPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeBlock(ctx); });
    removeBlockPosArg->addChild(removeBlockPathArg);
    removeBlockNode->addChild(removeBlockPosArg);
    removeNode->addChild(removeBlockNode);

    // /data remove entity <target> <path>
    auto removeEntityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto removeEntityTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto removeEntityPathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());

    removeEntityPathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeEntity(ctx); });
    removeEntityTargetArg->addChild(removeEntityPathArg);
    removeEntityNode->addChild(removeEntityTargetArg);
    removeNode->addChild(removeEntityNode);

    // /data remove storage <id> <path>
    auto removeStorageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("storage");
    auto removeStorageIdArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "id", ResourceLocationArgumentType::resourceLocation());
    auto removeStoragePathArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, NbtPath>>("path", NbtPathArgumentType::nbtPath());

    removeStoragePathArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeStorage(ctx); });
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

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "pos", source);
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
                        f32 scale = context.getArgument<f32>("scale");
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
                        f32 scale = context.getArgument<f32>("scale");
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

    ResourceLocation id = context.getArgument<ResourceLocation>("id");

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
                        f32 scale = context.getArgument<f32>("scale");
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

i32 DataCommand::_setBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();

    if (world == nullptr) {
        _sendError(source, "commands.data.block.failed.noWorld");
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "pos", source);
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
        i32 count = path.set(*data, [&value]() { return value->copy(); });

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
        _sendError(source, e.message());
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

        i32 count = path.set(*data, [&value]() { return value->copy(); });

        if (count == 0) {
            _sendError(source, "commands.data.set.pathNotFound");
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

i32 DataCommand::_setStorage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    ResourceLocation id = context.getArgument<ResourceLocation>("id");
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

        i32 count = path.set(*data, [&value]() { return value->copy(); });

        if (count == 0) {
            _sendError(source, "commands.data.set.pathNotFound");
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

// ========== merge 子命令实现 ==========

i32 DataCommand::_mergeBlock(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto world = source.world();

    if (world == nullptr) {
        _sendError(source, "commands.data.block.failed.noWorld");
        return 0;
    }

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "pos", source);
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

    ResourceLocation id = context.getArgument<ResourceLocation>("id");
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

    Vector3i pos = BlockPosArgumentType::getBlockPos(context, "pos", source);
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

    ResourceLocation id = context.getArgument<ResourceLocation>("id");
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

i32 DataCommand::_scaleValue(const nbt::tags::tag& tag, double scale)
{
    double value = 0.0;

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
            value = static_cast<double>(dynamic_cast<const nbt::tags::long_tag&>(tag).value);
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

} // namespace command
} // namespace mc
