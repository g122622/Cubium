#pragma once

#include "../../core/Types.hpp"
#include "../../entity/core/EntityDataManager.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 实体元数据序列化器
 *
 * 将 EntityDataManager 中的数据参数序列化为网络格式，
 * 用于 EntityMetadataPacket 和 SpawnMobPacket。
 *
 * MC 1.16.5 元数据格式：
 * - 每个条目：索引(1字节) + 类型ID(1字节) + 数据(变长)
 * - 结束标记：0xFF
 *
 * 类型ID映射：
 * 0: Byte (i8)
 * 1: VarInt (i32)
 * 2: Float (f32)
 * 3: std::string (UTF-8)
 * 4: TextComponent (JSON)
 * 5: OptChat (Optional JSON)
 * 6: Slot (ItemStack)
 * 7: Boolean (bool)
 * 8: Rotation (f32, f32, f32)
 * 9: Position (BlockPos)
 * 10: OptPosition (Optional BlockPos)
 * 11: Direction (u8)
 * 12: OptUUID (Optional UUID)
 * 13: OptBlockID (Optional VarInt)
 * 14: NBT (CompoundTag)
 * 15: Particle
 * 16: VillagerData (VarInt x3)
 * 17: OptVarInt (Optional VarInt)
 * 18: Pose (u8)
 */
class EntityMetadataSerializer {
public:
    /**
     * @brief 序列化 EntityDataManager 为网络格式
     * @param manager 数据管理器
     * @param dirtyOnly 是否只序列化脏数据
     * @return 序列化后的字节流
     */
    static std::vector<u8> serialize(const entity::EntityDataManager& manager, bool dirtyOnly = true);

    /**
     * @brief 反序列化网络格式到 EntityDataManager
     * @param data 字节流
     * @param manager 数据管理器
     * @return 是否成功
     */
    static bool deserialize(const std::vector<u8>& data, entity::EntityDataManager& manager);

    /**
     * @brief 序列化单个数据条目
     * @param id 参数ID
     * @param value 数据值
     * @param output 输出缓冲区
     */
    static void serializeEntry(u16 id, const entity::DataValue& value, std::vector<u8>& output);

private:
    // 写入变长整数
    static void writeVarInt(i32 value, std::vector<u8>& output);
    static void writeVarLong(i64 value, std::vector<u8>& output);

    // 读取变长整数
    static i32 readVarInt(const u8* data, size_t size, size_t& offset);
    static i64 readVarLong(const u8* data, size_t size, size_t& offset);

    // 写入字符串
    static void writeString(const std::string& str, std::vector<u8>& output);
    static std::string readString(const u8* data, size_t size, size_t& offset);
};

} // namespace mc::network
