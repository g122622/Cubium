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

#include "ScoreboardCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/StyleArgument.hpp"
#include "common/core/Types.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "common/util/json/JsonRead.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/util/text/ComponentUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/numbers/NumberFormat.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/CommandSuggestionProviders.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc {
namespace command {

namespace {

/// 原版这些分数持有者参数在注册时挂了 suggestScoreHolders()，命令树里带建议提供器位。
void attachScoreHolderSuggestions(const std::shared_ptr<CommandNode<ServerCommandSource>>& node)
{
    node->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
}

/// 把 `component` 参数文本解析成组件：能按 JSON 解析就用结构化组件，否则整段当纯文本。
/// 与原版 `ComponentArgument` 对裸字符串的处理一致（`"text"` 与 `text` 都合法）。
std::unique_ptr<text::ITextComponent> _parseCommandComponent(const std::string& text)
{
    auto json = mc::util::json::parse(text);
    if (json.success()) {
        auto component = text::ITextComponent::fromJson(json.value());
        if (component != nullptr) {
            return component;
        }
    }
    // 不是 JSON：按纯文本处理（/scoreboard ... fixed <contents> 允许直接写普通文字）
    return std::make_unique<text::StringTextComponent>(text);
}

/// 目标在反馈里的展示组件：`[名称]` 包裹并带悬浮提示（原版 Objective.getFormattedDisplayName）。
std::unique_ptr<text::ITextComponent> _objectiveDisplay(const scoreboard::ScoreObjective& objective)
{
    return objective.getFormattedDisplayName().deepCopy();
}

/// Java 字符串哈希（UTF-16 code unit 语义；记分板名限 ASCII，按字节计算等价）。
i32 _javaStringHash(std::string_view text)
{
    i32 hash = 0;
    for (const char rawChar : text) {
        hash = 31 * hash + static_cast<i32>(static_cast<unsigned char>(rawChar));
    }
    return hash;
}

/**
 * 按 Java HashMap 的迭代顺序返回下标。
 *
 * 原版 `/scoreboard objectives list` 直接遍历 `Map<String, Objective>`（HashMap），
 * 因此输出顺序是「桶号升序、桶内保持插入顺序」，而不是创建顺序。默认容量 16、负载
 * 因子 0.75，超出即翻倍；扩容迁移保持桶内相对顺序，故用最终容量一次性计算等价。
 *
 * @param names 按插入顺序排列的键
 * @return 迭代顺序的下标
 */
std::vector<size_t> _javaHashMapOrder(const std::vector<std::string>& names)
{
    size_t capacity = 16;
    while (names.size() > capacity * 3 / 4) {
        capacity *= 2;
    }

    std::vector<std::pair<size_t, size_t>> entries;
    entries.reserve(names.size());
    for (size_t index = 0; index < names.size(); ++index) {
        const u32 hash = static_cast<u32>(_javaStringHash(names[index]));
        const u32 spread = hash ^ (hash >> 16);
        entries.emplace_back(static_cast<size_t>(spread) & (capacity - 1), index);
    }
    std::stable_sort(entries.begin(),
        entries.end(),
        [](const std::pair<size_t, size_t>& left, const std::pair<size_t, size_t>& right) {
            return left.first < right.first;
        });

    std::vector<size_t> order;
    order.reserve(entries.size());
    for (const auto& entry : entries) {
        order.push_back(entry.second);
    }
    return order;
}

/// 分数持有者名列表的展示文本（单目标写名字，多目标写数量），用于成功消息。
std::string _describeTargets(const std::vector<std::string>& names)
{
    if (names.size() == 1) {
        return "'" + names.front() + "'";
    }
    return std::to_string(names.size()) + " score holders";
}

} // namespace

// 使用 mc::server 命名空间中的 ServerScoreboard
using ::mc::server::ServerScoreboard;

// 辅助函数：获取服务端记分板
static ServerScoreboard* getScoreboard(ServerCommandSource& source)
{
    auto* server = source.server();
    if (!server) {
        return nullptr;
    }
    return &server->scoreboard();
}

void ScoreboardCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto scoreboardNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("scoreboard");
    scoreboardNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(scoreboardNode,
        support::makeMetadata(
            "Manages scoreboard objectives and players.", "/scoreboard <objectives|players|teams> ...", 2, {}, true));

    // /scoreboard objectives ...
    auto objectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("objectives");

    // /scoreboard objectives add <name> <criteria> [displayName]
    auto addObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    // 原版 objectives add 的 objective 参数是单字串（brigadier:string 的 SingleWord）。
    auto objNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", StringArgumentType::word());
    // 原版判据参数名 criteria，类型是 objective_criteria。
    auto criteriaArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "criteria", ObjectiveCriteriaArgumentType::objectiveCriteria());
    criteriaArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addObjective(ctx);
    });
    objNameArg->addChild(criteriaArg);
    auto displayNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "displayName", ComponentArgumentType::component());
    displayNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addObjective(ctx);
    });
    criteriaArg->addChild(displayNameArg);
    addObjectivesNode->addChild(objNameArg);

    // /scoreboard objectives remove <name>
    auto removeObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    removeNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeObjective(ctx);
    });
    removeObjectivesNode->addChild(removeNameArg);

    // /scoreboard objectives modify <objective> displayName <displayName>
    auto modifyObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("modify");
    auto modifyNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    // 原版选项字面量是小写 displayname。
    auto displayNameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("displayname");
    auto modifyDisplayNameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "displayName", ComponentArgumentType::component());
    modifyDisplayNameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _modifyObjectiveDisplayName(ctx);
    });
    displayNameNode->addChild(modifyDisplayNameArg);
    modifyNameArg->addChild(displayNameNode);

    // /scoreboard objectives modify <objective> displayautoupdate <value>
    auto displayAutoUpdateNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("displayautoupdate");
    auto displayAutoUpdateArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("value", BoolArgumentType::boolArg());
    displayAutoUpdateArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _modifyObjectiveDisplayAutoUpdate(ctx);
    });
    displayAutoUpdateNode->addChild(displayAutoUpdateArg);
    modifyNameArg->addChild(displayAutoUpdateNode);

    // /scoreboard objectives modify <objective> numberformat ...
    // numberformat 本身可执行（等价于 blank），其下 fixed <contents> / styled <style> / blank。
    auto numberFormatNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("numberformat");
    // 未给子节点时原版传 null 格式（= 清除自定义格式），不是 blank。
    numberFormatNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _modifyObjectiveNumberFormat(ctx, std::nullopt);
    });
    auto styleNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("styled");
    auto styleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, StylePtr>>("style", StyleArgumentType::style());
    styleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        const auto style = ctx.getArgument<StylePtr>("style");
        return _modifyObjectiveNumberFormat(ctx, text::numbers::NumberFormat::styled(*style));
    });
    styleNode->addChild(styleArg);
    auto fixedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("fixed");
    auto contentsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "contents", ComponentArgumentType::component());
    contentsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        const auto contents = ctx.getArgument<ComponentTextPtr>("contents");
        return _modifyObjectiveNumberFormat(
            ctx, text::numbers::NumberFormat::fixed(_parseCommandComponent(contents->text)));
    });
    fixedNode->addChild(contentsArg);
    auto blankNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("blank");
    blankNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _modifyObjectiveNumberFormat(ctx, text::numbers::NumberFormat());
    });
    numberFormatNode->addChild(styleNode);
    numberFormatNode->addChild(fixedNode);
    numberFormatNode->addChild(blankNode);
    modifyNameArg->addChild(numberFormatNode);

    // /scoreboard objectives modify <objective> rendertype (hearts|integer)
    auto renderTypeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("rendertype");
    for (const char* kind : {"hearts", "integer"}) {
        auto kindNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>(kind);
        // 渲染类型是字面量分支，直接把对应枚举捕获进回调。
        const scoreboard::RenderType renderType =
            scoreboard::renderTypeParse(kind).value_or(scoreboard::RenderType::Integer);
        kindNode->setCommand([renderType](CommandContext<ServerCommandSource>& ctx) {
            return _setObjectiveRenderType(ctx, renderType);
        });
        renderTypeNode->addChild(kindNode);
    }
    modifyNameArg->addChild(renderTypeNode);

    modifyObjectivesNode->addChild(modifyNameArg);
    objectivesNode->addChild(modifyObjectivesNode);

    // /scoreboard objectives setdisplay <slot> [objective]
    auto setdisplayNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setdisplay");
    auto setdisplaySlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "slot", DisplaySlotArgumentType::displaySlot());
    setdisplaySlotArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setDisplaySlot(ctx);
    });
    auto setdisplayObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    setdisplayObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setDisplaySlot(ctx);
    });
    setdisplaySlotArg->addChild(setdisplayObjectiveArg);
    setdisplayNode->addChild(setdisplaySlotArg);
    objectivesNode->addChild(setdisplayNode);

    // /scoreboard objectives list
    auto listObjectivesNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listObjectivesNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listObjectives(ctx);
    });

    objectivesNode->addChild(addObjectivesNode);
    objectivesNode->addChild(removeObjectivesNode);
    objectivesNode->addChild(listObjectivesNode);
    scoreboardNode->addChild(objectivesNode);

    // /scoreboard players ...
    auto playersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("players");

    // /scoreboard players set <target> <objective> <score>
    auto setPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto setTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder(true));
    setTargetArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    auto setObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto scoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer());
    scoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _setScore(ctx);
    });
    setObjectiveArg->addChild(scoreArg);
    setTargetArg->addChild(setObjectiveArg);
    setPlayersNode->addChild(setTargetArg);

    // /scoreboard players add <target> <objective> <score>
    auto addPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto addTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder(true));
    addTargetArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    auto addObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto addScoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer(0));
    addScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _addScore(ctx);
    });
    addObjectiveArg->addChild(addScoreArg);
    addTargetArg->addChild(addObjectiveArg);
    addPlayersNode->addChild(addTargetArg);

    // /scoreboard players remove <target> <objective> <score>
    auto removePlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder(true));
    removeTargetArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    auto removeObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    auto removeScoreArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("score", IntegerArgumentType::integer(0));
    removeScoreArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _removeScore(ctx);
    });
    removeObjectiveArg->addChild(removeScoreArg);
    removeTargetArg->addChild(removeObjectiveArg);
    removePlayersNode->addChild(removeTargetArg);

    // /scoreboard players display name|numberformat <target> <objective> ...
    // 原版把 name / numberformat 两个字面量挂在 display 下，各自再挂 target / objective。
    auto displayPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("display");
    const auto makeDisplayTarget = [] {
        // 原版 display name / numberformat 的 targets 是多目标分数持有者。
        auto targets = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "targets", ScoreHolderArgumentType::scoreHolder(true));
        attachScoreHolderSuggestions(targets);
        auto objective = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
            "objective", ObjectiveNameArgumentType::objective());
        targets->addChild(objective);
        return std::make_pair(targets, objective);
    };

    // display name <targets> <objective> [<name>]
    auto playerDisplayNameNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("name");
    {
        auto [targets, objective] = makeDisplayTarget();
        // 原版 objective 本身可执行（清空该分数的自定义名），name <name> 覆盖显示名。
        objective->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _modifyPlayerDisplayName(ctx, std::nullopt);
        });
        auto nameArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
            "name", ComponentArgumentType::component());
        nameArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            const auto name = ctx.getArgument<ComponentTextPtr>("name");
            return _modifyPlayerDisplayName(ctx, name->text);
        });
        objective->addChild(nameArg);
        playerDisplayNameNode->addChild(targets);
    }
    displayPlayersNode->addChild(playerDisplayNameNode);

    // display numberformat <targets> <objective> (blank | styled <style> | fixed <contents>)
    auto displayNumberFormatNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("numberformat");
    {
        auto [targets, objective] = makeDisplayTarget();
        // 原版 objective 本身可执行（= 清除覆盖，回退到目标格式），其下 blank / styled / fixed。
        objective->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _modifyPlayerNumberFormat(ctx, std::nullopt);
        });
        auto blankNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("blank");
        blankNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            return _modifyPlayerNumberFormat(ctx, text::numbers::NumberFormat());
        });
        auto styledNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("styled");
        auto styleArg =
            std::make_shared<ArgumentCommandNode<ServerCommandSource, StylePtr>>("style", StyleArgumentType::style());
        styleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            const auto style = ctx.getArgument<StylePtr>("style");
            return _modifyPlayerNumberFormat(ctx, text::numbers::NumberFormat::styled(*style));
        });
        styledNode->addChild(styleArg);
        auto fixedNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("fixed");
        auto contentsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
            "contents", ComponentArgumentType::component());
        contentsArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
            const auto contents = ctx.getArgument<ComponentTextPtr>("contents");
            return _modifyPlayerNumberFormat(
                ctx, text::numbers::NumberFormat::fixed(_parseCommandComponent(contents->text)));
        });
        fixedNode->addChild(contentsArg);
        objective->addChild(blankNode);
        objective->addChild(styledNode);
        objective->addChild(fixedNode);
        displayNumberFormatNode->addChild(targets);
    }
    displayPlayersNode->addChild(displayNumberFormatNode);

    // /scoreboard players operation <targets> <objective> <op> <source> <sourceObjective>
    auto operationPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("operation");
    auto operationTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder(true));
    operationTargetArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    // 原版顺序：operation <target> <targetObjective> <op> <source> <sourceObjective>。
    auto operationObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targetObjective", ObjectiveNameArgumentType::objective());
    auto operationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "operation", ScoreOperationArgumentType::operation());
    auto operationSourceArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "source", ScoreHolderArgumentType::scoreHolder(true));
    operationSourceArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    auto operationSourceObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "sourceObjective", ObjectiveNameArgumentType::objective());
    operationSourceObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _operateScore(ctx);
    });
    operationSourceArg->addChild(operationSourceObjectiveArg);
    operationArg->addChild(operationSourceArg);
    operationObjectiveArg->addChild(operationArg);
    operationTargetArg->addChild(operationObjectiveArg);
    operationPlayersNode->addChild(operationTargetArg);

    // /scoreboard players reset <target> [objective]：原版这里是单个 target（不是 targets）。
    auto resetPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    auto resetTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder(true));
    resetTargetArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    resetTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _resetScore(ctx);
    });
    // /scoreboard players reset <targets> [<objective>]
    auto resetObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    resetObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _resetScore(ctx);
    });
    resetTargetArg->addChild(resetObjectiveArg);
    resetPlayersNode->addChild(resetTargetArg);

    // /scoreboard players get <target> <objective>：原版参数名是 target。
    auto getPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    // 原版 get 的 target 是单目标分数持有者。
    auto getTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target", ScoreHolderArgumentType::scoreHolder(false));
    attachScoreHolderSuggestions(getTargetArg);
    auto getObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    getObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _getScore(ctx);
    });
    getTargetArg->addChild(getObjectiveArg);
    getPlayersNode->addChild(getTargetArg);

    // /scoreboard players enable <target> <objective>
    // 用于启用 trigger 类型目标，让玩家可以再次使用 /trigger 命令
    auto enablePlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enable");
    auto enableTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "targets", ScoreHolderArgumentType::scoreHolder(true));
    enableTargetArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    auto enableObjectiveArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "objective", ObjectiveNameArgumentType::objective());
    enableObjectiveArg->setCustomSuggestions(std::make_shared<ServerSuggestionProvider<ServerCommandSource>>());
    enableObjectiveArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _enableTrigger(ctx);
    });
    enableTargetArg->addChild(enableObjectiveArg);
    enablePlayersNode->addChild(enableTargetArg);

    // /scoreboard players list [target]
    auto listPlayersNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    // 原版 list 的 target 是单目标分数持有者。
    auto listTargetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "target", ScoreHolderArgumentType::scoreHolder(false));
    attachScoreHolderSuggestions(listTargetArg);
    listTargetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listPlayers(ctx);
    });
    listPlayersNode->addChild(listTargetArg);
    listPlayersNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listAllPlayers(ctx);
    });

    playersNode->addChild(setPlayersNode);
    playersNode->addChild(addPlayersNode);
    playersNode->addChild(removePlayersNode);
    playersNode->addChild(resetPlayersNode);
    playersNode->addChild(getPlayersNode);
    playersNode->addChild(enablePlayersNode);
    playersNode->addChild(listPlayersNode);
    playersNode->addChild(displayPlayersNode);
    playersNode->addChild(operationPlayersNode);
    scoreboardNode->addChild(playersNode);

    dispatcher.registerCommand(scoreboardNode);
}

i32 ScoreboardCommand::_addObjective(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // 参数节点名与命令树里的登记名必须一致（objective/criteria），否则运行时取不到值。
    const std::string name = context.getArgument<std::string>("objective");
    const std::string criteria = context.getArgument<std::string>("criteria");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 验证目标名称长度
    if (name.length() > scoreboard::ScoreObjective::MAX_NAME_LENGTH) {
        std::ostringstream ss;
        ss << "Objective name '" << name << "' is too long (max " << scoreboard::ScoreObjective::MAX_NAME_LENGTH
           << " characters)";
        source.sendSuccess(::mc::text::literal(ss.str()), false);
        return 0;
    }

    // 检查目标是否已存在
    if (scoreboard->hasObjective(name)) {
        source.sendFailure(::mc::text::translatable("commands.scoreboard.objectives.add.duplicate"));
        return 0;
    }

    // 获取判据
    auto& registry = scoreboard::ScoreCriteriaRegistry::instance();
    auto* criteriaObj = registry.getCriteria(criteria);
    if (!criteriaObj) {
        std::ostringstream ss;
        ss << "Unknown scoreboard criteria '" << criteria << "'";
        source.sendSuccess(::mc::text::literal(ss.str()), false);
        return 0;
    }

    // 创建目标（原版可带一个显示名；未给时用目标名）。
    std::string displayNameText = name;
    if (context.hasArgument("displayName")) {
        displayNameText = context.getArgument<ComponentTextPtr>("displayName")->text;
    }
    auto displayName = std::make_unique<text::StringTextComponent>(displayNameText);
    auto* objective = scoreboard->addObjective(name, *criteriaObj, std::move(displayName));
    if (!objective) {
        source.sendSuccess(::mc::text::literal("Failed to create objective"), false);
        return 0;
    }

    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.objectives.add.success", _objectiveDisplay(*objective)), true);

    return 1;
}

i32 ScoreboardCommand::_removeObjective(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("objective");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(name);
    if (!objective) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", name));
        return 0;
    }

    // 移除目标：反馈内容必须在释放前取好（移除后目标指针失效）。
    auto display = _objectiveDisplay(*objective);
    scoreboard->removeObjective(*objective);

    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.objectives.remove.success", std::move(display)), true);

    return 1;
}

i32 ScoreboardCommand::_listObjectives(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    auto objectives = scoreboard->getObjectives();
    if (objectives.empty()) {
        source.sendSuccess(::mc::text::translatable("commands.scoreboard.objectives.list.empty"), false);
        return 1;
    }

    // 原版遍历的是 HashMap，输出顺序为哈希序；这里按同样规则重排，保证两侧列举一致。
    std::vector<std::string> objectiveNames;
    objectiveNames.reserve(objectives.size());
    for (const auto* objective : objectives) {
        objectiveNames.push_back(objective->getName());
    }
    const auto order = _javaHashMapOrder(objectiveNames);

    std::vector<std::unique_ptr<::mc::text::ITextComponent>> names;
    names.reserve(objectives.size());
    for (const size_t index : order) {
        names.push_back(_objectiveDisplay(*objectives[index]));
    }
    std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
    args.push_back(::mc::text::literal(std::to_string(objectives.size())));
    args.push_back(::mc::text::ComponentUtils::formatList(std::move(names)));
    source.sendSuccess(::mc::text::translatable("commands.scoreboard.objectives.list.success", std::move(args)), false);

    return 1;
}

i32 ScoreboardCommand::_setScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }

    // 检查判据是否只读
    if (objective->getCriteria().isReadOnly()) {
        std::ostringstream ss;
        ss << "Cannot set score for read-only criteria '" << objective->getCriteria().getName() << "'";
        source.sendSuccess(::mc::text::literal(ss.str()), false);
        return 0;
    }

    // 设置分数
    auto* scoreObj = scoreboard->getOrCreateScore(target, *objective);
    if (!scoreObj) {
        source.sendSuccess(::mc::text::literal("Failed to create score"), false);
        return 0;
    }
    scoreObj->setScorePoints(score);

    std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
    args.push_back(_objectiveDisplay(*objective));
    args.push_back(::mc::text::literal(target));
    args.push_back(::mc::text::literal(std::to_string(score)));
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.players.set.success.single", std::move(args)), true);

    return 1;
}

i32 ScoreboardCommand::_addScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }

    // 检查判据是否只读
    if (objective->getCriteria().isReadOnly()) {
        std::ostringstream ss;
        ss << "Cannot modify score for read-only criteria '" << objective->getCriteria().getName() << "'";
        source.sendSuccess(::mc::text::literal(ss.str()), false);
        return 0;
    }

    // 获取分数
    auto* scoreObj = scoreboard->getOrCreateScore(target, *objective);
    if (!scoreObj) {
        source.sendSuccess(::mc::text::literal("Failed to create score"), false);
        return 0;
    }
    scoreObj->addScore(score);

    // commands.scoreboard.players.add.success.single = "Added %s to %s for %s (now %s)"
    std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
    args.push_back(::mc::text::literal(std::to_string(score)));
    args.push_back(_objectiveDisplay(*objective));
    args.push_back(::mc::text::literal(target));
    args.push_back(::mc::text::literal(std::to_string(scoreObj->getScorePoints())));
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.players.add.success.single", std::move(args)), true);

    return 1;
}

i32 ScoreboardCommand::_removeScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");
    const i32 score = context.getArgument<i32>("score");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }

    // 检查判据是否只读
    if (objective->getCriteria().isReadOnly()) {
        std::ostringstream ss;
        ss << "Cannot modify score for read-only criteria '" << objective->getCriteria().getName() << "'";
        source.sendSuccess(::mc::text::literal(ss.str()), false);
        return 0;
    }

    // 获取分数
    auto* scoreObj = scoreboard->getOrCreateScore(target, *objective);
    if (!scoreObj) {
        source.sendSuccess(::mc::text::literal("Failed to create score"), false);
        return 0;
    }
    scoreObj->subtractScore(score);

    // commands.scoreboard.players.remove.success.single = "Removed %s from %s for %s (now %s)"
    std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
    args.push_back(::mc::text::literal(std::to_string(score)));
    args.push_back(_objectiveDisplay(*objective));
    args.push_back(::mc::text::literal(target));
    args.push_back(::mc::text::literal(std::to_string(scoreObj->getScorePoints())));
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.players.remove.success.single", std::move(args)), true);

    return 1;
}

i32 ScoreboardCommand::_resetScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("targets");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 原版：带 <objective> 时只清该目标下的分数，否则清空所有分数。
    if (context.hasArgument("objective")) {
        const std::string objectiveName = context.getArgument<std::string>("objective");
        auto* objective = scoreboard->getObjective(objectiveName);
        if (objective == nullptr) {
            source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
            return 0;
        }
        scoreboard->removeScore(target, objective);

        std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
        args.push_back(_objectiveDisplay(*objective));
        args.push_back(::mc::text::literal(target));
        source.sendSuccess(
            ::mc::text::translatable("commands.scoreboard.players.reset.specific.single", std::move(args)), true);
        return 1;
    }

    scoreboard->removeScore(target);
    source.sendSuccess(::mc::text::translatable("commands.scoreboard.players.reset.all.single", target), true);

    return 1;
}

i32 ScoreboardCommand::_getScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // 参数名必须与节点注册一致：`/scoreboard players get <target> <objective>` 的节点名是
    // "target"（见 getTargetArg），此前误读 "targets" 会抛 out_of_range。
    const std::string target = context.getArgument<std::string>("target");
    const std::string objectiveName = context.getArgument<std::string>("objective");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendFailure(::mc::text::literal("Scoreboard is not available"));
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }

    // 获取分数：无分数时原版用 commands.scoreboard.players.get.null（objective, 目标显示名）。
    auto* scoreObj = scoreboard->getScore(target, *objective);
    if (!scoreObj) {
        source.sendFailure(::mc::text::translatable("commands.scoreboard.players.get.null", {objectiveName, target}));
        return 0;
    }

    // 原版参数：目标显示名、分数、objective 的格式化显示名。
    std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
    args.push_back(::mc::text::literal(target));
    args.push_back(::mc::text::literal(std::to_string(scoreObj->getScorePoints())));
    args.push_back(_objectiveDisplay(*objective));
    source.sendSuccess(::mc::text::translatable("commands.scoreboard.players.get.success", std::move(args)), false);

    return 1;
}

i32 ScoreboardCommand::_enableTrigger(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string target = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 获取目标
    auto* objective = scoreboard->getObjective(objectiveName);
    if (!objective) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }

    // 检查判据是否为 trigger 类型
    auto& criteria = objective->getCriteria();
    if (criteria.getName() != scoreboard::TriggerCriteria::NAME) {
        source.sendFailure(::mc::text::translatable("commands.scoreboard.players.enable.invalid"));
        return 0;
    }

    // 获取或创建分数（这会为玩家"准备"触发器）
    auto* score = scoreboard->getOrCreateScore(target, *objective);
    if (!score) {
        source.sendSuccess(::mc::text::literal("Failed to create score"), false);
        return 0;
    }

    // 解锁触发器
    score->setLocked(false);

    std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
    args.push_back(_objectiveDisplay(*objective));
    args.push_back(::mc::text::literal(target));
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.players.enable.success.single", std::move(args)), true);

    return 1;
}

i32 ScoreboardCommand::_listPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    // 参数名必须与节点注册一致：`/scoreboard players list <target>` 的节点名是 "target"
    // （见 listTargetArg）。此前读 "targets"，所有带目标的 list 调用都会抛
    // out_of_range("Argument not found: targets")。
    const std::string target = context.getArgument<std::string>("target");

    // 获取记分板
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    // 获取玩家的所有目标名称
    auto objectiveNames = scoreboard->getPlayerObjectives(target);
    if (objectiveNames.empty()) {
        source.sendSuccess(::mc::text::translatable("commands.scoreboard.players.list.entity.empty", target), false);
        return 1;
    }

    std::string entries;
    for (size_t i = 0; i < objectiveNames.size(); ++i) {
        if (i > 0) {
            entries += ", ";
        }
        auto* objective = scoreboard->getObjective(objectiveNames[i]);
        auto* score = objective ? scoreboard->getScore(target, *objective) : nullptr;
        entries += objectiveNames[i];
        if (score) {
            entries += "=" + std::to_string(score->getScorePoints());
        }
    }
    auto listMessage = ::mc::text::translatable(
        "commands.scoreboard.players.list.entity.success", {target, std::to_string(objectiveNames.size())});
    listMessage->append(::mc::text::literal(entries));
    source.sendSuccess(std::move(listMessage), false);

    return 1;
}

i32 ScoreboardCommand::_modifyObjectiveDisplayName(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("objective");
    const std::string displayName = context.getArgument<ComponentTextPtr>("displayName")->text;

    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }
    auto* objective = scoreboard->getObjective(name);
    if (objective == nullptr) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", name));
        return 0;
    }
    objective->setDisplayName(std::make_unique<text::StringTextComponent>(displayName));
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.objectives.modify.displayname", {name, displayName}), false);
    return 1;
}

i32 ScoreboardCommand::_modifyObjectiveDisplayAutoUpdate(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("objective");
    const bool value = context.getArgument<bool>("value");

    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }
    auto* objective = scoreboard->getObjective(name);
    if (objective == nullptr) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", name));
        return 0;
    }

    objective->setDisplayAutoUpdate(value);
    source.sendSuccess(
        ::mc::text::literal("Set displayAutoUpdate of objective '" + name + "' to " + (value ? "true" : "false")),
        false);
    return 1;
}

i32 ScoreboardCommand::_setObjectiveRenderType(
    CommandContext<ServerCommandSource>& context, scoreboard::RenderType renderType)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("objective");

    auto* scoreboard = getScoreboard(source);
    if (scoreboard == nullptr) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    auto* objective = scoreboard->getObjective(name);
    if (objective == nullptr) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", name));
        return 0;
    }

    // 原版仅在类型真正变化时才写入并回报，避免重复设置时刷屏。
    if (objective->getRenderType() != renderType) {
        objective->setRenderType(renderType);
        source.sendSuccess(::mc::text::translatable("commands.scoreboard.objectives.modify.rendertype", name), true);
    }

    return 0;
}

i32 ScoreboardCommand::_setDisplaySlot(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string slotName = context.getArgument<std::string>("slot");
    // 原版槽位名：list / sidebar / below_name，以及 sidebar.team.<color>。
    const scoreboard::DisplaySlot slot = DisplaySlotArgumentType::toDisplaySlot(slotName);

    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }
    scoreboard::ScoreObjective* current = scoreboard->getObjectiveInDisplaySlot(slot);
    if (!context.hasArgument("objective")) {
        // /scoreboard objectives setdisplay <slot>：清空该槽位；本来就是空的则报 unchanged。
        if (current == nullptr) {
            source.sendFailure(::mc::text::translatable("commands.scoreboard.objectives.display.alreadyEmpty"));
            return 0;
        }
        scoreboard->setObjectiveInDisplaySlot(slot, nullptr);
        source.sendSuccess(::mc::text::translatable("commands.scoreboard.objectives.display.cleared", slotName), true);
        return 1;
    }

    auto* objective = scoreboard->getObjective(context.getArgument<std::string>("objective"));
    if (objective == nullptr) {
        source.sendFailure(
            ::mc::text::translatable("arguments.objective.notFound", context.getArgument<std::string>("objective")));
        return 0;
    }
    if (current == objective) {
        source.sendFailure(::mc::text::translatable("commands.scoreboard.objectives.display.alreadySet"));
        return 0;
    }

    scoreboard->setObjectiveInDisplaySlot(slot, objective);
    // 反馈里的目标名是注册名，不是显示名（显示名只在侧边栏渲染时使用）。
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.objectives.display.set", {slotName, objective->getName()}),
        false);
    return 1;
}

i32 ScoreboardCommand::_listAllPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }

    std::set<std::string> names;
    for (const auto& name : scoreboard->getTrackedPlayerNames()) {
        names.insert(name);
    }
    if (names.empty()) {
        source.sendSuccess(::mc::text::translatable("commands.scoreboard.players.list.empty"), false);
        return 0;
    }
    std::string namesText;
    for (const auto& name : names) {
        if (!namesText.empty()) {
            namesText += ", ";
        }
        namesText += name;
    }
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.players.list.success", {std::to_string(names.size()), namesText}),
        false);
    return static_cast<i32>(names.size());
}

i32 ScoreboardCommand::_modifyObjectiveNumberFormat(
    CommandContext<ServerCommandSource>& context, std::optional<text::numbers::NumberFormat> numberFormat)
{
    auto& source = context.getSource();
    const std::string name = context.getArgument<std::string>("objective");

    auto* scoreboard = getScoreboard(source);
    if (scoreboard == nullptr) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }
    auto* objective = scoreboard->getObjective(name);
    if (objective == nullptr) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", name));
        return 0;
    }

    objective->setNumberFormat(std::move(numberFormat));

    // 原版对「设置」与「清除」分别回报（objectiveFormat.set / .clear）。
    if (objective->getNumberFormat().has_value()) {
        source.sendSuccess(
            ::mc::text::translatable("commands.scoreboard.objectives.modify.objectiveFormat.set", name), false);
    } else {
        source.sendSuccess(
            ::mc::text::translatable("commands.scoreboard.objectives.modify.objectiveFormat.clear", name), false);
    }
    return 1;
}

i32 ScoreboardCommand::_modifyPlayerNumberFormat(
    CommandContext<ServerCommandSource>& context, std::optional<text::numbers::NumberFormat> numberFormat)
{
    auto& source = context.getSource();
    const std::string targets = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");

    auto* scoreboard = getScoreboard(source);
    if (scoreboard == nullptr) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }
    auto* objective = scoreboard->getObjective(objectiveName);
    if (objective == nullptr) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }

    // 原版 getNamesWithDefaultWildcard：`*` 展开为「所有已记录分数的持有者」，否则按逗号分隔。
    std::vector<std::string> holders;
    if (targets == "*") {
        holders = scoreboard->getTrackedPlayerNames();
    } else {
        std::istringstream stream(targets);
        std::string holder;
        while (std::getline(stream, holder, ',')) {
            if (!holder.empty()) {
                holders.push_back(holder);
            }
        }
    }
    if (holders.empty()) {
        source.sendFailure(::mc::text::literal("No score holders matched '" + targets + "'"));
        return 0;
    }

    // 原版对每个持有者 getOrCreatePlayerScore(...).numberFormatOverride(...)：即使该持有者本来
    // 没有分数也会建出条目（与 /scoreboard players display name 的行为一致）。
    for (const auto& holder : holders) {
        auto* score = scoreboard->getOrCreateScore(holder, *objective);
        if (score != nullptr) {
            score->setNumberFormatOverride(numberFormat);
        }
    }

    const std::string who = _describeTargets(holders);
    if (numberFormat.has_value()) {
        source.sendSuccess(
            ::mc::text::literal("Set number format of " + who + " in objective '" + objectiveName + "'"), false);
    } else {
        source.sendSuccess(
            ::mc::text::literal("Cleared number format of " + who + " in objective '" + objectiveName + "'"), false);
    }
    return static_cast<i32>(holders.size());
}

i32 ScoreboardCommand::_modifyPlayerDisplayName(
    CommandContext<ServerCommandSource>& context, const std::optional<std::string>& displayJson)
{
    auto& source = context.getSource();
    const std::string targets = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("objective");

    auto* scoreboard = getScoreboard(source);
    if (scoreboard == nullptr) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }
    auto* objective = scoreboard->getObjective(objectiveName);
    if (objective == nullptr) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }

    std::vector<std::string> holders;
    if (targets == "*") {
        holders = scoreboard->getTrackedPlayerNames();
    } else {
        std::istringstream stream(targets);
        std::string holder;
        while (std::getline(stream, holder, ',')) {
            if (!holder.empty()) {
                holders.push_back(holder);
            }
        }
    }
    if (holders.empty()) {
        source.sendFailure(::mc::text::literal("No score holders matched '" + targets + "'"));
        return 0;
    }

    // 每个持有者各持有一份组件：Score 拥有它，不能在多个条目间共享同一个对象。
    for (const auto& holder : holders) {
        auto* score = scoreboard->getOrCreateScore(holder, *objective);
        if (score != nullptr) {
            score->setDisplay(displayJson.has_value() ? _parseCommandComponent(*displayJson) : nullptr);
        }
    }

    const std::string who = _describeTargets(holders);
    if (displayJson.has_value()) {
        source.sendSuccess(
            ::mc::text::literal("Set display name of " + who + " in objective '" + objectiveName + "'"), false);
    } else {
        source.sendSuccess(
            ::mc::text::literal("Cleared display name of " + who + " in objective '" + objectiveName + "'"), false);
    }
    return static_cast<i32>(holders.size());
}

i32 ScoreboardCommand::_operateScore(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string targets = context.getArgument<std::string>("targets");
    const std::string objectiveName = context.getArgument<std::string>("targetObjective");
    const std::string operation = context.getArgument<std::string>("operation");
    const std::string sourceName = context.getArgument<std::string>("source");
    const std::string sourceObjectiveName = context.getArgument<std::string>("sourceObjective");

    auto* scoreboard = getScoreboard(source);
    if (!scoreboard) {
        source.sendSuccess(::mc::text::literal("Scoreboard is not available"), false);
        return 0;
    }
    auto* objective = scoreboard->getObjective(objectiveName);
    auto* sourceObjective = scoreboard->getObjective(sourceObjectiveName);
    if (objective == nullptr || sourceObjective == nullptr) {
        source.sendFailure(::mc::text::translatable("arguments.objective.notFound", objectiveName));
        return 0;
    }
    // 原版两边都走 getOrCreatePlayerScore：缺失的分数按 0 处理并在运算后落盘，
    // 而不是把「读不到」当成失败。
    auto* targetScore = scoreboard->getOrCreateScore(targets, *objective);
    auto* sourceScore = scoreboard->getOrCreateScore(sourceName, *sourceObjective);
    if (targetScore == nullptr || sourceScore == nullptr) {
        source.sendFailure(::mc::text::literal("Failed to create score"));
        return 0;
    }

    i32 result = targetScore->getScorePoints();
    const i32 rhs = sourceScore->getScorePoints();
    if (operation == "=") {
        result = rhs;
    } else if (operation == "+=") {
        result += rhs;
    } else if (operation == "-=") {
        result -= rhs;
    } else if (operation == "*=") {
        result *= rhs;
    } else if (operation == "/=" || operation == "%=") {
        if (rhs == 0) {
            // 原版对整数除零会抛出未捕获异常（玩家看到 command.failed）；这里保持失败语义。
            source.sendFailure(::mc::text::translatable("command.failed"));
            return 0;
        }
        result = operation == "/=" ? result / rhs : result % rhs;
    } else if (operation == "<" || operation == ">") {
        result = operation == "<" ? std::min(result, rhs) : std::max(result, rhs);
    } else if (operation == "><") {
        // 交换的是两个分数值，不是它们的位置；同一分数上交换自身等于不变。
        const i32 originalTarget = result;
        if (targetScore != sourceScore) {
            targetScore->setScorePoints(rhs);
            sourceScore->setScorePoints(originalTarget);
        }
        result = targetScore == sourceScore ? originalTarget : rhs;
    }
    targetScore->setScorePoints(result);

    std::vector<std::unique_ptr<::mc::text::ITextComponent>> args;
    args.push_back(_objectiveDisplay(*objective));
    args.push_back(::mc::text::literal(targets));
    args.push_back(::mc::text::literal(std::to_string(result)));
    source.sendSuccess(
        ::mc::text::translatable("commands.scoreboard.players.operation.success.single", std::move(args)), true);
    return result;
}

} // namespace command
} // namespace mc
