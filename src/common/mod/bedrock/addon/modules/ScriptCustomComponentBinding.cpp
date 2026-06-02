#include "ScriptCustomComponentBinding.hpp"

#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/mod/bedrock/addon/engine/QuickJSContext.hpp"

#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

// ============================================================================
// JS回调包装器 - 将JS函数包装为C++ std::function
// ============================================================================

/**
 * @brief JS回调包装器
 *
 * 持有JS函数引用，在C++事件派发时调用JS回调。
 * 通过ScriptObjectRegistry管理生命周期。
 */
class JSCallbackHolder {
public:
    JSCallbackHolder() = default;

    JSCallbackHolder(JSContext* ctx, JSValue func)
        : m_ctx(ctx)
    {
        m_func = JS_DupValue(ctx, func);
    }

    ~JSCallbackHolder()
    {
        if (m_ctx && !JS_IsUndefined(m_func)) {
            JS_FreeValue(m_ctx, m_func);
        }
    }

    // 不可复制
    JSCallbackHolder(const JSCallbackHolder&) = delete;
    JSCallbackHolder& operator=(const JSCallbackHolder&) = delete;

    // 可移动
    JSCallbackHolder(JSCallbackHolder&& other) noexcept
        : m_ctx(other.m_ctx)
        , m_func(other.m_func)
    {
        other.m_ctx = nullptr;
        other.m_func = JS_UNDEFINED;
    }

    JSCallbackHolder& operator=(JSCallbackHolder&& other) noexcept
    {
        if (this != &other) {
            if (m_ctx && !JS_IsUndefined(m_func)) {
                JS_FreeValue(m_ctx, m_func);
            }
            m_ctx = other.m_ctx;
            m_func = other.m_func;
            other.m_ctx = nullptr;
            other.m_func = JS_UNDEFINED;
        }
        return *this;
    }

    /** 调用JS回调，传入事件对象 */
    void call(JSValue eventObj)
    {
        if (!m_ctx || JS_IsUndefined(m_func)) {
            return;
        }

        JSValue ret = JS_Call(m_ctx, m_func, JS_UNDEFINED, 1, &eventObj);
        if (JS_IsException(ret)) {
            JSValue exc = JS_GetException(m_ctx);
            const char* msg = JS_ToCString(m_ctx, exc);
            spdlog::error("[BedrockAddon] JS callback threw: {}", msg ? msg : "unknown");
            JS_FreeCString(m_ctx, msg);
            JS_FreeValue(m_ctx, exc);
        }
        JS_FreeValue(m_ctx, ret);
    }

    [[nodiscard]] JSValue func() const { return m_func; }
    [[nodiscard]] JSContext* context() const { return m_ctx; }

private:
    JSContext* m_ctx = nullptr;
    JSValue m_func = JS_UNDEFINED;
};

// ============================================================================
// 方块组件回调包装器
// ============================================================================

/**
 * @brief 方块自定义组件的JS回调集合
 *
 * 每个回调对应Bedrock API的一个方块组件事件。
 * 注册到BlockComponentRegistry时，将此对象作为CustomComponent的回调。
 */
struct BlockJSCallbacks {
    JSCallbackHolder onStepOn;
    JSCallbackHolder onStepOff;
    JSCallbackHolder onPlace;
    JSCallbackHolder onPlayerBreak;
    JSCallbackHolder onPlayerInteract;
    JSCallbackHolder onPlayerPlaceBefore;
    JSCallbackHolder onEntityFallOn;
    JSCallbackHolder onRandomTick;
    JSCallbackHolder onTick;
    JSCallbackHolder onEntity;
    JSCallbackHolder onBreak;
    JSCallbackHolder onRedstoneUpdate;
    JSCallbackHolder onBlockStateChange;

    /** 是否有任何回调被设置 */
    [[nodiscard]] bool hasAnyCallback() const
    {
        return !JS_IsUndefined(onStepOn.func()) || !JS_IsUndefined(onStepOff.func()) ||
            !JS_IsUndefined(onPlace.func()) || !JS_IsUndefined(onPlayerBreak.func()) ||
            !JS_IsUndefined(onPlayerInteract.func()) || !JS_IsUndefined(onPlayerPlaceBefore.func()) ||
            !JS_IsUndefined(onEntityFallOn.func()) || !JS_IsUndefined(onRandomTick.func()) ||
            !JS_IsUndefined(onTick.func()) || !JS_IsUndefined(onEntity.func()) || !JS_IsUndefined(onBreak.func()) ||
            !JS_IsUndefined(onRedstoneUpdate.func()) || !JS_IsUndefined(onBlockStateChange.func());
    }
};

// ============================================================================
// 物品组件回调包装器
// ============================================================================

/**
 * @brief 物品自定义组件的JS回调集合
 */
struct ItemJSCallbacks {
    JSCallbackHolder onUse;
    JSCallbackHolder onUseOn;
    JSCallbackHolder onHitEntity;
    JSCallbackHolder onMineBlock;
    JSCallbackHolder onBeforeDurabilityDamage;
    JSCallbackHolder onCompleteUse;
    JSCallbackHolder onConsume;

    [[nodiscard]] bool hasAnyCallback() const
    {
        return !JS_IsUndefined(onUse.func()) || !JS_IsUndefined(onUseOn.func()) ||
            !JS_IsUndefined(onHitEntity.func()) || !JS_IsUndefined(onMineBlock.func()) ||
            !JS_IsUndefined(onBeforeDurabilityDamage.func()) || !JS_IsUndefined(onCompleteUse.func()) ||
            !JS_IsUndefined(onConsume.func());
    }
};

// ============================================================================
// 辅助函数：从JS组件对象中提取回调
// ============================================================================

/**
 * @brief 从JS对象中提取指定属性的函数值
 * @return JS_IsFunction的值，如果属性不存在或不是函数则返回JS_UNDEFINED
 */
static JSValue extractCallback(JSContext* ctx, JSValue obj, const char* propName)
{
    JSValue prop = JS_GetPropertyStr(ctx, obj, propName);
    if (JS_IsFunction(ctx, prop)) {
        return prop;
    }
    JS_FreeValue(ctx, prop);
    return JS_UNDEFINED;
}

// ============================================================================
// 注册方块自定义组件
// ============================================================================

bool registerBlockCustomComponentFromJS(const std::string& typeId, JSValue componentObj, JSContext* ctx)
{
    if (!JS_IsObject(componentObj)) {
        spdlog::error("[BedrockAddon] registerBlockCustomComponent: component must be an object");
        return false;
    }

    // 提取所有回调
    auto callbacks = std::make_shared<BlockJSCallbacks>();

    // 按Bedrock API的回调名称提取
    JSValue onStepOn = extractCallback(ctx, componentObj, "onStepOn");
    if (!JS_IsUndefined(onStepOn)) {
        callbacks->onStepOn = JSCallbackHolder(ctx, onStepOn);
        JS_FreeValue(ctx, onStepOn);
    }

    JSValue onStepOff = extractCallback(ctx, componentObj, "onStepOff");
    if (!JS_IsUndefined(onStepOff)) {
        callbacks->onStepOff = JSCallbackHolder(ctx, onStepOff);
        JS_FreeValue(ctx, onStepOff);
    }

    JSValue onPlace = extractCallback(ctx, componentObj, "onPlace");
    if (!JS_IsUndefined(onPlace)) {
        callbacks->onPlace = JSCallbackHolder(ctx, onPlace);
        JS_FreeValue(ctx, onPlace);
    }

    JSValue onPlayerBreak = extractCallback(ctx, componentObj, "onPlayerBreak");
    if (!JS_IsUndefined(onPlayerBreak)) {
        callbacks->onPlayerBreak = JSCallbackHolder(ctx, onPlayerBreak);
        JS_FreeValue(ctx, onPlayerBreak);
    }

    JSValue onPlayerInteract = extractCallback(ctx, componentObj, "onPlayerInteract");
    if (!JS_IsUndefined(onPlayerInteract)) {
        callbacks->onPlayerInteract = JSCallbackHolder(ctx, onPlayerInteract);
        JS_FreeValue(ctx, onPlayerInteract);
    }

    JSValue onBeforeOnPlayerPlace = extractCallback(ctx, componentObj, "beforeOnPlayerPlace");
    if (!JS_IsUndefined(onBeforeOnPlayerPlace)) {
        callbacks->onPlayerPlaceBefore = JSCallbackHolder(ctx, onBeforeOnPlayerPlace);
        JS_FreeValue(ctx, onBeforeOnPlayerPlace);
    }

    JSValue onEntityFallOn = extractCallback(ctx, componentObj, "onEntityFallOn");
    if (!JS_IsUndefined(onEntityFallOn)) {
        callbacks->onEntityFallOn = JSCallbackHolder(ctx, onEntityFallOn);
        JS_FreeValue(ctx, onEntityFallOn);
    }

    JSValue onRandomTick = extractCallback(ctx, componentObj, "onRandomTick");
    if (!JS_IsUndefined(onRandomTick)) {
        callbacks->onRandomTick = JSCallbackHolder(ctx, onRandomTick);
        JS_FreeValue(ctx, onRandomTick);
    }

    JSValue onTick = extractCallback(ctx, componentObj, "onTick");
    if (!JS_IsUndefined(onTick)) {
        callbacks->onTick = JSCallbackHolder(ctx, onTick);
        JS_FreeValue(ctx, onTick);
    }

    JSValue onEntity = extractCallback(ctx, componentObj, "onEntity");
    if (!JS_IsUndefined(onEntity)) {
        callbacks->onEntity = JSCallbackHolder(ctx, onEntity);
        JS_FreeValue(ctx, onEntity);
    }

    JSValue onBreak = extractCallback(ctx, componentObj, "onBreak");
    if (!JS_IsUndefined(onBreak)) {
        callbacks->onBreak = JSCallbackHolder(ctx, onBreak);
        JS_FreeValue(ctx, onBreak);
    }

    JSValue onRedstoneUpdate = extractCallback(ctx, componentObj, "onRedstoneUpdate");
    if (!JS_IsUndefined(onRedstoneUpdate)) {
        callbacks->onRedstoneUpdate = JSCallbackHolder(ctx, onRedstoneUpdate);
        JS_FreeValue(ctx, onRedstoneUpdate);
    }

    JSValue onBlockStateChange = extractCallback(ctx, componentObj, "onBlockStateChange");
    if (!JS_IsUndefined(onBlockStateChange)) {
        callbacks->onBlockStateChange = JSCallbackHolder(ctx, onBlockStateChange);
        JS_FreeValue(ctx, onBlockStateChange);
    }

    if (!callbacks->hasAnyCallback()) {
        spdlog::warn("[BedrockAddon] registerBlockCustomComponent: no callbacks found for {}", typeId);
    }

    // 创建BlockCustomComponent并注册到BlockComponentRegistry
    BlockCustomComponent component;
    component.name = typeId;

    // 为每个回调设置C++ wrapper（签名：void(EventType&, const CustomComponentParameters&)）
    if (!JS_IsUndefined(callbacks->onStepOn.func())) {
        component.onStepOn = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                 BlockComponentStepOnEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onStepOn.context());
            JS_SetPropertyStr(cb->onStepOn.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onStepOn.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onStepOn.context(), eventObj, "x", JS_NewInt32(cb->onStepOn.context(), event.blockX));
            JS_SetPropertyStr(cb->onStepOn.context(), eventObj, "y", JS_NewInt32(cb->onStepOn.context(), event.blockY));
            JS_SetPropertyStr(cb->onStepOn.context(), eventObj, "z", JS_NewInt32(cb->onStepOn.context(), event.blockZ));
            if (event.entityId.has_value()) {
                JS_SetPropertyStr(cb->onStepOn.context(),
                    eventObj,
                    "entityId",
                    JS_NewBigUint64(cb->onStepOn.context(), *event.entityId));
            }
            cb->onStepOn.call(eventObj);
            JS_FreeValue(cb->onStepOn.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onStepOff.func())) {
        component.onStepOff = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                  BlockComponentStepOffEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onStepOff.context());
            JS_SetPropertyStr(cb->onStepOff.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onStepOff.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(
                cb->onStepOff.context(), eventObj, "x", JS_NewInt32(cb->onStepOff.context(), event.blockX));
            JS_SetPropertyStr(
                cb->onStepOff.context(), eventObj, "y", JS_NewInt32(cb->onStepOff.context(), event.blockY));
            JS_SetPropertyStr(
                cb->onStepOff.context(), eventObj, "z", JS_NewInt32(cb->onStepOff.context(), event.blockZ));
            if (event.entityId.has_value()) {
                JS_SetPropertyStr(cb->onStepOff.context(),
                    eventObj,
                    "entityId",
                    JS_NewBigUint64(cb->onStepOff.context(), *event.entityId));
            }
            cb->onStepOff.call(eventObj);
            JS_FreeValue(cb->onStepOff.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onPlace.func())) {
        component.onPlace = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                BlockComponentOnPlaceEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onPlace.context());
            JS_SetPropertyStr(cb->onPlace.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onPlace.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onPlace.context(), eventObj, "x", JS_NewInt32(cb->onPlace.context(), event.blockX));
            JS_SetPropertyStr(cb->onPlace.context(), eventObj, "y", JS_NewInt32(cb->onPlace.context(), event.blockY));
            JS_SetPropertyStr(cb->onPlace.context(), eventObj, "z", JS_NewInt32(cb->onPlace.context(), event.blockZ));
            cb->onPlace.call(eventObj);
            JS_FreeValue(cb->onPlace.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onPlayerBreak.func())) {
        component.onPlayerBreak = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                      BlockComponentPlayerBreakEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onPlayerBreak.context());
            JS_SetPropertyStr(cb->onPlayerBreak.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onPlayerBreak.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(
                cb->onPlayerBreak.context(), eventObj, "x", JS_NewInt32(cb->onPlayerBreak.context(), event.blockX));
            JS_SetPropertyStr(
                cb->onPlayerBreak.context(), eventObj, "y", JS_NewInt32(cb->onPlayerBreak.context(), event.blockY));
            JS_SetPropertyStr(
                cb->onPlayerBreak.context(), eventObj, "z", JS_NewInt32(cb->onPlayerBreak.context(), event.blockZ));
            if (event.playerId.has_value()) {
                JS_SetPropertyStr(cb->onPlayerBreak.context(),
                    eventObj,
                    "playerId",
                    JS_NewBigUint64(cb->onPlayerBreak.context(), *event.playerId));
            }
            cb->onPlayerBreak.call(eventObj);
            JS_FreeValue(cb->onPlayerBreak.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onPlayerInteract.func())) {
        component.onPlayerInteract = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                         BlockComponentPlayerInteractEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onPlayerInteract.context());
            JS_SetPropertyStr(cb->onPlayerInteract.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onPlayerInteract.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onPlayerInteract.context(),
                eventObj,
                "x",
                JS_NewInt32(cb->onPlayerInteract.context(), event.blockX));
            JS_SetPropertyStr(cb->onPlayerInteract.context(),
                eventObj,
                "y",
                JS_NewInt32(cb->onPlayerInteract.context(), event.blockY));
            JS_SetPropertyStr(cb->onPlayerInteract.context(),
                eventObj,
                "z",
                JS_NewInt32(cb->onPlayerInteract.context(), event.blockZ));
            if (event.playerId.has_value()) {
                JS_SetPropertyStr(cb->onPlayerInteract.context(),
                    eventObj,
                    "playerId",
                    JS_NewBigUint64(cb->onPlayerInteract.context(), *event.playerId));
            }
            cb->onPlayerInteract.call(eventObj);
            JS_FreeValue(cb->onPlayerInteract.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onPlayerPlaceBefore.func())) {
        component.beforeOnPlayerPlace = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                            BlockComponentPlayerPlaceBeforeEvent& event,
                                            const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onPlayerPlaceBefore.context());
            JS_SetPropertyStr(cb->onPlayerPlaceBefore.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onPlayerPlaceBefore.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onPlayerPlaceBefore.context(),
                eventObj,
                "x",
                JS_NewInt32(cb->onPlayerPlaceBefore.context(), event.blockX));
            JS_SetPropertyStr(cb->onPlayerPlaceBefore.context(),
                eventObj,
                "y",
                JS_NewInt32(cb->onPlayerPlaceBefore.context(), event.blockY));
            JS_SetPropertyStr(cb->onPlayerPlaceBefore.context(),
                eventObj,
                "z",
                JS_NewInt32(cb->onPlayerPlaceBefore.context(), event.blockZ));
            if (event.playerId.has_value()) {
                JS_SetPropertyStr(cb->onPlayerPlaceBefore.context(),
                    eventObj,
                    "playerId",
                    JS_NewBigUint64(cb->onPlayerPlaceBefore.context(), *event.playerId));
            }
            // cancel属性可由JS修改
            JS_SetPropertyStr(cb->onPlayerPlaceBefore.context(),
                eventObj,
                "cancel",
                JS_NewBool(cb->onPlayerPlaceBefore.context(), event.cancel));
            cb->onPlayerPlaceBefore.call(eventObj);
            // 读取JS修改后的cancel值
            JSValue cancelVal = JS_GetPropertyStr(cb->onPlayerPlaceBefore.context(), eventObj, "cancel");
            event.cancel = JS_ToBool(cb->onPlayerPlaceBefore.context(), cancelVal);
            JS_FreeValue(cb->onPlayerPlaceBefore.context(), cancelVal);
            JS_FreeValue(cb->onPlayerPlaceBefore.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onEntityFallOn.func())) {
        component.onEntityFallOn = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                       BlockComponentEntityFallOnEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onEntityFallOn.context());
            JS_SetPropertyStr(cb->onEntityFallOn.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onEntityFallOn.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(
                cb->onEntityFallOn.context(), eventObj, "x", JS_NewInt32(cb->onEntityFallOn.context(), event.blockX));
            JS_SetPropertyStr(
                cb->onEntityFallOn.context(), eventObj, "y", JS_NewInt32(cb->onEntityFallOn.context(), event.blockY));
            JS_SetPropertyStr(
                cb->onEntityFallOn.context(), eventObj, "z", JS_NewInt32(cb->onEntityFallOn.context(), event.blockZ));
            if (event.entityId.has_value()) {
                JS_SetPropertyStr(cb->onEntityFallOn.context(),
                    eventObj,
                    "entityId",
                    JS_NewBigUint64(cb->onEntityFallOn.context(), *event.entityId));
            }
            JS_SetPropertyStr(cb->onEntityFallOn.context(),
                eventObj,
                "fallDistance",
                JS_NewFloat64(cb->onEntityFallOn.context(), static_cast<f64>(event.fallDistance)));
            cb->onEntityFallOn.call(eventObj);
            JS_FreeValue(cb->onEntityFallOn.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onRandomTick.func())) {
        component.onRandomTick = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                     BlockComponentRandomTickEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onRandomTick.context());
            JS_SetPropertyStr(cb->onRandomTick.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onRandomTick.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(
                cb->onRandomTick.context(), eventObj, "x", JS_NewInt32(cb->onRandomTick.context(), event.blockX));
            JS_SetPropertyStr(
                cb->onRandomTick.context(), eventObj, "y", JS_NewInt32(cb->onRandomTick.context(), event.blockY));
            JS_SetPropertyStr(
                cb->onRandomTick.context(), eventObj, "z", JS_NewInt32(cb->onRandomTick.context(), event.blockZ));
            cb->onRandomTick.call(eventObj);
            JS_FreeValue(cb->onRandomTick.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onTick.func())) {
        component.onTick = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                               BlockComponentTickEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onTick.context());
            JS_SetPropertyStr(cb->onTick.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onTick.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onTick.context(), eventObj, "x", JS_NewInt32(cb->onTick.context(), event.blockX));
            JS_SetPropertyStr(cb->onTick.context(), eventObj, "y", JS_NewInt32(cb->onTick.context(), event.blockY));
            JS_SetPropertyStr(cb->onTick.context(), eventObj, "z", JS_NewInt32(cb->onTick.context(), event.blockZ));
            cb->onTick.call(eventObj);
            JS_FreeValue(cb->onTick.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onEntity.func())) {
        component.onEntity = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                 BlockComponentEntityEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onEntity.context());
            JS_SetPropertyStr(cb->onEntity.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onEntity.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onEntity.context(), eventObj, "x", JS_NewInt32(cb->onEntity.context(), event.blockX));
            JS_SetPropertyStr(cb->onEntity.context(), eventObj, "y", JS_NewInt32(cb->onEntity.context(), event.blockY));
            JS_SetPropertyStr(cb->onEntity.context(), eventObj, "z", JS_NewInt32(cb->onEntity.context(), event.blockZ));
            JS_SetPropertyStr(cb->onEntity.context(),
                eventObj,
                "entityId",
                JS_NewBigUint64(cb->onEntity.context(), event.entitySourceId));
            cb->onEntity.call(eventObj);
            JS_FreeValue(cb->onEntity.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onBreak.func())) {
        component.onBreak = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                BlockComponentBreakEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onBreak.context());
            JS_SetPropertyStr(cb->onBreak.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onBreak.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onBreak.context(), eventObj, "x", JS_NewInt32(cb->onBreak.context(), event.blockX));
            JS_SetPropertyStr(cb->onBreak.context(), eventObj, "y", JS_NewInt32(cb->onBreak.context(), event.blockY));
            JS_SetPropertyStr(cb->onBreak.context(), eventObj, "z", JS_NewInt32(cb->onBreak.context(), event.blockZ));
            if (event.entitySourceId.has_value()) {
                JS_SetPropertyStr(cb->onBreak.context(),
                    eventObj,
                    "entityId",
                    JS_NewBigUint64(cb->onBreak.context(), *event.entitySourceId));
            }
            cb->onBreak.call(eventObj);
            JS_FreeValue(cb->onBreak.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onRedstoneUpdate.func())) {
        component.onRedstoneUpdate = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                         BlockComponentRedstoneUpdateEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onRedstoneUpdate.context());
            JS_SetPropertyStr(cb->onRedstoneUpdate.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onRedstoneUpdate.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onRedstoneUpdate.context(),
                eventObj,
                "x",
                JS_NewInt32(cb->onRedstoneUpdate.context(), event.blockX));
            JS_SetPropertyStr(cb->onRedstoneUpdate.context(),
                eventObj,
                "y",
                JS_NewInt32(cb->onRedstoneUpdate.context(), event.blockY));
            JS_SetPropertyStr(cb->onRedstoneUpdate.context(),
                eventObj,
                "z",
                JS_NewInt32(cb->onRedstoneUpdate.context(), event.blockZ));
            cb->onRedstoneUpdate.call(eventObj);
            JS_FreeValue(cb->onRedstoneUpdate.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onBlockStateChange.func())) {
        component.onBlockStateChange = [cb = std::shared_ptr<BlockJSCallbacks>(callbacks)](
                                           BlockComponentBlockStateChangeEvent& event,
                                           const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onBlockStateChange.context());
            JS_SetPropertyStr(cb->onBlockStateChange.context(),
                eventObj,
                "blockTypeId",
                JS_NewString(cb->onBlockStateChange.context(), event.blockTypeId.c_str()));
            JS_SetPropertyStr(cb->onBlockStateChange.context(),
                eventObj,
                "x",
                JS_NewInt32(cb->onBlockStateChange.context(), event.blockX));
            JS_SetPropertyStr(cb->onBlockStateChange.context(),
                eventObj,
                "y",
                JS_NewInt32(cb->onBlockStateChange.context(), event.blockY));
            JS_SetPropertyStr(cb->onBlockStateChange.context(),
                eventObj,
                "z",
                JS_NewInt32(cb->onBlockStateChange.context(), event.blockZ));
            cb->onBlockStateChange.call(eventObj);
            JS_FreeValue(cb->onBlockStateChange.context(), eventObj);
        };
    }

    // 注册到全局BlockComponentRegistry
    BlockComponentRegistry::instance().registerComponent(typeId, std::move(component));

    spdlog::info("[BedrockAddon] Registered block custom component for '{}'", typeId);

    return true;
}

// ============================================================================
// 注册物品自定义组件
// ============================================================================

bool registerItemCustomComponentFromJS(const std::string& typeId, JSValue componentObj, JSContext* ctx)
{
    if (!JS_IsObject(componentObj)) {
        spdlog::error("[BedrockAddon] registerItemCustomComponent: component must be an object");
        return false;
    }

    auto callbacks = std::make_shared<ItemJSCallbacks>();

    JSValue onUse = extractCallback(ctx, componentObj, "onUse");
    if (!JS_IsUndefined(onUse)) {
        callbacks->onUse = JSCallbackHolder(ctx, onUse);
        JS_FreeValue(ctx, onUse);
    }

    JSValue onUseOn = extractCallback(ctx, componentObj, "onUseOn");
    if (!JS_IsUndefined(onUseOn)) {
        callbacks->onUseOn = JSCallbackHolder(ctx, onUseOn);
        JS_FreeValue(ctx, onUseOn);
    }

    JSValue onHitEntity = extractCallback(ctx, componentObj, "onHitEntity");
    if (!JS_IsUndefined(onHitEntity)) {
        callbacks->onHitEntity = JSCallbackHolder(ctx, onHitEntity);
        JS_FreeValue(ctx, onHitEntity);
    }

    JSValue onMineBlock = extractCallback(ctx, componentObj, "onMineBlock");
    if (!JS_IsUndefined(onMineBlock)) {
        callbacks->onMineBlock = JSCallbackHolder(ctx, onMineBlock);
        JS_FreeValue(ctx, onMineBlock);
    }

    JSValue onBeforeDurabilityDamage = extractCallback(ctx, componentObj, "beforeDurabilityDamage");
    if (!JS_IsUndefined(onBeforeDurabilityDamage)) {
        callbacks->onBeforeDurabilityDamage = JSCallbackHolder(ctx, onBeforeDurabilityDamage);
        JS_FreeValue(ctx, onBeforeDurabilityDamage);
    }

    JSValue onCompleteUse = extractCallback(ctx, componentObj, "onCompleteUse");
    if (!JS_IsUndefined(onCompleteUse)) {
        callbacks->onCompleteUse = JSCallbackHolder(ctx, onCompleteUse);
        JS_FreeValue(ctx, onCompleteUse);
    }

    JSValue onConsume = extractCallback(ctx, componentObj, "onConsume");
    if (!JS_IsUndefined(onConsume)) {
        callbacks->onConsume = JSCallbackHolder(ctx, onConsume);
        JS_FreeValue(ctx, onConsume);
    }

    if (!callbacks->hasAnyCallback()) {
        spdlog::warn("[BedrockAddon] registerItemCustomComponent: no callbacks found for {}", typeId);
    }

    // 创建ItemCustomComponent并注册
    ItemCustomComponent component;
    component.name = typeId;

    if (!JS_IsUndefined(callbacks->onUse.func())) {
        component.onUse = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                              ItemComponentUseEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onUse.context());
            JS_SetPropertyStr(cb->onUse.context(),
                eventObj,
                "itemTypeId",
                JS_NewString(cb->onUse.context(), event.itemTypeId.c_str()));
            JS_SetPropertyStr(
                cb->onUse.context(), eventObj, "sourceId", JS_NewBigUint64(cb->onUse.context(), event.sourceId));
            cb->onUse.call(eventObj);
            JS_FreeValue(cb->onUse.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onUseOn.func())) {
        component.onUseOn = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                ItemComponentUseOnEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onUseOn.context());
            JS_SetPropertyStr(cb->onUseOn.context(),
                eventObj,
                "itemTypeId",
                JS_NewString(cb->onUseOn.context(), event.itemTypeId.c_str()));
            JS_SetPropertyStr(
                cb->onUseOn.context(), eventObj, "sourceId", JS_NewBigUint64(cb->onUseOn.context(), event.sourceId));
            JS_SetPropertyStr(cb->onUseOn.context(), eventObj, "x", JS_NewInt32(cb->onUseOn.context(), event.blockX));
            JS_SetPropertyStr(cb->onUseOn.context(), eventObj, "y", JS_NewInt32(cb->onUseOn.context(), event.blockY));
            JS_SetPropertyStr(cb->onUseOn.context(), eventObj, "z", JS_NewInt32(cb->onUseOn.context(), event.blockZ));
            JS_SetPropertyStr(cb->onUseOn.context(), eventObj, "face", JS_NewInt32(cb->onUseOn.context(), event.face));
            cb->onUseOn.call(eventObj);
            JS_FreeValue(cb->onUseOn.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onHitEntity.func())) {
        component.onHitEntity = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                    ItemComponentHitEntityEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onHitEntity.context());
            JS_SetPropertyStr(cb->onHitEntity.context(),
                eventObj,
                "itemTypeId",
                JS_NewString(cb->onHitEntity.context(), event.itemTypeId.c_str()));
            JS_SetPropertyStr(cb->onHitEntity.context(),
                eventObj,
                "attackingEntityId",
                JS_NewBigUint64(cb->onHitEntity.context(), event.attackingEntityId));
            JS_SetPropertyStr(cb->onHitEntity.context(),
                eventObj,
                "hitEntityId",
                JS_NewBigUint64(cb->onHitEntity.context(), event.hitEntityId));
            cb->onHitEntity.call(eventObj);
            JS_FreeValue(cb->onHitEntity.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onMineBlock.func())) {
        component.onMineBlock = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                    ItemComponentMineBlockEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onMineBlock.context());
            JS_SetPropertyStr(cb->onMineBlock.context(),
                eventObj,
                "itemTypeId",
                JS_NewString(cb->onMineBlock.context(), event.itemTypeId.c_str()));
            JS_SetPropertyStr(cb->onMineBlock.context(),
                eventObj,
                "sourceId",
                JS_NewBigUint64(cb->onMineBlock.context(), event.sourceId));
            JS_SetPropertyStr(
                cb->onMineBlock.context(), eventObj, "x", JS_NewInt32(cb->onMineBlock.context(), event.blockX));
            JS_SetPropertyStr(
                cb->onMineBlock.context(), eventObj, "y", JS_NewInt32(cb->onMineBlock.context(), event.blockY));
            JS_SetPropertyStr(
                cb->onMineBlock.context(), eventObj, "z", JS_NewInt32(cb->onMineBlock.context(), event.blockZ));
            cb->onMineBlock.call(eventObj);
            JS_FreeValue(cb->onMineBlock.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onBeforeDurabilityDamage.func())) {
        component.onBeforeDurabilityDamage = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                                 ItemComponentBeforeDurabilityDamageEvent& event,
                                                 const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onBeforeDurabilityDamage.context());
            JS_SetPropertyStr(cb->onBeforeDurabilityDamage.context(),
                eventObj,
                "itemTypeId",
                JS_NewString(cb->onBeforeDurabilityDamage.context(), event.itemTypeId.c_str()));
            JS_SetPropertyStr(cb->onBeforeDurabilityDamage.context(),
                eventObj,
                "attackingEntityId",
                JS_NewBigUint64(cb->onBeforeDurabilityDamage.context(), event.attackingEntityId));
            JS_SetPropertyStr(cb->onBeforeDurabilityDamage.context(),
                eventObj,
                "hitEntityId",
                JS_NewBigUint64(cb->onBeforeDurabilityDamage.context(), event.hitEntityId));
            JS_SetPropertyStr(cb->onBeforeDurabilityDamage.context(),
                eventObj,
                "durabilityDamage",
                JS_NewInt32(cb->onBeforeDurabilityDamage.context(), event.durabilityDamage));
            cb->onBeforeDurabilityDamage.call(eventObj);
            // 读取JS修改后的durabilityDamage值
            JSValue dmgVal = JS_GetPropertyStr(cb->onBeforeDurabilityDamage.context(), eventObj, "durabilityDamage");
            i32 newDmg = 0;
            if (JS_ToInt32(cb->onBeforeDurabilityDamage.context(), &newDmg, dmgVal) == 0) {
                event.durabilityDamage = newDmg;
            }
            JS_FreeValue(cb->onBeforeDurabilityDamage.context(), dmgVal);
            JS_FreeValue(cb->onBeforeDurabilityDamage.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onCompleteUse.func())) {
        component.onCompleteUse = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                      ItemComponentCompleteUseEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onCompleteUse.context());
            JS_SetPropertyStr(cb->onCompleteUse.context(),
                eventObj,
                "itemTypeId",
                JS_NewString(cb->onCompleteUse.context(), event.itemTypeId.c_str()));
            JS_SetPropertyStr(cb->onCompleteUse.context(),
                eventObj,
                "sourceId",
                JS_NewBigUint64(cb->onCompleteUse.context(), event.sourceId));
            JS_SetPropertyStr(cb->onCompleteUse.context(),
                eventObj,
                "useDuration",
                JS_NewInt32(cb->onCompleteUse.context(), event.useDuration));
            cb->onCompleteUse.call(eventObj);
            JS_FreeValue(cb->onCompleteUse.context(), eventObj);
        };
    }

    if (!JS_IsUndefined(callbacks->onConsume.func())) {
        component.onConsume = [cb = std::shared_ptr<ItemJSCallbacks>(callbacks)](
                                  ItemComponentConsumeEvent& event, const CustomComponentParameters&) {
            JSValue eventObj = JS_NewObject(cb->onConsume.context());
            JS_SetPropertyStr(cb->onConsume.context(),
                eventObj,
                "itemTypeId",
                JS_NewString(cb->onConsume.context(), event.itemTypeId.c_str()));
            JS_SetPropertyStr(cb->onConsume.context(),
                eventObj,
                "sourceId",
                JS_NewBigUint64(cb->onConsume.context(), event.sourceId));
            cb->onConsume.call(eventObj);
            JS_FreeValue(cb->onConsume.context(), eventObj);
        };
    }

    // 注册到全局ItemComponentRegistry
    ItemComponentRegistry::instance().registerComponent(typeId, std::move(component));

    spdlog::info("[BedrockAddon] Registered item custom component for '{}'", typeId);

    return true;
}

// ============================================================================
// JS全局方法 - BlockComponentRegistry.registerCustomComponent
// ============================================================================

static JSValue blockComponentRegistryRegister(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "blockComponentRegistry.registerCustomComponent requires 2 arguments");
    }

    // 第一个参数：typeId (string)
    const char* typeIdStr = JS_ToCString(ctx, argv[0]);
    if (!typeIdStr) {
        return JS_ThrowTypeError(ctx, "First argument must be a string (typeId)");
    }
    std::string typeId(typeIdStr);
    JS_FreeCString(ctx, typeIdStr);

    // 第二个参数：component (object)
    if (!JS_IsObject(argv[1])) {
        return JS_ThrowTypeError(ctx, "Second argument must be an object (component)");
    }

    bool success = registerBlockCustomComponentFromJS(typeId, JS_DupValue(ctx, argv[1]), ctx);
    if (!success) {
        return JS_ThrowTypeError(ctx, "Failed to register block custom component for '%s'", typeId.c_str());
    }

    return JS_UNDEFINED;
}

// ============================================================================
// JS全局方法 - ItemComponentRegistry.registerCustomComponent
// ============================================================================

static JSValue itemComponentRegistryRegister(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "itemComponentRegistry.registerCustomComponent requires 2 arguments");
    }

    // 第一个参数：typeId (string)
    const char* typeIdStr = JS_ToCString(ctx, argv[0]);
    if (!typeIdStr) {
        return JS_ThrowTypeError(ctx, "First argument must be a string (typeId)");
    }
    std::string typeId(typeIdStr);
    JS_FreeCString(ctx, typeIdStr);

    // 第二个参数：component (object)
    if (!JS_IsObject(argv[1])) {
        return JS_ThrowTypeError(ctx, "Second argument must be an object (component)");
    }

    bool success = registerItemCustomComponentFromJS(typeId, JS_DupValue(ctx, argv[1]), ctx);
    if (!success) {
        return JS_ThrowTypeError(ctx, "Failed to register item custom component for '%s'", typeId.c_str());
    }

    return JS_UNDEFINED;
}

// ============================================================================
// 导出注册函数 - 供MinecraftModuleFactory调用
// ============================================================================

void registerCustomComponentBindings(NativeModuleBuilder& builder, JSContext* ctx)
{
    // 注册 BlockComponentRegistry 全局对象
    JSValue blockRegObj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx,
        blockRegObj,
        "registerCustomComponent",
        JS_NewCFunction(ctx, blockComponentRegistryRegister, "registerCustomComponent", 2));
    builder.exportValue("blockComponentRegistry", blockRegObj);

    // 注册 ItemComponentRegistry 全局对象
    JSValue itemRegObj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx,
        itemRegObj,
        "registerCustomComponent",
        JS_NewCFunction(ctx, itemComponentRegistryRegister, "registerCustomComponent", 2));
    builder.exportValue("itemComponentRegistry", itemRegObj);

    spdlog::info("[BedrockAddon] Registered custom component bindings for @minecraft/server");
}

} // namespace mc::mod::bedrock::addon
