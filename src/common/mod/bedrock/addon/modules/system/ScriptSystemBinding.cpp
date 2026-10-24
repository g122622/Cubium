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

#include "common/mod/bedrock/addon/modules/system/ScriptSystemBinding.hpp"

#include "common/mod/bedrock/addon/binding/ScriptCallbackHolder.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptScheduler.hpp"
#include "common/mod/bedrock/addon/modules/types/ScriptWorldAccessor.hpp"

#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventData.hpp"

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {
namespace {

struct _ScheduledWork {
    ScriptValueHandle value;
    u64 cleanupId;

    ~_ScheduledWork()
    {
        if (auto* context = value.context()) {
            context->removeDisposeCallback(cleanupId);
        }
    }
};

void _reportException(IScriptBindingContext& context, void* result)
{
    if (context.isException(result)) {
        void* exception = context.getException();
        spdlog::error("[BedrockAddon] Scheduled script callback failed: {}", context.getExceptionMessage(exception));
        context.releaseValue(exception);
    }

    context.releaseValue(result);
}

void _invoke(const std::shared_ptr<_ScheduledWork>& work)
{
    if (auto* context = work->value.context()) {
        void* receiver = context->createUndefined();
        void* result = context->callFunction0(work->value.value(), receiver);
        context->releaseValue(receiver);
        _reportException(*context, result);
    }
}

void _track(IScriptBindingContext& context,
    ScriptScheduler& scheduler,
    const std::shared_ptr<_ScheduledWork>& work,
    ScriptScheduler::RunId id,
    bool job)
{
    // 任务与上下文独立持有生命周期：上下文先销毁时，必须主动取消调度。
    const std::weak_ptr<_ScheduledWork> weak = work;
    work->cleanupId = context.onDispose([weak, &scheduler, id, job]() {
        if (const auto item = weak.lock()) {
            item->cleanupId = 0;
            if (job) {
                scheduler.clearJob(id);
            } else {
                scheduler.clearRun(id);
            }
        }
    });
}

void _registerTimer(IScriptBindingContext& context,
    void* prototype,
    ScriptScheduler* scheduler,
    const char* name,
    bool recurring,
    bool immediate)
{
    context.registerMethod(
        prototype,
        name,
        [scheduler, recurring, immediate](IScriptBindingContext& bindings, void*, i32 argc, void** args) -> void* {
            if (argc < 1 || !bindings.isFunction(args[0])) {
                return bindings.throwTypeError("System scheduling requires a function");
            }

            if (scheduler == nullptr) {
                return bindings.throwInternalError("Script scheduler is not available");
            }

            u32 ticks = 1;
            if (!immediate && argc > 1 && !bindings.isUndefined(args[1])) {
                const auto value = bindings.toFloat64(args[1]);
                if (!value || !std::isfinite(*value) || *value < 0 || *value > std::numeric_limits<u32>::max()) {
                    return bindings.throwTypeError("Tick delay must be a finite nonnegative integer");
                }

                ticks = static_cast<u32>(*value);
            }

            // 调度器拥有回调副本；完成或取消任务时归还其独立脚本引用。
            auto work = std::make_shared<_ScheduledWork>(ScriptValueHandle(bindings, args[0]), 0);
            const auto callback = [work]() {
                _invoke(work);
            };
            const auto id =
                recurring ? scheduler->runInterval(callback, ticks) : scheduler->runTimeout(callback, ticks);
            _track(bindings, *scheduler, work, id, false);

            return bindings.createInt32(id);
        },
        immediate ? 1 : 2);
}

} // namespace

void registerSystemBindings(
    IScriptBindingContext& context, void* prototype, ScriptScheduler* scheduler, ScriptEventBus* events)
{
    context.registerMethod(
        prototype,
        "sendScriptEvent",
        [events](IScriptBindingContext& bindings, void*, i32 argc, void** args) -> void* {
            if (argc < 2 || !bindings.isString(args[0]) || !bindings.isString(args[1])) {
                return bindings.throwTypeError("sendScriptEvent requires an ID and message string");
            }

            const auto id = *bindings.toString(args[0]);
            const auto message = *bindings.toString(args[1]);
            const auto separator = id.find(':');
            if (separator == std::string::npos || separator == 0 || separator + 1 == id.size()) {
                return bindings.throwTypeError("Script event ID must contain a namespace and name");
            }

            if (message.size() > 2048) {
                return bindings.throwTypeError("Script event message exceeds 2048 bytes");
            }

            if (events == nullptr) {
                return bindings.throwInternalError("Script event bus is not available");
            }

            // 消息在后置事件批次中分发，发送方和接收方可以属于不同插件。
            ScriptEventData event;
            event.setString("id", id);
            event.setString("message", message);
            event.setString("sourceType", "Server");
            events->enqueueAfterEvent(typeid(ScriptMessageEvent), std::move(event));

            return bindings.createUndefined();
        },
        2);
    _registerTimer(context, prototype, scheduler, "run", false, true);
    _registerTimer(context, prototype, scheduler, "runTimeout", false, false);
    _registerTimer(context, prototype, scheduler, "runInterval", true, false);

    context.registerMethod(
        prototype,
        "clearRun",
        [scheduler](IScriptBindingContext& bindings, void*, i32 argc, void** args) -> void* {
            if (argc < 1 || !bindings.isNumber(args[0])) {
                return bindings.throwTypeError("system.clearRun requires a run ID");
            }

            if (scheduler != nullptr) {
                scheduler->clearRun(*bindings.toInt32(args[0]));
            }

            return bindings.createUndefined();
        },
        1);
    context.registerReadonlyProperty(prototype, "currentTick", [](IScriptBindingContext& bindings, void*) -> void* {
        return bindings.createFloat64(static_cast<f64>(ScriptWorldAccessor::instance().currentTick()));
    });
    context.registerMethod(
        prototype,
        "runJob",
        [scheduler](IScriptBindingContext& bindings, void*, i32 argc, void** args) -> void* {
            if (argc < 1 || !bindings.isObject(args[0])) {
                return bindings.throwTypeError("system.runJob requires a generator");
            }

            void* next = bindings.getProperty(args[0], "next");
            const bool valid = bindings.isFunction(next);
            bindings.releaseValue(next);
            if (!valid) {
                return bindings.throwTypeError("system.runJob requires a generator with a next method");
            }

            if (scheduler == nullptr) {
                return bindings.throwInternalError("Script scheduler is not available");
            }

            auto work = std::make_shared<_ScheduledWork>(ScriptValueHandle(bindings, args[0]), 0);
            // next() 每次只推进一次；时间片及公平轮转由调度器统一控制。
            const auto id = scheduler->runJob([work]() {
                auto* context = work->value.context();
                if (context == nullptr) {
                    return true;
                }

                void* next = context->getProperty(work->value.value(), "next");
                void* result = context->callFunction0(next, work->value.value());
                context->releaseValue(next);
                if (context->isException(result)) {
                    _reportException(*context, result);

                    return true;
                }

                if (!context->isObject(result)) {
                    context->releaseValue(result);
                    spdlog::error("[BedrockAddon] Generator next() did not return an object");

                    return true;
                }

                const bool done = context->getPropertyBool(result, "done").value_or(false);
                context->releaseValue(result);

                return done;
            });
            _track(bindings, *scheduler, work, id, true);

            return bindings.createInt32(id);
        },
        1);
    context.registerMethod(
        prototype,
        "clearJob",
        [scheduler](IScriptBindingContext& bindings, void*, i32 argc, void** args) -> void* {
            if (argc < 1 || !bindings.isNumber(args[0])) {
                return bindings.throwTypeError("system.clearJob requires a job ID");
            }

            if (scheduler != nullptr) {
                scheduler->clearJob(*bindings.toInt32(args[0]));
            }

            return bindings.createUndefined();
        },
        1);
}

} // namespace mc::mod::bedrock::addon
