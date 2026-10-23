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
            auto work = std::make_shared<_ScheduledWork>(ScriptValueHandle(bindings, args[0]), 0);
            const auto callback = [work]() { _invoke(work); };
            const auto id =
                recurring ? scheduler->runInterval(callback, ticks) : scheduler->runTimeout(callback, ticks);
            _track(bindings, *scheduler, work, id, false);
            return bindings.createInt32(id);
        },
        immediate ? 1 : 2);
}

} // namespace

void registerSystemBindings(IScriptBindingContext& context, void* prototype, ScriptScheduler* scheduler)
{
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
