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
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeRegistry.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/EntityResolver.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace mc {
namespace command {

// ============================================================================
// 实体获取辅助方法
// ============================================================================

bool AttributeCommand::_tryGetLivingEntityWithAttribute(CommandContext<ServerCommandSource>& context,
    ServerCommandSource& source,
    const std::string& attrName,
    LivingEntity*& outLivingEntity,
    entity::attribute::AttributeMap*& outAttrMap)
{
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return false;
    }

    // TODO: source.world() 间接调用 dimensionManager()，在测试环境中 BaseTestServer 的
    // dimensionManager() 会抛出 std::logic_error，导致命令无法在简易测试环境中执行。
    // 需要为 AttributeCommand 测试创建完整的 TestServer（参考 EntityResolverTestServer），
    // 或者在 EntityResolver 中增加异常安全保护。
    auto* world = source.world();
    if (world == nullptr) {
        source.sendError("No matching entities were found");
        return false;
    }

    const EntitySelector& selector = context.getArgument<EntitySelector>("target");
    Entity* entity = support::EntityResolver::resolveSingle(source, selector);
    if (entity == nullptr) {
        source.sendError("No matching entities were found");
        return false;
    }

    // 检查实体是否为 LivingEntity（MC 原版：非活体实体不支持属性命令）
    auto* livingEntity = dynamic_cast<LivingEntity*>(entity);
    if (livingEntity == nullptr) {
        source.sendError(_getEntityDisplayName(entity) + " is not a living entity");
        return false;
    }

    // 检查属性是否存在
    if (!_isKnownAttribute(attrName)) {
        source.sendError("Unknown attribute: " + attrName);
        return false;
    }

    auto& attrMap = livingEntity->attributes();
    if (!attrMap.hasAttribute(attrName)) {
        source.sendError(_getEntityDisplayName(livingEntity) + " doesn't have attribute " + attrName);
        return false;
    }

    outLivingEntity = livingEntity;
    outAttrMap = &attrMap;
    return true;
}

bool AttributeCommand::_tryGetAttributeInstance(CommandContext<ServerCommandSource>& context,
    ServerCommandSource& source,
    const std::string& attrName,
    LivingEntity*& outLivingEntity,
    entity::attribute::AttributeInstance*& outInstance)
{
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, outLivingEntity, attrMap)) {
        return false;
    }

    auto* instance = attrMap->getInstance(attrName);
    if (instance == nullptr) {
        source.sendError(_getEntityDisplayName(outLivingEntity) + " doesn't have attribute instance for " + attrName);
        return false;
    }

    outInstance = instance;
    return true;
}

std::string AttributeCommand::_getEntityDisplayName(Entity* entity)
{
    auto* player = dynamic_cast<Player*>(entity);
    if (player != nullptr) {
        return player->username();
    }
    if (entity->hasCustomName()) {
        return entity->customNameText();
    }
    return entity->getTypeId();
}

std::string AttributeCommand::_getEntityDisplayName(LivingEntity* livingEntity)
{
    return _getEntityDisplayName(static_cast<Entity*>(livingEntity));
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
    modifierValueNode->addChild(modifierValueGetNode);

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

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, livingEntity, attrMap)) {
        return 0;
    }

    f64 value = attrMap->getValue(attrName);
    i32 result = static_cast<i32>(value);

    std::ostringstream ss;
    ss << attrName << " for " << _getEntityDisplayName(livingEntity) << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

i32 AttributeCommand::_getAttributeWithScale(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    f32 scale = context.getArgument<f32>("scale");

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, livingEntity, attrMap)) {
        return 0;
    }

    f64 value = attrMap->getValue(attrName);
    i32 result = static_cast<i32>(value * static_cast<f64>(scale));

    std::ostringstream ss;
    ss << attrName << " for " << _getEntityDisplayName(livingEntity) << ": " << result;
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

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, livingEntity, attrMap)) {
        return 0;
    }

    f64 baseValue = attrMap->getBaseValue(attrName);
    i32 result = static_cast<i32>(baseValue);

    std::ostringstream ss;
    ss << "Base value of " << attrName << " for " << _getEntityDisplayName(livingEntity) << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

i32 AttributeCommand::_getBaseValueWithScale(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    f32 scale = context.getArgument<f32>("scale");

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, livingEntity, attrMap)) {
        return 0;
    }

    f64 baseValue = attrMap->getBaseValue(attrName);
    i32 result = static_cast<i32>(baseValue * static_cast<f64>(scale));

    std::ostringstream ss;
    ss << "Base value of " << attrName << " for " << _getEntityDisplayName(livingEntity) << ": " << result;
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

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeInstance* instance = nullptr;
    if (!_tryGetAttributeInstance(context, source, attrName, livingEntity, instance)) {
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
    ss << "Set base value of " << attrName << " to " << value << " for " << _getEntityDisplayName(livingEntity);
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

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, livingEntity, attrMap)) {
        return 0;
    }

    if (!attrMap->resetBaseValue(attrName)) {
        source.sendError(_getEntityDisplayName(livingEntity) + " doesn't have attribute " + attrName);
        return 0;
    }

    f64 resetValue = attrMap->getBaseValue(attrName);
    std::ostringstream ss;
    ss << "Reset base value of " << attrName << " to " << resetValue << " for " << _getEntityDisplayName(livingEntity);
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

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeInstance* instance = nullptr;
    if (!_tryGetAttributeInstance(context, source, attrName, livingEntity, instance)) {
        return 0;
    }

    // 检查修饰符是否已存在
    if (instance->hasModifier(modifierId)) {
        source.sendError("Modifier " + modifierId + " already exists on attribute " + attrName + " for " +
            _getEntityDisplayName(livingEntity));
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
    ss << " to attribute " << attrName << " for " << _getEntityDisplayName(livingEntity);
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

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeInstance* instance = nullptr;
    if (!_tryGetAttributeInstance(context, source, attrName, livingEntity, instance)) {
        return 0;
    }

    if (!instance->removeModifier(modifierId)) {
        source.sendError("Modifier " + modifierId + " doesn't exist on attribute " + attrName + " for " +
            _getEntityDisplayName(livingEntity));
        return 0;
    }

    std::ostringstream ss;
    ss << "Removed modifier " << modifierId << " from attribute " << attrName << " for "
       << _getEntityDisplayName(livingEntity);
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

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, livingEntity, attrMap)) {
        return 0;
    }

    if (!attrMap->hasModifier(attrName, modifierId)) {
        source.sendError("Modifier " + modifierId + " doesn't exist on attribute " + attrName + " for " +
            _getEntityDisplayName(livingEntity));
        return 0;
    }

    f64 modifierValue = attrMap->getModifierValue(attrName, modifierId);
    i32 result = static_cast<i32>(modifierValue);

    std::ostringstream ss;
    ss << "Modifier " << modifierId << " on attribute " << attrName << " for " << _getEntityDisplayName(livingEntity)
       << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

i32 AttributeCommand::_getModifierValueWithScale(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const std::string attrName = _normalizeAttributeName(context.getArgument<std::string>("attribute"));
    const std::string modifierId = context.getArgument<std::string>("id");
    f32 scale = context.getArgument<f32>("scale");

    LivingEntity* livingEntity = nullptr;
    entity::attribute::AttributeMap* attrMap = nullptr;
    if (!_tryGetLivingEntityWithAttribute(context, source, attrName, livingEntity, attrMap)) {
        return 0;
    }

    if (!attrMap->hasModifier(attrName, modifierId)) {
        source.sendError("Modifier " + modifierId + " doesn't exist on attribute " + attrName + " for " +
            _getEntityDisplayName(livingEntity));
        return 0;
    }

    f64 modifierValue = attrMap->getModifierValue(attrName, modifierId);
    i32 result = static_cast<i32>(modifierValue * static_cast<f64>(scale));

    std::ostringstream ss;
    ss << "Modifier " << modifierId << " on attribute " << attrName << " for " << _getEntityDisplayName(livingEntity)
       << ": " << result;
    source.sendMessage(ss.str());

    return result;
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string AttributeCommand::_normalizeAttributeName(const std::string& name)
{
    // 委托给 AttributeRegistry 进行名称规范化，自动处理前缀补全
    return entity::attribute::AttributeRegistry::instance().normalizeName(name);
}

bool AttributeCommand::_isKnownAttribute(const std::string& name) noexcept
{
    return entity::attribute::AttributeRegistry::instance().isKnown(name);
}

std::pair<f64, f64> AttributeCommand::_getAttributeRange(const std::string& name) noexcept
{
    return entity::attribute::AttributeRegistry::instance().getRange(name);
}

} // namespace command
} // namespace mc
