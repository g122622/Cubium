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

#include "ExecuteModifierNode.hpp"
#include "ExecuteRunNode.hpp"

#include "SummonCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/command/arguments/DimensionArgument.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/FunctionArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/command/arguments/NbtPathArgumentType.hpp"
#include "common/command/arguments/ResourceOrTagArgumentType.hpp"
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
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/command/support/EntityResolver.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {

namespace {

/**
 * @brief /execute 的条件节点（if / unless）
 *
 * 继承 ExecuteModifierNode 而不只是 LiteralCommandNode：条件必须像其它修改器那样参与
 * 链式派发——只有把自己的索引写进 commandModifier，ExecuteRunNode 才会把它拼进链段，
 * 执行期 _executeForwarded 才看得到 "if"/"unless" 这一段。做成纯字面量节点的话，
 * 条件链会一路派发到 run 而不经过链，命令照常执行、条件被完全忽略。
 *
 * 条件参数子树由 buildConditionNodes() 逐条对齐原版 ExecuteCommand.Conditions.build
 * 建出（客户端按这棵树补全与高亮）。
 */
class _ExecuteConditionNode final : public ExecuteModifierNode {
public:
    _ExecuteConditionNode(const char* name, i32 index)
        : ExecuteModifierNode(name, index)
    {}

    void parse(StringReader& reader, CommandContext<ServerCommandSource>& context) const override
    {
        ExecuteModifierNode::parse(reader, context);
        context.markParsed("executeCondition");
    }
};

/// 原版 /execute 的条件参数（storage 的 id、function 的 name、score 的持有者、summon 的实体、
/// store 的 bossbar/score/storage 目标）在注册时由命令自己挂了 suggestResource / suggestScoreHolders
/// 一类的提供器，命令树里带建议提供器位；这里给参数节点挂上自定义提供器以对齐。
void attachCommandSuggestions(const std::shared_ptr<CommandNode<ServerCommandSource>>& node)
{
    node->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
}

} // namespace

namespace {

/**
 * @brief 给修改器的每个末端参数（含其下挂的字面量）接上 run 节点
 *
 * 原版把「下一个修改器 / run」挂在参数节点下；Cubium 的修改器字面量之间不再互连，
 * 但参数之后必须能继续（`as @p run`、`as @p at @s run`），所以参数节点统一挂上 run。
 */
void attachRunToLeaves(const std::shared_ptr<CommandNode<ServerCommandSource>>& node,
    const std::shared_ptr<CommandNode<ServerCommandSource>>& runLink)
{
    for (const auto& [name, child] : node->getChildren()) {
        (void)name;
        if (!child) {
            continue;
        }
        // 跳过 run 链接自身：它刚被挂到某个参数节点下，继续沿它递归会经
        // run → command 绕回、无限下探（实测触发 STACK_OVERFLOW）。
        if (child == runLink) {
            continue;
        }
        if (child->getType() == NodeType::Argument) {
            child->addChild(runLink);
            // 不能在这里 return：参数节点自身也可能是**多参数链的中间层**
            // （`if block <pos> <block>`：pos 下还挂着 block 参数）。只给 pos 挂 run
            // 的话，输入读完 pos 之后 dispatcher 在 pos 下找不到能吃掉方块谓词的分支，
            // 于是报 Unknown argument。继续下探，让链上每个参数都能直接接 run。
        }
        attachRunToLeaves(child, runLink);
    }
}

/// 条件子树里的参数节点：名字 + 类型（值类型由参数类型自己决定）。
///
/// 原版只有「条件到此完整」的参数才挂命令（`if block <pos> <block>`、`if data ... <path>`），
/// 中间的参数一律不可执行；可执行的参数到执行链上再判定条件是否成立。
template <typename Type>
std::shared_ptr<CommandNode<ServerCommandSource>> conditionArg(
    const char* name, std::shared_ptr<Type> type, bool executable)
{
    auto node = std::make_shared<ArgumentCommandNode<ServerCommandSource, typename Type::Value>>(name, std::move(type));
    if (executable) {
        node->setCommand([](CommandContext<ServerCommandSource>&) -> i32 {
            throw CommandException(CommandErrorType::Unknown, "commands.execute.conditional");
        });
    }
    return node;
}

/// 字面量条件节点（`if loaded <pos>` 之类）。
std::shared_ptr<CommandNode<ServerCommandSource>> conditionLiteral(const char* name)
{
    return std::make_shared<LiteralCommandNode<ServerCommandSource>>(name);
}

/**
 * @brief 构建 if / unless 的条件子树（对齐原版 ExecuteCommand.Conditions.build）
 *
 * 条件成立与否在执行链上判定。
 * TODO: `/execute if` 的判定语义尚未实现，执行链目前把这类分支视为条件不成立。
 */
std::vector<std::pair<std::string, std::shared_ptr<CommandNode<ServerCommandSource>>>> buildConditionNodes()
{
    using Node = std::shared_ptr<CommandNode<ServerCommandSource>>;
    std::vector<std::pair<std::string, Node>> conditions;

    // if|unless biome <pos> <biome>
    {
        auto biomeNode = conditionLiteral("biome");
        auto pos = conditionArg("pos", BlockPosArgumentType::blockPos(), false);
        pos->addChild(
            conditionArg("biome", ResourceOrTagArgumentType::resourceOrTag("minecraft:worldgen/biome"), true));
        biomeNode->addChild(pos);
        conditions.emplace_back("biome", biomeNode);
    }

    // if|unless block <pos> <block>
    {
        auto blockNode = conditionLiteral("block");
        auto pos = conditionArg("pos", BlockPosArgumentType::blockPos(), false);
        pos->addChild(conditionArg("block", BlockPredicateArgumentType::blockPredicate(), true));
        blockNode->addChild(pos);
        conditions.emplace_back("block", blockNode);
    }

    // if|unless blocks <start> <end> <destination> [all|masked]
    {
        auto blocksNode = conditionLiteral("blocks");
        auto from = conditionArg("start", BlockPosArgumentType::blockPos(), false);
        auto to = conditionArg("end", BlockPosArgumentType::blockPos(), false);
        auto destination = conditionArg("destination", BlockPosArgumentType::blockPos(), false);
        for (const char* mode : {"all", "masked"}) {
            auto modeNode = conditionLiteral(mode);
            modeNode->setCommand([](CommandContext<ServerCommandSource>&) -> i32 {
                throw CommandException(CommandErrorType::Unknown, "commands.execute.conditional");
            });
            destination->addChild(modeNode);
        }
        to->addChild(destination);
        from->addChild(to);
        blocksNode->addChild(from);
        conditions.emplace_back("blocks", blocksNode);
    }

    // if|unless data <block <sourcePos>|entity <source>|storage <source>> <path>
    {
        auto dataNode = conditionLiteral("data");

        auto blockNode = conditionLiteral("block");
        auto sourcePos = conditionArg("sourcePos", BlockPosArgumentType::blockPos(), false);
        sourcePos->addChild(conditionArg("path", NbtPathArgumentType::nbtPath(), true));
        blockNode->addChild(sourcePos);

        auto entityNode = conditionLiteral("entity");
        auto source = conditionArg("source", EntityArgumentType::entity(), false);
        source->addChild(conditionArg("path", NbtPathArgumentType::nbtPath(), true));
        entityNode->addChild(source);

        auto storageNode = conditionLiteral("storage");
        auto storageId = conditionArg("source", ResourceLocationArgumentType::resourceLocation(), false);
        attachCommandSuggestions(storageId);
        storageId->addChild(conditionArg("path", NbtPathArgumentType::nbtPath(), true));
        storageNode->addChild(storageId);

        dataNode->addChild(blockNode);
        dataNode->addChild(entityNode);
        dataNode->addChild(storageNode);
        conditions.emplace_back("data", dataNode);
    }

    // if|unless dimension <dimension>
    {
        auto dimensionNode = conditionLiteral("dimension");
        dimensionNode->addChild(conditionArg("dimension", DimensionArgumentType::dimension(), true));
        conditions.emplace_back("dimension", dimensionNode);
    }

    // if|unless entity <entities>
    {
        auto entityNode = conditionLiteral("entity");
        entityNode->addChild(conditionArg("entities", EntityArgumentType::entities(), true));
        conditions.emplace_back("entity", entityNode);
    }

    // if|unless function <name>
    {
        auto functionNode = conditionLiteral("function");
        auto functionName = conditionArg("name", FunctionArgumentType::functions(), false);
        attachCommandSuggestions(functionName);
        functionNode->addChild(functionName);
        conditions.emplace_back("function", functionNode);
    }

    // if|unless items <block <pos>|entity <entities>> <slots> <item_predicate>
    {
        auto itemsNode = conditionLiteral("items");

        auto blockNode = conditionLiteral("block");
        auto pos = conditionArg("pos", BlockPosArgumentType::blockPos(), false);
        auto blockSlots = conditionArg("slots", ItemSlotsArgumentType::itemSlots(), false);
        blockSlots->addChild(conditionArg("item_predicate", ItemPredicateArgumentType::itemPredicate(), true));
        pos->addChild(blockSlots);
        blockNode->addChild(pos);

        auto entityNode = conditionLiteral("entity");
        auto entities = conditionArg("entities", EntityArgumentType::entities(), false);
        auto entitySlots = conditionArg("slots", ItemSlotsArgumentType::itemSlots(), false);
        entitySlots->addChild(conditionArg("item_predicate", ItemPredicateArgumentType::itemPredicate(), true));
        entities->addChild(entitySlots);
        entityNode->addChild(entities);

        itemsNode->addChild(blockNode);
        itemsNode->addChild(entityNode);
        conditions.emplace_back("items", itemsNode);
    }

    // if|unless loaded <pos>
    {
        auto loadedNode = conditionLiteral("loaded");
        loadedNode->addChild(conditionArg("pos", BlockPosArgumentType::blockPos(), true));
        conditions.emplace_back("loaded", loadedNode);
    }

    // if|unless predicate <predicate>
    {
        auto predicateNode = conditionLiteral("predicate");
        predicateNode->addChild(conditionArg("predicate", KeywordArgumentType::resourceOrTag("loot_predicate"), true));
        conditions.emplace_back("predicate", predicateNode);
    }

    // if|unless score <target> <targetObjective> <op> <source> <sourceObjective> | matches <range>
    {
        auto scoreNode = conditionLiteral("score");
        auto target = conditionArg("target", ScoreHolderArgumentType::scoreHolder(false), false);
        attachCommandSuggestions(target);
        auto objective = conditionArg("targetObjective", ObjectiveNameArgumentType::objective(), false);
        for (const char* op : {"<", "<=", "=", ">", ">="}) {
            auto opNode = conditionLiteral(op);
            auto source = conditionArg("source", ScoreHolderArgumentType::scoreHolder(false), false);
            attachCommandSuggestions(source);
            source->addChild(conditionArg("sourceObjective", ObjectiveNameArgumentType::objective(), true));
            opNode->addChild(source);
            objective->addChild(opNode);
        }
        auto matchesNode = conditionLiteral("matches");
        matchesNode->addChild(conditionArg("range", NumericRangeArgumentType::of("int_range"), true));
        objective->addChild(matchesNode);
        target->addChild(objective);
        scoreNode->addChild(target);
        conditions.emplace_back("score", scoreNode);
    }

    auto stopwatch = conditionLiteral("stopwatch");
    auto stopwatchId = conditionArg("id", ResourceLocationArgumentType::resourceLocation(), false);
    attachCommandSuggestions(stopwatchId);
    stopwatchId->addChild(conditionArg("range", NumericRangeArgumentType::of("float_range"), true));
    stopwatch->addChild(stopwatchId);
    conditions.emplace_back("stopwatch", stopwatch);

    return conditions;
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
    executeNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(executeNode,
        support::makeMetadata("Executes a command.",
            "/execute as|at|in|on|positioned|rotated|align|anchored|facing|if|unless|store|run ...",
            2,
            {},
            false));

    // /execute 的修改器链：每个修改器节点（ExecuteModifierNode）在解析阶段把索引写进上下文，
    // 下发的 run 节点据此逐段解析链上剩余 token，执行器再按同一串索引改写命令源。
    // 原版把「下一个节点」挂在每个 run 的子节点上；Cubium 的解析器不二次派发，改为在 run
    // 节点内用注册表（字面量 → 修改器节点）继续解析。
    std::vector<std::shared_ptr<ExecuteModifierNode>> modifiers;

    // 链尾的执行器：按 "commandChain" 依次改写命令源后执行 command 参数。
    const CommandCallback<ServerCommandSource> forwardingCommand = [](CommandContext<ServerCommandSource>& ctx) {
        return _executeForwarded(ctx);
    };

    // ========== run <command> ==========
    auto runNode = std::make_shared<ExecuteRunNode>(-1);
    // command 是「任何长度的剩余文本」（原版 run → command 的 greedyString）：链中间，run
    // 节点先按字面量继续读修改器链；链尾则由 run 节点把原文一次性交给这个子节点，它读走
    // 全部剩余文本，dispatcher 之后不会再把它当成新候选。
    auto runCommandArg = std::make_shared<CommandPlaceholderNode>();
    runCommandArg->setCommand(forwardingCommand);
    // 原版的 run 下没有 command 子节点，这个占位参数只供本实现复用派发，快照里跳过。
    runCommandArg->setTreeHidden(true);
    runNode->addChild(runCommandArg);
    executeNode->addChild(runNode);

    const auto modifier = [&](const char* name) {
        auto node = std::make_shared<ExecuteModifierNode>(name, static_cast<i32>(modifiers.size()));
        executeNode->addChild(node);
        modifiers.push_back(node);
        return node;
    };
    const auto child = [](const std::string& name) {
        return std::make_shared<LiteralCommandNode<ServerCommandSource>>(name);
    };

    // ========== as <targets> ==========
    auto asNode = modifier("as");
    auto asTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    asNode->addChild(asTargetsArg);

    // ========== at <targets> ==========
    auto atNode = modifier("at");
    auto atTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    atNode->addChild(atTargetsArg);

    // ========== in <dimension> ==========
    auto inNode = modifier("in");
    auto inDimArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, DimensionId>>(
        "dimension", DimensionArgumentType::dimension());
    // 原版 execute in 的 dimension 参数没有 .suggests()，命令树里不带建议提供器标识
    // （声明了会让客户端写 minecraft:ask_server，与原版不一致）。
    inNode->addChild(inDimArg);

    // ========== on <attacker|controller|leasher|origin|owner|passengers|target|vehicle> <entity> ==========
    // 同上：8 个字面量必须先于任何参数子节点注册，dispatcher 才会先尝试 on 的子分支。
    auto onNode = modifier("on");
    for (const char* name :
        {"attacker", "controller", "leasher", "origin", "owner", "passengers", "target", "vehicle"}) {
        auto onTargetNode = child(name);
        onNode->addChild(onTargetNode);
    }

    // ========== positioned <pos> | positioned as <targets> | positioned over <heightmap> ==========
    auto positionedNode = modifier("positioned");
    auto positionedAsNode = child("as");
    positionedAsNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities()));
    positionedNode->addChild(positionedAsNode);
    auto positionedOverNode = child("over");
    // 原版 positioned over <heightmap:block_pos> world_surface：heightmap 参数是方块位置语法
    // 的类型别名，后面固定跟 world_surface 字面量。
    auto heightmapArg = conditionArg("heightmap", PositionArgumentAliasType::of("heightmap"), false);
    positionedOverNode->addChild(heightmapArg);
    positionedNode->addChild(positionedOverNode);
    // 注意：pos 必须放在 as / over 之后注册。Vec3ArgumentType 在输入为空时也返回「全部相对」
    // 的坐标（不抛异常），所以 `vector<arg>` 迭代器会把 `as` / `over` 交给 pos 的 parse，
    // 它按 readUnquotedString 吃掉一个 token 后因坐标不合法把游标还原，随后又退回「空坐标」，
    // dispatcher 便不再尝试后面的字面量子节点。字面量先注册能保证 `as` / `over` 优先匹配。
    positionedNode->addChild(
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3()));

    // ========== rotated <rotation> | rotated as <targets> ==========
    auto rotatedNode = modifier("rotated");
    auto rotatedAsNode = child("as");
    rotatedAsNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities()));
    rotatedNode->addChild(rotatedAsNode);
    // 原版 execute 树的参数名是 rot（不是 rotate 命令里的 rotation）。
    rotatedNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "rot", RotationArgumentType::rotation()));

    // ========== align <axes>（x / y / z / xy / xz / yz / xyz）==========
    auto alignNode = modifier("align");
    alignNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "axes", SwizzleArgumentType::swizzle()));

    // ========== anchored <eyes|feet> ==========
    auto anchoredNode = modifier("anchored");
    anchoredNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "anchor", EntityAnchorArgumentType::entityAnchor()));

    // ========== facing <pos> | facing entity <targets> [eyes|feet] ==========
    auto facingNode = modifier("facing");
    facingNode->addChild(
        std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>("pos", Vec3ArgumentType::vec3()));
    auto facingEntityNode = child("entity");
    auto facingTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    // 原版 facing entity <targets> <anchor>：锚点是 entity_anchor 参数，不是字面量。
    facingTargetsArg->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "anchor", EntityAnchorArgumentType::entityAnchor()));
    facingEntityNode->addChild(facingTargetsArg);
    facingNode->addChild(facingEntityNode);

    // ========== summon <entity> ==========
    // 原版 /execute ... summon <entity>：在改写后的命令源位置生成实体，实体 id 参数是 resource。
    auto summonNode = child("summon");
    auto summonEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "entity", std::make_shared<ResourceArgumentType>("minecraft:entity_type", "resource"));
    // 原版为此参数挂的是 minecraft:summonable_entities（服务端只回可生成的实体类型）。
    summonEntityArg->setCustomSuggestions(
        std::make_shared<RegisteredSuggestionProvider<ServerCommandSource>>("minecraft:summonable_entities"));
    summonNode->addChild(summonEntityArg);
    executeNode->addChild(summonNode);

    // ========== store <result|success> <block|bossbar|entity|score|storage> ... ==========
    // 形状对齐原版 ExecuteCommand.addStoreArguments：
    //   block <pos> <path> <类型> [<scale>]
    //   bossbar <id> (value|max)
    //   entity <target> <path> <类型> [<scale>]
    //   score <targets> <objective>
    //   storage <id> <path> <类型> [<scale>]
    // 原版 store 子树没有任何可执行节点：命中 store 只表示「等下一条命令的结果」，链上还要
    // 继续读 run。执行语义尚未接入，落到执行链时按「store 不完整」报错，不静默成功。
    auto storeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("store");
    for (const char* kind : {"result", "success"}) {
        auto kindNode = child(kind);

        // block <targetPos> <path> <类型> [<scale>]
        {
            auto blockNode = child("block");
            auto pos = conditionArg("targetPos", BlockPosArgumentType::blockPos(), false);
            auto path = conditionArg("path", NbtPathArgumentType::nbtPath(), false);
            for (const char* type : {"byte", "short", "int", "long", "float", "double"}) {
                auto typeNode = child(type);
                typeNode->addChild(conditionArg("scale", DoubleArgumentType::doubleArg(), false));
                path->addChild(typeNode);
            }
            pos->addChild(path);
            blockNode->addChild(pos);
            kindNode->addChild(blockNode);
        }

        // bossbar <id> (value|max)
        {
            auto bossbarNode = child("bossbar");
            auto id = conditionArg("id", ResourceLocationArgumentType::resourceLocation(), false);
            attachCommandSuggestions(id);
            for (const char* field : {"value", "max"}) {
                auto fieldNode = child(field);
                id->addChild(fieldNode);
            }
            bossbarNode->addChild(id);
            kindNode->addChild(bossbarNode);
        }

        // entity <target> <path> <类型> [<scale>]
        {
            auto entityNode = child("entity");
            // 原版 store entity 的 target 是单实体参数（网络上 single 位为 1）。
            auto target = conditionArg("target", EntityArgumentType::entity(), false);
            auto path = conditionArg("path", NbtPathArgumentType::nbtPath(), false);
            for (const char* type : {"byte", "short", "int", "long", "float", "double"}) {
                auto typeNode = child(type);
                typeNode->addChild(conditionArg("scale", DoubleArgumentType::doubleArg(), false));
                path->addChild(typeNode);
            }
            target->addChild(path);
            entityNode->addChild(target);
            kindNode->addChild(entityNode);
        }

        // score <targets> <objective>
        {
            auto scoreNode = child("score");
            auto targets = conditionArg("targets", ScoreHolderArgumentType::scoreHolder(true), false);
            attachCommandSuggestions(targets);
            targets->addChild(conditionArg("objective", ObjectiveNameArgumentType::objective(), false));

            scoreNode->addChild(targets);
            kindNode->addChild(scoreNode);
        }

        // storage <target> <path> <类型> [<scale>]
        {
            auto storageNode = child("storage");
            auto target = conditionArg("target", ResourceLocationArgumentType::resourceLocation(), false);
            attachCommandSuggestions(target);
            auto path = conditionArg("path", NbtPathArgumentType::nbtPath(), false);
            for (const char* type : {"byte", "short", "int", "long", "float", "double"}) {
                auto typeNode = child(type);
                typeNode->addChild(conditionArg("scale", DoubleArgumentType::doubleArg(), false));
                path->addChild(typeNode);
            }
            target->addChild(path);
            storageNode->addChild(target);
            kindNode->addChild(storageNode);
        }

        storeNode->addChild(kindNode);
    }
    executeNode->addChild(storeNode);

    // ========== if / unless <条件> ==========
    // 条件子树逐条对齐原版 ExecuteCommand.Conditions.build：客户端只按这棵树补全与高亮，
    // 少一层参数就等于命令不可用。参数形状完整（可解析、可补全），执行分支见「执行链」。
    //
    // 条件节点必须登记进 modifiers：ExecuteRunNode 靠 commandModifier 里的索引把它拼进
    // 链段，执行期 _executeForwarded 才认得出 "if"/"unless"。不登记的话条件链会直接
    // 派发到 run，命令照常执行、条件被静默忽略。
    auto ifNode = std::make_shared<_ExecuteConditionNode>("if", static_cast<i32>(modifiers.size()));
    modifiers.push_back(ifNode);
    auto unlessNode = std::make_shared<_ExecuteConditionNode>("unless", static_cast<i32>(modifiers.size()));
    modifiers.push_back(unlessNode);
    for (auto& conditionNode : {ifNode, unlessNode}) {
        for (const auto& [name, node] : buildConditionNodes()) {
            (void)name;
            conditionNode->addChild(node);
        }
        executeNode->addChild(conditionNode);
    }

    // 重定向目标只观察已由分发器持有的节点，末端保留本地解析脚手架。
    runNode->setRedirect(dispatcher.getRoot(), RedirectModifier::None);
    const auto redirectLeaves = [&](const auto& self,
                                    const std::shared_ptr<CommandNode<ServerCommandSource>>& node,
                                    RedirectModifier mode) -> void {
        if (node->getChildren().empty()) {
            node->setRedirect(executeNode, mode);
            return;
        }
        for (const auto& [name, next] : node->getChildren()) {
            (void)name;
            self(self, next, mode);
        }
    };
    for (const auto& node : modifiers) {
        const std::string name = node->getName();
        redirectLeaves(redirectLeaves,
            node,
            name == "as" || name == "at" || name == "on" ? RedirectModifier::Fork : RedirectModifier::Single);
    }
    for (const std::shared_ptr<CommandNode<ServerCommandSource>>& node :
        std::vector<std::shared_ptr<CommandNode<ServerCommandSource>>>{
            positionedAsNode, rotatedAsNode, facingEntityNode, ifNode, unlessNode}) {
        redirectLeaves(redirectLeaves, node, RedirectModifier::Fork);
    }
    redirectLeaves(redirectLeaves, storeNode, RedirectModifier::Single);
    summonEntityArg->setRedirect(executeNode, RedirectModifier::Single);

    // 每个修改器的末端只挂一个 run 节点；后面的修改器由 run 节点按「字面量 → 索引」自行
    // 解析（不挂成子节点，否则 dispatcher 会一路派发下去，分不清哪一段才是链尾）。
    // 但「末端的参数」之后还能接下一个修改器：原版把下一个修改器挂在参数节点下，
    // 这里同样给每个参数节点（含字面量后的参数）接上 run，`as @p run` / `as @p at @s run`
    // 两条路径都能由 dispatcher 派发到。
    for (size_t i = 0; i < modifiers.size(); ++i) {
        auto& modNode = modifiers[i];
        auto runLink = std::make_shared<ExecuteRunNode>(static_cast<i32>(i));
        // 原版把「下一个修改器 / run」挂成一个重定向节点，并不出现在快照里；这里挂在参数
        // 节点上的 run 链接纯属派发脚手架，快照与网络下发时跳过。
        runLink->setTreeHidden(true);
        auto runCommand = std::make_shared<CommandPlaceholderNode>();
        runCommand->setCommand(forwardingCommand);
        runCommand->setTreeHidden(true);
        runLink->addChild(runCommand);
        // if/unless 不把 run 挂在自身这一层：它们的下一跳必然是条件名字面量
        // （block/biome/...）。挂在这里的话 dispatcher 会先尝试 run，把
        // `if block <pos> <block> run ...` 里的 block 当成 run 前的多余输入，
        // 使整条条件链解析失败。其余修改器行为保持不变。
        // if/unless 不把 run 挂在自身这一层：它们的下一跳必然是条件名字面量
        // （block/biome/...）。挂在这里的话 dispatcher 会先尝试 run，把
        // `if block <pos> <block> run ...` 里的 block 当成 run 前的多余输入。
        const std::string modName = modNode->getName();
        if (modName != "if" && modName != "unless") {
            modNode->addChild(runLink);
        }
        attachRunToLeaves(modNode, runLink);
    }

    // run 节点在 parse 时才读取这两张表，注册结束后统一回填（表里存的是 shared_ptr，生命周期随命令树）。
    ExecuteRunNode::setModifiers(
        std::vector<std::shared_ptr<CommandNode<ServerCommandSource>>>(modifiers.begin(), modifiers.end()));

    dispatcher.registerCommand(executeNode);
}

// ========== 子命令实现 ==========

namespace {

/**
 * @brief 把一条执行链改写成的命令源折叠成一份（原版 ExecuteCommand.chain 的语义）。
 *
 * as/at 对多个目标时原版会逐目标执行；这里按第一个匹配目标折叠，与 Cubium 既有的
 * EntityResolver 语义一致（多目标展开留给后续批次）。
 */
ServerCommandSource applyChain(
    ServerCommandSource source, const EntitySelector& selector, const std::string& verb, bool& matched)
{
    matched = false;
    if (verb == "as") {
        auto entities = support::EntityResolver::resolve(source, selector);
        if (!entities.empty() && entities.front() != nullptr) {
            matched = true;
            return source.withEntity(*entities.front());
        }
        return source;
    }
    // at
    auto entities = support::EntityResolver::resolve(source, selector);
    if (!entities.empty() && entities.front() != nullptr) {
        matched = true;
        Entity* entity = entities.front();
        ServerCommandSource modified = source.withPosition(Vector3d(static_cast<f64>(entity->position().x),
            static_cast<f64>(entity->position().y),
            static_cast<f64>(entity->position().z)));
        modified = modified.withRotation(Vector2f(entity->yaw(), entity->pitch()));
        modified = modified.withDimension(entity->dimension());
        return modified;
    }
    return source;
}

} // namespace

i32 ExecuteCommand::_executeForwarded(CommandContext<ServerCommandSource>& context)
{
    // 说明：这里原先在解析到条件节点时直接抛 commands.execute.conditional.notImplemented
    // （「条件求值接入前，完整条件链必须显式失败」）。条件求值现已接入——if/unless 作为
    // 修改器进入执行链，由下方 verb 分支调用 _evaluateBlockCondition 求值——故移除该拦截。

    auto& source = context.getSource();

    // 一次 /execute 只跑一遍：execute 下的 run 和每个修改器末端的 run 挂着同一个执行器，
    // dispatcher 回溯候选时它们都会在「字段有剩余」的成功上下文里被遍历。标记记录了
    // 「本次解析是否走到了链尾 run」，只有它才执行；执行过的再被遍历到就直接跳过。
    const auto marker = context.getArgumentOr<ExecuteParseMarker>(kParseMarkerParam, ExecuteParseMarker{});
    if (!marker.complete || !context.isPendingHandOff()) {
        return 0;
    }
    context.setArgument(kParseMarkerParam, ExecuteParseMarker{});

    const std::string command = context.getArgumentOr<std::string>("command", "");
    if (command.empty()) {
        source.sendError("commands.execute.failed.emptyCommand");
        return 0;
    }

    // 从 begin 位置开始把剩余链跑在 target 上，返回嵌套命令的结果累加。
    //
    // `as` 是多路分叉：选择器命中 n 个实体就各跑一次剩余链（对齐 vanilla
    // RedirectModifier.Fork），累加各自结果。此前的线性 while 只取 entities.front()，
    // `execute as @e[type=cow] run kill @s` 于是只杀掉第一只牛。
    std::function<i32(ServerCommandSource, size_t)> runChain = [&](ServerCommandSource target, size_t begin) -> i32 {
        i32 total = 0;
        while (begin < marker.chain.size()) {
            size_t end = marker.chain.find('|', begin);
            if (end == std::string::npos) {
                end = marker.chain.size();
            }
            const std::string verb = marker.chain.substr(begin, end - begin);
            begin = end + 1;

            if (verb == "as") {
                auto entities = support::EntityResolver::resolve(
                    target, context.getArgumentOr<EntitySelector>("targets", EntitySelector()));
                // 选择器没选中任何实体时链中断（对齐 vanilla：不执行、贡献 0）。
                if (entities.empty()) {
                    return total;
                }
                for (Entity* entity : entities) {
                    if (entity == nullptr) {
                        continue;
                    }
                    total += runChain(target.withEntity(*entity), begin);
                }
                return total;
            }

            if (verb == "at") {
                bool matched = false;
                target = applyChain(
                    target, context.getArgumentOr<EntitySelector>("targets", EntitySelector()), verb, matched);
                if (!matched) {
                    return total;
                }
            } else if (verb == "in") {
                const DimensionId targetDimId = context.getArgumentOr<DimensionId>("dimension", source.dimensionId());
                auto* server = target.server();
                if (server == nullptr) {
                    target.sendError("commands.execute.failed.noServer");
                    return total;
                }
                if (!server->dimensionManager().hasDimension(targetDimId)) {
                    target.sendError("commands.execute.in.invalidDimension");
                    return total;
                }
                ServerCommandSource modified = target.withDimension(targetDimId);
                if (target.dimensionId() != targetDimId) {
                    const DimensionType sourceDimType = DimensionType::fromId(target.dimensionId());
                    const DimensionType dimType = DimensionType::fromId(targetDimId);
                    if (sourceDimType.coordinateScale() != dimType.coordinateScale()) {
                        modified = modified.withPosition(
                            DimensionType::transformPosition(target.position(), sourceDimType, dimType));
                    }
                }
                target = modified;
            } else if (verb == "positioned") {
                target = target.withPosition(Vec3ArgumentType::getVec3(context, "pos", target));
            } else if (verb == "rotated") {
                auto coords = context.getArgumentOr<Coordinates::Ptr>("rot", nullptr);
                if (coords) {
                    const Vector3d rotated = coords->getPosition(target.position(), target.rotation());
                    target = target.withRotation(Vector2f(static_cast<f32>(rotated.x), static_cast<f32>(rotated.y)));
                }
            } else if (verb == "align") {
                const std::string axes = context.getArgumentOr<std::string>("axes", "");
                const Vector3d& pos = target.position();
                auto pick = [&axes](char axis) {
                    return axes.find(axis) != std::string::npos;
                };
                target = target.withPosition(Vector3d(pick('x') ? std::floor(pos.x) : pos.x,
                    pick('y') ? std::floor(pos.y) : pos.y,
                    pick('z') ? std::floor(pos.z) : pos.z));
            } else if (verb == "anchored") {
                const std::string anchor = context.getArgumentOr<std::string>("anchor", "feet");
                target = target.withAnchor(anchor == "eyes" ? EntityAnchorType::Eyes : EntityAnchorType::Feet);
            } else if (verb == "facing") {
                Vector3d from = target.position();
                Vector3d to = from;
                if (context.hasArgument("pos")) {
                    to = Vec3ArgumentType::getVec3(context, "pos", target);
                } else {
                    auto entities = support::EntityResolver::resolve(
                        target, context.getArgumentOr<EntitySelector>("targets", EntitySelector()));
                    if (!entities.empty() && entities.front() != nullptr) {
                        Entity* entity = entities.front();
                        to = Vector3d(static_cast<f64>(entity->position().x),
                            static_cast<f64>(entity->position().y),
                            static_cast<f64>(entity->position().z));
                    }
                }
                const f64 dx = to.x - from.x;
                const f64 dy = to.y - from.y;
                const f64 dz = to.z - from.z;
                const f64 horizontal = std::sqrt(dx * dx + dz * dz);
                const f32 yaw = static_cast<f32>(std::atan2(dz, dx) * 180.0 / 3.14159265358979323846 - 90.0);
                const f32 pitch = static_cast<f32>(-std::atan2(dy, horizontal) * 180.0 / 3.14159265358979323846);
                target = target.withRotation(Vector2f(yaw, pitch));
            } else if (verb == "if" || verb == "unless") {
                // 条件不成立时整条链中断、嵌套命令不执行、返回 0（对齐 vanilla
                // ExecuteCommand 的条件失败语义）。unless 即 if 取反。
                const bool conditionMet = _evaluateBlockCondition(target, context);
                if (conditionMet == (verb == "unless")) {
                    return total;
                }
            }
            // on / store 等：执行链已能解析，语义留待后续批次。
        }

        return total + executeNestedCommand(target, command);
    };

    return runChain(source, 0);
}

bool ExecuteCommand::_evaluateBlockCondition(
    const ServerCommandSource& target, CommandContext<ServerCommandSource>& context)
{
    // 目前只有 `if block <pos> <block>` 落了参数树；其他条件名仍是占位字面量，
    // 解析时不会写入 pos / block，这里按「条件不成立」处理。
    // TODO: 其余条件落地后，这里要按条件名分派，而不是只认 block。
    if (!context.hasArgument("pos") || !context.hasArgument("block")) {
        return false;
    }

    auto* world = target.world();
    if (world == nullptr) {
        return false;
    }

    const Vector3d resolved = Vec3ArgumentType::getVec3(context, "pos", target);
    const BlockPos blockPos(static_cast<i32>(std::floor(resolved.x)),
        static_cast<i32>(std::floor(resolved.y)),
        static_cast<i32>(std::floor(resolved.z)));

    const BlockState* actual = world->getBlockState(blockPos);
    if (actual == nullptr) {
        return false;
    }

    const BlockStateInput wanted = context.getArgumentOr<BlockStateInput>("block", BlockStateInput{});
    const BlockState* wantedState = wanted.state();
    if (wantedState == nullptr) {
        return false;
    }

    // 方块类型必须一致。
    if (actual->getBlock().blockId() != wantedState->getBlock().blockId()) {
        return false;
    }

    // 谓词里显式写出的属性必须逐一致；未写出的属性不限制（对齐 vanilla BlockPredicate）。
    for (const IProperty* prop : wanted.explicitProps()) {
        const auto actualIdx = actual->getValueIndex(*prop);
        const auto wantedIdx = wantedState->getValueIndex(*prop);
        if (!actualIdx.has_value() || !wantedIdx.has_value() || *actualIdx != *wantedIdx) {
            return false;
        }
    }
    return true;
}

} // namespace command
} // namespace mc
