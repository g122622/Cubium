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
#include "common/command/arguments/DimensionArgument.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
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
#include "server/command/support/EntityResolver.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>

namespace mc {
namespace command {

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
        if (child->getType() == NodeType::Argument) {
            child->addChild(runLink);
            continue;
        }
        attachRunToLeaves(child, runLink);
    }
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
    auto runNode = std::make_shared<ExecuteRunNode>(-1, forwardingCommand);
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
    inDimArg->setCustomSuggestions(std::make_shared<DimensionSuggestionProvider<ServerCommandSource>>());
    inNode->addChild(inDimArg);

    // ========== on <attacker|controller|leasher|origin|owner|passengers|target|vehicle> <entity> ==========
    // 同上：8 个字面量必须先于任何参数子节点注册，dispatcher 才会先尝试 on 的子分支。
    auto onNode = modifier("on");
    for (const char* name :
        {"attacker", "controller", "leasher", "origin", "owner", "passengers", "target", "vehicle"}) {
        auto onTargetNode = child(name);
        onTargetNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "entity", EntityArgumentType::entity()));
        onNode->addChild(onTargetNode);
    }

    // ========== positioned <pos> | positioned as <targets> | positioned over <heightmap> ==========
    auto positionedNode = modifier("positioned");
    auto positionedAsNode = child("as");
    positionedAsNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities()));
    positionedNode->addChild(positionedAsNode);
    auto positionedOverNode = child("over");
    positionedOverNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "heightmap", StringArgumentType::string()));
    positionedNode->addChild(positionedOverNode);
    // 注意：pos 必须放在 as / over 之后注册。Vec3ArgumentType 在输入为空时也返回「全部相对」
    // 的坐标（不抛异常），所以 `vector<arg>` 迭代器会把 `as` / `over` 交给 pos 的 parse，
    // 它按 readUnquotedString 吃掉一个 token 后因坐标不合法把游标还原，随后又退回「空坐标」，
    // dispatcher 便不再尝试后面的字面量子节点。字面量先注册能保证 `as` / `over` 优先匹配。
    positionedNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", Vec3ArgumentType::vec3()));

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
    alignNode->addChild(
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("axes", SwizzleArgumentType::swizzle()));

    // ========== anchored <eyes|feet> ==========
    auto anchoredNode = modifier("anchored");
    anchoredNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "anchor", EntityAnchorArgumentType::entityAnchor()));

    // ========== facing <pos> | facing entity <targets> [eyes|feet] ==========
    auto facingNode = modifier("facing");
    facingNode->addChild(std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", Vec3ArgumentType::vec3()));
    auto facingEntityNode = child("entity");
    auto facingTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    facingTargetsArg->addChild(child("eyes"));
    facingTargetsArg->addChild(child("feet"));
    facingEntityNode->addChild(facingTargetsArg);
    facingNode->addChild(facingEntityNode);

    // ========== summon <entity> ==========
    // 原版 /execute ... summon <entity>：在改写后的命令源位置生成实体，实体 id 参数是 resource。
    auto summonNode = child("summon");
    auto summonEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "entity", ResourceEntityArgumentType::entity());
    summonEntityArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return SummonCommand::summonAtSource(ctx); });
    summonNode->addChild(summonEntityArg);
    executeNode->addChild(summonNode);

    // ========== store <result|success> <block|bossbar|entity|score|storage> ... ==========
    // store 是单例节点：每层只有一个分支，没有可展开的子节点。
    auto storeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("store");
    storeNode->addChild(child("result"));
    storeNode->addChild(child("success"));
    executeNode->addChild(storeNode);

    // ========== if / unless <条件> ==========
    // 条件列表与原版一致；条件的具体参数树尚未落地，先按字面量占位（执行时见 _executeCondition*）。
    for (const char* kind : {"if", "unless"}) {
        auto conditionNode = child(kind);
        for (const char* name : {"biome",
                 "block",
                 "blocks",
                 "data",
                 "dimension",
                 "entity",
                 "function",
                 "items",
                 "loaded",
                 "predicate",
                 "score",
                 "stopwatch"}) {
            conditionNode->addChild(child(name));
        }
        executeNode->addChild(conditionNode);
    }

    // 每个修改器的末端只挂一个 run 节点；后面的修改器由 run 节点按「字面量 → 索引」自行
    // 解析（不挂成子节点，否则 dispatcher 会一路派发下去，分不清哪一段才是链尾）。
    // 但「末端的参数」之后还能接下一个修改器：原版把下一个修改器挂在参数节点下，
    // 这里同样给每个参数节点（含字面量后的参数）接上 run，`as @p run` / `as @p at @s run`
    // 两条路径都能由 dispatcher 派发到。
    for (size_t i = 0; i < modifiers.size(); ++i) {
        auto& modNode = modifiers[i];
        auto runLink = std::make_shared<ExecuteRunNode>(static_cast<i32>(i), forwardingCommand);
        // 原版把「下一个修改器 / run」挂成一个重定向节点，并不出现在快照里；这里挂在参数
        // 节点上的 run 链接纯属派发脚手架，快照与网络下发时跳过。
        runLink->setTreeHidden(true);
        auto runCommand = std::make_shared<CommandPlaceholderNode>();
        runCommand->setCommand(forwardingCommand);
        runCommand->setTreeHidden(true);
        runLink->addChild(runCommand);
        modNode->addChild(runLink);
        attachRunToLeaves(modNode, runLink);
    }

    // run 节点在 parse 时才读取这两张表，注册结束后统一回填（表里存的是 shared_ptr，生命周期随命令树）。
    ExecuteRunNode::setModifiers(std::vector<std::shared_ptr<CommandNode<ServerCommandSource>>>(
        modifiers.begin(), modifiers.end()));
    ExecuteRunNode::setTopLevelRun(runNode);

    dispatcher.registerCommand(executeNode);
}

// ========== 子命令实现 ==========

i32 ExecuteCommand::_executeRun(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::string command = context.getArgument<std::string>("command");

    return executeNestedCommand(source, command);
}

// ========== 执行链 ==========

namespace {

/**
 * @brief 把一条执行链改写成的命令源折叠成一份（原版 ExecuteCommand.chain 的语义）。
 *
 * as/at 对多个目标时原版会逐目标执行；这里按第一个匹配目标折叠，与 Cubium 既有的
 * EntityResolver 语义一致（多目标展开留给后续批次）。
 */
ServerCommandSource applyChain(ServerCommandSource source, const EntitySelector& selector, const std::string& verb,
    bool& matched)
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

    ServerCommandSource target = source;
    size_t begin = 0;
    while (begin < marker.chain.size()) {
        size_t end = marker.chain.find('|', begin);
        if (end == std::string::npos) {
            end = marker.chain.size();
        }
        const std::string verb = marker.chain.substr(begin, end - begin);
        begin = end + 1;

        if (verb == "as" || verb == "at") {
            bool matched = false;
            target = applyChain(
                target, context.getArgumentOr<EntitySelector>("targets", EntitySelector()), verb, matched);
            if (!matched) {
                // 原版：选择器没选中任何实体时链中断，命令不执行、返回 0。
                return 0;
            }
        } else if (verb == "in") {
            const DimensionId targetDimId = context.getArgumentOr<DimensionId>("dimension", source.dimensionId());
            auto* server = target.server();
            if (server == nullptr) {
                target.sendError("commands.execute.failed.noServer");
                return 0;
            }
            if (!server->dimensionManager().hasDimension(targetDimId)) {
                target.sendError("commands.execute.in.invalidDimension");
                return 0;
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
                target = target.withRotation(
                    Vector2f(static_cast<f32>(rotated.x), static_cast<f32>(rotated.y)));
            }
        } else if (verb == "align") {
            const std::string axes = context.getArgumentOr<std::string>("axes", "");
            const Vector3d& pos = target.position();
            auto pick = [&axes](char axis) { return axes.find(axis) != std::string::npos; };
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
        }
        // on / store 等：执行链已能解析，语义留待后续批次。
    }

    return executeNestedCommand(target, command);
}

} // namespace command
} // namespace mc
