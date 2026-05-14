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

#include "AttributeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <sstream>
#include <unordered_set>

namespace mc {
namespace command {

void AttributeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto attributeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("attribute");
    attributeNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(attributeNode,
        support::makeMetadata("Gets, sets, or resets an entity attribute.",
            "/attribute <target> <attribute> (get|set|base|modifier)",
            2,
            {},
            true));

    // /attribute <target> <attribute>
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());

    auto attributeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "attribute", StringArgumentType::string());

    // /attribute <target> <attribute> get
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    getNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return getAttribute(ctx); });

    // /attribute <target> <attribute> set <value>
    auto setNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto valueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("value", FloatArgumentType::floatArg());
    valueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return setAttributeBase(ctx); });
    setNode->addChild(valueArg);

    attributeArg->addChild(getNode);
    attributeArg->addChild(setNode);
    targetArg->addChild(attributeArg);
    attributeNode->addChild(targetArg);

    dispatcher.registerCommand(attributeNode);
}

i32 AttributeCommand::getAttribute(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    auto* world = source.world();
    if (world == nullptr) {
        source.sendMessage("World not available");
        return 0;
    }

    // 获取目标实体
    const EntitySelector& selector = context.getArgument<EntitySelector>("target");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No matching entities were found");
        return 0;
    }

    const std::string attrName = context.getArgument<std::string>("attribute");
    std::string normalizedAttrName = normalizeAttributeName(attrName);

    if (playerIds.size() > 1) {
        source.sendMessage("Only one entity is allowed, but the provided selector allows more");
        return 0;
    }

    PlayerId playerId = playerIds[0];

    // 通过 ServerPlayerEntityManager 获取玩家实体
    Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
    if (player == nullptr) {
        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            source.sendMessage("Player not found");
            return 0;
        }
        source.sendMessage("Player entity not available");
        return 0;
    }

    // 检查属性是否为已知的标准属性
    if (!isKnownAttribute(normalizedAttrName)) {
        source.sendError("Unknown attribute: " + attrName);
        return 0;
    }

    // 从实体属性系统获取实际值
    f64 baseValue = player->attributes().getBaseValue(normalizedAttrName);
    f64 currentValue = player->attributes().getValue(normalizedAttrName, baseValue);

    std::ostringstream ss;
    ss << normalizedAttrName << " for " << player->username() << ": ";
    ss << currentValue << " (base: " << baseValue << ")";

    source.sendMessage(ss.str());
    return 1;
}

i32 AttributeCommand::setAttributeBase(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    auto* world = source.world();
    if (world == nullptr) {
        source.sendMessage("World not available");
        return 0;
    }

    // 获取目标实体
    const EntitySelector& selector = context.getArgument<EntitySelector>("target");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No matching entities were found");
        return 0;
    }

    if (playerIds.size() > 1) {
        source.sendMessage("Only one entity is allowed, but the provided selector allows more");
        return 0;
    }

    const std::string attrName = context.getArgument<std::string>("attribute");
    std::string normalizedAttrName = normalizeAttributeName(attrName);
    f32 value = context.getArgument<f32>("value");

    PlayerId playerId = playerIds[0];

    // 验证属性范围
    if (!isKnownAttribute(normalizedAttrName)) {
        source.sendError("Unknown attribute: " + attrName);
        return 0;
    }

    // 检查值是否在有效范围内
    auto [minVal, maxVal] = getAttributeRange(normalizedAttrName);
    if (value < minVal || value > maxVal) {
        std::ostringstream ss;
        ss << "Value " << value << " is out of range [" << minVal << ", " << maxVal << "]";
        source.sendMessage(ss.str());
        return 0;
    }

    // 通过 ServerPlayerEntityManager 获取玩家实体
    Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
    if (player == nullptr) {
        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            source.sendMessage("Player not found");
            return 0;
        }
        source.sendMessage("Player entity not available");
        return 0;
    }

    // 设置属性基础值
    player->attributes().setBaseValue(normalizedAttrName, static_cast<f64>(value));

    std::ostringstream ss;
    ss << "Set base value of " << normalizedAttrName << " to " << value << " for " << player->username();
    source.sendMessage(ss.str());

    return 1;
}

std::string AttributeCommand::normalizeAttributeName(const std::string& name)
{
    std::string normalized = name;

    // 移除 minecraft: 前缀
    if (normalized.find("minecraft:") == 0) {
        normalized = normalized.substr(10);
    }

    // 添加 generic. 前缀（如果需要）
    if (normalized.find("generic.") != 0 && normalized.find("horse.") != 0) {
        // 常见的通用属性需要 generic. 前缀
        static const std::vector<std::string> genericAttrs = {"max_health",
            "follow_range",
            "knockback_resistance",
            "movement_speed",
            "flying_speed",
            "attack_damage",
            "attack_knockback",
            "attack_speed",
            "armor",
            "armor_toughness",
            "luck",
            "max_absorption",
            "breath_max",
            "jump_boost"};

        for (const auto& attr : genericAttrs) {
            if (normalized == attr) {
                normalized = "generic." + attr;
                break;
            }
        }
    }

    return normalized;
}

bool AttributeCommand::isKnownAttribute(const std::string& name) noexcept
{
    using namespace entity::attribute;

    static const std::unordered_set<std::string> knownAttrs = {
        Attributes::MAX_HEALTH,
        Attributes::FOLLOW_RANGE,
        Attributes::KNOCKBACK_RESISTANCE,
        Attributes::MOVEMENT_SPEED,
        Attributes::FLYING_SPEED,
        Attributes::ATTACK_DAMAGE,
        Attributes::ATTACK_KNOCKBACK,
        Attributes::ATTACK_SPEED,
        Attributes::ARMOR,
        Attributes::ARMOR_TOUGHNESS,
        Attributes::LUCK,
        Attributes::MAX_ABSORPTION,
        Attributes::BREATH_MAX,
        Attributes::JUMP_BOOST,
        Attributes::HORSE_JUMP_STRENGTH,
    };

    return knownAttrs.count(name) > 0;
}

f64 AttributeCommand::getAttributeDefaultValue(const std::string& name) noexcept
{
    using namespace entity::attribute;

    static const std::unordered_map<std::string, f64> defaultValues = {
        {Attributes::MAX_HEALTH, 20.0},
        {Attributes::FOLLOW_RANGE, 32.0},
        {Attributes::KNOCKBACK_RESISTANCE, 0.0},
        {Attributes::MOVEMENT_SPEED, 0.1}, // 玩家默认
        {Attributes::FLYING_SPEED, 0.4},
        {Attributes::ATTACK_DAMAGE, 2.0},
        {Attributes::ATTACK_KNOCKBACK, 0.0},
        {Attributes::ATTACK_SPEED, 4.0},
        {Attributes::ARMOR, 0.0},
        {Attributes::ARMOR_TOUGHNESS, 0.0},
        {Attributes::LUCK, 0.0},
        {Attributes::MAX_ABSORPTION, 0.0},
        {Attributes::BREATH_MAX, 300.0},
        {Attributes::JUMP_BOOST, 0.42},
        {Attributes::HORSE_JUMP_STRENGTH, 0.7},
    };

    auto it = defaultValues.find(name);
    if (it != defaultValues.end()) {
        return it->second;
    }
    return 0.0;
}

std::pair<f64, f64> AttributeCommand::getAttributeRange(const std::string& name) noexcept
{
    using namespace entity::attribute;

    static const std::unordered_map<std::string, std::pair<f64, f64>> ranges = {
        {Attributes::MAX_HEALTH, {0.0, 1024.0}},
        {Attributes::FOLLOW_RANGE, {0.0, 2048.0}},
        {Attributes::KNOCKBACK_RESISTANCE, {0.0, 1.0}},
        {Attributes::MOVEMENT_SPEED, {0.0, 1024.0}},
        {Attributes::FLYING_SPEED, {0.0, 1024.0}},
        {Attributes::ATTACK_DAMAGE, {0.0, 2048.0}},
        {Attributes::ATTACK_KNOCKBACK, {0.0, 5.0}},
        {Attributes::ATTACK_SPEED, {0.0, 1024.0}},
        {Attributes::ARMOR, {0.0, 30.0}},
        {Attributes::ARMOR_TOUGHNESS, {0.0, 20.0}},
        {Attributes::LUCK, {-1024.0, 1024.0}},
        {Attributes::MAX_ABSORPTION, {0.0, 2048.0}},
        {Attributes::BREATH_MAX, {0.0, 6000.0}},
        {Attributes::JUMP_BOOST, {0.0, 8.0}},
        {Attributes::HORSE_JUMP_STRENGTH, {0.0, 2.0}},
    };

    auto it = ranges.find(name);
    if (it != ranges.end()) {
        return it->second;
    }
    return {0.0, 1024.0}; // 默认范围
}

} // namespace command
} // namespace mc
