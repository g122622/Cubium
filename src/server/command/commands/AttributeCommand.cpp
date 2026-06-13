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
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
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
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/assert/AssertAll.hpp"
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

// ============================================================================
// 实体获取辅助方法
// ============================================================================

bool AttributeCommand::_tryGetLivingEntityWithAttribute(CommandContext<ServerCommandSource>& context,
    ServerCommandSource& source,
    const std::string& attrName,
    Player*& outPlayer,
    entity::attribute::AttributeMap*& outAttrMap)
{
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return false;
    }

    auto* world = source.world();
    if (world == nullptr) {
        source.sendMessage("World not available");
        return false;
    }

    const EntitySelector& selector = context.getArgument<EntitySelector>("target");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No matching entities were found");
        return false;
    }

    if (playerIds.size() > 1) {
        source.sendMessage("Only one entity is allowed, but the provided selector allows more");
        return false;
    }

    PlayerId playerId = playerIds[0];

    // TODO: 当前仅支持 Player 实体（通过 resolvePlayerIds 获取），
    // MC 原版 /attribute 命令支持所有 LivingEntity（僵尸、马等）。
    // 需要扩展实体选择器系统以支持非玩家活体实体后，此处应改为通用的 LivingEntity 获取逻辑。
    // 获取玩家实体
    Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
    if (player == nullptr) {
        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            source.sendMessage("Player not found");
            return false;
        }
        source.sendMessage("Player entity not available");
        return false;
    }

    // Player 继承自 LivingEntity，dynamic_cast 用于未来扩展到非玩家活体实体
    auto* livingEntity = dynamic_cast<LivingEntity*>(player);
    MC_ASSERT_RELEASE(livingEntity != nullptr);

    // 检查属性是否存在
    if (!_isKnownAttribute(attrName)) {
        source.sendError("Unknown attribute: " + attrName);
        return false;
    }

    auto& attrMap = livingEntity->attributes();
    if (!attrMap.hasAttribute(attrName)) {
        source.sendError("Entity " + player->username() + " doesn't have attribute " + attrName);
        return false;
    }

    outPlayer = player;
    outAttrMap = &attrMap;
    return true;
}

bool AttributeCommand::_tryGetAttributeInstance(CommandContext<ServerCommandSource>& context,
    ServerCommandSource& source,
    const std::string& attrName,
    Player*& outPlayer,
    entity::attribute::AttributeInstance*& outInstance)
{
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return false;
    }

    auto* world = source.world();
    if (world == nullptr) {
        source.sendMessage("World not available");
        return false;
    }

    const EntitySelector& selector = context.getArgument<EntitySelector>("target");
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No matching entities were found");
        return false;
    }

    if (playerIds.size() > 1) {
        source.sendMessage("Only one entity is allowed, but the provided selector allows more");
        return false;
    }

    PlayerId playerId = playerIds[0];

    // TODO: 同 _tryGetLivingEntityWithAttribute，当前仅支持 Player 实体，需扩展到所有 LivingEntity。
    // 获取玩家实体
    Player* player = server->playerEntityManager().getPlayerEntity(playerId, *world);
    if (player == nullptr) {
        auto* playerData = server->playerManager().getPlayer(playerId);
        if (playerData == nullptr) {
            source.sendMessage("Player not found");
            return false;
        }
        source.sendMessage("Player entity not available");
        return false;
    }

    // Player 继承自 LivingEntity，dynamic_cast 用于未来扩展到非玩家活体实体
    auto* livingEntity = dynamic_cast<LivingEntity*>(player);
    MC_ASSERT_RELEASE(livingEntity != nullptr);

    // 检查属性是否存在
    if (!_isKnownAttribute(attrName)) {
        source.sendError("Unknown attribute: " + attrName);
        return false;
    }

    auto& attrMap = livingEntity->attributes();
    if (!attrMap.hasAttribute(attrName)) {
        source.sendError("Entity " + player->username() + " doesn't have attribute " + attrName);
        return false;
    }

    auto* instance = attrMap.getInstance(attrName);
    if (instance == nullptr) {
        source.sendError("Entity " + player->username() + " doesn't have attribute instance for " + attrName);
        return false;
    }

    outPlayer = player;
    outInstance = instance;
    return true;
}

// ============================================================================
// 命令注册
// ============================================================================

void AttributeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto attributeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("attribute");
    attributeNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(attributeNode,
        support::makeMetadata("Gets, sets, or resets an entity attribute.",
            "/attribute <target> <attribute> (get|base|modifier)",
            2,
            {},
            true));

    // /attribute <target>
    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target", EntityArgumentType::entity());

    // /attribute <target> <attribute>
    auto attributeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "attribute", StringArgumentType::string());

    // ============================================================================
    // /attribute <target> <attribute> get [scale]
    // ============================================================================
    auto getNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    getNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getAttribute(ctx); });

    auto getScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("scale", FloatArgumentType::floatArg());
    getScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getAttributeWithScale(ctx); });

    getNode->addChild(getScaleArg);

    // ============================================================================
    // /attribute <target> <attribute> base
    // ============================================================================
    auto baseNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("base");

    // /attribute <target> <attribute> base set <value>
    auto baseSetNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("set");
    auto baseSetValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("value", FloatArgumentType::floatArg());
    baseSetValueArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setBaseValue(ctx); });
    baseSetNode->addChild(baseSetValueArg);

    // /attribute <target> <attribute> base get [scale]
    auto baseGetNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");
    baseGetNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getBaseValue(ctx); });

    auto baseGetScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("scale", FloatArgumentType::floatArg());
    baseGetScaleArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getBaseValueWithScale(ctx); });
    baseGetNode->addChild(baseGetScaleArg);

    // /attribute <target> <attribute> base reset
    auto baseResetNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reset");
    baseResetNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _resetBaseValue(ctx); });

    baseNode->addChild(baseSetNode);
    baseNode->addChild(baseGetNode);
    baseNode->addChild(baseResetNode);

    // ============================================================================
    // /attribute <target> <attribute> modifier
    // ============================================================================
    auto modifierNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("modifier");

    // /attribute <target> <attribute> modifier add <id> <value> (add_value|add_multiplied_base|add_multiplied_total)
    auto modifierAddNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");

    auto modifierIdArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());

    auto modifierValueArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("value", FloatArgumentType::floatArg());

    auto addValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add_value");
    addValueNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _addModifierAddValue(ctx); });

    auto addMultipliedBaseNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add_multiplied_base");
    addMultipliedBaseNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _addModifierMultiplyBase(ctx); });

    auto addMultipliedTotalNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add_multiplied_total");
    addMultipliedTotalNode->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _addModifierMultiplyTotal(ctx); });

    modifierValueArg->addChild(addValueNode);
    modifierValueArg->addChild(addMultipliedBaseNode);
    modifierValueArg->addChild(addMultipliedTotalNode);
    modifierIdArg->addChild(modifierValueArg);
    modifierAddNode->addChild(modifierIdArg);

    // /attribute <target> <attribute> modifier remove <id>
    auto modifierRemoveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");

    auto removeIdArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());
    removeIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _removeModifier(ctx); });
    modifierRemoveNode->addChild(removeIdArg);

    // /attribute <target> <attribute> modifier value get <id> [scale]
    auto modifierValueNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("value");

    auto modifierValueGetNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("get");

    auto modifierValueGetIdArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("id", StringArgumentType::string());
    modifierValueGetIdArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _getModifierValue(ctx); });

    auto modifierValueGetScaleArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, f32>>("scale", FloatArgumentType::floatArg());
    modifierValueGetScaleArg->setCommand(
        [](CommandContext<ServerCommandSource>& ctx) { return _getModifierValueWithScale(ctx); });

    modifierValueGetIdArg->addChild(modifierValueGetScaleArg);
    modifierValueGetNode->addChild(modifierValueGetIdArg);
    modifierNode->addChild(modifierValueNode);

    modifierNode->addChild(modifierAddNode);
    modifierNode->addChild(modifierRemoveNode);
    modifierNode->addChild(modifierValueNode);

    // 组装命令树
    attributeArg->addChild(getNode);
    attributeArg->addChild(baseNode);
    attributeArg->addChild(modifierNode);
    targetArg->addChild(attributeArg);
    attributeNode->addChild(targetArg);

    dispatcher.registerCommand(attributeNode);
}

// ============================================================================
// get - 获取属性最终值（含修饰符）
// ============================================================================

i32 AttributeCommand::_getAttribute(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));

    Player* player = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, player, attrMap)) {
        return 0;
    }

    f64 value = attrMap->getValue(attrName);
    i32 result = static_cast<i32>(value);

    std::ostringstream ss;
    ss << attrName << " for " << player->username() << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

i32 AttributeCommand::_getAttributeWithScale(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    f32 scale = context.getArgument<f32>("scale");

    Player* player = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, player, attrMap)) {
        return 0;
    }

    f64 value = attrMap->getValue(attrName);
    i32 result = static_cast<i32>(value * static_cast<f64>(scale));

    std::ostringstream ss;
    ss << attrName << " for " << player->username() << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

// ============================================================================
// base get - 获取属性基础值（不含修饰符）
// ============================================================================

i32 AttributeCommand::_getBaseValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));

    Player* player = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, player, attrMap)) {
        return 0;
    }

    f64 baseValue = attrMap->getBaseValue(attrName);
    i32 result = static_cast<i32>(baseValue);

    std::ostringstream ss;
    ss << "Base value of " << attrName << " for " << player->username() << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

i32 AttributeCommand::_getBaseValueWithScale(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    f32 scale = context.getArgument<f32>("scale");

    Player* player = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, player, attrMap)) {
        return 0;
    }

    f64 baseValue = attrMap->getBaseValue(attrName);
    i32 result = static_cast<i32>(baseValue * static_cast<f64>(scale));

    std::ostringstream ss;
    ss << "Base value of " << attrName << " for " << player->username() << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

// ============================================================================
// base set - 设置属性基础值
// ============================================================================

i32 AttributeCommand::_setBaseValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    f32 value = context.getArgument<f32>("value");

    Player* player = nullptr;
    entity::attribute::AttributeInstance* instance = nullptr;
    if (!_tryGetAttributeInstance(context, source, attrName, player, instance)) {
        return 0;
    }

    // 检查值是否在有效范围内
    auto [minVal, maxVal] = _getAttributeRange(attrName);
    if (static_cast<f64>(value) < minVal || static_cast<f64>(value) > maxVal) {
        std::ostringstream ss;
        ss << "Value " << value << " is out of range [" << minVal << ", " << maxVal << "]";
        source.sendMessage(ss.str());
        return 0;
    }

    instance->setBaseValue(static_cast<f64>(value));

    std::ostringstream ss;
    ss << "Set base value of " << attrName << " to " << value << " for " << player->username();
    source.sendMessage(ss.str());

    return 1;
}

// ============================================================================
// base reset - 重置属性基础值为默认值
// ============================================================================

i32 AttributeCommand::_resetBaseValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));

    Player* player = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, player, attrMap)) {
        return 0;
    }

    if (!attrMap->resetBaseValue(attrName)) {
        source.sendError("Entity " + player->username() + " doesn't have attribute " + attrName);
        return 0;
    }

    f64 resetValue = attrMap->getBaseValue(attrName);
    std::ostringstream ss;
    ss << "Reset base value of " << attrName << " to " << resetValue << " for " << player->username();
    source.sendMessage(ss.str());

    return 1;
}

// ============================================================================
// modifier add - 添加属性修饰符
// ============================================================================

i32 AttributeCommand::_addModifierAddValue(CommandContext<ServerCommandSource>& context)
{
    return _addModifierImpl(context, entity::attribute::Operation::Addition);
}

i32 AttributeCommand::_addModifierMultiplyBase(CommandContext<ServerCommandSource>& context)
{
    return _addModifierImpl(context, entity::attribute::Operation::MultiplyBase);
}

i32 AttributeCommand::_addModifierMultiplyTotal(CommandContext<ServerCommandSource>& context)
{
    return _addModifierImpl(context, entity::attribute::Operation::MultiplyTotal);
}

i32 AttributeCommand::_addModifierImpl(
    CommandContext<ServerCommandSource>& context, entity::attribute::Operation operation)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    const std::string modifierId = context.getArgument<std::string>("id");
    f32 value = context.getArgument<f32>("value");

    Player* player = nullptr;
    entity::attribute::AttributeInstance* instance = nullptr;
    if (!_tryGetAttributeInstance(context, source, attrName, player, instance)) {
        return 0;
    }

    // 检查修饰符是否已存在
    if (instance->hasModifier(modifierId)) {
        source.sendError(
            "Modifier " + modifierId + " already exists on attribute " + attrName + " for " + player->username());
        return 0;
    }

    // 创建并添加修饰符
    entity::attribute::AttributeModifier modifier(modifierId, modifierId, static_cast<f64>(value), operation);
    instance->addModifier(modifier);

    // 操作类型的可读名称
    const char* operationName = "add_value";
    if (operation == entity::attribute::Operation::MultiplyBase) {
        operationName = "add_multiplied_base";
    } else if (operation == entity::attribute::Operation::MultiplyTotal) {
        operationName = "add_multiplied_total";
    }

    std::ostringstream ss;
    ss << "Added modifier " << modifierId << " with value " << value << " and operation " << operationName;
    ss << " to attribute " << attrName << " for " << player->username();
    source.sendMessage(ss.str());

    return 1;
}

// ============================================================================
// modifier remove - 移除属性修饰符
// ============================================================================

i32 AttributeCommand::_removeModifier(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    const std::string modifierId = context.getArgument<std::string>("id");

    Player* player = nullptr;
    entity::attribute::AttributeInstance* instance = nullptr;
    if (!_tryGetAttributeInstance(context, source, attrName, player, instance)) {
        return 0;
    }

    if (!instance->removeModifier(modifierId)) {
        source.sendError(
            "Modifier " + modifierId + " doesn't exist on attribute " + attrName + " for " + player->username());
        return 0;
    }

    std::ostringstream ss;
    ss << "Removed modifier " << modifierId << " from attribute " << attrName << " for " << player->username();
    source.sendMessage(ss.str());

    return 1;
}

// ============================================================================
// modifier value get - 获取修饰符值
// ============================================================================

i32 AttributeCommand::_getModifierValue(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    const std::string modifierId = context.getArgument<std::string>("id");

    Player* player = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, player, attrMap)) {
        return 0;
    }

    if (!attrMap->hasModifier(attrName, modifierId)) {
        source.sendError(
            "Modifier " + modifierId + " doesn't exist on attribute " + attrName + " for " + player->username());
        return 0;
    }

    f64 modifierValue = attrMap->getModifierValue(attrName, modifierId);
    i32 result = static_cast<i32>(modifierValue);

    std::ostringstream ss;
    ss << "Modifier " << modifierId << " on attribute " << attrName << " for " << player->username() << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

i32 AttributeCommand::_getModifierValueWithScale(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    const std::string modifierId = context.getArgument<std::string>("id");
    f32 scale = context.getArgument<f32>("scale");

    Player* player = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, player, attrMap)) {
        return 0;
    }

    if (!attrMap->hasModifier(attrName, modifierId)) {
        source.sendError(
            "Modifier " + modifierId + " doesn't exist on attribute " + attrName + " for " + player->username());
        return 0;
    }

    f64 modifierValue = attrMap->getModifierValue(attrName, modifierId);
    i32 result = static_cast<i32>(modifierValue * static_cast<f64>(scale));

    std::ostringstream ss;
    ss << "Modifier " << modifierId << " on attribute " << attrName << " for " << player->username() << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string AttributeCommand::_normalizeAttributeName(const std::string& name)
{
    std::string normalized = name;

    // 移除 minecraft: 前缀
    constexpr std::string_view minecraftPrefix = "minecraft:";
    if (normalized.starts_with(minecraftPrefix)) {
        normalized = normalized.substr(minecraftPrefix.size());
    }

    // 添加 generic. 前缀（如果需要）
    if (normalized.find("generic.") != 0 && normalized.find("horse.") != 0 && normalized.find("zombie.") != 0 &&
        normalized.find("forge.") != 0) {
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

// TODO: _isKnownAttribute、_getAttributeDefaultValue、_getAttributeRange 三个方法使用硬编码的
// 属性名和范围值，与 Attributes.hpp 中的工厂函数定义可能不同步。未来应迁移到通过属性
// 注册表（AttributeRegistry）动态查询，避免手动维护两份数据。
bool AttributeCommand::_isKnownAttribute(const std::string& name) noexcept
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
        Attributes::ZOMBIE_SPAWN_REINFORCEMENTS,
        Attributes::ENTITY_GRAVITY,
        Attributes::SWIM_SPEED,
    };

    return knownAttrs.count(name) > 0;
}

f64 AttributeCommand::_getAttributeDefaultValue(const std::string& name) noexcept
{
    using namespace entity::attribute;

    static const std::unordered_map<std::string, f64> defaultValues = {
        {Attributes::MAX_HEALTH, 20.0},
        {Attributes::FOLLOW_RANGE, 32.0},
        {Attributes::KNOCKBACK_RESISTANCE, 0.0},
        {Attributes::MOVEMENT_SPEED, 0.1},
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
        {Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, 0.0},
        {Attributes::ENTITY_GRAVITY, 0.08},
        {Attributes::SWIM_SPEED, 1.0},
    };

    auto it = defaultValues.find(name);
    if (it != defaultValues.end()) {
        return it->second;
    }
    return 0.0;
}

std::pair<f64, f64> AttributeCommand::_getAttributeRange(const std::string& name) noexcept
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
        {Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, {0.0, 1.0}},
        {Attributes::ENTITY_GRAVITY, {-8.0, 8.0}},
        {Attributes::SWIM_SPEED, {0.0, 1024.0}},
    };

    auto it = ranges.find(name);
    if (it != ranges.end()) {
        return it->second;
    }
    return {0.0, 1024.0};
}

} // namespace command
} // namespace mc
