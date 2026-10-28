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
#include "common/command/arguments/ResourceArgument.hpp"
#include "common/command/arguments/TimeArgument.hpp"
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
    effectNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(effectNode,
        support::makeMetadata("Gives or removes status effects from players.",
            "/effect (give|clear) <targets> <effect> [<seconds>] [<amplifier>] [<hideParticles>]",
            2,
            {},
            true));

    // /effect give <player> <effect> [<seconds>] [<amplifier>] [<hideParticles>]
    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");

    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());

    // 效果类型节点：原版是 ResourceArgument（mob_effect 注册表，命令树里叫 resource）。
    auto effectNameNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "effect", std::make_shared<ResourceArgumentType>("minecraft:mob_effect", "resource"));

    // 可选参数。原版 /effect give 的顺序是 seconds|infinite → amplifier → hideParticles，
    // 秒数参数叫 seconds、类型是 integer（单位 tick）。
    auto secondsNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "seconds", IntegerArgumentType::integer(1, 1000000));
    secondsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _giveEffect(ctx);
    });

    // infinite（不限时长）与 seconds 并列。
    auto infiniteNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("infinite");
    infiniteNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _giveEffect(ctx);
    });

    // 原版 amplifier 是 0..255 的 integer（放大等级上限）。
    auto amplifierNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "amplifier", IntegerArgumentType::integer(0, 255));
    amplifierNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _giveEffect(ctx);
    });
    // infinite 是 effect 参数的直接子节点（与 seconds 并列），不是 seconds 的子节点。
    effectNameNode->addChild(infiniteNode);
    secondsNode->addChild(amplifierNode);

    auto hideParticlesNode =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("hideParticles", BoolArgumentType::boolArg());
    hideParticlesNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _giveEffect(ctx);
    });

    // 默认使用效果名称
    effectNameNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _giveEffect(ctx);
    });

    amplifierNode->addChild(hideParticlesNode);
    auto infiniteAmplifier = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "amplifier", IntegerArgumentType::integer(0, 255));
    infiniteAmplifier->setCommand(amplifierNode->getCommand());
    auto infiniteParticles =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, bool>>("hideParticles", BoolArgumentType::boolArg());
    infiniteParticles->setCommand(hideParticlesNode->getCommand());
    infiniteAmplifier->addChild(infiniteParticles);
    infiniteNode->addChild(infiniteAmplifier);
    effectNameNode->addChild(secondsNode);
    playerNode->addChild(effectNameNode);
    giveNode->addChild(playerNode);

    // /effect clear <player> [<effect>]：clear 本身可执行（不带目标时对命令源自身生效）。
    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _clearAllEffects(ctx);
    });

    auto clearPlayerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::entities());
    clearPlayerNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _clearAllEffects(ctx);
    });

    auto clearEffectNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "effect", std::make_shared<ResourceArgumentType>("minecraft:mob_effect", "resource"));
    clearEffectNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _clearSpecificEffect(ctx);
    });

    clearPlayerNode->addChild(clearEffectNode);
    clearNode->addChild(clearPlayerNode);

    effectNode->addChild(giveNode);
    effectNode->addChild(clearNode);
    dispatcher.registerCommand(effectNode);
}

i32 EffectCommand::_giveEffect(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("targets");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const std::string effectName = context.getArgument<ResourceLocation>("effect").toString();

    auto effectTypeOpt = support::tryParseEffectType(effectName);
    if (!effectTypeOpt.has_value()) {
        source.sendError("Unknown effect: " + effectName);
        return 0;
    }
    entity::effect::EffectType effectType = effectTypeOpt.value();

    // 获取可选参数
    // 时间是 tick（原版 TimeArgument 的单位是 s/d/t）；默认 30 秒。
    i32 durationTicks = 600;
    i32 amplifier = 0; // 默认等级0
    bool hideParticles = false;

    if (context.hasArgument("infinite")) {
        durationTicks = -1;
    } else if (context.hasArgument("seconds")) {
        durationTicks = context.getArgument<i32>("seconds");
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
        entity::effect::EffectInstance effect(effectType, durationTicks, amplifier, false, !hideParticles);
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

    // 不带 targets 时对命令源自身生效（原版 clear 的隐式目标是 @s）。
    std::vector<PlayerId> playerIds;
    if (context.hasArgument("targets")) {
        playerIds = support::resolvePlayerIds(source, context.getArgument<EntitySelector>("targets"));
    } else if (source.isPlayer()) {
        playerIds.push_back(source.playerId());
    }

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

    const auto& selector = context.getArgument<EntitySelector>("targets");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const std::string effectName = context.getArgument<ResourceLocation>("effect").toString();

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
