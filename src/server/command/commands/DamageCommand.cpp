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

#include "DamageCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"

#include <memory>
#include <vector>

namespace mc::command {

void DamageCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 原版形状：/damage <target> <amount>（以及可选的伤害类型/来源分支，尚未实现）。
    auto damageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("damage");
    damageNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        damageNode, support::makeMetadata("Damages entities.", "/damage <target> <amount>", 2, {}, true));

    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto amountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>(
        "amount", FloatArgumentType::floatArg());
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _damage(ctx); });
    targetArg->addChild(amountArg);
    damageNode->addChild(targetArg);

    dispatcher.registerCommand(damageNode);
}

i32 DamageCommand::_damage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const f32 amount = context.getArgument<f32>("amount");
    if (amount <= 0.0f) {
        source.sendError("Damage amount must be positive");
        return 0;
    }

    auto targets = support::EntityResolver::resolve(source, context.getArgument<EntitySelector>("target"));
    i32 damaged = 0;
    for (Entity* entity : targets) {
        auto* living = dynamic_cast<LivingEntity*>(entity);
        if (living == nullptr) {
            continue;
        }
        // 原版以命令源实体为伤害来源；无实体（控制台/命令方块）时退化为通用伤害。
        Entity* attacker = source.entity();
        if (attacker != nullptr && attacker != entity) {
            EntityDamageSource damageSource(DamageType::MobAttack, attacker);
            if (living->hurt(damageSource, amount)) {
                ++damaged;
            }
        } else {
            auto damageSource = DamageSources::generic();
            if (living->hurt(damageSource, amount)) {
                ++damaged;
            }
        }
    }

    if (damaged == 0) {
        source.sendError("No entity was damaged");
        return 0;
    }
    source.sendMessage("Applied damage to " + std::to_string(damaged) + " entities");
    return damaged;
}

} // namespace mc::command
