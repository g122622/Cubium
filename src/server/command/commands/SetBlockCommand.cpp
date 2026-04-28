#include "SetBlockCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"

#include <sstream>

namespace mc {
namespace command {

namespace {

/**
 * @brief 解析方块ID
 *
 * 格式：minecraft:stone 或 stone
 * TODO: 支持方块状态属性 [facing=north,half=bottom]
 *
 * @param input 输入字符串
 * @return 方块指针，失败返回nullptr
 */
Block* parseBlockId(const String& input) {
    auto& registry = BlockRegistry::instance();

    // 解析资源位置
    String namespace_;
    String path;
    size_t colonPos = input.find(':');
    if (colonPos != String::npos) {
        namespace_ = input.substr(0, colonPos);
        path = input.substr(colonPos + 1);
    } else {
        namespace_ = "minecraft";
        path = input;
    }

    // 去除状态属性部分
    size_t bracketPos = path.find('[');
    if (bracketPos != String::npos) {
        path = path.substr(0, bracketPos);
    }

    ResourceLocation location(namespace_, path);
    return registry.getBlock(location);
}

/**
 * @brief 获取方块的默认状态
 *
 * @param blockId 方块ID字符串
 * @return 方块状态指针，失败返回nullptr
 */
const BlockState* resolveBlockState(const String& blockId) {
    Block* block = parseBlockId(blockId);
    if (block == nullptr) {
        return nullptr;
    }
    return &block->defaultState();
}

/**
 * @brief 执行 setblock 命令
 */
i32 executeSetBlock(CommandContext<ServerCommandSource>& context, bool onlyIfAir, bool doDrop) {
    auto& source = context.getSource();
    Vector3i position = context.getArgument<Vector3i>("pos");
    String blockInput = context.getArgument<String>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendMessage("commands.setblock.failed.noWorld");
        return 0;
    }

    // 获取方块状态
    const BlockState* state = resolveBlockState(blockInput);
    if (state == nullptr) {
        std::ostringstream ss;
        ss << "commands.setblock.failed.invalidBlock: " << blockInput;
        source.sendMessage(ss.str());
        return 0;
    }

    // keep模式：仅当目标位置为空气时放置
    if (onlyIfAir) {
        const BlockState* existingBlock = world->getBlockState(position.x, position.y, position.z);
        if (existingBlock != nullptr && !existingBlock->isAir()) {
            source.sendMessage("commands.setblock.failed.alreadyExists");
            return 0;
        }
    }

    // TODO: destroy模式需要先破坏原有方块并掉落物品

    // 放置方块
    bool success = world->setBlock(position.x, position.y, position.z, state);
    if (!success) {
        source.sendMessage("commands.setblock.failed");
        return 0;
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Block placed at " << position.x << ", " << position.y << ", " << position.z;
    source.sendMessage(ss.str());

    return 1;
}

} // namespace

void SetBlockCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto setblockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setblock");
    setblockNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        setblockNode,
        support::makeMetadata(
            "Changes a block to another block.",
            "/setblock <pos> <block> [destroy|keep|replace]",
            2,
            {},
            false));

    // /setblock <pos> <block> - 默认replace模式
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3i>>(
        "pos",
        BlockPosArgumentType::blockPos()
    );

    auto blockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "block",
        StringArgumentType::string()
    );
    blockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBlock(ctx);
    });

    // /setblock <pos> <block> destroy
    auto destroyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("destroy");
    destroyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBlockDestroy(ctx);
    });

    // /setblock <pos> <block> keep
    auto keepNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("keep");
    keepNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBlockKeep(ctx);
    });

    // /setblock <pos> <block> replace
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
    replaceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBlockReplace(ctx);
    });

    blockArg->addChild(destroyNode);
    blockArg->addChild(keepNode);
    blockArg->addChild(replaceNode);
    posArg->addChild(blockArg);
    setblockNode->addChild(posArg);

    dispatcher.registerCommand(setblockNode);
}

i32 SetBlockCommand::setBlock(CommandContext<ServerCommandSource>& context) {
    // 默认replace模式
    return executeSetBlock(context, false, false);
}

i32 SetBlockCommand::setBlockDestroy(CommandContext<ServerCommandSource>& context) {
    // destroy模式：先破坏再放置
    return executeSetBlock(context, false, true);
}

i32 SetBlockCommand::setBlockKeep(CommandContext<ServerCommandSource>& context) {
    // keep模式：仅当目标位置为空气时放置
    return executeSetBlock(context, true, false);
}

i32 SetBlockCommand::setBlockReplace(CommandContext<ServerCommandSource>& context) {
    // replace模式：直接替换
    return executeSetBlock(context, false, false);
}

} // namespace command
} // namespace mc
