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

#include "common/core/Result.hpp"
#include "common/world/attribute/AttributeType.hpp"
#include "common/world/attribute/EnvironmentAttribute.hpp"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace attribute {

class EnvironmentAttributeMap;

template <typename Value>
[[nodiscard]] std::function<Result<void>(AttributeModifierOperation operation, const void* jsonArgument, void* map)>
makeJsonArgumentApplier(const EnvironmentAttribute<Value>& attribute);

/// 环境属性注册表内一条属性的只读视图
struct EnvironmentAttributeEntry {
    /// 注册名（不含 `minecraft:` 前缀）
    std::string name;
    /// 属性对象地址（`EnvironmentAttribute<Value>` 的静态实例）
    const void* attribute;
    /// 该属性是否参与网络同步
    bool isSyncable;
    /// 该属性是否与位置相关
    bool isPositional;

    /// 判断某修改器操作是否被属性类型允许
    std::function<bool(AttributeModifierOperation)> allowModifier;

    /// 按数据包参数登记属性表条目
    ///
    /// 由 EnvironmentAttributeRegistryDataPack.cpp 提供定义（该文件同时可见属性表与 JSON 的定义），
    /// 从而本头文件可与属性表头互相独立。
    std::function<Result<void>(AttributeModifierOperation operation, const void* jsonArgument, void* map)>
        applyJsonArgument;
};

/**
 * @brief 环境属性类型注册表
 *
 * MC 1.21.11 BuiltInRegistries.ATTRIBUTE_TYPE。
 * 按注册名索引属性类型，供数据包 `attribute_type` 引用解析与序列化。
 * 与 MC 一致，首个注册的类型（BOOLEAN）是内置默认项。
 */
class AttributeTypeRegistry {
public:
    struct Entry {
        /// 注册名（不含 `minecraft:` 前缀）
        std::string name;
        /// 类型在类型链上的投影：只暴露名称与修饰符库校验
        const void* type;
        /// 类型名 → 是否允许某修改器
        std::function<bool(AttributeModifierOperation)> allowModifier;
    };

    /**
     * @brief 按名查找类型条目
     *
     * @param name 注册名（接受 `minecraft:xxx` 与 `xxx` 两种写法）
     * @return 条目指针，未注册时返回 nullptr
     */
    [[nodiscard]] static const Entry* find(std::string_view name);

    /**
     * @brief 全部已注册类型（按注册顺序）
     */
    [[nodiscard]] static const std::map<std::string, Entry, std::less<>>& all();

    /**
     * @brief 注册一个属性类型
     *
     * 重复注册返回 AlreadyExists。
     *
     * @param name 注册名（不含命名空间前缀）
     * @param type 类型对象（必须为静态对象）
     * @return 注册结果
     */
    template <typename Value>
    static Result<void> registerType(std::string_view name, const AttributeType<Value>& type)
    {
        if (name.empty()) {
            return Error::invalidArgument("Attribute type name cannot be empty");
        }
        return _insert(
            std::string(name), Entry{std::string(name), &type, [&type](AttributeModifierOperation operation) {
                                         return type.isAllowedModifier(operation);
                                     }});
    }

private:
    static Result<void> _insert(std::string name, Entry entry);
};

/**
 * @brief 环境属性注册表
 *
 * MC 1.21.11 BuiltInRegistries.ENVIRONMENT_ATTRIBUTE。
 * 以属性对象地址为身份登记属性及其注册名，供数据包 `environment_attribute`
 * 引用解析、按名查询以及基于注册序号的网络同步遍历使用。
 */
class EnvironmentAttributeRegistry {
public:
    /**
     * @brief 注册一个环境属性
     *
     * 重复注册会被断言拦截。
     *
     * @param name 注册名（不含命名空间前缀，可含 `/` 分组前缀）
     * @param attribute 属性对象（必须为静态对象，地址即身份）
     * @return 注册结果
     */
    template <typename Value>
    static Result<void> registerAttribute(std::string_view name, const EnvironmentAttribute<Value>& attribute)
    {
        if (name.empty()) {
            return Error::invalidArgument("Environment attribute name cannot be empty");
        }
        const AttributeType<Value>& type = attribute.type();
        EnvironmentAttributeEntry entry;
        entry.name = std::string(name);
        entry.attribute = &attribute;
        entry.isSyncable = attribute.isSyncable();
        entry.isPositional = attribute.isPositional();
        entry.allowModifier = [&type](AttributeModifierOperation operation) {
            return type.isAllowedModifier(operation);
        };
        entry.applyJsonArgument = makeJsonArgumentApplier(attribute);
        return _insertAttribute(std::move(entry.name), entry);
    }

    /**
     * @brief 按属性地址查找注册名
     *
     * @param attribute 属性对象地址
     * @return 注册名，未注册时返回 nullptr
     */
    [[nodiscard]] static const std::string* attributeName(const void* attribute);

    /**
     * @brief 按注册名查找属性地址
     *
     * @param name 注册名（接受 `minecraft:xxx` 与 `xxx` 两种写法）
     * @return 属性对象地址，未注册时返回 nullptr
     */
    [[nodiscard]] static const void* find(const std::string_view name);

    /**
     * @brief 全部已注册属性（按注册顺序）
     */
    [[nodiscard]] static const std::vector<EnvironmentAttributeEntry>& attributes();

private:
    static Result<void> _insertAttribute(std::string name, EnvironmentAttributeEntry entry);
};

} // namespace attribute
} // namespace world
} // namespace mc
