#pragma once

#include "common/core/Types.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>
#include <quickjs.h>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本对象注册表
 *
 * 管理C++对象在JS中的包装和生命周期。
 * 通过QuickJS的class ID和opaque机制关联C++/JS对象。
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
     * @param rt JS运行时
     * @return 新分配的class ID
     */
    static JSClassID allocateClassId(JSRuntime* rt)
    {
        JSClassID id = 1;
        return JS_NewClassID(rt, &id);
    }

    /**
     * @brief 将C++对象包装为JS对象
     *
     * @param ctx JS上下文
     * @param classId 类ID
     * @param proto 原型对象
     * @param ptr C++对象指针
     * @param owned 是否由JS管理生命周期（GC时delete）
     * @param typeName 类型名（调试用）
     * @param destroy 自定义销毁回调（可选，owned=true时默认delete）
     * @return JS对象值
     */
    static JSValue wrap(JSContext* ctx,
        JSClassID classId,
        JSValue proto,
        void* ptr,
        bool owned,
        const char* typeName,
        void (*destroy)(void*) = nullptr);

    /**
     * @brief 从JS对象获取C++指针
     *
     * @param ctx JS上下文
     * @param val JS值
     * @param classId 期望的类ID（0表示不检查类型）
     * @return C++对象指针
     */
    static void* unwrap(JSContext* ctx, JSValue val, JSClassID classId = 0);

    /**
     * @brief 检查JS值是否为指定类的实例
     */
    static bool isInstanceOf(JSContext* ctx, JSValue val, JSClassID classId);

    /**
     * @brief 使关联的C++指针失效（不释放）
     *
     * 当C++对象被外部销毁时调用，防止悬空指针。
     */
    static void invalidate(JSContext* ctx, JSValue val, JSClassID classId);

    /**
     * @brief 存储在JS对象opaque中的数据
     */
    struct ObjectData {
        void* ptr;
        bool owned;
        const char* typeName;
        void (*destroy)(void*);
    };
};

/**
 * @brief 原生模块构建器
 *
 * 辅助在QuickJS上下文中注册原生C++模块。
 * 使用QuickJS C API的JS_NewCModule + JS_SetModuleExport模式。
 *
 * 使用示例：
 * @code
 * NativeModuleBuilder builder(ctx, "@minecraft/server");
 * builder.exportFunction("getVersion", myGetVersion);
 * builder.exportConst("VERSION", 2);
 * builder.exportClass("Entity", entityClassId);
 * builder.finalize();
 * @endcode
 */
class NativeModuleBuilder {
public:
    using NativeFunction = JSCFunction;

    /**
     * @brief 构造模块构建器
     *
     * @param ctx JS上下文
     * @param moduleName 模块名，如 "@minecraft/server"
     */
    NativeModuleBuilder(JSContext* ctx, const std::string& moduleName);

    /**
     * @brief 导出C函数
     *
     * @param name 导出名
     * @param func 函数指针
     * @param length 参数个数（用于JS的length属性）
     */
    NativeModuleBuilder& exportFunction(const std::string& name, NativeFunction* func, int length = 0);

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
     * @param value JS值（调用者负责引用管理）
     */
    NativeModuleBuilder& exportValue(const std::string& name, JSValue value);

    /**
     * @brief 在模块中创建一个类并导出构造函数
     *
     * 注册类定义到运行时，创建原型对象，导出构造函数。
     * 返回原型对象供添加方法和属性。
     *
     * @param className 类名
     * @param classId 类ID（由ScriptObjectRegistry::allocateClassId()分配）
     * @param finalizer GC finalizer（可选，默认为空）
     * @return 原型JSValue，用于添加方法/属性
     */
    JSValue exportClass(const std::string& className, JSClassID classId, JSClassFinalizer* finalizer = nullptr);

    /**
     * @brief 完成模块注册
     *
     * 必须在所有导出操作完成后调用。
     * @return 是否成功
     */
    bool finalize();

    /**
     * @brief 获取JS上下文
     */
    [[nodiscard]] JSContext* context() const { return m_ctx; }

    /**
     * @brief 获取模块名
     */
    [[nodiscard]] const std::string& moduleName() const { return m_moduleName; }

private:
    JSContext* m_ctx;
    std::string m_moduleName;
    JSModuleDef* m_module = nullptr;
    bool m_finalized = false;
};

/**
 * @brief 类注册辅助器
 *
 * 简化QuickJS类的原型方法/属性注册。
 * 与NativeModuleBuilder::exportClass配合使用。
 *
 * 使用示例：
 * @code
 * JSClassID entityClassId = ScriptObjectRegistry::allocateClassId(rt);
 * JSValue proto = builder.exportClass("Entity", entityClassId);
 *
 * ClassRegistrar<Entity> registrar(ctx, entityClassId, proto);
 * registrar.method("getId", [](JSContext* ctx, JSValue this_val, int, JSValueConst*) -> JSValue {
 *     auto* entity = registrar.unwrap(this_val);
 *     if (!entity) return JS_ThrowTypeError(ctx, "Invalid Entity");
 *     return JS_NewInt32(ctx, entity->getId());
 * });
 * @endcode
 *
 * @tparam T C++类型
 */
template <typename T>
class ClassRegistrar {
public:
    ClassRegistrar(JSContext* ctx, JSClassID classId, JSValue proto)
        : m_ctx(ctx)
        , m_classId(classId)
        , m_proto(proto)
    {}

    /**
     * @brief 注册实例方法
     *
     * @param name JS方法名
     * @param func C函数指针
     * @param length 参数个数
     */
    ClassRegistrar& method(const std::string& name, JSCFunction* func, int length = 0)
    {
        JSValue fn = JS_NewCFunction(m_ctx, func, name.c_str(), length);
        JS_SetPropertyStr(m_ctx, m_proto, name.c_str(), fn);
        return *this;
    }

    /**
     * @brief 注册只读属性（getter only）
     *
     * @param name 属性名
     * @param getter getter函数
     */
    ClassRegistrar& readonlyProperty(const std::string& name, JSCFunction* getter)
    {
        JSAtom atom = JS_NewAtomLen(m_ctx, name.c_str(), name.size());
        JSValue getterFn = JS_NewCFunction(m_ctx, getter, name.c_str(), 0);
        JS_DefinePropertyGetSet(m_ctx, m_proto, atom, getterFn, JS_UNDEFINED, 0);
        JS_FreeAtom(m_ctx, atom);
        return *this;
    }

    /**
     * @brief 注册可读写属性
     *
     * @param name 属性名
     * @param getter getter函数
     * @param setter setter函数
     */
    ClassRegistrar& property(const std::string& name, JSCFunction* getter, JSCFunction* setter)
    {
        JSAtom atom = JS_NewAtomLen(m_ctx, name.c_str(), name.size());
        std::string getterName = "get " + name;
        std::string setterName = "set " + name;
        JSValue getterFn = JS_NewCFunction(m_ctx, getter, getterName.c_str(), 0);
        JSValue setterFn = JS_NewCFunction(m_ctx, setter, setterName.c_str(), 1);
        JS_DefinePropertyGetSet(m_ctx, m_proto, atom, getterFn, setterFn, 0);
        JS_FreeAtom(m_ctx, atom);
        return *this;
    }

    /**
     * @brief 从JS对象获取C++指针
     */
    [[nodiscard]] T* unwrap(JSValueConst thisVal) const
    {
        void* ptr = ScriptObjectRegistry::unwrap(m_ctx, thisVal, m_classId);
        return static_cast<T*>(ptr);
    }

    /**
     * @brief 将C++对象包装为JS值
     *
     * @param obj C++对象指针
     * @param owned 是否由JS管理生命周期
     */
    [[nodiscard]] JSValue wrap(T* obj, bool owned = false) const
    {
        if (!obj) return JS_NULL;
        return ScriptObjectRegistry::wrap(m_ctx,
            m_classId,
            m_proto,
            obj,
            owned,
            typeid(T).name(),
            owned ? [](void* p) { delete static_cast<T*>(p); } : nullptr);
    }

    /**
     * @brief 获取原型对象
     */
    [[nodiscard]] JSValue prototype() const { return m_proto; }

    /**
     * @brief 获取类ID
     */
    [[nodiscard]] JSClassID classId() const { return m_classId; }

private:
    JSContext* m_ctx;
    JSClassID m_classId;
    JSValue m_proto;
};

} // namespace mc::mod::bedrock::addon
