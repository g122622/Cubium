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
 */

#include "common/mod/bedrock/addon/modules/ScriptCustomComponentBinding.hpp"

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptCallbackHolder.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/mod/bedrock/addon/component/CustomComponentParameters.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"

#include <memory>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

// ============================================================================
// 方块组件回调包装器
// ============================================================================

struct BlockJSCallbacks {
    ScriptCallbackHolder onStepOn;
    ScriptCallbackHolder onStepOff;
    ScriptCallbackHolder onPlace;
    ScriptCallbackHolder onPlayerBreak;
    ScriptCallbackHolder onPlayerInteract;
    ScriptCallbackHolder onPlayerPlaceBefore;
    ScriptCallbackHolder onEntityFallOn;
    ScriptCallbackHolder onRandomTick;
    ScriptCallbackHolder onTick;
    ScriptCallbackHolder onEntity;
    ScriptCallbackHolder onBreak;
    ScriptCallbackHolder onRedstoneUpdate;
    ScriptCallbackHolder onBlockStateChange;

    [[nodiscard]] bool hasAnyCallback() const
    {
        return onStepOn.isValid() || onStepOff.isValid() || onPlace.isValid() || onPlayerBreak.isValid() ||
            onPlayerInteract.isValid() || onPlayerPlaceBefore.isValid() || onEntityFallOn.isValid() ||
            onRandomTick.isValid() || onTick.isValid() || onEntity.isValid() || onBreak.isValid() ||
            onRedstoneUpdate.isValid() || onBlockStateChange.isValid();
    }
};

// ============================================================================
// 物品组件回调包装器
// ============================================================================

struct ItemJSCallbacks {
    ScriptCallbackHolder onUse;
    ScriptCallbackHolder onUseOn;
    ScriptCallbackHolder onHitEntity;
    ScriptCallbackHolder onMineBlock;
    ScriptCallbackHolder onBeforeDurabilityDamage;
    ScriptCallbackHolder onCompleteUse;
    ScriptCallbackHolder onConsume;

    [[nodiscard]] bool hasAnyCallback() const
    {
        return onUse.isValid() || onUseOn.isValid() || onHitEntity.isValid() || onMineBlock.isValid() ||
            onBeforeDurabilityDamage.isValid() || onCompleteUse.isValid() || onConsume.isValid();
    }
};

// ============================================================================
// 辅助函数：从JS组件对象中提取回调
// ============================================================================

static ScriptCallbackHolder extractCallback(IScriptBindingContext& ctx, void* obj, const char* propName)
{
    void* prop = ctx.getProperty(obj, propName);
    ScriptCallbackHolder holder;
    if (ctx.isFunction(prop)) {
        holder = ScriptCallbackHolder(ctx, prop);
    }
    ctx.releaseValue(prop);
    return holder;
}

// ============================================================================
// 注册方块自定义组件
// ============================================================================

bool registerBlockCustomComponentFromJS(const std::string& typeId, void* componentObj, IScriptBindingContext& ctx)
{
    if (!ctx.isObject(componentObj)) {
        spdlog::error("[BedrockAddon] registerBlockCustomComponent: component must be an object");
        return false;
    }

    auto callbacks = std::make_shared<BlockJSCallbacks>();

    callbacks->onStepOn = extractCallback(ctx, componentObj, "onStepOn");
    callbacks->onStepOff = extractCallback(ctx, componentObj, "onStepOff");
    callbacks->onPlace = extractCallback(ctx, componentObj, "onPlace");
    callbacks->onPlayerBreak = extractCallback(ctx, componentObj, "onPlayerBreak");
    callbacks->onPlayerInteract = extractCallback(ctx, componentObj, "onPlayerInteract");
    callbacks->onPlayerPlaceBefore = extractCallback(ctx, componentObj, "beforeOnPlayerPlace");
    callbacks->onEntityFallOn = extractCallback(ctx, componentObj, "onEntityFallOn");
    callbacks->onRandomTick = extractCallback(ctx, componentObj, "onRandomTick");
    callbacks->onTick = extractCallback(ctx, componentObj, "onTick");
    callbacks->onEntity = extractCallback(ctx, componentObj, "onEntity");
    callbacks->onBreak = extractCallback(ctx, componentObj, "onBreak");
    callbacks->onRedstoneUpdate = extractCallback(ctx, componentObj, "onRedstoneUpdate");
    callbacks->onBlockStateChange = extractCallback(ctx, componentObj, "onBlockStateChange");

    if (!callbacks->hasAnyCallback()) {
        spdlog::warn("[BedrockAddon] registerBlockCustomComponent: no callbacks found for {}", typeId);
    }

    BlockCustomComponent component;
    component.name = typeId;

    if (callbacks->onStepOn.isValid()) {
        component.onStepOn = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                 BlockComponentStepOnEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onStepOn.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            if (event.entityId.has_value()) {
                ctx->setPropertyInt64(eventObj, "entityId", static_cast<i64>(*event.entityId));
            }
            void* ret = cb->onStepOn.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onStepOff.isValid()) {
        component.onStepOff = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                  BlockComponentStepOffEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onStepOff.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            if (event.entityId.has_value()) {
                ctx->setPropertyInt64(eventObj, "entityId", static_cast<i64>(*event.entityId));
            }
            void* ret = cb->onStepOff.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onPlace.isValid()) {
        component.onPlace = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                BlockComponentOnPlaceEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onPlace.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            void* ret = cb->onPlace.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onPlayerBreak.isValid()) {
        component.onPlayerBreak = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                      BlockComponentPlayerBreakEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onPlayerBreak.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            if (event.playerId.has_value()) {
                ctx->setPropertyInt64(eventObj, "playerId", static_cast<i64>(*event.playerId));
            }
            void* ret = cb->onPlayerBreak.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onPlayerInteract.isValid()) {
        component.onPlayerInteract = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                         BlockComponentPlayerInteractEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onPlayerInteract.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            if (event.playerId.has_value()) {
                ctx->setPropertyInt64(eventObj, "playerId", static_cast<i64>(*event.playerId));
            }
            void* ret = cb->onPlayerInteract.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onPlayerPlaceBefore.isValid()) {
        component.beforeOnPlayerPlace = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                            BlockComponentPlayerPlaceBeforeEvent& event,
                                            const CustomComponentParameters&) {
            auto* ctx = cb->onPlayerPlaceBefore.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            if (event.playerId.has_value()) {
                ctx->setPropertyInt64(eventObj, "playerId", static_cast<i64>(*event.playerId));
            }
            ctx->setPropertyBool(eventObj, "cancel", event.cancel);
            void* ret = cb->onPlayerPlaceBefore.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            // 读取JS修改后的cancel值
            auto cancelVal = ctx->getPropertyBool(eventObj, "cancel");
            if (cancelVal) {
                event.cancel = *cancelVal;
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onEntityFallOn.isValid()) {
        component.onEntityFallOn = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                       BlockComponentEntityFallOnEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onEntityFallOn.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            if (event.entityId.has_value()) {
                ctx->setPropertyInt64(eventObj, "entityId", static_cast<i64>(*event.entityId));
            }
            ctx->setPropertyFloat(eventObj, "fallDistance", static_cast<f64>(event.fallDistance));
            void* ret = cb->onEntityFallOn.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onRandomTick.isValid()) {
        component.onRandomTick = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                     BlockComponentRandomTickEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onRandomTick.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            void* ret = cb->onRandomTick.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onTick.isValid()) {
        component.onTick = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                               BlockComponentTickEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onTick.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            void* ret = cb->onTick.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onEntity.isValid()) {
        component.onEntity = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                 BlockComponentEntityEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onEntity.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            ctx->setPropertyInt64(eventObj, "entityId", static_cast<i64>(event.entitySourceId));
            void* ret = cb->onEntity.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onBreak.isValid()) {
        component.onBreak = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                BlockComponentBreakEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onBreak.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            if (event.entitySourceId.has_value()) {
                ctx->setPropertyInt64(eventObj, "entityId", static_cast<i64>(*event.entitySourceId));
            }
            void* ret = cb->onBreak.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onRedstoneUpdate.isValid()) {
        component.onRedstoneUpdate = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                         BlockComponentRedstoneUpdateEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onRedstoneUpdate.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            void* ret = cb->onRedstoneUpdate.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onBlockStateChange.isValid()) {
        component.onBlockStateChange = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                           BlockComponentBlockStateChangeEvent& event,
                                           const CustomComponentParameters&) {
            auto* ctx = cb->onBlockStateChange.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "blockTypeId", event.blockTypeId);
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            void* ret = cb->onBlockStateChange.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    BlockComponentRegistry::instance().registerComponent(typeId, std::move(component));
    spdlog::info("[BedrockAddon] Registered block custom component for '{}'", typeId);
    return true;
}

// ============================================================================
// 注册物品自定义组件
// ============================================================================

bool registerItemCustomComponentFromJS(const std::string& typeId, void* componentObj, IScriptBindingContext& ctx)
{
    if (!ctx.isObject(componentObj)) {
        spdlog::error("[BedrockAddon] registerItemCustomComponent: component must be an object");
        return false;
    }

    auto callbacks = std::make_shared<ItemJSCallbacks>();

    callbacks->onUse = extractCallback(ctx, componentObj, "onUse");
    callbacks->onUseOn = extractCallback(ctx, componentObj, "onUseOn");
    callbacks->onHitEntity = extractCallback(ctx, componentObj, "onHitEntity");
    callbacks->onMineBlock = extractCallback(ctx, componentObj, "onMineBlock");
    callbacks->onBeforeDurabilityDamage = extractCallback(ctx, componentObj, "beforeDurabilityDamage");
    callbacks->onCompleteUse = extractCallback(ctx, componentObj, "onCompleteUse");
    callbacks->onConsume = extractCallback(ctx, componentObj, "onConsume");

    if (!callbacks->hasAnyCallback()) {
        spdlog::warn("[BedrockAddon] registerItemCustomComponent: no callbacks found for {}", typeId);
    }

    ItemCustomComponent component;
    component.name = typeId;

    if (callbacks->onUse.isValid()) {
        component.onUse = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                              ItemComponentUseEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onUse.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "itemTypeId", event.itemTypeId);
            ctx->setPropertyInt64(eventObj, "sourceId", static_cast<i64>(event.sourceId));
            void* ret = cb->onUse.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onUseOn.isValid()) {
        component.onUseOn = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                ItemComponentUseOnEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onUseOn.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "itemTypeId", event.itemTypeId);
            ctx->setPropertyInt64(eventObj, "sourceId", static_cast<i64>(event.sourceId));
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            ctx->setPropertyInt(eventObj, "face", event.face);
            void* ret = cb->onUseOn.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onHitEntity.isValid()) {
        component.onHitEntity = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                    ItemComponentHitEntityEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onHitEntity.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "itemTypeId", event.itemTypeId);
            ctx->setPropertyInt64(eventObj, "attackingEntityId", static_cast<i64>(event.attackingEntityId));
            ctx->setPropertyInt64(eventObj, "hitEntityId", static_cast<i64>(event.hitEntityId));
            void* ret = cb->onHitEntity.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onMineBlock.isValid()) {
        component.onMineBlock = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                    ItemComponentMineBlockEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onMineBlock.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "itemTypeId", event.itemTypeId);
            ctx->setPropertyInt64(eventObj, "sourceId", static_cast<i64>(event.sourceId));
            ctx->setPropertyInt(eventObj, "x", event.blockX);
            ctx->setPropertyInt(eventObj, "y", event.blockY);
            ctx->setPropertyInt(eventObj, "z", event.blockZ);
            void* ret = cb->onMineBlock.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onBeforeDurabilityDamage.isValid()) {
        component.onBeforeDurabilityDamage = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                                 ItemComponentBeforeDurabilityDamageEvent& event,
                                                 const CustomComponentParameters&) {
            auto* ctx = cb->onBeforeDurabilityDamage.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "itemTypeId", event.itemTypeId);
            ctx->setPropertyInt64(eventObj, "attackingEntityId", static_cast<i64>(event.attackingEntityId));
            ctx->setPropertyInt64(eventObj, "hitEntityId", static_cast<i64>(event.hitEntityId));
            ctx->setPropertyInt(eventObj, "durabilityDamage", event.durabilityDamage);
            void* ret = cb->onBeforeDurabilityDamage.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            // 读取JS修改后的durabilityDamage值
            auto dmgVal = ctx->getPropertyInt(eventObj, "durabilityDamage");
            if (dmgVal) {
                event.durabilityDamage = *dmgVal;
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onCompleteUse.isValid()) {
        component.onCompleteUse = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                      ItemComponentCompleteUseEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onCompleteUse.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "itemTypeId", event.itemTypeId);
            ctx->setPropertyInt64(eventObj, "sourceId", static_cast<i64>(event.sourceId));
            ctx->setPropertyInt(eventObj, "useDuration", event.useDuration);
            void* ret = cb->onCompleteUse.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    if (callbacks->onConsume.isValid()) {
        component.onConsume = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                  ItemComponentConsumeEvent& event, const CustomComponentParameters&) {
            auto* ctx = cb->onConsume.context();
            void* eventObj = ctx->createObject();
            ctx->setPropertyString(eventObj, "itemTypeId", event.itemTypeId);
            ctx->setPropertyInt64(eventObj, "sourceId", static_cast<i64>(event.sourceId));
            void* ret = cb->onConsume.call(eventObj);
            if (ctx->isException(ret)) {
                void* exc = ctx->getException();
                spdlog::error("[BedrockAddon] JS callback threw: {}", ctx->getExceptionMessage(exc));
                ctx->releaseValue(exc);
            }
            ctx->releaseValue(ret);
            ctx->releaseValue(eventObj);
        };
    }

    ItemComponentRegistry::instance().registerComponent(typeId, std::move(component));
    spdlog::info("[BedrockAddon] Registered item custom component for '{}'", typeId);
    return true;
}

// ============================================================================
// 导出注册函数 - 供MinecraftModuleFactory调用
// ============================================================================

void registerCustomComponentBindings(NativeModuleBuilder& builder)
{
    auto& ctx = builder.context();

    // 注册 BlockComponentRegistry 全局对象
    void* blockRegObj = ctx.createObject();
    ctx.registerMethod(
        blockRegObj,
        "registerCustomComponent",
        [](IScriptBindingContext& cbCtx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 2) {
                return cbCtx.throwTypeError("blockComponentRegistry.registerCustomComponent requires 2 arguments");
            }

            auto typeId = cbCtx.toString(args[0]);
            if (!typeId) {
                return cbCtx.throwTypeError("First argument must be a string (typeId)");
            }

            if (!cbCtx.isObject(args[1])) {
                return cbCtx.throwTypeError("Second argument must be an object (component)");
            }

            // 保留component对象引用
            void* componentObj = args[1];
            cbCtx.retainValue(componentObj);
            bool success = registerBlockCustomComponentFromJS(*typeId, componentObj, cbCtx);
            cbCtx.releaseValue(componentObj);

            if (!success) {
                return cbCtx.throwTypeError(
                    ("Failed to register block custom component for '" + *typeId + "'").c_str());
            }

            return cbCtx.createUndefined();
        },
        2);
    builder.exportValue("blockComponentRegistry", blockRegObj);
    ctx.releaseValue(blockRegObj);

    // 注册 ItemComponentRegistry 全局对象
    void* itemRegObj = ctx.createObject();
    ctx.registerMethod(
        itemRegObj,
        "registerCustomComponent",
        [](IScriptBindingContext& cbCtx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 2) {
                return cbCtx.throwTypeError("itemComponentRegistry.registerCustomComponent requires 2 arguments");
            }

            auto typeId = cbCtx.toString(args[0]);
            if (!typeId) {
                return cbCtx.throwTypeError("First argument must be a string (typeId)");
            }

            if (!cbCtx.isObject(args[1])) {
                return cbCtx.throwTypeError("Second argument must be an object (component)");
            }

            void* componentObj = args[1];
            cbCtx.retainValue(componentObj);
            bool success = registerItemCustomComponentFromJS(*typeId, componentObj, cbCtx);
            cbCtx.releaseValue(componentObj);

            if (!success) {
                return cbCtx.throwTypeError(("Failed to register item custom component for '" + *typeId + "'").c_str());
            }

            return cbCtx.createUndefined();
        },
        2);
    builder.exportValue("itemComponentRegistry", itemRegObj);
    ctx.releaseValue(itemRegObj);

    spdlog::info("[BedrockAddon] Registered custom component bindings for @minecraft/server");
}

} // namespace mc::mod::bedrock::addon
