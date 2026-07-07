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

Result<std::unique_ptr<Entity>> EntityDeserializer::deserialize(const nbt::tags::compound_tag& tag, IWorld* world)
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
    auto entity = entityType->create(world);
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

    // 5. 处理 Passengers 递归加载
    // 当前运行时的实体管理器以 EntityId 互相关联，Passenger 列表只保存 EntityId。
    // 因此这里不能像原版那样只在内存里临时拼出骑乘树，否则 passenger unique_ptr 会在
    // 当前作用域结束后释放，留下无效的乘客关系。
    //
    // 这里的策略是：
    // - 如果没有 world，则拒绝加载带乘客的实体，避免构造悬空关系。
    // - 如果有 world，则把乘客递归反序列化并真正 spawn 进世界，再调用 startRiding() 建立关系。
    if (const auto* passengersList = nbt_helper::tryGetList(tag, nbt_keys::PASSENGERS)) {
        if (world == nullptr) {
            return Error(ErrorCode::InvalidState, "Cannot deserialize entity passengers without world context");
        }

        if (passengersList->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*passengersList);
            for (const auto& passengerTag : compoundList.value) {
                auto passengerResult = deserialize(passengerTag, world);
                if (passengerResult.failed()) {
                    return passengerResult.error();
                }

                auto passenger = passengerResult.value();
                if (passenger == nullptr) {
                    return Error(ErrorCode::InvalidData, "Passenger deserialized to null entity");
                }

                EntityId passengerId = world->spawnEntity(std::move(passenger));
                if (passengerId == 0) {
                    return Error(ErrorCode::InvalidState, "Failed to spawn deserialized passenger entity");
                }

                Entity* spawnedPassenger = world->getEntity(passengerId);
                if (spawnedPassenger == nullptr) {
                    return Error(ErrorCode::InvalidState, "Spawned passenger entity not found in world");
                }

                if (!spawnedPassenger->startRiding(*entity)) {
                    spawnedPassenger->remove();
                    return Error(ErrorCode::InvalidState,
                        fmt::format("Failed to attach passenger '{}' to vehicle '{}'",
                            spawnedPassenger->getTypeId(),
                            entity->getTypeId()));
                }
                // TODO(vehicle-id-lifecycle): 主实体 vehicle 此时尚未 spawn 进世界，其 id
                // 仍为构造时的 0（== INVALID_ENTITY_ID）。乘客已 spawn 并通过 startRiding
                // 把 m_vehicle 记为 vehicle.id()==0。当调用方随后把 vehicle spawn 进世界、
                // 由 world 分配真实 id（例如 5）后，乘客的 m_vehicle 仍指向 0，骑乘关系会失效。
                // 当前 Entity::startRiding 已容忍 vehicle.id()==0（见 step1/step2 修复），
                // 故 deserialize 阶段可成功建立关系；但调用方在 spawn vehicle 后需同步刷新
                // 所有乘客的 m_vehicle 指向新 id（或改为先 spawn vehicle 再挂乘客）。
                // 这是一个独立的生产缺陷，留待后续修复。
            }
        }
    }

    return entity;
}

Result<std::unique_ptr<Entity>> EntityDeserializer::deserializeFromBinary(const std::vector<u8>& data, IWorld* world)
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

        return deserialize(*root, world);
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
