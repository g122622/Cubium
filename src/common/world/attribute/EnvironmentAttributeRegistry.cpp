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

#include "common/world/attribute/EnvironmentAttributeRegistry.hpp"

#include "common/util/assert/AssertAll.hpp"
#include "common/world/attribute/EnvironmentAttributeMap.hpp"

#include <algorithm>
#include <unordered_map>

namespace mc {
namespace world {
namespace attribute {
namespace {

/// 去掉可选的 `minecraft:` 命名空间前缀（其他命名空间原样保留，查不到即视为未注册）
std::string_view _stripDefaultNamespace(std::string_view name)
{
    constexpr std::string_view DEFAULT_NAMESPACE = "minecraft:";
    if (name.starts_with(DEFAULT_NAMESPACE)) {
        return name.substr(DEFAULT_NAMESPACE.size());
    }
    return name;
}

struct _RegistryStorage {
    std::map<std::string, AttributeTypeRegistry::Entry, std::less<>> typesByName;
    std::unordered_map<const void*, std::string> attributeNamesByAddress;
    std::vector<EnvironmentAttributeEntry> attributes;
};

_RegistryStorage& _storage()
{
    static _RegistryStorage storage;
    return storage;
}

} // namespace

Result<void> AttributeTypeRegistry::_insert(std::string name, Entry entry)
{
    auto& types = _storage().typesByName;
    if (types.contains(name)) {
        return Error(ErrorCode::AlreadyExists, "Attribute type already registered: " + name);
    }
    types.emplace(std::move(name), std::move(entry));
    return Result<void>::ok();
}

const AttributeTypeRegistry::Entry* AttributeTypeRegistry::find(std::string_view name)
{
    const auto& types = _storage().typesByName;
    const auto it = types.find(_stripDefaultNamespace(name));
    return it == types.end() ? nullptr : &it->second;
}

const std::map<std::string, AttributeTypeRegistry::Entry, std::less<>>& AttributeTypeRegistry::all()
{
    return _storage().typesByName;
}

Result<void> EnvironmentAttributeRegistry::_insertAttribute(std::string name, EnvironmentAttributeEntry entry)
{
    auto& storage = _storage();
    // 重复登记（同一属性或同名属性被数据包/内置定义重复装配）直接忽略，保证注册入口可安全重入
    for (const auto& existing : storage.attributes) {
        if (existing.name == name || existing.attribute == entry.attribute) {
            return Result<void>::ok();
        }
    }

    storage.attributeNamesByAddress.emplace(entry.attribute, entry.name);
    storage.attributes.emplace_back(std::move(entry));
    return Result<void>::ok();
}

const void* EnvironmentAttributeRegistry::find(std::string_view name)
{
    const std::string qualified = name.starts_with("minecraft:") ? std::string(name) : "minecraft:" + std::string(name);

    const auto& attributes = _storage().attributes;
    const auto it = std::find_if(attributes.begin(),
        attributes.end(),
        [&qualified](const EnvironmentAttributeEntry& entry) { return entry.name == qualified; });
    return it == attributes.end() ? nullptr : it->attribute;
}

const std::string* EnvironmentAttributeRegistry::attributeName(const void* attribute)
{
    const auto& names = _storage().attributeNamesByAddress;
    const auto it = names.find(attribute);
    return it == names.end() ? nullptr : &it->second;
}

const std::vector<EnvironmentAttributeEntry>& EnvironmentAttributeRegistry::attributes()
{
    return _storage().attributes;
}

} // namespace attribute
} // namespace world
} // namespace mc