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

#pragma once

#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/core/ScriptResult.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <quickjs.h>

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
 * @brief JSValue转换器模板基类
 *
 * 将JSValue转换为C++类型（特化版本）。
 * 默认实现返回std::nullopt，需要为具体类型提供特化。
 *
 * @tparam T 目标C++类型
 */
template <typename T>
struct JSValueConverter {
    /**
     * @brief 从JSValue转换为C++类型
     * @param ctx JS上下文
     * @param val JS值
     * @return 转换结果，失败返回std::nullopt
     */
    static std::optional<T> fromJS(JSContext* ctx, JSValue val) noexcept { return std::nullopt; }

    /**
     * @brief 从C++类型转换为JSValue
     * @param ctx JS上下文
     * @param val C++值
     * @return JS值
     */
    static JSValue toJS(JSContext* ctx, const T& val) noexcept { return JS_UNDEFINED; }
};

/**
 * @brief bool类型的JSValue转换器特化
 */
template <>
struct JSValueConverter<bool> {
    /**
     * @brief 从JSValue转换为bool
     * @param ctx JS上下文
     * @param val JS值
     * @return 布尔值，非bool类型返回std::nullopt
     */
    static std::optional<bool> fromJS(JSContext* ctx, JSValue val) noexcept
    {
        if (!JS_IsBool(val)) return std::nullopt;
        return JS_ToBool(ctx, val) != 0;
    }

    /**
     * @brief 从bool转换为JSValue
     * @param ctx JS上下文
     * @param val 布尔值
     * @return JS布尔值
     */
    static JSValue toJS(JSContext* ctx, const bool& val) noexcept { return JS_NewBool(ctx, val ? 1 : 0); }
};

/**
 * @brief i32类型的JSValue转换器特化
 */
template <>
struct JSValueConverter<i32> {
    /**
     * @brief 从JSValue转换为i32
     * @param ctx JS上下文
     * @param val JS值
     * @return 32位整数，非数值类型返回std::nullopt
     */
    static std::optional<i32> fromJS(JSContext* ctx, JSValue val) noexcept
    {
        if (!JS_IsNumber(val)) return std::nullopt;
        f64 num;
        if (JS_ToFloat64(ctx, &num, val) != 0) return std::nullopt;
        return static_cast<i32>(num);
    }

    /**
     * @brief 从i32转换为JSValue
     * @param ctx JS上下文
     * @param val 32位整数
     * @return JS数值
     */
    static JSValue toJS(JSContext* ctx, const i32& val) noexcept { return JS_NewInt32(ctx, val); }
};

/**
 * @brief f64类型的JSValue转换器特化
 */
template <>
struct JSValueConverter<f64> {
    /**
     * @brief 从JSValue转换为f64
     * @param ctx JS上下文
     * @param val JS值
     * @return 64位浮点数，非数值类型返回std::nullopt
     */
    static std::optional<f64> fromJS(JSContext* ctx, JSValue val) noexcept
    {
        if (!JS_IsNumber(val)) return std::nullopt;
        f64 num;
        if (JS_ToFloat64(ctx, &num, val) != 0) return std::nullopt;
        return num;
    }

    /**
     * @brief 从f64转换为JSValue
     * @param ctx JS上下文
     * @param val 64位浮点数
     * @return JS数值
     */
    static JSValue toJS(JSContext* ctx, const f64& val) noexcept { return JS_NewFloat64(ctx, val); }
};

/**
 * @brief std::string类型的JSValue转换器特化
 */
template <>
struct JSValueConverter<std::string> {
    /**
     * @brief 从JSValue转换为std::string
     * @param ctx JS上下文
     * @param val JS值
     * @return 字符串，非字符串类型返回std::nullopt
     * @note 返回的字符串是拷贝，原始JS字符串会被释放
     */
    static std::optional<std::string> fromJS(JSContext* ctx, JSValue val) noexcept
    {
        if (!JS_IsString(val)) return std::nullopt;
        const char* str = JS_ToCString(ctx, val);
        if (!str) return std::nullopt;
        std::string result(str);
        JS_FreeCString(ctx, str);
        return result;
    }

    /**
     * @brief 从std::string转换为JSValue
     * @param ctx JS上下文
     * @param val 字符串
     * @return JS字符串
     */
    static JSValue toJS(JSContext* ctx, const std::string& val) noexcept
    {
        return JS_NewStringLen(ctx, val.c_str(), val.size());
    }
};

/**
 * @brief std::string_view类型的JSValue转换器特化
 * @note 仅支持toJS转换，fromJS需要所有权转移故不支持
 */
template <>
struct JSValueConverter<std::string_view> {
    /**
     * @brief 从std::string_view转换为JSValue
     * @param ctx JS上下文
     * @param val 字符串视图
     * @return JS字符串
     */
    static JSValue toJS(JSContext* ctx, const std::string_view& val) noexcept
    {
        return JS_NewStringLen(ctx, val.data(), val.size());
    }
};

/**
 * @brief 便捷函数：从JSValue转换为C++类型
 * @tparam T 目标C++类型
 * @param ctx JS上下文
 * @param val JS值
 * @return 转换结果，失败返回std::nullopt
 */
template <typename T>
std::optional<T> jsValueTo(JSContext* ctx, JSValue val) noexcept
{
    return JSValueConverter<T>::fromJS(ctx, val);
}

/**
 * @brief 便捷函数：从C++类型转换为JSValue
 * @tparam T 源C++类型
 * @param ctx JS上下文
 * @param val C++值
 * @return JS值
 */
template <typename T>
JSValue jsValueFrom(JSContext* ctx, const T& val) noexcept
{
    return JSValueConverter<T>::toJS(ctx, val);
}

/**
 * @brief 将JSValue数组转换为C++ vector
 *
 * @tparam T 元素类型
 * @param ctx JS上下文
 * @param val JS数组值
 * @return 元素类型为T的vector，转换失败返回std::nullopt
 * @note 会遍历整个数组并转换每个元素
 */
template <typename T>
std::optional<std::vector<T>> jsArrayToVector(JSContext* ctx, JSValue val) noexcept
{
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
 *
 * @tparam T 元素类型
 * @param ctx JS上下文
 * @param vec C++ vector
 * @return JS数组
 */
template <typename T>
JSValue vectorToJSArray(JSContext* ctx, const std::vector<T>& vec) noexcept
{
    JSValue arr = JS_NewArray(ctx);
    for (u32 i = 0; i < static_cast<u32>(vec.size()); ++i) {
        JSValue elem = JSValueConverter<T>::toJS(ctx, vec[i]);
        JS_SetPropertyUint32(ctx, arr, i, elem);
    }
    return arr;
}

} // namespace mc::mod::bedrock::addon
