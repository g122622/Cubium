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
 * The copyright notice and this permission notice shall be included in all
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

#include "DataAccessor.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <sstream>

// Bring operator<< for nbt::tags::tag into scope for ADL
using mc::nbt::operator<<;

namespace mc {
namespace command {

// ========== BlockDataAccessor 实现 ==========

BlockDataAccessor::BlockDataAccessor(IWorld* world, const BlockPos& pos)
    : m_world(world)
    , m_pos(pos)
    , m_blockEntity(nullptr)
{
    if (m_world != nullptr) {
        m_blockEntity = m_world->getBlockEntity(pos);
    }
}

std::unique_ptr<nbt::tags::compound_tag> BlockDataAccessor::getData() const
{
    auto compound = std::make_unique<nbt::tags::compound_tag>();

    if (m_blockEntity == nullptr) {
        throw CommandException(CommandErrorType::NbtPathNotFound,
            "No block entity at position " + std::to_string(m_pos.x) + ", " + std::to_string(m_pos.y) + ", " +
                std::to_string(m_pos.z),
            0);
    }

    // 保存方块实体数据到 NBT
    // BlockEntity 使用 JSON 格式，需要转换
    nlohmann::json jsonData;
    m_blockEntity->save(jsonData);

    // 将 JSON 转换为 NBT
    // 注意：这是简化的转换，可能需要更完整的实现
    compound->put("id", jsonData.value("id", ""));
    compound->put("x", jsonData.value("x", m_pos.x));
    compound->put("y", jsonData.value("y", m_pos.y));
    compound->put("z", jsonData.value("z", m_pos.z));

    // 保存其他数据
    if (jsonData.contains("Items")) {
        // 容器物品列表
        auto itemsArray = std::make_unique<nbt::tags::compound_list_tag>();
        for (const auto& item : jsonData["Items"]) {
            nbt::tags::compound_tag itemCompound;
            itemCompound.put("Slot", static_cast<i8>(item.value("Slot", 0)));
            itemCompound.put("id", item.value("id", ""));
            itemCompound.put("Count", static_cast<i8>(item.value("Count", 1)));
            itemsArray->value.push_back(std::move(itemCompound));
        }
        compound->value["Items"] = std::move(itemsArray);
    }

    // 保存自定义名称
    if (jsonData.contains("CustomName")) {
        compound->put("CustomName", jsonData["CustomName"].get<std::string>());
    }

    // 保存锁
    if (jsonData.contains("Lock")) {
        compound->put("Lock", jsonData["Lock"].get<std::string>());
    }

    // 存储 JSON 原始数据作为 "jsonData" 字段（临时方案）
    compound->put("_jsonData", jsonData.dump());

    return compound;
}

void BlockDataAccessor::mergeData(const nbt::tags::compound_tag& data)
{
    if (m_blockEntity == nullptr) {
        throw CommandException(CommandErrorType::NbtPathNotFound,
            "No block entity at position " + std::to_string(m_pos.x) + ", " + std::to_string(m_pos.y) + ", " +
                std::to_string(m_pos.z),
            0);
    }

    // 从 NBT 转换为 JSON 并加载到方块实体
    nlohmann::json jsonData;

    // 如果有原始 JSON 数据，先加载它
    auto jsonIt = data.value.find("_jsonData");
    if (jsonIt != data.value.end() && jsonIt->second->id() == nbt::TagId::String) {
        std::string jsonStr = dynamic_cast<const nbt::tags::string_tag&>(*jsonIt->second).value;
        try {
            jsonData = nlohmann::json::parse(jsonStr);
        }
        catch (...) {
            jsonData = nlohmann::json::object();
        }
    }

    // 合并 NBT 数据到 JSON
    for (const auto& [key, value] : data.value) {
        if (key == "_jsonData" || key == "x" || key == "y" || key == "z") {
            continue; // 跳过特殊字段和坐标
        }

        // 根据 NBT 类型转换
        switch (value->id()) {
            case nbt::TagId::Byte:
                jsonData[key] = dynamic_cast<const nbt::tags::byte_tag&>(*value).value != 0;
                break;
            case nbt::TagId::Short:
                jsonData[key] = dynamic_cast<const nbt::tags::short_tag&>(*value).value;
                break;
            case nbt::TagId::Int:
                jsonData[key] = dynamic_cast<const nbt::tags::int_tag&>(*value).value;
                break;
            case nbt::TagId::Long:
                jsonData[key] = dynamic_cast<const nbt::tags::long_tag&>(*value).value;
                break;
            case nbt::TagId::Float:
                jsonData[key] = dynamic_cast<const nbt::tags::float_tag&>(*value).value;
                break;
            case nbt::TagId::Double:
                jsonData[key] = dynamic_cast<const nbt::tags::double_tag&>(*value).value;
                break;
            case nbt::TagId::String:
                jsonData[key] = dynamic_cast<const nbt::tags::string_tag&>(*value).value;
                break;
            default:
                // 其他类型暂不处理
                break;
        }
    }

    // 加载到方块实体
    m_blockEntity->load(jsonData);
    m_blockEntity->setChanged();
}

std::string BlockDataAccessor::getDisplayName() const
{
    return "block at " + std::to_string(m_pos.x) + ", " + std::to_string(m_pos.y) + ", " + std::to_string(m_pos.z);
}

std::string BlockDataAccessor::getModifiedMessage() const
{
    return "Modified block data at " + std::to_string(m_pos.x) + ", " + std::to_string(m_pos.y) + ", " +
        std::to_string(m_pos.z);
}

std::string BlockDataAccessor::getQueryMessage(const nbt::tags::tag& nbt) const
{
    std::ostringstream ss;
    ss << nbt::contexts::mojangson << nbt;
    return ss.str();
}

std::string BlockDataAccessor::getGetMessage(const NbtPath& path, double scale, i32 value) const
{
    std::ostringstream ss;
    ss << nbt::contexts::mojangson;
    ss << "Found " << path.toString() << " = ";
    if (scale != 1.0) {
        ss << value << " (scaled by " << scale << ")";
    } else {
        ss << value;
    }
    return ss.str();
}

// ========== EntityDataAccessor 实现 ==========

EntityDataAccessor::EntityDataAccessor(Entity* entity)
    : m_entity(entity)
{}

bool EntityDataAccessor::isPlayer() const
{
    return m_entity != nullptr && dynamic_cast<const Player*>(m_entity) != nullptr;
}

std::unique_ptr<nbt::tags::compound_tag> EntityDataAccessor::getData() const
{
    if (m_entity == nullptr) {
        throw CommandException(CommandErrorType::NbtPathNotFound, "Entity not found", 0);
    }

    auto compound = std::make_unique<nbt::tags::compound_tag>();

    // 基本实体数据
    compound->put("id", m_entity->getTypeId());
    compound->put("CustomName", m_entity->customNameText());

    // 位置和旋转
    auto pos = m_entity->position();
    {
        nbt::tags::double_list_tag posList;
        posList.value.push_back(pos.x);
        posList.value.push_back(pos.y);
        posList.value.push_back(pos.z);
        compound->value["Pos"] = std::make_unique<nbt::tags::double_list_tag>(std::move(posList));
    }

    // UUID
    compound->put("UUID", m_entity->uuid());

    // 标签
    const auto& tags = m_entity->getTags();
    if (!tags.empty()) {
        auto tagsList = std::make_unique<nbt::tags::string_list_tag>();
        for (const auto& tag : tags) {
            tagsList->value.push_back(tag);
        }
        compound->value["Tags"] = std::move(tagsList);
    }

    // 生物特有数据
    auto* livingEntity = dynamic_cast<const LivingEntity*>(m_entity);
    if (livingEntity != nullptr) {
        compound->put("Health", livingEntity->health());
        // TODO: 添加吸收值 getter/setter 到 LivingEntity
        // compound->put("AbsorptionAmount", livingEntity->absorption());

        // 效果
        // TODO: 添加药水效果
    }

    return compound;
}

void EntityDataAccessor::mergeData(const nbt::tags::compound_tag& data)
{
    if (m_entity == nullptr) {
        throw CommandException(CommandErrorType::NbtPathNotFound, "Entity not found", 0);
    }

    // 玩家实体不允许直接修改 NBT
    if (isPlayer()) {
        throw CommandException(CommandErrorType::NbtPathInvalidType, "Cannot modify player data directly", 0);
    }

    // 合并数据
    auto it = data.value.find("CustomName");
    if (it != data.value.end() && it->second->id() == nbt::TagId::String) {
        m_entity->setCustomName(dynamic_cast<const nbt::tags::string_tag&>(*it->second).value);
    }

    it = data.value.find("Tags");
    if (it != data.value.end() && it->second->id() == nbt::TagId::List) {
        auto* list = dynamic_cast<const nbt::tags::list_tag*>(it->second.get());
        if (list != nullptr && list->element_id() == nbt::TagId::String) {
            m_entity->clearTags();
            for (size_t i = 0; i < list->size(); ++i) {
                auto tag = (*list)[i];
                if (tag && tag->id() == nbt::TagId::String) {
                    m_entity->addTag(dynamic_cast<const nbt::tags::string_tag&>(*tag).value);
                }
            }
        }
    }

    // 生物特有数据
    auto* livingEntity = dynamic_cast<LivingEntity*>(m_entity);
    if (livingEntity != nullptr) {
        it = data.value.find("Health");
        if (it != data.value.end() && it->second->id() == nbt::TagId::Float) {
            livingEntity->setHealth(dynamic_cast<const nbt::tags::float_tag&>(*it->second).value);
        }

        // TODO: 添加吸收值 setter 到 LivingEntity
        // it = data.value.find("AbsorptionAmount");
        // if (it != data.value.end() && it->second->id() == nbt::TagId::Float) {
        //     livingEntity->setAbsorption(dynamic_cast<const nbt::tags::float_tag&>(*it->second).value);
        // }
    }
}

std::string EntityDataAccessor::getDisplayName() const
{
    if (m_entity == nullptr) {
        return "Unknown entity";
    }
    std::string name = m_entity->customNameText();
    if (name.empty()) {
        name = m_entity->getTypeId();
    }
    return name;
}

std::string EntityDataAccessor::getModifiedMessage() const
{
    return "Modified entity data for " + getDisplayName();
}

std::string EntityDataAccessor::getQueryMessage(const nbt::tags::tag& nbt) const
{
    std::ostringstream ss;
    ss << nbt::contexts::mojangson << nbt;
    return ss.str();
}

std::string EntityDataAccessor::getGetMessage(const NbtPath& path, double scale, i32 value) const
{
    std::ostringstream ss;
    ss << nbt::contexts::mojangson;
    ss << getDisplayName() << " has " << path.toString() << " = ";
    if (scale != 1.0) {
        ss << value << " (scaled by " << scale << ")";
    } else {
        ss << value;
    }
    return ss.str();
}

// ========== StorageDataAccessor 实现 ==========

StorageDataAccessor::StorageDataAccessor(CommandStorage* storage, const ResourceLocation& id)
    : m_storage(storage)
    , m_id(id)
{}

std::unique_ptr<nbt::tags::compound_tag> StorageDataAccessor::getData() const
{
    if (m_storage == nullptr) {
        throw CommandException(CommandErrorType::NbtPathNotFound, "Command storage not available", 0);
    }

    return m_storage->get(m_id);
}

void StorageDataAccessor::mergeData(const nbt::tags::compound_tag& data)
{
    if (m_storage == nullptr) {
        throw CommandException(CommandErrorType::NbtPathNotFound, "Command storage not available", 0);
    }

    auto existing = m_storage->get(m_id);
    if (existing == nullptr) {
        existing = std::make_unique<nbt::tags::compound_tag>();
    }

    // 合并数据
    for (const auto& [key, value] : data.value) {
        existing->value[key] = value->copy();
    }

    m_storage->set(m_id, *existing);
}

std::string StorageDataAccessor::getDisplayName() const
{
    return m_id.toString();
}

std::string StorageDataAccessor::getModifiedMessage() const
{
    return "Modified storage " + m_id.toString();
}

std::string StorageDataAccessor::getQueryMessage(const nbt::tags::tag& nbt) const
{
    std::ostringstream ss;
    ss << nbt::contexts::mojangson << nbt;
    return ss.str();
}

std::string StorageDataAccessor::getGetMessage(const NbtPath& path, double scale, i32 value) const
{
    std::ostringstream ss;
    ss << nbt::contexts::mojangson;
    ss << "Storage " << m_id.toString() << " has " << path.toString() << " = ";
    if (scale != 1.0) {
        ss << value << " (scaled by " << scale << ")";
    } else {
        ss << value;
    }
    return ss.str();
}

// ========== CommandStorage 实现 ==========

std::unique_ptr<nbt::tags::compound_tag> CommandStorage::get(const ResourceLocation& id)
{
    std::string key = id.toString();
    auto it = m_storage.find(key);
    if (it != m_storage.end() && it->second != nullptr) {
        // 返回深拷贝
        auto copy = it->second->copy();
        return std::unique_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(copy.release()));
    }
    return std::make_unique<nbt::tags::compound_tag>();
}

void CommandStorage::set(const ResourceLocation& id, const nbt::tags::compound_tag& data)
{
    std::string key = id.toString();
    auto copy = data.copy();
    m_storage[key] = std::unique_ptr<nbt::tags::compound_tag>(dynamic_cast<nbt::tags::compound_tag*>(copy.release()));
    m_dirty = true;
}

bool CommandStorage::exists(const ResourceLocation& id) const
{
    std::string key = id.toString();
    auto it = m_storage.find(key);
    return it != m_storage.end() && it->second != nullptr;
}

std::vector<ResourceLocation> CommandStorage::listAll() const
{
    std::vector<ResourceLocation> result;
    for (const auto& [key, value] : m_storage) {
        result.push_back(ResourceLocation(key));
    }
    return result;
}

void CommandStorage::clear(const ResourceLocation& id)
{
    std::string key = id.toString();
    m_storage.erase(key);
    m_dirty = true;
}

void CommandStorage::save(nlohmann::json& json) const
{
    for (const auto& [key, value] : m_storage) {
        if (value != nullptr) {
            std::ostringstream ss;
            ss << nbt::contexts::mojangson << *value;
            json[key] = ss.str();
        }
    }
}

void CommandStorage::load(const nlohmann::json& json)
{
    m_storage.clear();
    for (auto it = json.begin(); it != json.end(); ++it) {
        if (it->is_string()) {
            std::string nbtStr = it->get<std::string>();
            std::istringstream ss(nbtStr);
            ss >> nbt::contexts::mojangson;
            try {
                auto compound = nbt::tags::compound_tag::read(ss);
                if (compound) {
                    m_storage[it.key()] = std::move(compound);
                }
            }
            catch (...) {
                // 解析失败，跳过
            }
        }
    }
    m_dirty = false;
}

} // namespace command
} // namespace mc
