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

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ResourceArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/command/errors/CommandErrors.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/damage/tag/DamageTypeTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/NumberParsing.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/text/ComponentFactory.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"

#include <memory>
#include <vector>

namespace mc::command {

namespace {

/**
 * @brief `/damage` 的伤害类型参数。
 *
 * 原版 ResourceArgument 在解析期就查 `minecraft:damage_type` 注册表，未知类型是语法
 * 错误（`argument.resource.not_found`）；本项目此前只在执行期忽略类型，未知 id 会被
 * 当成通用伤害静默执行。
 */
class DamageTypeArgumentType final : public ArgumentType<ResourceLocation> {
public:
    [[nodiscard]] Result<ResourceLocation, CommandError> parse(StringReader& reader) override
    {
        auto parsed = readResourceLocationId(reader);
        if (parsed.failed()) {
            return parsed;
        }

        const ResourceLocation id = parsed.value();
        if (!DamageTypeNames::fromResourceLocation(id).has_value()) {
            auto message = std::make_shared<::mc::text::TranslationTextComponent>("argument.resource.not_found");
            message->addParam(::mc::text::literal(id.toString()));
            message->addParam(::mc::text::literal("minecraft:damage_type"));
            return CommandError(CommandErrorType::EntityNotFound,
                std::shared_ptr<const ::mc::text::ITextComponent>(std::move(message)),
                reader.getCursor());
        }
        return id;
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "resource";
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"registry", "minecraft:damage_type"}};
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:generic", "minecraft:magic", "minecraft:generic_kill"};
    }
};

} // namespace

void DamageCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    // 原版形状：/damage <target> <amount> [<damageType> [at <location>|by <entity> [from <cause>]]]。
    auto damageNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("damage");
    damageNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        damageNode, support::makeMetadata("Damages entities.", "/damage <target> <amount>", 2, {}, true));

    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());
    auto amountArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("amount", FloatArgumentType::floatArg(0.0f));
    amountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _damage(ctx);
    });

    // /damage <target> <amount> <damageType> [at <location>|by <entity> [from <cause>]]
    auto damageTypeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
        "damageType", std::make_shared<DamageTypeArgumentType>());
    damageTypeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _damage(ctx);
    });

    auto atNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("at");
    auto atLocationArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "location", Vec3ArgumentType::vec3());
    atLocationArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _damage(ctx);
    });
    atNode->addChild(atLocationArg);

    auto byNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("by");
    auto byEntityArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "entity", EntityArgumentType::entity());
    byEntityArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _damage(ctx);
    });
    auto fromNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("from");
    auto fromCauseArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "cause", EntityArgumentType::entity());
    fromCauseArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _damage(ctx);
    });
    fromNode->addChild(fromCauseArg);
    byEntityArg->addChild(fromNode);
    byNode->addChild(byEntityArg);

    damageTypeArg->addChild(atNode);
    damageTypeArg->addChild(byNode);

    amountArg->addChild(damageTypeArg);
    targetArg->addChild(amountArg);
    damageNode->addChild(targetArg);

    dispatcher.registerCommand(damageNode);
}

i32 DamageCommand::_damage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const f32 amount = context.getArgument<f32>("amount");

    Entity* target = support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("target"));
    if (target == nullptr) {
        source.sendFailure(::mc::text::translatable("argument.entity.notfound.entity"));
        return 0;
    }

    // 构造伤害来源：原版默认是 generic；给出类型后按 at/by[/from] 依次补齐位置与实体。
    std::unique_ptr<DamageSource> damageSource = std::make_unique<EnvironmentalDamage>(DamageType::Generic);
    if (context.hasArgument("damageType")) {
        const ResourceLocation typeId = context.getArgument<ResourceLocation>("damageType");
        const auto damageType = DamageTypeNames::fromResourceLocation(typeId);
        // 解析期已按注册表校验，正常不会缺失；缺失说明注册表与解析器不一致。
        MC_ASSERT_RELEASE(damageType.has_value());

        if (context.hasArgument("entity")) {
            Entity* attacker =
                support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("entity"));
            MC_ASSERT_RELEASE(attacker != nullptr);
            if (context.hasArgument("cause")) {
                Entity* cause =
                    support::EntityResolver::resolveSingle(source, context.getArgument<EntitySelector>("cause"));
                MC_ASSERT_RELEASE(cause != nullptr);
                damageSource = std::make_unique<IndirectEntityDamageSource>(*damageType, attacker, cause, false);
            } else {
                damageSource = std::make_unique<EntityDamageSource>(*damageType, attacker);
            }
        } else if (context.hasArgument("location")) {
            const Vector3d position = Vec3ArgumentType::getVec3(context, "location", source);
            damageSource = std::make_unique<EnvironmentalDamage>(*damageType,
                math::Vector3f(
                    static_cast<f32>(position.x), static_cast<f32>(position.y), static_cast<f32>(position.z)));
        } else {
            damageSource = std::make_unique<EnvironmentalDamage>(*damageType);
        }
    }

    if (!target->hurt(*damageSource, amount)) {
        // 创造模式玩家、盔甲架等目标会拒绝该类伤害，原版同样报「invulnerable」。
        source.sendFailure(::mc::text::translatable("commands.damage.invulnerable"));
        return 0;
    }

    std::vector<std::unique_ptr<::mc::text::ITextComponent>> params;
    params.push_back(::mc::text::literal(util::javaFloatToString(amount)));
    params.push_back(target->getDisplayName());
    source.sendSuccess(::mc::text::translatable("commands.damage.success", std::move(params)), true);
    return 1;
}

} // namespace mc::command
