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
#include "common/command/arguments/NbtPath.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

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
    if (m_blockEntity == nullptr) {
        throw CommandException(CommandErrorType::NbtPathNotFound,
            "No block entity at position " + std::to_string(m_pos.x) + ", " + std::to_string(m_pos.y) + ", " +
                std::to_string(m_pos.z),
            0);
    }

    // 使用 BlockEntity 原生 NBT 序列化，获取完整数据
    auto compound = std::make_unique<nbt::tags::compound_tag>();
    m_blockEntity->saveToNBT(*compound);
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

    // 获取当前完整数据
    nbt::tags::compound_tag current;
    m_blockEntity->saveToNBT(current);

    // 合并：将传入数据的每个键值对覆盖到当前数据中
    for (const auto& [key, value] : data.value) {
        current.value[key] = value->copy();
    }

    // 使用 BlockEntity 原生 NBT 反序列化加载合并后的数据
    m_blockEntity->loadFromNBT(current);
    m_blockEntity->setChanged();

    // 通知客户端方块更新
    if (m_world != nullptr) {
        m_world->notifyBlockUpdate(m_pos);
    }
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

    // 使用 Entity 原生 NBT 序列化，获取完整数据
    // 参考 MC Java: NbtPredicate.getEntityTagToCompare(entity) -> entity.saveWithoutId()
    auto compound = std::make_unique<nbt::tags::compound_tag>();
    m_entity->writeToNBT(*compound);
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

    // 参考 MC Java: EntityDataAccessor.setData()
    // 1. 保存当前 UUID（加载 NBT 会覆盖 UUID，需要恢复）
    auto savedUuid = m_entity->uuid();

    // 2. 获取当前完整数据
    nbt::tags::compound_tag current;
    m_entity->writeToNBT(current);

    // 3. 合并：将传入数据的每个键值对覆盖到当前数据中
    for (const auto& [key, value] : data.value) {
        current.value[key] = value->copy();
    }

    // 4. 使用 Entity 原生 NBT 反序列化加载合并后的数据
    auto result = m_entity->readFromNBT(current);
    if (!result.success()) {
        throw CommandException(
            CommandErrorType::NbtPathInvalidType, "Failed to load entity data: " + result.error().message(), 0);
    }

    // 5. 恢复 UUID（UUID 不应被 /data 命令修改）
    m_entity->setUuid(savedUuid);
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
