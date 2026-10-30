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

// 绑定层句柄生命周期测试：类原型句柄的所有权归绑定上下文（借用语义），
// 上下文析构时必须全部归还，否则句柄自身与其持有的 JS 类原型引用图一并泄漏。
// 回归背景：nightly 集成测试 20/24 分片因 LeakSanitizer 报出 5.7 MB / 55013 次分配泄漏
// 而以 SIGABRT 退出（句柄未释放 → 原型引用计数非 0 → JS_FreeRuntime 回收不掉原型对象）。

#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassBinding.hpp"
#include "common/mod/bedrock/addon/binding/ScriptClassRegistry.hpp"
#include "common/mod/bedrock/addon/core/IScriptContext.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/IScriptRuntime.hpp"

#include <cstddef>
#include <optional>

#include <gtest/gtest.h>

using namespace mc::mod::bedrock::addon;

namespace {

ContextConfig _makeConfig()
{
    // ContextConfig 无默认值，必须显式传入内存/栈上限（见 core/README.md 坑 2）。
    return ContextConfig{
        Capabilities{},                   // 无特殊能力
        std::nullopt,                     // 无特殊权限
        DEFAULT_CONTEXT_MEMORY_BYTES,     // 64MB 内存限制
        DEFAULT_CONTEXT_STACK_SIZE_BYTES, // 4MB 栈限制
    };
}

std::size_t _probeHandleCount(IScriptEngine& engine)
{
    auto probe = engine.runtime().createContext(_makeConfig());
    if (probe == nullptr) {
        return static_cast<std::size_t>(-1);
    }

    return probe->bindingContext().liveClassProtoHandleCount();
}

} // namespace

TEST(ScriptBindingHandleLifetimeTest, ContextDestructionReleasesClassProtoHandles)
{
    auto engine = createScriptEngine();
    ASSERT_NE(engine, nullptr);
    ASSERT_TRUE(engine->initialize());

    const std::size_t baseline = _probeHandleCount(*engine);
    ASSERT_NE(baseline, static_cast<std::size_t>(-1));

    constexpr int kProtoCount = 8;
    {
        auto context = engine->runtime().createContext(_makeConfig());
        ASSERT_NE(context, nullptr);
        auto& bindings = context->bindingContext();

        for (int i = 0; i < kProtoCount; ++i) {
            const mc::u64 classId = bindings.allocateClassId();
            ASSERT_TRUE(bindings.registerClass(classId, "HandleProbeClass", true));
            void* proto = bindings.createClassProto(classId);
            ASSERT_NE(proto, nullptr);
            bindings.registerMethod(proto,
                "probe",
                [](IScriptBindingContext& ctx, void* /*thisVal*/, mc::i32 /*argc*/, void** /*args*/) -> void* {
                    return ctx.createInt32(1);
                });
        }

        // 上下文存活期间句柄必须在位（调用方借用，不得释放）。
        EXPECT_EQ(bindings.liveClassProtoHandleCount(), baseline + kProtoCount);
    }

    // 上下文析构后：句柄所有权归上下文，必须已全部释放（否则句柄与原型引用图双双泄漏）。
    EXPECT_EQ(_probeHandleCount(*engine), baseline)
        << "绑定上下文析构后仍有类原型句柄未释放（createClassProto 的句柄所有权未归还）";

    engine->shutdown();
}

TEST(ScriptBindingHandleLifetimeTest, ClassRegistriesAreIsolatedBetweenContexts)
{
    auto engine = createScriptEngine();
    ASSERT_TRUE(engine->initialize());
    auto first = engine->runtime().createContext(_makeConfig());
    auto second = engine->runtime().createContext(_makeConfig());
    auto& firstBindings = first->bindingContext();
    auto& secondBindings = second->bindingContext();
    const auto firstId = firstBindings.allocateClassId();
    const auto secondId = secondBindings.allocateClassId();
    ASSERT_TRUE(firstBindings.registerClass(firstId, "Entity", true));
    ASSERT_TRUE(secondBindings.registerClass(secondId, "Entity", true));
    void* firstProto = firstBindings.createClassProto(firstId);
    void* secondProto = secondBindings.createClassProto(secondId);
    firstBindings.classRegistry().registerClass(firstId, firstProto, "Entity");
    secondBindings.classRegistry().registerClass(secondId, secondProto, "Entity");
    EXPECT_EQ(firstBindings.classRegistry().classIdByName("Entity"), firstId);
    EXPECT_EQ(secondBindings.classRegistry().classIdByName("Entity"), secondId);
    EXPECT_EQ(firstBindings.classRegistry().protoByName("Entity"), firstProto);
    first.reset();
    EXPECT_EQ(secondBindings.classRegistry().protoByName("Entity"), secondProto);
    EXPECT_EQ(secondBindings.classRegistry().classIdByName("Entity"), secondId);
    second.reset();
    engine->shutdown();
}

TEST(ScriptBindingHandleLifetimeTest, RepeatedContextRecreationDoesNotAccumulateHandles)
{
    // 引擎在同一进程内反复重建上下文（GameTestServer 多次运行、脚本热重载）时，
    // 句柄计数不得随次数单调增长。
    auto engine = createScriptEngine();
    ASSERT_NE(engine, nullptr);
    ASSERT_TRUE(engine->initialize());

    const std::size_t baseline = _probeHandleCount(*engine);
    ASSERT_NE(baseline, static_cast<std::size_t>(-1));

    for (int round = 0; round < 4; ++round) {
        auto context = engine->runtime().createContext(_makeConfig());
        ASSERT_NE(context, nullptr);
        auto& bindings = context->bindingContext();
        const mc::u64 classId = bindings.allocateClassId();
        ASSERT_TRUE(bindings.registerClass(classId, "RoundProbeClass", true));
        ASSERT_NE(bindings.createClassProto(classId), nullptr);
        EXPECT_EQ(bindings.liveClassProtoHandleCount(), baseline + 1);
    }

    EXPECT_EQ(_probeHandleCount(*engine), baseline);

    engine->shutdown();
}

TEST(ScriptBindingHandleLifetimeTest, WrappedInstancesCarryOpaqueWhenClassRegisteredAfterPrototype)
{
    // 回归背景：GameTest 中 `new ItemStack(...)` 之后，useItemOnBlock/setItem 一律报
    // "first arg must be an ItemStack"。根因是类原型被建成普通对象（内置 Object 类），
    // 以它为原型的实例同为普通对象，JS_SetOpaque 对内置类对象静默失败，opaque 永远为 NULL。
    // 该缺陷只在"类注册晚于原型创建"时暴露，因此本用例故意不先注册类：
    // createClassProto 必须让原型自带目标 class，构造出的实例才可能承载 opaque。
    //
    // 另一个前置条件：createFunction 产出的 C 函数必须可被 `new`。QuickJS 的
    // JS_CFUNC_generic_magic 不带 is_constructor 位，脚本侧 `new Xxx()` 会在
    // JS_IsConstructor 检查处直接抛 "not a constructor"，根本进不了构造回调；
    // bindings 层因此改用 JS_CFUNC_constructor_or_func_magic（见 QuickJSBindingContext::createFunction）。
    auto engine = createScriptEngine();
    ASSERT_NE(engine, nullptr);
    ASSERT_TRUE(engine->initialize());
    auto context = engine->runtime().createContext(_makeConfig());
    ASSERT_NE(context, nullptr);
    auto& bindings = context->bindingContext();

    const mc::u64 classId = bindings.allocateClassId();
    // 必须先做引擎侧类注册（JS_NewClass + finalizer）：createClassProto 用
    // JS_NewObjectProtoClass(..., classId) 构建原型，要求该 classId 已在 QuickJS 注册，
    // 否则得到的原型挂在一个未注册的类上，上下文销毁时遍历这条链路会访问到无效对象
    // （表现为 0xC0000005）。只调 classRegistry().registerClass 只是登记 C++ 侧查找表，
    // 不注册引擎类——同文件其它用例都是先 bindings.registerClass 再建原型。
    ASSERT_TRUE(bindings.registerClass(classId, "OpaqueProbeClass", true));
    bindings.classRegistry().registerClass(classId, bindings.createClassProto(classId), "OpaqueProbeClass");
    const mc::u64 resolvedClassId = bindings.classRegistry().classIdByName("OpaqueProbeClass");
    void* proto = bindings.classRegistry().proto(resolvedClassId);
    ASSERT_NE(proto, nullptr);

    // 导出构造函数并暴露到全局，使脚本能执行 `new OpaqueProbeClass()`。
    void* ctor = bindings.createFunction(
        [classId](IScriptBindingContext& ctx, void* /*thisVal*/, mc::i32 /*argc*/, void** /*args*/) -> void* {
            static int s_payload = 0;

            return ScriptObjectRegistry::wrap(
                ctx, classId, ctx.classRegistry().proto(classId), &s_payload, false, "OpaqueProbeClass", nullptr);
        },
        "OpaqueProbeClass",
        0);
    void* global = bindings.getGlobalObject();
    ASSERT_NE(global, nullptr);
    bindings.setProperty(global, "OpaqueProbeClass", ctor);
    bindings.releaseValue(ctor);

    auto evaluated =
        context->evaluate("globalThis.probeInstance = new OpaqueProbeClass();", "opaque-probe.js", EvalFlags::None);
    ASSERT_TRUE(evaluated.success()) << "脚本执行失败：" << evaluated.errorMessage();

    // 实例是 "new" 出来的 JS 对象，必须先经 opaque 解出 C++ 指针才能判定包装是否生效。
    void* instance = bindings.getProperty(global, "probeInstance");
    ASSERT_NE(instance, nullptr);
    EXPECT_TRUE(bindings.isObject(instance)) << "new OpaqueProbeClass() 未产出对象";
    EXPECT_NE(bindings.getOpaque(instance, resolvedClassId), nullptr)
        << "class 注册晚于原型创建时，实例 opaque 为空（内置 Object 类不允许挂 opaque）";
    bindings.releaseValue(instance);
    // getGlobalObject 返回的是**新句柄**（内部 wrapValue(JS_GetGlobalObject)），所有权归调用方，
    // 须显式释放（同生产路径 ScriptEventBinding::_same）。不释放会让句柄活到
    // JS_FreeContext 之后，表现为 shutdown 阶段的间歇性 0xC0000005。
    bindings.releaseValue(global);

    engine->shutdown();
}