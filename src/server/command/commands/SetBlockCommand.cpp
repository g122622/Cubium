#include "SetBlockCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/entity/loot/LootTable.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/drop/BlockDropHandler.hpp"

#include <sstream>

namespace mc {
namespace command {

namespace {

/**
 * @brief 执行 setblock 命令
 *
 * @param context 命令上下文
 * @param onlyIfAir keep模式：仅当目标位置为空气时放置
 * @param doDrop destroy模式：破坏原有方块并掉落物品
 * @return 成功放置返回1，失败返回0
 */
i32 executeSetBlock(CommandContext<ServerCommandSource>& context, bool onlyIfAir, bool doDrop) {
    auto& source = context.getSource();
    Vector3i position = context.getArgument<Vector3i>("pos");
    BlockStateInput blockInput = context.getArgument<BlockStateInput>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.setblock.failed.noWorld");
        return 0;
    }

    // 获取方块状态
    const BlockState* state = blockInput.state();
    if (state == nullptr) {
        source.sendError("commands.setblock.failed.invalidBlock");
        return 0;
    }

    // 将位置转换为 BlockPos
    BlockPos pos(position.x, position.y, position.z);

    // keep模式：仅当目标位置为空气时放置
    if (onlyIfAir) {
        const BlockState* existingBlock = world->getBlockState(position.x, position.y, position.z);
        if (existingBlock != nullptr && !existingBlock->isAir()) {
            source.sendError("commands.setblock.failed.alreadyExists");
            return 0;
        }
    }

    // destroy模式：先破坏原有方块并掉落物品
    if (doDrop) {
        // 获取原有方块状态
        const BlockState* oldState = world->getBlockState(position.x, position.y, position.z);
        if (oldState != nullptr && !oldState->isAir()) {
            // 播放方块破坏效果（粒子 + 音效）
            // eventID 2001，data 为方块状态ID
            world->playEvent(world::WorldEvents::BREAK_BLOCK_EFFECTS, pos, static_cast<i32>(oldState->stateId()));

            // 获取掉落表管理器
            const loot::LootTableManager* lootTableManager = world->lootTableManager();

            // 生成掉落物
            if (lootTableManager != nullptr) {
                // 使用基于位置的随机种子
                math::Random rng(static_cast<u64>(world->seed() ^ static_cast<u64>(position.x ^ position.z)));

                // 生成掉落物列表
                // 注意：destroy 模式不使用工具，所以 tool = nullptr
                std::vector<ItemStack> drops = BlockDropHandler::generateDrops(
                    *world, pos, *oldState, nullptr, nullptr, *lootTableManager);

                // 生成物品实体
                if (!drops.empty()) {
                    BlockDropHandler::spawnDrops(*world, pos, drops, "");
                }

                // 处理经验掉落（矿石）
                BlockDropHandler::handleBlockBreakExperience(*world, pos, *oldState, nullptr, rng);
            }
        }
    }

    // 放置方块
    bool success = world->setBlockState(position.x, position.y, position.z, state);
    if (!success) {
        source.sendError("commands.setblock.failed");
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

    auto blockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, BlockStateInput>>(
        "block",
        BlockStateArgumentType::blockState()
    );
    blockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setBlockState(ctx);
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

i32 SetBlockCommand::setBlockState(CommandContext<ServerCommandSource>& context) {
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
