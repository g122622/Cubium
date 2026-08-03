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

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc::entity::serialization {
namespace nbt_helper {

// ========== 安全读取函数 ==========

std::optional<i8> tryGetByte(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Byte) {
        return dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
    }
    return std::nullopt;
}

std::optional<i16> tryGetShort(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Short) {
        return dynamic_cast<const nbt::tags::short_tag&>(*it->second).value;
    }
    return std::nullopt;
}

std::optional<i32> tryGetInt(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Int) {
        return dynamic_cast<const nbt::tags::int_tag&>(*it->second).value;
    }
    return std::nullopt;
}

std::optional<i64> tryGetLong(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Long) {
        return dynamic_cast<const nbt::tags::long_tag&>(*it->second).value;
    }
    return std::nullopt;
}

std::optional<f32> tryGetFloat(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Float) {
        return dynamic_cast<const nbt::tags::float_tag&>(*it->second).value;
    }
    return std::nullopt;
}

std::optional<f64> tryGetDouble(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Double) {
        return dynamic_cast<const nbt::tags::double_tag&>(*it->second).value;
    }
    return std::nullopt;
}

std::optional<std::string> tryGetString(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::String) {
        return dynamic_cast<const nbt::tags::string_tag&>(*it->second).value;
    }
    return std::nullopt;
}

std::optional<bool> tryGetBool(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto byteVal = tryGetByte(tag, key);
    return byteVal.has_value() ? std::optional<bool>(*byteVal != 0) : std::nullopt;
}

const nbt::tags::compound_tag* tryGetCompound(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Compound) {
        return &dynamic_cast<const nbt::tags::compound_tag&>(*it->second);
    }
    return nullptr;
}

const nbt::tags::list_tag* tryGetList(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it != tag.value.end() && it->second->id() == nbt::TagId::List) {
        return &dynamic_cast<const nbt::tags::list_tag&>(*it->second);
    }
    return nullptr;
}

// ========== MC 格式列表读写 ==========

void putDoubleList(nbt::tags::compound_tag& tag, const std::string& key, const std::vector<f64>& values)
{
    auto list = std::make_unique<nbt::tags::double_list_tag>();
    for (f64 v : values) {
        list->value.push_back(v);
    }
    tag.value.emplace(key, std::move(list));
}

void putFloatList(nbt::tags::compound_tag& tag, const std::string& key, const std::vector<f32>& values)
{
    auto list = std::make_unique<nbt::tags::float_list_tag>();
    for (f32 v : values) {
        list->value.push_back(v);
    }
    tag.value.emplace(key, std::move(list));
}

std::vector<f64> getDoubleList(const nbt::tags::compound_tag& tag, const std::string& key)
{
    std::vector<f64> result;
    auto* list = tryGetList(tag, key);
    if (!list) {
        return result;
    }

    if (list->element_id() == nbt::TagId::Double) {
        auto& doubleList = dynamic_cast<const nbt::tags::double_list_tag&>(*list);
        for (f64 v : doubleList.value) {
            result.push_back(v);
        }
    }
    return result;
}

std::vector<f32> getFloatList(const nbt::tags::compound_tag& tag, const std::string& key)
{
    std::vector<f32> result;
    auto* list = tryGetList(tag, key);
    if (!list) {
        return result;
    }

    if (list->element_id() == nbt::TagId::Float) {
        auto& floatList = dynamic_cast<const nbt::tags::float_list_tag&>(*list);
        for (f32 v : floatList.value) {
            result.push_back(v);
        }
    }
    return result;
}

void putIntList(nbt::tags::compound_tag& tag, const std::string& key, const std::vector<i32>& values)
{
    auto list = std::make_unique<nbt::tags::int_list_tag>();
    for (i32 v : values) {
        list->value.push_back(v);
    }
    tag.value.emplace(key, std::move(list));
}

std::vector<i32> getIntList(const nbt::tags::compound_tag& tag, const std::string& key)
{
    std::vector<i32> result;
    auto* list = tryGetList(tag, key);
    if (!list) {
        return result;
    }

    if (list->element_id() == nbt::TagId::Int) {
        auto& intList = dynamic_cast<const nbt::tags::int_list_tag&>(*list);
        for (i32 v : intList.value) {
            result.push_back(v);
        }
    }
    return result;
}

// ========== UUID 读写 ==========

void putUuid(nbt::tags::compound_tag& tag, const std::string& uuid)
{
    if (uuid.empty()) {
        return;
    }

    // 将 32 位十六进制 UUID 转换为两个 i64
    auto uuidBytes = util::uuidFromString(uuid);
    if (uuidBytes.size() != 16) {
        return;
    }

    i64 most = (static_cast<i64>(uuidBytes[0]) << 56) | (static_cast<i64>(uuidBytes[1]) << 48) |
        (static_cast<i64>(uuidBytes[2]) << 40) | (static_cast<i64>(uuidBytes[3]) << 32) |
        (static_cast<i64>(uuidBytes[4]) << 24) | (static_cast<i64>(uuidBytes[5]) << 16) |
        (static_cast<i64>(uuidBytes[6]) << 8) | static_cast<i64>(uuidBytes[7]);

    i64 least = (static_cast<i64>(uuidBytes[8]) << 56) | (static_cast<i64>(uuidBytes[9]) << 48) |
        (static_cast<i64>(uuidBytes[10]) << 40) | (static_cast<i64>(uuidBytes[11]) << 32) |
        (static_cast<i64>(uuidBytes[12]) << 24) | (static_cast<i64>(uuidBytes[13]) << 16) |
        (static_cast<i64>(uuidBytes[14]) << 8) | static_cast<i64>(uuidBytes[15]);

    tag.put("UUIDMost", most);
    tag.put("UUIDLeast", least);
}

std::string getUuid(const nbt::tags::compound_tag& tag)
{
    auto most = tryGetLong(tag, "UUIDMost");
    auto least = tryGetLong(tag, "UUIDLeast");
    if (!most.has_value() || !least.has_value()) {
        return "";
    }

    // 将两个 i64 转换回 16 字节 UUID
    std::array<u8, 16> uuidBytes{};
    i64 m = most.value();
    i64 l = least.value();
    for (i32 i = 7; i >= 0; --i) {
        uuidBytes[i] = static_cast<u8>(m & 0xFF);
        m >>= 8;
    }
    for (i32 i = 15; i >= 8; --i) {
        uuidBytes[i] = static_cast<u8>(l & 0xFF);
        l >>= 8;
    }

    return util::uuidToString(uuidBytes);
}

// ========== AttributeMap 序列化 ==========

void writeAttributeMap(
    nbt::tags::compound_tag& tag, const std::string& key, const entity::attribute::AttributeMap& attrMap)
{
    auto list = std::make_unique<nbt::tags::compound_list_tag>();

    for (const auto& [name, instance] : attrMap.allInstances()) {
        nbt::tags::compound_tag attrTag;
        attrTag.put(nbt_keys::ATTR_NAME, name);
        attrTag.put(nbt_keys::ATTR_BASE, instance->baseValue());

        // 写入修改器列表（如果有）
        const auto& modifiers = instance->modifiers();
        if (!modifiers.empty()) {
            auto modList = std::make_unique<nbt::tags::compound_list_tag>();
            for (const auto& mod : modifiers) {
                nbt::tags::compound_tag modTag;

                // UUID 拆分为 UUIDMost + UUIDLeast
                // 修改器 ID 是字符串格式，需要转换为两个 i64
                auto uuidBytes = util::uuidFromString(mod.id());
                if (uuidBytes.size() == 16) {
                    i64 most = (static_cast<i64>(uuidBytes[0]) << 56) | (static_cast<i64>(uuidBytes[1]) << 48) |
                        (static_cast<i64>(uuidBytes[2]) << 40) | (static_cast<i64>(uuidBytes[3]) << 32) |
                        (static_cast<i64>(uuidBytes[4]) << 24) | (static_cast<i64>(uuidBytes[5]) << 16) |
                        (static_cast<i64>(uuidBytes[6]) << 8) | static_cast<i64>(uuidBytes[7]);

                    i64 least = (static_cast<i64>(uuidBytes[8]) << 56) | (static_cast<i64>(uuidBytes[9]) << 48) |
                        (static_cast<i64>(uuidBytes[10]) << 40) | (static_cast<i64>(uuidBytes[11]) << 32) |
                        (static_cast<i64>(uuidBytes[12]) << 24) | (static_cast<i64>(uuidBytes[13]) << 16) |
                        (static_cast<i64>(uuidBytes[14]) << 8) | static_cast<i64>(uuidBytes[15]);

                    modTag.put(nbt_keys::ATTR_MOD_UUID_MOST, most);
                    modTag.put(nbt_keys::ATTR_MOD_UUID_LEAST, least);
                }

                modTag.put(nbt_keys::ATTR_MOD_NAME, mod.name());
                modTag.put(nbt_keys::ATTR_MOD_OPERATION, static_cast<i8>(mod.operation()));
                modTag.put(nbt_keys::ATTR_MOD_AMOUNT, mod.amount());

                modList->value.push_back(std::move(modTag));
            }
            attrTag.value.emplace(nbt_keys::ATTR_MODIFIERS, std::move(modList));
        }

        list->value.push_back(std::move(attrTag));
    }

    if (!list->value.empty()) {
        tag.value.emplace(key, std::move(list));
    }
}

void readAttributeMap(
    const nbt::tags::compound_tag& tag, const std::string& key, entity::attribute::AttributeMap& attrMap)
{
    const auto* listTag = tryGetList(tag, key);
    if (!listTag || listTag->element_id() != nbt::TagId::Compound) {
        return;
    }

    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*listTag);
    for (const auto& attrTag : compoundList.value) {
        auto nameOpt = tryGetString(attrTag, nbt_keys::ATTR_NAME);
        if (!nameOpt.has_value()) {
            continue;
        }

        const std::string& attrName = nameOpt.value();

        // 只处理已注册的属性
        auto* instance = attrMap.getInstance(attrName);
        if (instance == nullptr) {
            continue;
        }

        // 清除已有的修改器，然后从 NBT 重新加载。
        // 参考 MC Java: AttributeInstance.apply() 在设置新值前会先清除所有修改器。
        // 这确保反序列化时不会出现修改器重复叠加（例如效果系统已应用的修改器
        // 和 NBT 中保存的修改器同时存在）。
        instance->clearModifiers();

        // 读取基础值
        if (auto baseOpt = tryGetDouble(attrTag, nbt_keys::ATTR_BASE)) {
            instance->setBaseValue(*baseOpt);
        }

        // 读取修改器
        const auto* modListTag = tryGetList(attrTag, nbt_keys::ATTR_MODIFIERS);
        if (modListTag && modListTag->element_id() == nbt::TagId::Compound) {
            const auto& modCompoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*modListTag);
            for (const auto& modTag : modCompoundList.value) {
                auto modNameOpt = tryGetString(modTag, nbt_keys::ATTR_MOD_NAME);
                auto amountOpt = tryGetDouble(modTag, nbt_keys::ATTR_MOD_AMOUNT);
                auto opOpt = tryGetByte(modTag, nbt_keys::ATTR_MOD_OPERATION);

                if (!modNameOpt.has_value() || !amountOpt.has_value() || !opOpt.has_value()) {
                    continue;
                }

                // 从 UUIDMost/UUIDLeast 构建修改器 ID
                std::string modId;
                auto mostOpt = tryGetLong(modTag, nbt_keys::ATTR_MOD_UUID_MOST);
                auto leastOpt = tryGetLong(modTag, nbt_keys::ATTR_MOD_UUID_LEAST);
                if (mostOpt.has_value() && leastOpt.has_value()) {
                    std::array<u8, 16> uuidBytes{};
                    i64 m = mostOpt.value();
                    i64 l = leastOpt.value();
                    for (i32 i = 7; i >= 0; --i) {
                        uuidBytes[i] = static_cast<u8>(m & 0xFF);
                        m >>= 8;
                    }
                    for (i32 i = 15; i >= 8; --i) {
                        uuidBytes[i] = static_cast<u8>(l & 0xFF);
                        l >>= 8;
                    }
                    modId = util::uuidToString(uuidBytes);
                } else {
                    // 如果没有 UUID，使用名称作为 ID
                    modId = modNameOpt.value();
                }

                auto operation = static_cast<entity::attribute::Operation>(*opOpt);
                entity::attribute::AttributeModifier modifier(modId, modNameOpt.value(), *amountOpt, operation);
                instance->addModifier(modifier);
            }
        }
    }
}

} // namespace nbt_helper
} // namespace mc::entity::serialization
