#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/UuidUtils.hpp"

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
    for (int i = 7; i >= 0; --i) {
        uuidBytes[i] = static_cast<u8>(m & 0xFF);
        m >>= 8;
    }
    for (int i = 15; i >= 8; --i) {
        uuidBytes[i] = static_cast<u8>(l & 0xFF);
        l >>= 8;
    }

    return util::uuidToString(uuidBytes);
}

} // namespace nbt_helper
} // namespace mc::entity::serialization
