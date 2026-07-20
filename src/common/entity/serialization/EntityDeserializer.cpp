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

#include "EntityDeserializer.hpp"
#include "EntityNbtKeys.hpp"
#include "NbtHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/world/IWorld.hpp"
#include <sstream>
#include <spdlog/spdlog.h>
#include <zlib.h>

namespace mc::entity::serialization {

Result<std::unique_ptr<Entity>> EntityDeserializer::deserialize(const nbt::tags::compound_tag& tag)
{
    // 1. 读取 "id" 标签获取实体类型字符串
    auto typeIdOpt = nbt_helper::tryGetString(tag, nbt_keys::ID);
    if (!typeIdOpt.has_value()) {
        return Error(ErrorCode::InvalidData, "Entity NBT missing 'id' tag");
    }

    const std::string& typeId = typeIdOpt.value();

    // 2. 通过 EntityRegistry 查找 EntityType
    const EntityType* entityType = EntityRegistry::instance().getType(typeId);
    if (entityType == nullptr) {
        return Error(ErrorCode::InvalidEntity, fmt::format("Unknown entity type: {}", typeId));
    }

    // 3. 调用 EntityType::create() 创建实例
    // 创建时实体 id 为 0（== INVALID_ENTITY_ID），由后续 spawnEntity 分配真实 id
    auto entity = entityType->create(nullptr);
    if (entity == nullptr) {
        return Error(ErrorCode::InvalidData, fmt::format("Failed to create entity of type: {}", typeId));
    }

    // 设置实体的类型ID
    entity->setTypeId(typeId);

    // 4. 调用 Entity::readFromNBT() 填充数据
    auto result = entity->readFromNBT(tag);
    if (!result.success()) {
        return Error(ErrorCode::InvalidData, fmt::format("Failed to read entity NBT: {}", result.error().message()));
    }

    // 5. 暂存 Passengers NBT 到实体的 m_pendingPassengersNbt
    // 不在反序列化阶段 spawn 乘客：主实体此时尚未 spawn，id 仍为 0，
    // 若此时 spawn 乘客并 startRiding，乘客的 m_vehicle 会被记为 0，
    // 后续主实体 spawn 时 id 改写为真实值，乘客 m_vehicle 失效。
    // 改为由调用方在 spawn 主实体后调用 attachPassengers 处理。
    if (const auto* passengersList = nbt_helper::tryGetList(tag, nbt_keys::PASSENGERS)) {
        if (passengersList->element_id() == nbt::TagId::Compound) {
            const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*passengersList);
            for (const auto& passengerTag : compoundList.value) {
                entity->_appendPendingPassengerNbt(passengerTag);
            }
        }
    }

    return entity;
}

Result<void> EntityDeserializer::attachPassengers(Entity& vehicle, IWorld& world)
{
    // 没有待处理乘客则直接返回
    if (!vehicle.hasPendingPassengersNbt()) {
        return Result<void>::ok();
    }

    // 取走待处理乘客 NBT 列表（takePendingPassengersNbt 会清空 vehicle 的字段）
    auto pendingPassengers = vehicle.takePendingPassengersNbt();

    for (auto& passengerTag : pendingPassengers) {
        // 递归反序列化乘客（同样不在此处 spawn 乘客的乘客）
        auto passengerResult = deserialize(passengerTag);
        if (passengerResult.failed()) {
            return passengerResult.error();
        }

        auto passenger = passengerResult.value();
        if (passenger == nullptr) {
            return Error(ErrorCode::InvalidData, "Passenger deserialized to null entity");
        }

        // 把乘客 spawn 进世界，由 world 分配真实 id
        EntityInstanceId passengerId = world.spawnEntity(std::move(passenger));
        if (passengerId == 0) {
            return Error(ErrorCode::InvalidState, "Failed to spawn deserialized passenger entity");
        }

        Entity* spawnedPassenger = world.getEntity(passengerId);
        if (spawnedPassenger == nullptr) {
            return Error(ErrorCode::InvalidState, "Spawned passenger entity not found in world");
        }

        // 此时 vehicle 已被 spawn（由调用方在调用本方法前完成），id 为真实值，
        // startRiding 会把乘客的 m_vehicle 记为 vehicle.id()（真实 id），关系持久有效
        if (!spawnedPassenger->startRiding(vehicle)) {
            spawnedPassenger->remove();
            return Error(ErrorCode::InvalidState,
                fmt::format("Failed to attach passenger '{}' to vehicle '{}'",
                    spawnedPassenger->getTypeId(),
                    vehicle.getTypeId()));
        }

        // 递归处理乘客自身的待处理乘客（多层骑乘，如 Boat → Zombie → BabyZombie）
        auto subResult = attachPassengers(*spawnedPassenger, world);
        if (subResult.failed()) {
            return subResult.error();
        }
    }

    return Result<void>::ok();
}

Result<std::unique_ptr<Entity>> EntityDeserializer::deserializeFromBinary(const std::vector<u8>& data)
{
    if (data.empty()) {
        return Error(ErrorCode::InvalidData, "Empty entity data");
    }

    // gzip 解压
    z_stream stream = {};
    stream.next_in = const_cast<u8*>(data.data());
    stream.avail_in = static_cast<u32>(data.size());

    if (inflateInit2(&stream, MAX_WBITS + 16) != Z_OK) {
        return Error(ErrorCode::InvalidData, "Failed to initialize zlib decompression");
    }

    std::vector<u8> decompressed;
    decompressed.reserve(data.size() * 4);

    u8 buffer[8192];
    int ret = Z_OK;
    do {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&stream);
            return Error(ErrorCode::InvalidData, fmt::format("Decompression error: {}", ret));
        }
        auto decoded = sizeof(buffer) - stream.avail_out;
        decompressed.insert(decompressed.end(), buffer, buffer + decoded);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);

    // 解析 NBT
    try {
        std::istringstream iss(std::string(decompressed.begin(), decompressed.end()));
        iss >> nbt::contexts::java;
        auto root = nbt::tags::compound_tag::read(iss);
        if (!root) {
            return Error(ErrorCode::InvalidData, "Failed to parse entity NBT");
        }

        return deserialize(*root);
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, fmt::format("Failed to deserialize entity from binary: {}", e.what()));
    }
}

Result<std::vector<u8>> EntityDeserializer::serializeToBinary(const Entity& entity)
{
    // 创建 NBT 标签并序列化实体数据
    nbt::tags::compound_tag tag;

    // 写入实体类型 ID
    tag.put(nbt_keys::ID, entity.getTypeId());

    // 调用实体的序列化方法
    entity.writeToNBT(tag);

    // 序列化 NBT 为二进制
    std::ostringstream oss;
    oss << nbt::contexts::java;
    nbt::operator<<(oss, tag);
    std::string nbtStr = oss.str();
    std::vector<u8> nbtData(nbtStr.begin(), nbtStr.end());

    // gzip 压缩（与 MC Java 版格式一致）
    z_stream stream = {};
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return Error(ErrorCode::CompressionFailed, "Failed to initialize zlib compression");
    }

    stream.next_in = const_cast<u8*>(nbtData.data());
    stream.avail_in = static_cast<u32>(nbtData.size());

    std::vector<u8> result;
    result.reserve(nbtData.size());

    u8 buffer[8192];
    int ret = Z_OK;
    do {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        ret = deflate(&stream, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&stream);
            return Error(ErrorCode::CompressionFailed, "Compression error");
        }
        auto encoded = sizeof(buffer) - stream.avail_out;
        result.insert(result.end(), buffer, buffer + encoded);
    } while (ret != Z_STREAM_END);

    deflateEnd(&stream);
    return result;
}

} // namespace mc::entity::serialization
