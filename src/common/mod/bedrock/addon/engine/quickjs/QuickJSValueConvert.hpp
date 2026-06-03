#pragma once

#include "common/mod/bedrock/addon/core/ScriptResult.hpp"
#include <quickjs.h>
#include <string>
#include <vector>
#include <optional>
#include <type_traits>

namespace mc::mod::bedrock::addon {

/**
 * @brief QuickJS值转换工具
 *
 * 提供ScriptValue与JSValue之间的类型转换，以及
 * C++原生类型与JSValue之间的直接转换。
 * 用于模块绑定层中注册C++ API到JS。
 */

// 前向声明
class QuickJSContext;

/**
 * @brief 将JSValue转换为C++类型（特化版本）
 *
 * 默认实现返回std::nullopt，需要为具体类型提供特化。
 */
template <typename T>
struct JSValueConverter {
    static std::optional<T> fromJS(JSContext* ctx, JSValue val) { return std::nullopt; }
    static JSValue toJS(JSContext* ctx, const T& val) { return JS_UNDEFINED; }
};

template <>
struct JSValueConverter<bool> {
    static std::optional<bool> fromJS(JSContext* ctx, JSValue val) {
        if (!JS_IsBool(val)) return std::nullopt;
        return JS_ToBool(ctx, val) != 0;
    }
    static JSValue toJS(JSContext* ctx, const bool& val) { return JS_NewBool(ctx, val ? 1 : 0); }
};

template <>
struct JSValueConverter<i32> {
    static std::optional<i32> fromJS(JSContext* ctx, JSValue val) {
        if (!JS_IsNumber(val)) return std::nullopt;
        f64 num;
        if (JS_ToFloat64(ctx, &num, val) != 0) return std::nullopt;
        return static_cast<i32>(num);
    }
    static JSValue toJS(JSContext* ctx, const i32& val) { return JS_NewInt32(ctx, val); }
};

template <>
struct JSValueConverter<f64> {
    static std::optional<f64> fromJS(JSContext* ctx, JSValue val) {
        if (!JS_IsNumber(val)) return std::nullopt;
        f64 num;
        if (JS_ToFloat64(ctx, &num, val) != 0) return std::nullopt;
        return num;
    }
    static JSValue toJS(JSContext* ctx, const f64& val) { return JS_NewFloat64(ctx, val); }
};

template <>
struct JSValueConverter<std::string> {
    static std::optional<std::string> fromJS(JSContext* ctx, JSValue val) {
        if (!JS_IsString(val)) return std::nullopt;
        const char* str = JS_ToCString(ctx, val);
        if (!str) return std::nullopt;
        std::string result(str);
        JS_FreeCString(ctx, str);
        return result;
    }
    static JSValue toJS(JSContext* ctx, const std::string& val) {
        return JS_NewStringLen(ctx, val.c_str(), val.size());
    }
};

template <>
struct JSValueConverter<std::string_view> {
    static JSValue toJS(JSContext* ctx, const std::string_view& val) {
        return JS_NewStringLen(ctx, val.data(), val.size());
    }
};

/**
 * @brief 便捷函数：从JSValue转换为C++类型
 */
template <typename T>
std::optional<T> jsValueTo(JSContext* ctx, JSValue val) {
    return JSValueConverter<T>::fromJS(ctx, val);
}

/**
 * @brief 便捷函数：从C++类型转换为JSValue
 */
template <typename T>
JSValue jsValueFrom(JSContext* ctx, const T& val) {
    return JSValueConverter<T>::toJS(ctx, val);
}

/**
 * @brief 将JSValue数组转换为C++ vector
 *
 * @param ctx JS上下文
 * @param val JS数组值
 * @return 元素类型为T的vector，转换失败返回std::nullopt
 */
template <typename T>
std::optional<std::vector<T>> jsArrayToVector(JSContext* ctx, JSValue val) {
    if (!JS_IsArray(ctx, val)) return std::nullopt;

    JSValue lengthVal = JS_GetPropertyStr(ctx, val, "length");
    u32 length = 0;
    if (JS_ToUint32(ctx, &length, lengthVal) != 0) {
        JS_FreeValue(ctx, lengthVal);
        return std::nullopt;
    }
    JS_FreeValue(ctx, lengthVal);

    std::vector<T> result;
    result.reserve(length);
    for (u32 i = 0; i < length; ++i) {
        JSValue elem = JS_GetPropertyUint32(ctx, val, i);
        auto converted = JSValueConverter<T>::fromJS(ctx, elem);
        JS_FreeValue(ctx, elem);
        if (!converted) return std::nullopt;
        result.push_back(std::move(*converted));
    }
    return result;
}

/**
 * @brief 将C++ vector转换为JSValue数组
 */
template <typename T>
JSValue vectorToJSArray(JSContext* ctx, const std::vector<T>& vec) {
    JSValue arr = JS_NewArray(ctx);
    for (u32 i = 0; i < static_cast<u32>(vec.size()); ++i) {
        JSValue elem = JSValueConverter<T>::toJS(ctx, vec[i]);
        JS_SetPropertyUint32(ctx, arr, i, elem);
    }
    return arr;
}

} // namespace mc::mod::bedrock::addon
