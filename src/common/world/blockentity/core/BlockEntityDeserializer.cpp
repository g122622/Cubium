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

#include "BlockEntityDeserializer.hpp"
#include "BlockEntityRegistry.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <zconf.h>
#include <zlib.h>

namespace mc::blockentity {

// ============================================================================
// NBT 辅助函数（匿名命名空间）
// ============================================================================

namespace {

/**
 * @brief 尝试从复合标签中获取字符串值
 * @param tag NBT 复合标签
 * @param key 键名
 * @return 字符串值，若不存在或类型不匹配则返回 nullopt
 */
std::optional<std::string> tryGetString(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return std::nullopt;
    }
    const auto* strTag = dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
    if (!strTag) {
        return std::nullopt;
    }
    return strTag->value;
}

/**
 * @brief 尝试从复合标签中获取整数值
 * @param tag NBT 复合标签
 * @param key 键名
 * @return 整数值，若不存在或类型不匹配则返回 nullopt
 */
std::optional<i32> tryGetInt(const nbt::tags::compound_tag& tag, const std::string& key)
{
    auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return std::nullopt;
    }
    const auto* intTag = dynamic_cast<const nbt::tags::int_tag*>(it->second.get());
    if (!intTag) {
        return std::nullopt;
    }
    return intTag->value;
}

/**
 * @brief gzip 解压缩
 * @param compressed 压缩数据
 * @return 解压后的数据或错误
 */
Result<std::vector<u8>> gzipDecompress(const std::vector<u8>& compressed)
{
    if (compressed.empty()) {
        return Error(ErrorCode::InvalidData, "Empty compressed data");
    }

    z_stream stream = {};
    stream.next_in = const_cast<u8*>(compressed.data());
    stream.avail_in = static_cast<u32>(compressed.size());

    if (inflateInit2(&stream, MAX_WBITS + 16) != Z_OK) {
        return Error(ErrorCode::InvalidData, "Failed to initialize zlib decompression");
    }

    std::vector<u8> result;
    result.reserve(compressed.size() * 4);

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
        result.insert(result.end(), buffer, buffer + decoded);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    return result;
}

/**
 * @brief gzip 压缩
 * @param data 原始数据
 * @return 压缩后的数据或错误
 */
Result<std::vector<u8>> gzipCompress(const std::vector<u8>& data)
{
    if (data.empty()) {
        return Error(ErrorCode::InvalidData, "Empty data to compress");
    }

    z_stream stream = {};
    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return Error(ErrorCode::InvalidData, "Failed to initialize zlib compression");
    }

    stream.next_in = const_cast<u8*>(data.data());
    stream.avail_in = static_cast<u32>(data.size());

    std::vector<u8> result;
    result.reserve(data.size());

    u8 buffer[8192];
    int ret = Z_OK;
    do {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        ret = deflate(&stream, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&stream);
            return Error(ErrorCode::InvalidData, "Compression error");
        }
        auto encoded = sizeof(buffer) - stream.avail_out;
        result.insert(result.end(), buffer, buffer + encoded);
    } while (ret != Z_STREAM_END);

    deflateEnd(&stream);
    return result;
}

} // anonymous namespace

// ============================================================================
// BlockEntityDeserializer 实现
// ============================================================================

Result<std::unique_ptr<BlockEntity>> BlockEntityDeserializer::deserialize(const nbt::tags::compound_tag& tag)
{
    // 1. 读取 "id" 标签
    auto idOpt = tryGetString(tag, "id");
    if (!idOpt.has_value()) {
        return Error(ErrorCode::InvalidData, "Block entity NBT missing 'id' tag");
    }

    // 2. 解析类型
    ResourceLocation id(idOpt.value());
    BlockEntityType type = blockEntityTypeFromId(id);
    if (type == BlockEntityType::Unknown) {
        spdlog::warn("BlockEntityDeserializer: Unknown block entity type '{}', skipping", idOpt.value());
        return std::unique_ptr<BlockEntity>(nullptr);
    }

    // 3. 读取位置
    auto xOpt = tryGetInt(tag, "x");
    auto yOpt = tryGetInt(tag, "y");
    auto zOpt = tryGetInt(tag, "z");
    if (!xOpt || !yOpt || !zOpt) {
        return Error(ErrorCode::InvalidData, "Block entity NBT missing position (x/y/z)");
    }

    BlockPos pos(*xOpt, *yOpt, *zOpt);

    // 4. 通过注册表创建实例
    auto blockEntity = BlockEntityRegistry::instance().create(type, pos);
    if (!blockEntity) {
        spdlog::warn("BlockEntityDeserializer: Failed to create block entity of type '{}' at ({}, {}, {})",
            idOpt.value(),
            pos.x,
            pos.y,
            pos.z);
        return std::unique_ptr<BlockEntity>(nullptr);
    }

    // 5. 加载 NBT 数据
    if (!blockEntity->loadFromNBT(tag)) {
        spdlog::warn("BlockEntityDeserializer: Failed to load NBT for block entity '{}' at ({}, {}, {})",
            idOpt.value(),
            pos.x,
            pos.y,
            pos.z);
    }

    return blockEntity;
}

Result<std::vector<std::unique_ptr<BlockEntity>>> BlockEntityDeserializer::deserializeListFromBinary(
    const std::vector<u8>& data)
{
    std::vector<std::unique_ptr<BlockEntity>> result;

    if (data.empty()) {
        return result;
    }

    // 解压缩
    auto decompressedResult = gzipDecompress(data);
    if (!decompressedResult.success()) {
        return decompressedResult.error();
    }

    const auto& decompressed = decompressedResult.value();

    // 解析 NBT
    try {
        std::istringstream iss(std::string(decompressed.begin(), decompressed.end()));
        iss >> nbt::contexts::java;
        auto root = nbt::tags::compound_tag::read(iss);

        // 根标签可能是包含 "block_entities" 列表的复合标签，或直接是列表
        // 也可能是列表标签 — 尝试两种格式
        if (root) {
            // 先检查是否包含 block_entities 列表
            auto it = root->value.find("block_entities");
            if (it != root->value.end()) {
                const auto* listTag = dynamic_cast<const nbt::tags::list_tag*>(it->second.get());
                if (listTag && listTag->element_id() == nbt::TagId::Compound) {
                    const auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*listTag);
                    for (const auto& element : compoundList.value) {
                        auto entityResult = deserialize(element);
                        if (!entityResult.success()) {
                            continue;
                        }
                        // 注意：Result<unique_ptr<T>>::value() 按值返回（内部 takeValue 会
                        // 把 m_value 置空），每次调用都"取走"实体。不能在条件里调用一次、
                        // 再在 push_back 里调用第二次（第二次取到空 unique_ptr，混入空指针）。
                        auto blockEntity = entityResult.value();
                        if (blockEntity) {
                            result.push_back(std::move(blockEntity));
                        }
                    }
                    return result;
                }
            }

            // 否则当作单个方块实体
            auto entityResult = deserialize(*root);
            if (entityResult.success()) {
                auto blockEntity = entityResult.value();
                if (blockEntity) {
                    result.push_back(std::move(blockEntity));
                }
            }
        }
    }
    catch (const std::exception& e) {
        return Error(
            ErrorCode::InvalidData, fmt::format("Failed to deserialize block entity list from binary: {}", e.what()));
    }

    return result;
}

Result<std::vector<u8>> BlockEntityDeserializer::serializeListToBinary(
    const std::vector<std::reference_wrapper<const BlockEntity>>& blockEntities)
{
    if (blockEntities.empty()) {
        return std::vector<u8>();
    }

    // 构建 NBT 列表
    nbt::tags::compound_list_tag listTag;

    for (const auto& entityRef : blockEntities) {
        const BlockEntity& entity = entityRef.get();

        nbt::tags::compound_tag compound;
        entity.saveToNBT(compound);
        listTag.value.push_back(std::move(compound));
    }

    // 包裹在根复合标签中
    nbt::tags::compound_tag root;
    root.value.emplace("block_entities", std::make_unique<nbt::tags::compound_list_tag>(std::move(listTag)));

    // 序列化为二进制
    std::ostringstream oss;
    oss << nbt::contexts::java;
    nbt::operator<<(oss, root);
    std::string nbtStr = oss.str();
    std::vector<u8> nbtData(nbtStr.begin(), nbtStr.end());

    // gzip 压缩
    return gzipCompress(nbtData);
}

} // namespace mc::blockentity
