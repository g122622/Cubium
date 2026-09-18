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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/data/ChunkData.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace mc::network {

// ============================================================================
// 区块序列化器 - 将区块数据序列化为网络传输格式
// ============================================================================

/**
 * @brief 区块数据的二进制网络序列化与反序列化
 *
 * 双向共用：服务端序列化后经 LocalTransport 直传（集成服）或经
 * VanillaChunkWire 翻译为 Java 线格式下发（远程服），客户端反序列化。
 * 两侧必须保持字节级一致，故本类留在 common。
 */
class ChunkSerializer {
public:
    // 序列化区块数据到包格式
    static Result<std::vector<u8>> serializeChunk(const ChunkData& chunk);
    static std::vector<u8> serializeSection(const ChunkSection& section);

    // 反序列化区块数据
    static Result<std::unique_ptr<ChunkData>> deserializeChunk(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);
    /** 直接对原始字节缓冲反序列化（零拷贝视图，不拥有 data 生命周期，调用方负责保活）。 */
    static Result<std::unique_ptr<ChunkData>> deserializeChunk(ChunkCoord x, ChunkCoord z, const u8* data, size_t size);
    static Result<std::unique_ptr<ChunkSection>> deserializeChunkSection(const u8* data, size_t size);

    // 计算序列化后的大小
    static size_t calculateChunkSize(const ChunkData& chunk);
    static size_t calculateSectionSize(const ChunkSection& section);

    // 区块段位掩码计算
    static u32 calculateSectionMask(const ChunkData& chunk);
};

} // namespace mc::network
