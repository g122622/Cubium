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
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace mc::world::storage::reader::java {

/**
 * @brief Java 版 .mca 区域文件读取器
 *
 * 解析 Minecraft Java 版 Anvil 格式的 .mca 区域文件。
 * 每个 .mca 文件覆盖 32x32 个区块（一个区域）。
 *
 * 文件格式：
 * - 偏移表：1024 个 4 字节条目（256 个 X × 256 个 Z）
 * - 时间戳表：1024 个 4 字节条目
 * - 区块数据：以扇区为单位存储（每扇区 4096 字节）
 */
class RegionFile {
public:
    /**
     * @brief 构造区域文件读取器
     * @param path .mca 文件路径
     */
    explicit RegionFile(const std::filesystem::path& path);

    ~RegionFile();

    RegionFile(const RegionFile&) = delete;
    RegionFile& operator=(const RegionFile&) = delete;
    RegionFile(RegionFile&&) noexcept;
    RegionFile& operator=(RegionFile&&) noexcept;

    /**
     * @brief 打开区域文件并读取头部
     * @return 成功或错误
     */
    Result<void> open();

    /**
     * @brief 关闭区域文件
     */
    void close();

    /**
     * @brief 检查是否已打开
     */
    [[nodiscard]] bool isOpen() const { return m_isOpen; }

    /**
     * @brief 检查指定区块是否存在
     * @param localX 区域内 X 坐标（0-31）
     * @param localZ 区域内 Z 坐标（0-31）
     */
    [[nodiscard]] bool hasChunk(i32 localX, i32 localZ) const;

    /**
     * @brief 读取区块的原始 NBT 数据（已解压）
     * @param localX 区域内 X 坐标（0-31）
     * @param localZ 区域内 Z 坐标（0-31）
     * @return 解压后的 NBT 字节流
     */
    Result<std::vector<u8>> readChunkData(i32 localX, i32 localZ);

    /**
     * @brief 列举区域内所有存在的区块坐标
     * @return (localX, localZ) 列表
     */
    [[nodiscard]] std::vector<std::pair<i32, i32>> listChunks() const;

    /**
     * @brief 获取区域 X 坐标
     */
    [[nodiscard]] i32 regionX() const { return m_regionX; }

    /**
     * @brief 获取区域 Z 坐标
     */
    [[nodiscard]] i32 regionZ() const { return m_regionZ; }

    /**
     * @brief 获取文件路径
     */
    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    static constexpr i32 SECTOR_SIZE = 4096;
    static constexpr i32 HEADER_SIZE = 8192;
    static constexpr i32 CHUNKS_PER_REGION = 1024;

    struct ChunkLocation {
        u32 offset;     // 扇区偏移（以 4096 字节为单位）
        u8 sectorCount; // 占用的扇区数
    };

    enum class CompressionType : u8 {
        None = 0,
        GZip = 1,
        ZLib = 2,
        Uncompressed = 3,
        LZ4 = 4,
        External = 128,
    };

    Result<void> readHeader();
    Result<std::vector<u8>> decompress(CompressionType type, const std::vector<u8>& data);

    std::filesystem::path m_path;
    std::ifstream m_stream;
    std::array<ChunkLocation, CHUNKS_PER_REGION> m_locations{};
    std::array<u32, CHUNKS_PER_REGION> m_timestamps{};
    bool m_isOpen = false;
    i32 m_regionX = 0;
    i32 m_regionZ = 0;
};

} // namespace mc::world::storage::reader::java
