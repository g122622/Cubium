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

#include "LevelDBKey.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mc::world::storage::reader::bedrock {

namespace {

/**
 * @brief 将字符串视图转换为字节数组
 * @param value 输入字符串
 * @return 字节数组
 */
std::vector<u8> toBytes(std::string_view value)
{
    return std::vector<u8>(value.begin(), value.end());
}

/**
 * @brief 以小端序追加 32 位整数到字节数组
 * @param bytes 目标字节数组
 * @param value 要追加的整数值
 */
void appendLe32(std::vector<u8>& bytes, i32 value)
{
    bytes.push_back(static_cast<u8>(value & 0xFF));
    bytes.push_back(static_cast<u8>((value >> 8) & 0xFF));
    bytes.push_back(static_cast<u8>((value >> 16) & 0xFF));
    bytes.push_back(static_cast<u8>((value >> 24) & 0xFF));
}

} // namespace

bool LevelDBKey::startsWith(const std::vector<u8>& input, const std::vector<u8>& prefix)
{
    return input.size() >= prefix.size() && std::equal(prefix.begin(), prefix.end(), input.begin());
}

std::string LevelDBKey::extractSuffix(const std::vector<u8>& input, const std::vector<u8>& prefix)
{
    return std::string(input.begin() + static_cast<std::ptrdiff_t>(prefix.size()), input.end());
}

std::vector<u8> LevelDBKey::key(DimensionId dimension, const ChunkPos& pos, ChunkType type)
{
    std::vector<u8> bytes;
    // 主世界键长度: 4(x) + 4(z) + 1(type) = 9
    // 其他维度键长度: 4(x) + 4(z) + 4(dimension) + 1(type) = 13
    bytes.reserve(dimension == OVERWORLD_DIMENSION ? 9 : 13);
    appendLe32(bytes, pos.x);
    appendLe32(bytes, pos.z);
    if (dimension != OVERWORLD_DIMENSION) {
        appendLe32(bytes, dimension);
    }
    bytes.push_back(static_cast<u8>(type));
    return bytes;
}

std::vector<u8> LevelDBKey::key(DimensionId dimension, const ChunkPos& pos, i8 y, ChunkType type)
{
    auto bytes = key(dimension, pos, type);
    bytes.push_back(static_cast<u8>(y));
    return bytes;
}

std::vector<u8> LevelDBKey::key(const std::vector<u8>& prefix, DimensionId dimension, const ChunkPos& pos)
{
    std::vector<u8> bytes;
    // 主世界键长度: prefix + 4(x) + 4(z) = prefix + 8
    // 其他维度键长度: prefix + 4(x) + 4(z) + 4(dimension) = prefix + 12
    bytes.reserve(prefix.size() + (dimension == OVERWORLD_DIMENSION ? 8 : 12));
    bytes.insert(bytes.end(), prefix.begin(), prefix.end());
    appendLe32(bytes, pos.x);
    appendLe32(bytes, pos.z);
    if (dimension != OVERWORLD_DIMENSION) {
        appendLe32(bytes, dimension);
    }
    return bytes;
}

std::vector<u8> LevelDBKey::key(const std::vector<u8>& prefix, std::string_view suffix)
{
    std::vector<u8> bytes;
    bytes.reserve(prefix.size() + suffix.size());
    bytes.insert(bytes.end(), prefix.begin(), prefix.end());
    bytes.insert(bytes.end(), suffix.begin(), suffix.end());
    return bytes;
}

std::vector<u8> LevelDBKey::chunkPrefix(DimensionId dimension, const ChunkPos& pos)
{
    std::vector<u8> bytes;
    // 主世界前缀长度: 4(x) + 4(z) = 8
    // 其他维度前缀长度: 4(x) + 4(z) + 4(dimension) = 12
    bytes.reserve(dimension == OVERWORLD_DIMENSION ? 8 : 12);
    appendLe32(bytes, pos.x);
    appendLe32(bytes, pos.z);
    if (dimension != OVERWORLD_DIMENSION) {
        appendLe32(bytes, dimension);
    }
    return bytes;
}

const std::vector<u8>& LevelDBKey::actorPrefix()
{
    static const auto value = toBytes("actorprefix");
    return value;
}

const std::vector<u8>& LevelDBKey::biomeIdsTable()
{
    static const auto value = toBytes("BiomeIdsTable");
    return value;
}

const std::vector<u8>& LevelDBKey::digpPrefix()
{
    static const auto value = toBytes("digp");
    return value;
}

const std::vector<u8>& LevelDBKey::dimensionNameIdTable()
{
    static const auto value = toBytes("DimensionNameIdTable");
    return value;
}

const std::vector<u8>& LevelDBKey::localPlayer()
{
    static const auto value = toBytes("~local_player");
    return value;
}

const std::vector<u8>& LevelDBKey::mapPrefix()
{
    static const auto value = toBytes("map_");
    return value;
}

const std::vector<u8>& LevelDBKey::portals()
{
    static const auto value = toBytes("portals");
    return value;
}

const std::vector<u8>& LevelDBKey::posTrackDb()
{
    static const auto value = toBytes("PosTrackDB-0x");
    return value;
}

const std::vector<u8>& LevelDBKey::posTrackDbLastId()
{
    static const auto value = toBytes("PositionTrackDB-LastId");
    return value;
}

} // namespace mc::world::storage::reader::bedrock
