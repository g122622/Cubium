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

#include "common/mod/bedrock/addon/binding/ScriptCallbackHolder.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/event/AfterEventSignal.hpp"
#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventData.hpp"

#include <algorithm>
#include <any>
#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {
namespace {

using _Initializer = std::function<void(IScriptBindingContext&, void*, const std::string&)>;

struct _Subscription {
    ScriptCallbackHolder callback;
    ScriptEventHandler handle;
};

struct _Signal {
    IScriptBindingContext* context;
    ScriptEventBus* bus;
    EventSignalInfo info;
    _Initializer initialize;
    std::vector<_Subscription> subscriptions;
    u64 cleanupId;

    ~_Signal()
    {
        close();
    }

    void close()
    {
        if (context == nullptr) {
            return;
        }

        // 先断开总线订阅，再释放脚本函数，防止后续批次进入正在销毁的上下文。
        context->removeDisposeCallback(cleanupId);
        for (const auto& subscription : subscriptions) {
            if (info.isBefore) {
                bus->beforeEvents().unsubscribe(subscription.handle);
            } else {
                bus->afterEvents().unsubscribe(subscription.handle);
            }
        }

        subscriptions.clear();
        context = nullptr;
        bus = nullptr;
    }
};

bool _same(IScriptBindingContext& context, void* left, void* right)
{
    void* global = context.getGlobalObject();
    void* object = context.getProperty(global, "Object");
    void* compare = context.getProperty(object, "is");
    std::array<void*, 2> arguments{left, right};
    void* result = context.callFunction(compare, object, 2, arguments.data());
    const bool same = context.toBool(result).value_or(false);
    context.releaseValue(result);
    context.releaseValue(compare);
    context.releaseValue(object);
    context.releaseValue(global);

    return same;
}

bool _contains(IScriptBindingContext& context, void* list, const std::string& value)
{
    const auto count = context.getPropertyInt(list, "length").value_or(0);
    for (i32 index = 0; index < count; ++index) {
        if (context.getPropertyString(list, std::to_string(index).c_str()) == value) {
            return true;
        }
    }

    return false;
}

bool _matches(IScriptBindingContext& context, void* event, const ScriptValueHandle& options)
{
    if (options.value() == nullptr) {
        return true;
    }

    void* eventTypes = context.getProperty(options.value(), "eventTypes");
    if (context.isObject(eventTypes)) {
        const auto eventId = context.getPropertyString(event, "eventId");
        const bool found = eventId && _contains(context, eventTypes, *eventId);
        context.releaseValue(eventTypes);
        if (!found) {
            return false;
        }
    } else {
        context.releaseValue(eventTypes);
    }

    // 脚本消息只按冒号之前的命名空间筛选，避免处理其他包的消息通道。
    void* namespaces = context.getProperty(options.value(), "namespaces");
    if (context.isObject(namespaces)) {
        const std::string id = context.getPropertyString(event, "id").value_or("");
        const bool found = _contains(context, namespaces, id.substr(0, id.find(':')));
        context.releaseValue(namespaces);
        if (!found) {
            return false;
        }
    } else {
        context.releaseValue(namespaces);
    }

    // 不同事件使用不同主体字段，统一取出主体后再应用类型及身份过滤。
    void* entity = context.createUndefined();
    for (const char* key : {"entity", "hurtEntity", "deadEntity", "target", "player"}) {
        context.releaseValue(entity);
        entity = context.getProperty(event, key);
        if (context.isObject(entity)) {
            break;
        }
    }

    const auto type = context.isObject(entity) ? context.getPropertyString(entity, "typeId") : std::nullopt;
    const auto id = context.isObject(entity) ? context.getPropertyString(entity, "id") : std::nullopt;
    bool matches = true;
    void* types = context.getProperty(options.value(), "entityTypes");
    if (context.isObject(types)) {
        matches = type && _contains(context, types, *type);
    }

    context.releaseValue(types);

    void* filter = context.getProperty(options.value(), "entityFilter");
    if (context.isObject(filter)) {
        const auto expected = context.getPropertyString(filter, "type");
        if (expected) {
            matches = matches && type == expected;
        }
        // TODO: 统一实体查询器接入后补齐距离、标签和家族等 EntityFilter 条件。
    }

    context.releaseValue(filter);

    void* entities = context.getProperty(options.value(), "entities");
    if (context.isObject(entities)) {
        bool found = false;
        const auto count = context.getPropertyInt(entities, "length").value_or(0);
        for (i32 index = 0; index < count; ++index) {
            void* candidate = context.getProperty(entities, std::to_string(index).c_str());
            found = found || (id && context.getPropertyString(candidate, "id") == id);
            context.releaseValue(candidate);
        }

        matches = matches && found;
    }

    context.releaseValue(entities);
    context.releaseValue(entity);

    return matches;
}

bool _invoke(const ScriptCallbackHolder& callback,
    const ScriptValueHandle& options,
    const _Initializer& initialize,
    const std::string& name,
    const ScriptEventData& data,
    bool before)
{
    auto* context = callback.context();
    if (context == nullptr) {
        return data.cancelled;
    }

    // 每个订阅者使用所属上下文的游戏对象包装，不能跨插件复用事件 JS 对象。
    void* object = data.createObject(*context);
    if (before) {
        context->setPropertyBool(object, "cancel", data.cancelled);
    }

    if (initialize) {
        initialize(*context, object, name);
    }

    if (!_matches(*context, object, options)) {
        context->releaseValue(object);

        return data.cancelled;
    }

    void* result = callback.call(object);
    if (context->isException(result)) {
        void* exception = context->getException();
        spdlog::error("[BedrockAddon] Event '{}' callback failed: {}", name, context->getExceptionMessage(exception));
        context->releaseValue(exception);
    }

    // 回调结束后读取取消标志，由前置分发链统一回传到原生游戏事件。
    const bool cancelled = before && context->getPropertyBool(object, "cancel").value_or(data.cancelled);
    context->releaseValue(result);
    context->releaseValue(object);

    return cancelled;
}

} // namespace

void registerEventBindings(IScriptBindingContext& context,
    void* target,
    ScriptEventBus& bus,
    const std::vector<EventSignalInfo>& signals,
    _Initializer initializeEvent)
{
    const auto classId = context.allocateClassId();
    context.registerClass(classId, "EventSignal", true);
    void* prototype = context.createClassProto(classId);
    context.registerMethod(
        prototype,
        "subscribe",
        [](IScriptBindingContext& context, void* receiver, i32 argc, void** args) -> void* {
            if (argc < 1 || !context.isFunction(args[0])) {
                return context.throwTypeError("subscribe requires a callback");
            }

            auto* pointer = static_cast<std::shared_ptr<_Signal>*>(ScriptObjectRegistry::unwrap(context, receiver, 0));
            if (pointer == nullptr || (*pointer)->bus == nullptr) {
                return context.throwTypeError("Invalid event signal");
            }

            auto& signal = **pointer;
            for (const auto& subscription : signal.subscriptions) {
                if (_same(context, subscription.callback.func(), args[0])) {
                    return context.dupValue(args[0]);
                }
            }

            ScriptValueHandle options;
            if (argc > 1 && !context.isUndefined(args[1])) {
                if (!context.isObject(args[1])) {
                    return context.throwTypeError("Event options must be an object");
                }

                options = ScriptValueHandle(context, args[1]);
            }

            // 订阅须持有独立句柄，原生调用结束后参数包装句柄即失效。
            ScriptCallbackHolder callback(context, args[0]);
            ScriptEventHandler handle;
            if (signal.info.isBefore) {
                handle = signal.bus->beforeEvents().subscribe(signal.info.typeIdx,
                    [callback, options, initialize = signal.initialize, name = signal.info.name](std::any& value) {
                        auto& data = std::any_cast<ScriptEventData&>(value);
                        data.cancelled = _invoke(callback, options, initialize, name, data, true) || data.cancelled;

                        return data.cancelled;
                    });
            } else {
                handle = signal.bus->afterEvents().subscribe(signal.info.typeIdx,
                    [callback, options, initialize = signal.initialize, name = signal.info.name](
                        const std::any& value) {
                        _invoke(
                            callback, options, initialize, name, std::any_cast<const ScriptEventData&>(value), false);
                    });
            }

            signal.subscriptions.push_back({std::move(callback), handle});

            return context.dupValue(args[0]);
        },
        2);
    context.registerMethod(
        prototype,
        "unsubscribe",
        [](IScriptBindingContext& context, void* receiver, i32 argc, void** args) -> void* {
            if (argc < 1 || !context.isFunction(args[0])) {
                return context.throwTypeError("unsubscribe requires a callback");
            }

            auto* pointer = static_cast<std::shared_ptr<_Signal>*>(ScriptObjectRegistry::unwrap(context, receiver, 0));
            if (pointer == nullptr || (*pointer)->bus == nullptr) {
                return context.throwTypeError("Invalid event signal");
            }

            auto& signal = **pointer;
            for (auto it = signal.subscriptions.begin(); it != signal.subscriptions.end();) {
                if (_same(context, it->callback.func(), args[0])) {
                    if (signal.info.isBefore) {
                        signal.bus->beforeEvents().unsubscribe(it->handle);
                    } else {
                        signal.bus->afterEvents().unsubscribe(it->handle);
                    }

                    it = signal.subscriptions.erase(it);
                } else {
                    ++it;
                }
            }

            return context.createUndefined();
        },
        1);
    void* before = context.createObject();
    void* after = context.createObject();
    for (const auto& info : signals) {
        auto signal = std::make_shared<_Signal>(&context, &bus, info, initializeEvent, std::vector<_Subscription>{}, 0);
        const std::weak_ptr<_Signal> weak = signal;
        signal->cleanupId = context.onDispose([weak]() {
            if (auto state = weak.lock()) {
                state->close();
            }
        });
        auto* owned = std::make_unique<std::shared_ptr<_Signal>>(std::move(signal)).release();
        void* value = ScriptObjectRegistry::wrap(
            context,
            classId,
            prototype,
            owned,
            true,
            "EventSignal",
            [](void* pointer) {
                delete static_cast<std::shared_ptr<_Signal>*>(pointer);
            },
            0,
            nullptr);
        context.setProperty(info.isBefore ? before : after, info.name.c_str(), value);
        context.releaseValue(value);
    }

    context.setProperty(target, "beforeEvents", before);
    context.setProperty(target, "afterEvents", after);
    context.releaseValue(before);
    context.releaseValue(after);
}

} // namespace mc::mod::bedrock::addon
