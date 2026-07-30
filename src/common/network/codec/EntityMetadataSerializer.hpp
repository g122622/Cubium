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

#include "common/core/Types.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include <vector>

namespace mc::network {

/**
 * @brief 实体元数据序列化器
 *
 * 将 EntityDataManager 中的数据参数序列化为网络格式，
 * 用于 ir::play::SetEntityData 与 SpawnMob 的元数据段。
 *
 * MC 1.21.11 元数据格式（对齐 net.minecraft.network.syncher.SynchedEntityData.DataValue）：
 * - 每个条目：index(1 字节) + serializerId(VarInt) + value(codec 编码)
 * - 结束标记：0xFF（ClientboundSetEntityDataPacket.EOF_MARKER）
 * - serializerId 取自 EntityDataSerializers 静态注册顺序（注册序即整数 ID）
 *
 * 本项目 DataValue 变体 → 1.21.11 serializerId 映射：
 *   i8            → 0  BYTE
 *   i32           → 1  INT (VAR_INT)
 *   i64           → 2  LONG (VAR_LONG)
 *   f32           → 3  FLOAT（大端）
 *   string        → 4  STRING (VarInt len + UTF-8)
 *   bool          → 8  BOOLEAN
 *   Vector3i      → 10 BLOCK_POS（i64 packed，X26/Z26/Y12，BlockPos.asLong）
 *   Vector2f      → 9  ROTATIONS（f32×3，z 补 0）
 *   Vector3f      → 9  ROTATIONS（f32×3，大端）
 *   ItemStackView → 7  ITEM_STACK（OPTIONAL_STREAM_CODEC：VarInt(count)，
 *                  count<=0 即空；否则 VarInt(itemId)+DataComponentPatch 字节，patch 无外层
 *                  长度前缀，以自身 VarInt(addedCount)+VarInt(removedCount) 自终止）
 *
 * 注：1.16.5 用单字节 typeId 且编号不同（Boolean=7/Rotation=8/Position=9）；
 *     1.21.11 改 VarInt serializerId 且 BOOLEAN=8/ROTATIONS=9/BLOCK_POS=10，
 *     BlockPos 由 VarInt×3 改为单 i64 packed。ItemStackView 经 ItemEntity.DATA_ITEM_PARAM
 *     注册（掉落物物品本体同步）；Vector3f/Vector2f/Vector3i 当前无注册参数，仅为完备性保留。
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
    static void _writeVarInt(i32 value, std::vector<u8>& output) noexcept;
    static void _writeVarLong(i64 value, std::vector<u8>& output) noexcept;

    // 读取变长整数
    static i32 _readVarInt(const u8* data, size_t size, size_t& offset) noexcept;
    static i64 _readVarLong(const u8* data, size_t size, size_t& offset) noexcept;

    // 写入字符串
    static void _writeString(const std::string& str, std::vector<u8>& output) noexcept;
    static std::string _readString(const u8* data, size_t size, size_t& offset) noexcept;

    // 大端序定长读写（对齐 Java FriendlyByteBuf：FLOAT/i64 均大端）
    static void _writeBigEndianF32(f32 value, std::vector<u8>& output) noexcept;
    static void _writeBigEndianI64(i64 value, std::vector<u8>& output) noexcept;
    static bool _readBigEndianF32(const u8* data, size_t size, size_t& offset, f32& out) noexcept;
    static bool _readBigEndianI64(const u8* data, size_t size, size_t& offset, i64& out) noexcept;
};

} // namespace mc::network
