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

#include "common/mod/bedrock/addon/content/base/properties/AddonDynamicProperties.hpp"
#include "common/mod/bedrock/addon/content/base/properties/AddonPropertyValue.hpp"

namespace mc::mod::bedrock::addon {

std::optional<nlohmann::json> AddonDynamicProperties::get(const std::string& pack, const std::string& name) const
{
    const auto scope = m_values.find(pack);
    if (scope == m_values.end()) {
        return std::nullopt;
    }

    const auto value = scope->find(name);
    return value != scope->end() ? std::optional<nlohmann::json>(*value) : std::nullopt;
}

Result<void> AddonDynamicProperties::set(const std::string& pack, const std::string& name, const nlohmann::json& value)
{
    if (pack.empty() || name.empty()) {
        return Error(ErrorCode::InvalidArgument, "Dynamic property requires a pack identity and nonempty key");
    }

    MC_TRY(validateDynamicPropertyValue(value));
    const auto previous = get(pack, name);
    if ((!previous && value.is_null()) || (previous && *previous == value)) {
        return Result<void>::ok();
    }

    m_dirty = true;
    if (value.is_null()) {
        if (auto scope = m_values.find(pack); scope != m_values.end()) {
            scope->erase(name);
            if (scope->empty()) {
                m_values.erase(scope);
            }
        }
    } else {
        m_values[pack][name] = value;
    }
    return Result<void>::ok();
}

std::vector<std::string> AddonDynamicProperties::ids(const std::string& pack) const
{
    std::vector<std::string> names;
    if (const auto scope = m_values.find(pack); scope != m_values.end()) {
        for (const auto& [name, value] : scope->items()) {
            names.push_back(name);
        }
    }
    return names;
}

Result<void> AddonDynamicProperties::setMany(const std::string& pack, const nlohmann::json& values)
{
    if (pack.empty() || !values.is_object()) {
        return Error(ErrorCode::InvalidArgument, "Dynamic property batch requires a pack identity and an object");
    }

    // 校验和应用分离，避免后面的非法值令前面的写入部分提交。
    for (const auto& [name, value] : values.items()) {
        if (name.empty()) {
            return Error(ErrorCode::InvalidArgument, "Dynamic property requires a nonempty key");
        }
        MC_TRY(validateDynamicPropertyValue(value));
    }

    for (const auto& [name, value] : values.items()) {
        MC_TRY(set(pack, name, value));
    }
    return Result<void>::ok();
}

void AddonDynamicProperties::clear(const std::string& pack)
{
    m_dirty = m_values.erase(pack) > 0 || m_dirty;
}

nlohmann::json AddonDynamicProperties::save() const
{
    nlohmann::json saved = nlohmann::json::object();
    for (const auto& [pack, scope] : m_values.items()) {
        for (const auto& [name, value] : scope.items()) {
            saved[pack][name] = encodeAddonPropertyValue(value);
        }
    }
    return saved;
}

Result<void> AddonDynamicProperties::load(const nlohmann::json& saved)
{
    if (!saved.is_object()) {
        return Error(ErrorCode::InvalidData, "Invalid dynamic property table");
    }

    AddonDynamicProperties restored;
    for (const auto& [pack, scope] : saved.items()) {
        if (pack.empty() || !scope.is_object()) {
            return Error(ErrorCode::InvalidData, "Invalid dynamic property scope");
        }
        for (const auto& [name, encoded] : scope.items()) {
            auto value = decodeAddonPropertyValue(encoded);
            if (value.failed()) {
                return value.error();
            }
            if (value.value().is_null()) {
                return Error(ErrorCode::InvalidData, "Stored dynamic property cannot be null");
            }
            MC_TRY(restored.set(pack, name, value.value()));
        }
    }
    restored.markSaved();
    *this = std::move(restored);
    return Result<void>::ok();
}

bool AddonDynamicProperties::isDirty() const
{
    return m_dirty;
}

void AddonDynamicProperties::markSaved()
{
    m_dirty = false;
}

} // namespace mc::mod::bedrock::addon
