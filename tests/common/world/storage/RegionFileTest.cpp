/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software are
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "common/world/storage/reader/java/RegionFile.hpp"
#include "common/TempDirHelper.hpp"
#include <filesystem>
#include <gtest/gtest.h>

namespace mc::world::storage::reader::java {
namespace {

class RegionFileTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // PID + 纳秒 + 计数器组合，跨进程唯一，避免 CTest -j16 同名目录撞锁
        m_tmpDir = mc::test::makeUniqueTestDir("mc_region_test");
    }

    void TearDown() override { mc::test::removeTestDir(m_tmpDir); }

    /// 创建一个最简的 .mca 文件（空区域，无区块数据）
    void createEmptyRegionFile(const std::filesystem::path& path)
    {
        std::ofstream file(path, std::ios::binary);
        // 偏移表：1024 个 4 字节条目，全零
        std::vector<u8> offsetTable(4096, 0);
        file.write(reinterpret_cast<const char*>(offsetTable.data()), static_cast<std::streamsize>(offsetTable.size()));
        // 时间戳表：1024 个 4 字节条目，全零
        file.write(reinterpret_cast<const char*>(offsetTable.data()), static_cast<std::streamsize>(offsetTable.size()));
        file.close();
    }

    std::filesystem::path m_tmpDir;
};

TEST_F(RegionFileTest, OpenNonExistentFile)
{
    RegionFile region(m_tmpDir / "nonexistent.mca");
    auto result = region.open();
    EXPECT_TRUE(result.failed());
}

TEST_F(RegionFileTest, OpenEmptyRegionFile)
{
    auto path = m_tmpDir / "r.0.0.mca";
    createEmptyRegionFile(path);

    RegionFile region(path);
    auto result = region.open();
    EXPECT_TRUE(result.success());
    EXPECT_EQ(region.regionX(), 0);
    EXPECT_EQ(region.regionZ(), 0);

    // 空区域不应包含任何区块
    auto chunks = region.listChunks();
    EXPECT_TRUE(chunks.empty());

    region.close();
}

TEST_F(RegionFileTest, HasChunkReturnsFalseForEmptyRegion)
{
    auto path = m_tmpDir / "r.0.0.mca";
    createEmptyRegionFile(path);

    RegionFile region(path);
    auto result = region.open();
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(region.hasChunk(0, 0));
    EXPECT_FALSE(region.hasChunk(1, 1));
    region.close();
}

TEST_F(RegionFileTest, ParseRegionCoordinatesFromFilename)
{
    auto path = m_tmpDir / "r.-1.2.mca";
    createEmptyRegionFile(path);

    RegionFile region(path);
    auto result = region.open();
    EXPECT_TRUE(result.success());
    EXPECT_EQ(region.regionX(), -1);
    EXPECT_EQ(region.regionZ(), 2);
    region.close();
}

TEST_F(RegionFileTest, ReadChunkFromEmptyRegionReturnsEmpty)
{
    auto path = m_tmpDir / "r.0.0.mca";
    createEmptyRegionFile(path);

    RegionFile region(path);
    auto result = region.open();
    EXPECT_TRUE(result.success());

    auto chunkResult = region.readChunkData(0, 0);
    EXPECT_TRUE(chunkResult.failed());
    region.close();
}

TEST_F(RegionFileTest, CloseWithoutOpenIsSafe)
{
    RegionFile region(m_tmpDir / "r.0.0.mca");
    // 应该不会崩溃
    region.close();
}

TEST_F(RegionFileTest, IsOpenReturnsFalseBeforeOpen)
{
    RegionFile region(m_tmpDir / "r.0.0.mca");
    EXPECT_FALSE(region.isOpen());
}

} // namespace
} // namespace mc::world::storage::reader::java
