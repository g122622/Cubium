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

#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/event/AfterEventSignal.hpp"
#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"

#include <any>
#include <memory>
#include <typeindex>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

// ============================================================================
// 信号数据 — 持有对ScriptEventBus和事件类型的引用
// ============================================================================

struct SignalData {
    ScriptEventBus* eventBus;
    std::type_index eventType;
    bool isBefore;
    IScriptBindingContext* bindingCtx;
};

// ============================================================================
// 创建信号JS对象
// ============================================================================

static void* createSignalObject(
    IScriptBindingContext& ctx, u64 signalClassId, ScriptEventBus& eventBus, std::type_index eventType, bool isBefore)
{
    // 必须经 ScriptObjectRegistry::wrap 创建：EventSignal 类的 finalizer 会把 opaque 无条件
    // 按 ObjectData* 解释，若直接 setOpaque 一个 SignalData*，字段偏移将完全错位——会把
    // bindingCtx 当作 destroy 函数指针调用，并越界读取 entityId。owned=true 使 GC 时经
    // destroy 回调释放 SignalData。
    auto* data = std::make_unique<SignalData>(&eventBus, eventType, isBefore, &ctx).release();
    return ScriptObjectRegistry::wrap(
        ctx, signalClassId, ctx.createClassProto(signalClassId), data, true, "EventSignal", [](void* p) {
            delete static_cast<SignalData*>(p);
        });
}

// ============================================================================
// 公共接口 — 由MinecraftModuleFactory.cpp调用
// ============================================================================

void registerEventBindings(
    IScriptBindingContext& ctx, void* worldObj, ScriptEventBus& eventBus, const std::vector<EventSignalInfo>& signals)
{
    // 分配信号类ID
    u64 signalClassId = ScriptObjectRegistry::allocateClassId(ctx);
    ctx.registerClass(signalClassId, "EventSignal", true);

    // 注册信号类的方法
    void* signalProto = ctx.createClassProto(signalClassId);

    // subscribe方法（beforeEvent版本）
    ctx.registerMethod(
        signalProto,
        "subscribe",
        [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isFunction(args[0])) {
                return ctx.throwTypeError("subscribe requires a function argument");
            }

            auto* data = static_cast<SignalData*>(ctx.getOpaque(thisVal, 0));
            if (!data || !data->eventBus) {
                return ctx.throwInternalError("Invalid event signal");
            }

            void* callback = args[0];
            ctx.retainValue(callback);
            auto* bindingCtx = data->bindingCtx;
            auto* eventBus = data->eventBus;
            std::type_index eventType = data->eventType;
            bool isBefore = data->isBefore;

            if (isBefore) {
                auto handle = eventBus->beforeEvents().subscribe(
                    eventType, [bindingCtx, callback, eventType](std::any& eventData) {
                        void* eventObj = bindingCtx->createObject();
                        bindingCtx->setPropertyBool(eventObj, "cancel", false);

                        void* undef = bindingCtx->createUndefined();
                        void* ret = bindingCtx->callFunction1(callback, undef, eventObj);

                        if (bindingCtx->isException(ret)) {
                            void* exc = bindingCtx->getException();
                            auto msg = bindingCtx->getExceptionMessage(exc);
                            spdlog::warn("[BedrockAddon] beforeEvent callback error: {}", msg);
                            bindingCtx->releaseValue(exc);
                        }

                        bindingCtx->releaseValue(ret);
                        bindingCtx->releaseValue(undef);
                        bindingCtx->releaseValue(eventObj);
                    });

                void* handleObj = bindingCtx->createObject();
                bindingCtx->setPropertyInt(handleObj, "id", static_cast<i32>(handle.id));
                return handleObj;
            } else {
                auto handle = eventBus->afterEvents().subscribe(
                    eventType, [bindingCtx, callback, eventType](const std::any& eventData) {
                        void* eventObj = bindingCtx->createObject();

                        void* undef = bindingCtx->createUndefined();
                        void* ret = bindingCtx->callFunction1(callback, undef, eventObj);

                        if (bindingCtx->isException(ret)) {
                            void* exc = bindingCtx->getException();
                            auto msg = bindingCtx->getExceptionMessage(exc);
                            spdlog::warn("[BedrockAddon] afterEvent callback error: {}", msg);
                            bindingCtx->releaseValue(exc);
                        }

                        bindingCtx->releaseValue(ret);
                        bindingCtx->releaseValue(undef);
                        bindingCtx->releaseValue(eventObj);
                    });

                void* handleObj = bindingCtx->createObject();
                bindingCtx->setPropertyInt(handleObj, "id", static_cast<i32>(handle.id));
                return handleObj;
            }
        },
        1);

    // unsubscribe方法
    ctx.registerMethod(
        signalProto,
        "unsubscribe",
        [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
            if (argc < 1 || !ctx.isObject(args[0])) {
                return ctx.throwTypeError("unsubscribe requires a handle object");
            }

            auto* data = static_cast<SignalData*>(ctx.getOpaque(thisVal, 0));
            if (!data || !data->eventBus) {
                return ctx.createUndefined();
            }

            auto idVal = ctx.getPropertyInt(args[0], "id");
            if (!idVal) {
                return ctx.throwTypeError("Invalid event handle");
            }

            ScriptEventHandler handle(static_cast<u64>(*idVal), data->eventType);

            bool success = false;
            if (data->isBefore) {
                success = data->eventBus->beforeEvents().unsubscribe(handle);
            } else {
                success = data->eventBus->afterEvents().unsubscribe(handle);
            }

            return ctx.createBoolean(success);
        },
        1);

    // 创建beforeEvents和afterEvents容器对象
    void* beforeEventsObj = ctx.createObject();
    void* afterEventsObj = ctx.createObject();

    for (const auto& info : signals) {
        void* signal = createSignalObject(ctx, signalClassId, eventBus, info.typeIdx, info.isBefore);
        ctx.retainValue(signal); // 保留信号对象引用

        if (info.isBefore) {
            ctx.setProperty(beforeEventsObj, info.name.c_str(), signal);
        } else {
            ctx.setProperty(afterEventsObj, info.name.c_str(), signal);
        }
    }

    // 将beforeEvents和afterEvents添加到world对象
    ctx.setProperty(worldObj, "beforeEvents", beforeEventsObj);
    ctx.setProperty(worldObj, "afterEvents", afterEventsObj);

    // 释放临时对象
    ctx.releaseValue(beforeEventsObj);
    ctx.releaseValue(afterEventsObj);
    ctx.releaseValue(signalProto);

    spdlog::info("[BedrockAddon] Event bindings registered ({} signals)", signals.size());
}

} // namespace mc::mod::bedrock::addon
