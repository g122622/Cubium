// Copyright (c) 2024 Cubium Project
// SPDX-License-Identifier: MIT

#pragma once

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/binding/IScriptBindingContext.hpp"

#include <functional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本对象注册表
 *
 * 管理C++对象在JS中的包装和生命周期。
 * 通过class ID和opaque机制关联C++/JS对象。
 *
 * 生命周期管理：
 * - owned对象：JS GC时自动delete C++对象
 * - 非owned对象：C++对象由游戏管理，JS仅持有指针引用
 */
class ScriptObjectRegistry {
public:
    /**
     * @brief 为指定类分配新的class ID
     *
     * @param ctx 绑定上下文
     * @return 新分配的class ID
     */
    static u64 allocateClassId(IScriptBindingContext& ctx) { return ctx.allocateClassId(); }

    /**
     * @brief 将C++对象包装为JS对象
     *
     * @param ctx 绑定上下文
     * @param classId 类ID
     * @param proto 原型对象句柄
     * @param ptr C++对象指针
     * @param owned 是否由JS管理生命周期（GC时delete）
     * @param typeName 类型名（调试用）
     * @param destroy 自定义销毁回调（可选，owned=true时默认delete）
     * @param entityId 实体实例ID（可选，仅 Entity 系传非0值）：非0时登记到 ScriptHandleRegistry，
     *        实体销毁时 ScriptHandleRegistry::invalidateAll 置本句柄 ptr=nullptr，防 UAF。
     *        见 ScriptHandleRegistry.hpp 问题背景（owned=false Entity 句柄跨 tick 持裸 Entity*，
     *        实体路径B立即free后悬垂，isOnFire 等回调 UAF 段错误）。
     * @return JS对象句柄（调用者拥有所有权）
     */
    [[nodiscard]] static void* wrap(IScriptBindingContext& ctx,
        u64 classId,
        void* proto,
        void* ptr,
        bool owned,
        const char* typeName,
        void (*destroy)(void*) = nullptr,
        EntityInstanceId entityId = 0);

    /**
     * @brief 从JS对象获取C++指针
     *
     * @param ctx 绑定上下文
     * @param val JS值句柄
     * @param classId 期望的类ID（0表示不检查类型）
     * @return C++对象指针；若实体已销毁（ScriptHandleRegistry 置 nullptr）返回 nullptr
     */
    [[nodiscard]] static void* unwrap(IScriptBindingContext& ctx, void* val, u64 classId = 0);

    /**
     * @brief 检查JS值是否为指定类的实例
     */
    [[nodiscard]] static bool isInstanceOf(IScriptBindingContext& ctx, void* val, u64 classId);

    /**
     * @brief 使关联的C++指针失效（不释放）
     *
     * 当C++对象被外部销毁时调用，防止悬空指针。
     */
    static void invalidate(IScriptBindingContext& ctx, void* val, u64 classId);

    /**
     * @brief 存储在JS对象opaque中的数据
     *
     * entityId 字段：仅 Entity 系 JS 对象（Entity 本身 + OnFire/Health/Movement/Equippable 等组件
     * 对象，均 opaque 持同一 mc::Entity*）非0。wrap 时登记到 ScriptHandleRegistry，实体销毁时
     * invalidateAll 置 ptr=nullptr；QuickJS finalizer 在 delete data 前据此调 unregisterHandle。
     */
    struct ObjectData {
        void* ptr;
        bool owned;
        const char* typeName;
        void (*destroy)(void*);
        EntityInstanceId entityId = 0;
    };
};

/**
 * @brief 原生模块构建器
 *
 * 辅助在脚本上下文中注册原生C++模块。
 * 通过IScriptBindingContext抽象接口操作，不依赖具体引擎。
 *
 * 使用示例：
 * @code
 * NativeModuleBuilder builder(ctx, "@minecraft/server");
 * builder.exportConst("VERSION", 2);
 * builder.exportClass("Entity", entityClassId);
 * builder.finalize();
 * @endcode
 */
class NativeModuleBuilder {
public:
    /**
     * @brief 构造模块构建器
     *
     * @param ctx 绑定上下文
     * @param moduleName 模块名，如 "@minecraft/server"
     */
    NativeModuleBuilder(IScriptBindingContext& ctx, const std::string& moduleName);

    /**
     * @brief 导出整型常量
     */
    NativeModuleBuilder& exportConst(const std::string& name, i32 value);

    /**
     * @brief 导出浮点常量
     */
    NativeModuleBuilder& exportConstFloat(const std::string& name, f64 value);

    /**
     * @brief 导出字符串常量
     */
    NativeModuleBuilder& exportConstString(const std::string& name, const std::string& value);

    /**
     * @brief 导出JS值
     *
     * @param name 导出名
     * @param value JS值句柄（调用者负责引用管理）
     */
    NativeModuleBuilder& exportValue(const std::string& name, void* value);

    /**
     * @brief 在模块中创建一个类并导出构造函数
     *
     * 注册类定义到运行时，创建原型对象，导出构造函数。
     * 返回原型对象句柄供添加方法和属性。
     *
     * @param className 类名
     * @param classId 类ID（由ScriptObjectRegistry::allocateClassId()分配）
     * @return 原型对象句柄，用于添加方法/属性
     */
    [[nodiscard]] void* exportClass(const std::string& className, u64 classId);

    /**
     * @brief 在模块中创建一个类并导出可真实调用的构造函数
     *
     * 与无构造回调的重载不同，此重载允许传入 ctorCallback 实现 `new ClassName(...)`
     * 的真实构造（对齐官方基岩 API，如 `new ItemStack(typeId, amount)`）。回调内负责创建
     * C++ 对象并 wrap 成 JS 值返回；不传此重载的类仍走抛 TypeError 的 stub 构造。
     *
     * @param className 类名
     * @param classId 类ID
     * @param ctorCallback 构造回调（签名同 ScriptMethodCallback）
     * @return 原型对象句柄
     */
    [[nodiscard]] void* exportClass(const std::string& className, u64 classId, ScriptMethodCallback ctorCallback);

    /**
     * @brief 完成模块注册
     *
     * 必须在所有导出操作完成后调用。
     * @return 是否成功
     */
    bool finalize();

    /**
     * @brief 获取绑定上下文
     */
    [[nodiscard]] IScriptBindingContext& context() const { return m_ctx; }

    /**
     * @brief 获取模块名
     */
    [[nodiscard]] const std::string& moduleName() const { return m_moduleName; }

private:
    IScriptBindingContext& m_ctx;
    std::string m_moduleName;
    bool m_finalized = false;
};

/**
 * @brief 类注册辅助器
 *
 * 简化脚本类的原型方法/属性注册。
 * 与NativeModuleBuilder::exportClass配合使用。
 *
 * 使用示例：
 * @code
 * u64 entityClassId = ScriptObjectRegistry::allocateClassId(ctx);
 * void* proto = builder.exportClass("Entity", entityClassId);
 *
 * ClassRegistrar<Entity> registrar(ctx, entityClassId, proto);
 * registrar.method("getId", [](IScriptBindingContext& ctx, void* thisVal, i32 argc, void** args) -> void* {
 *     auto* entity = registrar.unwrap(thisVal);
 *     if (!entity) return ctx.throwTypeError("Invalid Entity");
 *     return ctx.createInt32(entity->getId());
 * });
 * @endcode
 *
 * @tparam T C++类型
 */
template <typename T>
class ClassRegistrar {
public:
    ClassRegistrar(IScriptBindingContext& ctx, u64 classId, void* proto)
        : m_ctx(ctx)
        , m_classId(classId)
        , m_proto(proto)
    {}

    /**
     * @brief 注册实例方法（使用引擎无关回调）
     *
     * @param name JS方法名
     * @param callback 方法回调
     * @param length 参数个数
     */
    ClassRegistrar& method(const std::string& name, ScriptMethodCallback callback, i32 length = 0)
    {
        m_ctx.registerMethod(m_proto, name.c_str(), std::move(callback), length);
        return *this;
    }

    /**
     * @brief 注册静态方法（挂在构造函数对象上，而非原型）
     *
     * JS 中静态方法通过 `Class.staticMethod(...)` 调用，挂在构造函数对象（`Class` 本身）上，
     * 实例方法则挂在原型上。`registerMethod` 只能挂原型（实例方法），故静态方法需另行从原型
     * 取回构造函数对象（`proto.constructor`）再挂函数。官方基岩 API 大量使用静态方法
     * （如 `BlockPermutation.resolve(type, states)`），本方法补齐此能力。
     *
     * 前提：类的构造函数已由 exportClass + setConstructor 关联（`proto.constructor` 指向构造函数）。
     * exportClass 内部调 setConstructor 建立 proto→ctor 双向关联，故注册期 exportClass 返回 proto 后
     * 即可调用本方法。
     *
     * @param name 静态方法名
     * @param callback 方法回调（thisVal 为构造函数对象，通常静态方法不使用 thisVal）
     * @param length 参数个数
     */
    ClassRegistrar& staticMethod(const std::string& name, ScriptMethodCallback callback, i32 length = 0)
    {
        // 从原型取构造函数对象。getProperty 返回 owned 句柄，用完需 release。
        void* ctor = m_ctx.getProperty(m_proto, "constructor");
        if (ctor == nullptr) {
            return *this; // 理论不发生（exportClass 必建 constructor 关联）；防御性跳过
        }
        m_ctx.setPropertyFunction(ctor, name.c_str(), std::move(callback), length);
        m_ctx.releaseValue(ctor);
        return *this;
    }

    /**
     * @brief 注册只读属性（getter only，使用引擎无关回调）
     *
     * @param name 属性名
     * @param getter getter回调
     */
    ClassRegistrar& readonlyProperty(const std::string& name, ScriptGetterCallback getter)
    {
        m_ctx.registerReadonlyProperty(m_proto, name.c_str(), std::move(getter));
        return *this;
    }

    /**
     * @brief 注册可读写属性（使用引擎无关回调）
     *
     * @param name 属性名
     * @param getter getter回调
     * @param setter setter回调
     */
    ClassRegistrar& property(const std::string& name, ScriptGetterCallback getter, ScriptSetterCallback setter)
    {
        m_ctx.registerProperty(m_proto, name.c_str(), std::move(getter), std::move(setter));
        return *this;
    }

    /**
     * @brief 从JS对象获取C++指针
     */
    [[nodiscard]] T* unwrap(void* thisVal) const
    {
        void* ptr = ScriptObjectRegistry::unwrap(m_ctx, thisVal, m_classId);
        return static_cast<T*>(ptr);
    }

    /**
     * @brief 将C++对象包装为JS值
     *
     * @param obj C++对象指针
     * @param owned 是否由JS管理生命周期
     * @param entityId 实体实例ID（仅 Entity 系传非0，透传 ScriptObjectRegistry::wrap 登记
     *        ScriptHandleRegistry 防 UAF）；非 Entity 类型默认 0 不登记。
     */
    [[nodiscard]] void* wrap(T* obj, bool owned = false, EntityInstanceId entityId = 0) const
    {
        if (!obj) return m_ctx.createNull();
        return ScriptObjectRegistry::wrap(m_ctx,
            m_classId,
            m_proto,
            obj,
            owned,
            typeid(T).name(),
            owned ? [](void* p) { delete static_cast<T*>(p); } : nullptr,
            entityId);
    }

    /**
     * @brief 获取原型对象句柄
     */
    [[nodiscard]] void* prototype() const { return m_proto; }

    /**
     * @brief 获取类ID
     */
    [[nodiscard]] u64 classId() const { return m_classId; }

private:
    IScriptBindingContext& m_ctx;
    u64 m_classId;
    void* m_proto;
};

} // namespace mc::mod::bedrock::addon
