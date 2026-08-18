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

#include "EffectCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp" // LivingEntity::addEffect/hasEffect/removeEffect（实体层效果）
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp" // Player（实体旁路解析目标类型）
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EffectResolver.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp" // getPlayerEntity（实体旁路解析）
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>

namespace mc {
namespace command {
namespace {

/**
 * @brief 经实体管理器解析 PlayerId 到 Player 实体（含 SimulatedPlayer 旁路）。
 *
 * EffectCommand 此前经 PlayerManager.getPlayer 拿 ServerPlayerData 写其 effects 向量（网络数据层），
 * 对不进 PlayerManager 的 SimulatedPlayer 返 nullptr 跳过失效；且即便真实玩家，ServerPlayerData.effects
 * 与脚本侧 Entity.getEffect 读的 LivingEntity::effectManager（实体层）不互通，致 /effect 成功但脚本读不到。
 * 统一改走实体层 LivingEntity 的 effect 方法：经 ServerPlayerEntityManager 解析实体（SimulatedPlayer 已
 * 注册映射，真实玩家亦在 EntityManager），调 LivingEntity::addEffect 等写实体 m_effectManager，与脚本对齐。
 *
 * @return Player* 实体指针；解析失败（PlayerId 无映射或世界为空）返回 nullptr。
 */
[[nodiscard]] Player* resolvePlayerEntity(ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    if (server == nullptr) {
        return nullptr;
    }
    auto* world = source.world();
    if (world == nullptr) {
        return nullptr;
    }
    return server->playerEntityManager().getPlayerEntity(playerId, *world);
}

} // namespace

void EffectCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto effectNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("effect");
    effectNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(effectNode,
        support::makeMetadata("Gives or removes status effects from players.",
            "/effect (give|clear) <player> [<effect>] [<seconds>] [<amplifier>] [<hideParticles>]",
            2,
            {},
            true));

    // /effect give <player> <effect> [<seconds>] [<amplifier>] [<hideParticles>]
    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");

    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());

    // 效果类型节点 - 使用字符串参数
    auto effectNameNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("effect", StringArgumentType::word());

    // 可选参数
    auto secondsNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "seconds", IntegerArgumentType::integer(0, 1000000));
    secondsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _giveEffect(ctx); });

    auto amplifierNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "amplifier", IntegerArgumentType::integer(0, 255));
    amplifierNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _giveEffect(ctx); });

    auto hideParticlesNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("hideParticles", BoolArgumentType::boolArg());
    hideParticlesNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _giveEffect(ctx); });

    // 默认使用效果名称
    effectNameNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _giveEffect(ctx); });

    amplifierNode->addChild(hideParticlesNode);
    secondsNode->addChild(amplifierNode);
    effectNameNode->addChild(secondsNode);
    playerNode->addChild(effectNameNode);
    giveNode->addChild(playerNode);

    // /effect clear <player> [<effect>]
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");

    auto clearPlayerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());
    clearPlayerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearAllEffects(ctx); });

    auto clearEffectNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("effect", StringArgumentType::word());
    clearEffectNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearSpecificEffect(ctx); });

    clearPlayerNode->addChild(clearEffectNode);
    clearNode->addChild(clearPlayerNode);

    effectNode->addChild(giveNode);
    effectNode->addChild(clearNode);
    dispatcher.registerCommand(effectNode);
}

i32 EffectCommand::_giveEffect(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const std::string& effectName = context.getArgument<std::string>("effect");

    auto effectTypeOpt = support::tryParseEffectType(effectName);
    if (!effectTypeOpt.has_value()) {
        source.sendError("Unknown effect: " + effectName);
        return 0;
    }
    entity::effect::EffectType effectType = effectTypeOpt.value();

    // 获取可选参数
    i32 seconds = 30;  // 默认30秒
    i32 amplifier = 0; // 默认等级0
    bool hideParticles = false;

    if (context.hasArgument("seconds")) {
        seconds = context.getArgument<i32>("seconds");
    }
    if (context.hasArgument("amplifier")) {
        amplifier = context.getArgument<i32>("amplifier");
    }
    if (context.hasArgument("hideParticles")) {
        hideParticles = context.getArgument<bool>("hideParticles");
    }

    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        // 统一经实体管理器解析实体（含 SimulatedPlayer 旁路），写实体层 LivingEntity::m_effectManager。
        // 此前经 PlayerManager.getPlayer 写 ServerPlayerData.effects（网络数据层），对 SimulatedPlayer 失效，
        // 且对真实玩家与脚本 Entity.getEffect（读实体 effectManager）层错配。改走实体层后两端对齐。
        Player* player = resolvePlayerEntity(source, playerId);
        if (player == nullptr) {
            continue;
        }

        // 添加效果：LivingEntity::addEffect 内部走 m_effectManager.addEffect（合并/覆盖同类型）。
        entity::effect::EffectInstance effect(effectType, seconds * 20, amplifier, false, !hideParticles);
        player->addEffect(std::move(effect));
        successCount++;
    }

    std::ostringstream ss;
    ss << "Gave " << support::getEffectCommandName(effectType);
    if (amplifier > 0) {
        ss << " " << (amplifier + 1);
    }
    ss << " to " << successCount << " player(s)";
    source.sendMessage(ss.str());

    return successCount;
}

i32 EffectCommand::_clearAllEffects(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        // 经实体管理器解析实体（含 SimulatedPlayer 旁路），操作实体层 LivingEntity::m_effectManager。
        Player* player = resolvePlayerEntity(source, playerId);
        if (player == nullptr) {
            continue;
        }

        size_t effectCount = player->effectManager().getAllEffects().size();
        player->removeAllEffects();
        successCount += static_cast<i32>(effectCount);
    }

    source.sendMessage("Cleared all effects");
    return successCount;
}

i32 EffectCommand::_clearSpecificEffect(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const std::string& effectName = context.getArgument<std::string>("effect");

    auto effectTypeOpt = support::tryParseEffectType(effectName);
    if (!effectTypeOpt.has_value()) {
        source.sendError("Unknown effect: " + effectName);
        return 0;
    }
    entity::effect::EffectType effectType = effectTypeOpt.value();

    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        // 经实体管理器解析实体（含 SimulatedPlayer 旁路），操作实体层 LivingEntity::m_effectManager。
        Player* player = resolvePlayerEntity(source, playerId);
        if (player == nullptr) {
            continue;
        }

        if (player->hasEffect(effectType)) {
            player->removeEffect(effectType);
            successCount++;
        }
    }

    std::ostringstream ss;
    ss << "Cleared " << support::getEffectCommandName(effectType) << " from " << successCount << " player(s)";
    source.sendMessage(ss.str());

    return successCount;
}

} // namespace command
} // namespace mc
