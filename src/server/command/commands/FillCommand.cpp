#include "FillCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/WorldConstants.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/world/ServerWorld.hpp"

#include <sstream>
#include <algorithm>

namespace mc {
namespace command {

namespace {

/**
 * @brief 方块填充模式
 */
enum class FillMode {
    Replace,    // 替换所有方块
    Destroy,    // 破坏原有方块并掉落物品
    Hollow,     // 空心填充（仅外壳，内部填充空气）
    Keep,       // 仅替换空气
    Outline     // 轮廓填充（仅外壳，内部保持不变）
};

/**
 * @brief 解析方块ID
 */
Block* parseBlockId(const String& input) {
    auto& registry = BlockRegistry::instance();

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
 */
const BlockState* resolveBlockState(const String& blockId) {
    Block* block = parseBlockId(blockId);
    if (block == nullptr) {
        return nullptr;
    }
    return &block->defaultState();
}

/**
 * @brief 计算填充区域的方块数量
 */
i32 calculateBlockCount(const Vector3i& from, const Vector3i& to) {
    i32 minX = std::min(from.x, to.x);
    i32 maxX = std::max(from.x, to.x);
    i32 minY = std::min(from.y, to.y);
    i32 maxY = std::max(from.y, to.y);
    i32 minZ = std::min(from.z, to.z);
    i32 maxZ = std::max(from.z, to.z);

    return (maxX - minX + 1) * (maxY - minY + 1) * (maxZ - minZ + 1);
}

/**
 * @brief 执行填充操作
 */
i32 executeFill(CommandContext<ServerCommandSource>& context, FillMode mode, const String& filterBlock = "") {
    auto& source = context.getSource();
    Vector3i from = context.getArgument<Vector3i>("from");
    Vector3i to = context.getArgument<Vector3i>("to");
    String blockInput = context.getArgument<String>("block");

    // 获取世界
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("commands.fill.failed.noWorld");
        return 0;
    }

    // 检查方块数量限制
    i32 blockCount = calculateBlockCount(from, to);
    if (blockCount > 32768) {
        source.sendError("commands.fill.failed.tooManyBlocks");
        return 0;
    }

    // 获取填充方块
    const BlockState* fillState = resolveBlockState(blockInput);
    if (fillState == nullptr) {
        std::ostringstream ss;
        ss << "commands.fill.failed.invalidBlock: " << blockInput;
        source.sendMessage(ss.str());
        return 0;
    }

    // 获取过滤方块（replace模式）
    const BlockState* filterState = nullptr;
    if (!filterBlock.empty() && mode == FillMode::Replace) {
        filterState = resolveBlockState(filterBlock);
        if (filterState == nullptr) {
            std::ostringstream ss;
            ss << "commands.fill.failed.invalidBlock: " << filterBlock;
            source.sendMessage(ss.str());
            return 0;
        }
    }

    // 计算边界
    i32 minX = std::min(from.x, to.x);
    i32 maxX = std::max(from.x, to.x);
    i32 minY = std::min(from.y, to.y);
    i32 maxY = std::max(from.y, to.y);
    i32 minZ = std::min(from.z, to.z);
    i32 maxZ = std::max(from.z, to.z);

    i32 blocksModified = 0;

    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 z = minZ; z <= maxZ; ++z) {
            for (i32 x = minX; x <= maxX; ++x) {
                // 检查是否在有效高度范围内
                if (y < world::MIN_BUILD_HEIGHT || y >= world::MAX_BUILD_HEIGHT) {
                    continue;
                }

                // 判断当前位置是否需要填充
                bool shouldFill = false;
                bool isShell = false;

                switch (mode) {
                    case FillMode::Replace:
                        if (filterState == nullptr) {
                            shouldFill = true;
                        } else {
                            const BlockState* currentState = world->getBlockState(x, y, z);
                            if (currentState != nullptr && currentState->getBlock().blockId() == filterState->getBlock().blockId()) {
                                shouldFill = true;
                            }
                        }
                        break;

                    case FillMode::Destroy:
                        // TODO: 破坏模式需要掉落物品
                        shouldFill = true;
                        break;

                    case FillMode::Hollow:
                        // 空心填充：仅外壳填充，内部填充空气
                        isShell = (x == minX || x == maxX || y == minY || y == maxY || z == minZ || z == maxZ);
                        if (isShell) {
                            shouldFill = true;
                        } else {
                            // 内部填充空气
                            const BlockState* airState = BlockRegistry::instance().getBlockState(0);
                            if (airState != nullptr) {
                                const BlockState* currentState = world->getBlockState(x, y, z);
                                if (currentState == nullptr || !currentState->isAir()) {
                                    world->setBlockState(x, y, z, airState);
                                    blocksModified++;
                                }
                            }
                        }
                        break;

                    case FillMode::Keep:
                        // 仅替换空气
                        {
                            const BlockState* currentState = world->getBlockState(x, y, z);
                            if (currentState == nullptr || currentState->isAir()) {
                                shouldFill = true;
                            }
                        }
                        break;

                    case FillMode::Outline:
                        // 轮廓填充：仅外壳填充，内部保持不变
                        isShell = (x == minX || x == maxX || y == minY || y == maxY || z == minZ || z == maxZ);
                        shouldFill = isShell;
                        break;
                }

                if (shouldFill) {
                    if (world->setBlockState(x, y, z, fillState)) {
                        blocksModified++;
                    }
                }
            }
        }
    }

    // 发送反馈
    std::ostringstream ss;
    ss << "Filled " << blocksModified << " blocks";
    source.sendMessage(ss.str());

    return blocksModified;
}

} // namespace

void FillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto fillNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("fill");
    fillNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        fillNode,
        support::makeMetadata(
            "Fills all or parts of a region with a specific block.",
            "/fill <from> <to> <block> [destroy|hollow|keep|outline|replace]",
            2,
            {},
            false));

    // /fill <from> <to> <block>
    auto fromArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3i>>(
        "from",
        BlockPosArgumentType::blockPos()
    );

    auto toArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3i>>(
        "to",
        BlockPosArgumentType::blockPos()
    );

    auto blockArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "block",
        StringArgumentType::string()
    );
    blockArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return fill(ctx);
    });

    // /fill <from> <to> <block> destroy
    auto destroyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("destroy");
    destroyNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return fillDestroy(ctx);
    });

    // /fill <from> <to> <block> hollow
    auto hollowNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("hollow");
    hollowNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return fillHollow(ctx);
    });

    // /fill <from> <to> <block> keep
    auto keepNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("keep");
    keepNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return fillKeep(ctx);
    });

    // /fill <from> <to> <block> outline
    auto outlineNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("outline");
    outlineNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return fillOutline(ctx);
    });

    // /fill <from> <to> <block> replace
    auto replaceNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replace");
    replaceNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return fillReplace(ctx);
    });

    // /fill <from> <to> <block> replace <filter>
    auto filterArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "filter",
        StringArgumentType::string()
    );
    filterArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        auto& source = ctx.getSource();
        Vector3i from = ctx.getArgument<Vector3i>("from");
        Vector3i to = ctx.getArgument<Vector3i>("to");
        String blockInput = ctx.getArgument<String>("block");
        String filterInput = ctx.getArgument<String>("filter");

        server::ServerWorld* world = source.world();
        if (world == nullptr) {
            source.sendError("commands.fill.failed.noWorld");
            return 0;
        }

        i32 blockCount = calculateBlockCount(from, to);
        if (blockCount > 32768) {
            source.sendError("commands.fill.failed.tooManyBlocks");
            return 0;
        }

        const BlockState* fillState = resolveBlockState(blockInput);
        if (fillState == nullptr) {
            std::ostringstream ss;
            ss << "commands.fill.failed.invalidBlock: " << blockInput;
            source.sendMessage(ss.str());
            return 0;
        }

        return executeFill(ctx, FillMode::Replace, filterInput);
    });

    replaceNode->addChild(filterArg);
    blockArg->addChild(destroyNode);
    blockArg->addChild(hollowNode);
    blockArg->addChild(keepNode);
    blockArg->addChild(outlineNode);
    blockArg->addChild(replaceNode);
    toArg->addChild(blockArg);
    fromArg->addChild(toArg);
    fillNode->addChild(fromArg);

    dispatcher.registerCommand(fillNode);
}

i32 FillCommand::fill(CommandContext<ServerCommandSource>& context) {
    return executeFill(context, FillMode::Replace);
}

i32 FillCommand::fillDestroy(CommandContext<ServerCommandSource>& context) {
    return executeFill(context, FillMode::Destroy);
}

i32 FillCommand::fillHollow(CommandContext<ServerCommandSource>& context) {
    return executeFill(context, FillMode::Hollow);
}

i32 FillCommand::fillKeep(CommandContext<ServerCommandSource>& context) {
    return executeFill(context, FillMode::Keep);
}

i32 FillCommand::fillOutline(CommandContext<ServerCommandSource>& context) {
    return executeFill(context, FillMode::Outline);
}

i32 FillCommand::fillReplace(CommandContext<ServerCommandSource>& context) {
    return executeFill(context, FillMode::Replace);
}

} // namespace command
} // namespace mc
