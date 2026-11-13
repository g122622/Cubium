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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp" // LivingEntity::addEffect/hasEffect/removeEffect（实体层效果）
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EffectResolver.hpp"
#include "server/command/support/EntityResolver.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace command {
namespace {

/**
 * @brief 效果显示名（翻译组件）。
 *
 * 原版用 `MobEffect.getDisplayName()`，即 `effect.<ns>.<path>` 翻译键；客户端按自己的语言
 * 渲染成 "Speed" 之类的名称，不能在这里查表硬编码英文。
 */
[[nodiscard]] std::unique_ptr<text::ITextComponent> _effectDisplayName(const ResourceLocation& effectId)
{
    return ::mc::text::translatable("effect." + effectId.namespace_() + "." + effectId.path());
}

/**
 * @brief 把实体集合中的有效目标（活体实体）挑出来。
 *
 * `/effect` 的目标参数是 `entities()`，只有 LivingEntity 才能承受效果；非活体目标
 * （掉落物、盔甲架等）在循环里被静默跳过，这与原版一致。
 */
[[nodiscard]] std::vector<LivingEntity*> _livingTargets(const std::vector<Entity*>& entities)
{
    std::vector<LivingEntity*> livingTargets;
    for (Entity* entity : entities) {
        if (auto* living = dynamic_cast<LivingEntity*>(entity); living != nullptr) {
            livingTargets.push_back(living);
        }
    }
    return livingTargets;
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
    // 秒数参数叫 seconds、类型是 integer（单位秒，非瞬发效果执行时换算成 20 倍 tick）。
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
    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    const ResourceLocation effectId = context.getArgument<ResourceLocation>("effect");
    auto effectTypeOpt = support::tryParseEffectType(effectId.toString());
    if (!effectTypeOpt.has_value()) {
        source.sendFailure(::mc::text::translatable("commands.effect.give.failed"));
        return 0;
    }
    const entity::effect::EffectType effectType = effectTypeOpt.value();

    // 时长换算：瞬发效果把 seconds 直接当 tick 用；infinite 传 -1；其余按秒 ×20。
    // 未给出 seconds 时瞬发效果 1 tick、普通效果 600 tick。
    const bool instant = entity::effect::isInstantEffect(effectType);
    i32 durationTicks = instant ? 1 : 600;
    if (context.hasArgument("infinite")) {
        durationTicks = -1;
    } else if (context.hasArgument("seconds")) {
        const i32 seconds = context.getArgument<i32>("seconds");
        durationTicks = instant ? seconds : seconds * 20;
    }

    i32 amplifier = 0;
    bool hideParticles = false;
    if (context.hasArgument("amplifier")) {
        amplifier = context.getArgument<i32>("amplifier");
    }
    if (context.hasArgument("hideParticles")) {
        hideParticles = context.getArgument<bool>("hideParticles");
    }

    i32 successCount = 0;
    for (LivingEntity* target : _livingTargets(entities)) {
        // addEffect 在目标免疫或已有更强效果时返回 false，此时该目标不计入成功数。
        entity::effect::EffectInstance effect(effectType, durationTicks, amplifier, false, !hideParticles, true);
        if (target->addEffect(std::move(effect))) {
            successCount++;
        }
    }

    if (successCount == 0) {
        source.sendFailure(::mc::text::translatable("commands.effect.give.failed"));
        return 0;
    }

    auto effectName = _effectDisplayName(effectId);
    if (entities.size() == 1) {
        std::vector<std::unique_ptr<text::ITextComponent>> params;
        params.push_back(std::move(effectName));
        params.push_back(entities.front()->getDisplayName());
        source.sendSuccess(::mc::text::translatable("commands.effect.give.success.single", std::move(params)), true);
    } else {
        std::vector<std::unique_ptr<text::ITextComponent>> params;
        params.push_back(std::move(effectName));
        params.push_back(::mc::text::literal(std::to_string(entities.size())));
        source.sendSuccess(::mc::text::translatable("commands.effect.give.success.multiple", std::move(params)), true);
    }

    return successCount;
}

i32 EffectCommand::_clearAllEffects(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 不带 targets 时对命令源自身生效（原版 clear 的隐式目标是执行实体）。
    std::vector<Entity*> entities;
    if (context.hasArgument("targets")) {
        entities = support::EntityResolver::resolve(source, context.getArgument<EntitySelector>("targets"));
    } else if (source.entity() != nullptr) {
        entities.push_back(source.entity());
    }

    if (entities.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    i32 successCount = 0;
    for (LivingEntity* target : _livingTargets(entities)) {
        if (!target->effectManager().getAllEffects().empty()) {
            target->removeAllEffects();
            successCount++;
        }
    }

    if (successCount == 0) {
        source.sendFailure(::mc::text::translatable("commands.effect.clear.everything.failed"));
        return 0;
    }

    if (entities.size() == 1) {
        source.sendSuccess(::mc::text::translatable(
                               "commands.effect.clear.everything.success.single", entities.front()->getDisplayName()),
            true);
    } else {
        source.sendSuccess(::mc::text::translatable(
                               "commands.effect.clear.everything.success.multiple", static_cast<i32>(entities.size())),
            true);
    }

    return successCount;
}

i32 EffectCommand::_clearSpecificEffect(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const auto& selector = context.getArgument<EntitySelector>("targets");
    auto entities = support::EntityResolver::resolve(source, selector);
    if (entities.empty()) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    const ResourceLocation effectId = context.getArgument<ResourceLocation>("effect");
    auto effectTypeOpt = support::tryParseEffectType(effectId.toString());
    if (!effectTypeOpt.has_value()) {
        source.sendFailure(::mc::text::translatable("commands.effect.clear.specific.failed"));
        return 0;
    }
    const entity::effect::EffectType effectType = effectTypeOpt.value();

    i32 successCount = 0;
    for (LivingEntity* target : _livingTargets(entities)) {
        if (target->hasEffect(effectType)) {
            target->removeEffect(effectType);
            successCount++;
        }
    }

    if (successCount == 0) {
        source.sendFailure(::mc::text::translatable("commands.effect.clear.specific.failed"));
        return 0;
    }

    if (entities.size() == 1) {
        std::vector<std::unique_ptr<text::ITextComponent>> params;
        params.push_back(_effectDisplayName(effectId));
        params.push_back(entities.front()->getDisplayName());
        source.sendSuccess(
            ::mc::text::translatable("commands.effect.clear.specific.success.single", std::move(params)), true);
    } else {
        std::vector<std::unique_ptr<text::ITextComponent>> params;
        params.push_back(_effectDisplayName(effectId));
        params.push_back(::mc::text::literal(std::to_string(entities.size())));
        source.sendSuccess(
            ::mc::text::translatable("commands.effect.clear.specific.success.multiple", std::move(params)), true);
    }

    return successCount;
}

} // namespace command
} // namespace mc
