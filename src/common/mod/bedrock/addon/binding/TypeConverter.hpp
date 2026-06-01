#pragma once

#include "common/core/Types.hpp"
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <quickjs.h>

namespace mc::mod::bedrock::addon {

/**
 * @brief 高级类型转换注册表
 *
 * 扩展QuickJSValueConvert的基础类型转换，支持：
 * - 枚举类型与i32的双向转换
 * - 对象属性读写
 * - 回调函数包装
 *
 * 与QuickJSValueConvert不同，此头文件专注于模块绑定层的
 * 高级转换模式，而非基础类型转换。
 */

/**
 * @brief 从JS对象读取属性
 *
 * @param ctx JS上下文
 * @param obj JS对象
 * @param key 属性名
 * @return 属性值，失败返回JS_UNDEFINED
 */
inline JSValue getObjectProperty(JSContext* ctx, JSValue obj, const char* key)
{
    return JS_GetPropertyStr(ctx, obj, key);
}

/**
 * @brief 向JS对象写入属性
 *
 * @param ctx JS上下文
 * @param obj JS对象
 * @param key 属性名
 * @param value 属性值
 * @return 0成功，-1失败
 */
inline int setObjectProperty(JSContext* ctx, JSValue obj, const char* key, JSValue value)
{
    return JS_SetPropertyStr(ctx, obj, key, value);
}

/**
 * @brief 从JS对象读取i32属性
 */
inline std::optional<i32> getObjectPropertyInt(JSContext* ctx, JSValue obj, const char* key)
{
    JSValue val = JS_GetPropertyStr(ctx, obj, key);
    if (JS_IsUndefined(val) || !JS_IsNumber(val)) {
        JS_FreeValue(ctx, val);
        return std::nullopt;
    }
    i32 result;
    if (JS_ToInt32(ctx, &result, val) != 0) {
        JS_FreeValue(ctx, val);
        return std::nullopt;
    }
    JS_FreeValue(ctx, val);
    return result;
}

/**
 * @brief 从JS对象读取f64属性
 */
inline std::optional<f64> getObjectPropertyFloat(JSContext* ctx, JSValue obj, const char* key)
{
    JSValue val = JS_GetPropertyStr(ctx, obj, key);
    if (JS_IsUndefined(val) || !JS_IsNumber(val)) {
        JS_FreeValue(ctx, val);
        return std::nullopt;
    }
    f64 result;
    if (JS_ToFloat64(ctx, &result, val) != 0) {
        JS_FreeValue(ctx, val);
        return std::nullopt;
    }
    JS_FreeValue(ctx, val);
    return result;
}

/**
 * @brief 从JS对象读取bool属性
 */
inline std::optional<bool> getObjectPropertyBool(JSContext* ctx, JSValue obj, const char* key)
{
    JSValue val = JS_GetPropertyStr(ctx, obj, key);
    if (JS_IsUndefined(val) || !JS_IsBool(val)) {
        JS_FreeValue(ctx, val);
        return std::nullopt;
    }
    bool result = JS_ToBool(ctx, val) != 0;
    JS_FreeValue(ctx, val);
    return result;
}

/**
 * @brief 从JS对象读取字符串属性
 */
inline std::optional<std::string> getObjectPropertyString(JSContext* ctx, JSValue obj, const char* key)
{
    JSValue val = JS_GetPropertyStr(ctx, obj, key);
    if (JS_IsUndefined(val) || !JS_IsString(val)) {
        JS_FreeValue(ctx, val);
        return std::nullopt;
    }
    const char* str = JS_ToCString(ctx, val);
    std::string result = str ? str : "";
    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, val);
    return result;
}

/**
 * @brief 枚举类型与JS数值的双向转换
 *
 * JS中枚举值表示为整数。
 */
template <typename E>
std::optional<E> jsToEnum(JSContext* ctx, JSValue val)
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    if (!JS_IsNumber(val)) return std::nullopt;
    i32 intVal;
    if (JS_ToInt32(ctx, &intVal, val) != 0) return std::nullopt;
    return static_cast<E>(intVal);
}

template <typename E>
JSValue enumToJs(JSContext* ctx, E val)
{
    static_assert(std::is_enum_v<E>, "E must be an enum type");
    return JS_NewInt32(ctx, static_cast<i32>(val));
}

/**
 * @brief 创建JS对象
 */
inline JSValue createObject(JSContext* ctx)
{
    return JS_NewObject(ctx);
}

/**
 * @brief 创建JS数组
 */
inline JSValue createArray(JSContext* ctx, u32 length = 0)
{
    return JS_NewArray(ctx);
}

/**
 * @brief 向JS对象添加i32属性
 */
inline void setObjectInt(JSContext* ctx, JSValue obj, const char* key, i32 value)
{
    JS_SetPropertyStr(ctx, obj, key, JS_NewInt32(ctx, value));
}

/**
 * @brief 向JS对象添加f64属性
 */
inline void setObjectFloat(JSContext* ctx, JSValue obj, const char* key, f64 value)
{
    JS_SetPropertyStr(ctx, obj, key, JS_NewFloat64(ctx, value));
}

/**
 * @brief 向JS对象添加bool属性
 */
inline void setObjectBool(JSContext* ctx, JSValue obj, const char* key, bool value)
{
    JS_SetPropertyStr(ctx, obj, key, JS_NewBool(ctx, value ? 1 : 0));
}

/**
 * @brief 向JS对象添加字符串属性
 */
inline void setObjectString(JSContext* ctx, JSValue obj, const char* key, const std::string& value)
{
    JS_SetPropertyStr(ctx, obj, key, JS_NewStringLen(ctx, value.c_str(), value.size()));
}

/**
 * @brief 向JS对象添加null属性
 */
inline void setObjectNull(JSContext* ctx, JSValue obj, const char* key)
{
    JS_SetPropertyStr(ctx, obj, key, JS_NULL);
}

/**
 * @brief 向JS数组设置i32元素
 */
inline void setArrayInt(JSContext* ctx, JSValue arr, u32 index, i32 value)
{
    JS_SetPropertyUint32(ctx, arr, index, JS_NewInt32(ctx, value));
}

/**
 * @brief 向JS数组设置字符串元素
 */
inline void setArrayString(JSContext* ctx, JSValue arr, u32 index, const std::string& value)
{
    JS_SetPropertyUint32(ctx, arr, index, JS_NewStringLen(ctx, value.c_str(), value.size()));
}

} // namespace mc::mod::bedrock::addon
