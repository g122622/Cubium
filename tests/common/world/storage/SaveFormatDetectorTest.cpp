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

#include "common/TempDirHelper.hpp"
#include "common/world/storage/backend/JavaAnvilBackend.hpp"
#include "common/world/storage/core/SaveFormat.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace mc::world::storage {
namespace {

class SaveFormatDetectorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // PID + 纳秒 + 计数器组合，跨进程唯一，避免 CTest -j16 同名目录撞锁
        m_tmpDir = mc::test::makeUniqueTestDir("mc_saveformat_test");
    }

    void TearDown() override { mc::test::removeTestDir(m_tmpDir); }

    void createFile(const std::filesystem::path& path, const std::vector<u8>& data = {})
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);
        if (!data.empty()) {
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }
        file.close();
    }

    std::filesystem::path m_tmpDir;
};

TEST_F(SaveFormatDetectorTest, NonExistentDirectory)
{
    auto result = SaveFormatDetector::detect(m_tmpDir / "nonexistent");
    EXPECT_TRUE(result.failed());
}

TEST_F(SaveFormatDetectorTest, EmptyDirectoryDefaultsToNative)
{
    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::Native);
    EXPECT_FALSE(result.value().readonly);
}

TEST_F(SaveFormatDetectorTest, DetectJavaAnvilByRegionFiles)
{
    std::filesystem::path regionDir = m_tmpDir / "region";
    std::filesystem::create_directories(regionDir);
    createFile(regionDir / "r.0.0.mca");

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::JavaAnvil);
    EXPECT_TRUE(result.value().readonly);
}

TEST_F(SaveFormatDetectorTest, DetectBedrockLDBByDbFiles)
{
    std::filesystem::path dbDir = m_tmpDir / "db";
    std::filesystem::create_directories(dbDir);
    createFile(dbDir / "000001.ldb");
    createFile(dbDir / "MANIFEST-000002");

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::BedrockLDB);
    EXPECT_TRUE(result.value().readonly);
}

TEST_F(SaveFormatDetectorTest, DetectNativeRocksDBByOptionsFile)
{
    std::filesystem::path dbDir = m_tmpDir / "db";
    std::filesystem::create_directories(dbDir);
    createFile(dbDir / "CURRENT");
    createFile(dbDir / "OPTIONS-000005");
    createFile(dbDir / "000009.sst");

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::Native);
    EXPECT_FALSE(result.value().readonly);
}

TEST_F(SaveFormatDetectorTest, JavaAnvilTakesPriorityOverDb)
{
    // Both region/ and db/ exist - Java Anvil should be detected
    std::filesystem::path regionDir = m_tmpDir / "region";
    std::filesystem::create_directories(regionDir);
    createFile(regionDir / "r.0.0.mca");

    std::filesystem::path dbDir = m_tmpDir / "db";
    std::filesystem::create_directories(dbDir);
    createFile(dbDir / "CURRENT");
    createFile(dbDir / "OPTIONS-000005");

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::JavaAnvil);
}

TEST_F(SaveFormatDetectorTest, BedrockLdbWithLogFiles)
{
    std::filesystem::path dbDir = m_tmpDir / "db";
    std::filesystem::create_directories(dbDir);
    createFile(dbDir / "000001.log");
    createFile(dbDir / "MANIFEST-000002");

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::BedrockLDB);
}

TEST_F(SaveFormatDetectorTest, DbWithLdbAndOptionsIsNative)
{
    // .ldb file exists but also has OPTIONS- → RocksDB, not LevelDB
    std::filesystem::path dbDir = m_tmpDir / "db";
    std::filesystem::create_directories(dbDir);
    createFile(dbDir / "000001.ldb");
    createFile(dbDir / "CURRENT");
    createFile(dbDir / "OPTIONS-000005");

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::Native);
}

TEST_F(SaveFormatDetectorTest, EmptyDbDirectoryDefaultsToNative)
{
    std::filesystem::path dbDir = m_tmpDir / "db";
    std::filesystem::create_directories(dbDir);

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::Native);
}

TEST_F(SaveFormatDetectorTest, RegionDirWithNoMcaDefaultsToNative)
{
    std::filesystem::path regionDir = m_tmpDir / "region";
    std::filesystem::create_directories(regionDir);
    createFile(regionDir / "dummy.txt");

    auto result = SaveFormatDetector::detect(m_tmpDir);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value().format, SaveFormat::Native);
}

TEST_F(SaveFormatDetectorTest, BackendOpenConsumesProvidedFormatInfoWithoutRedetecting)
{
    std::filesystem::path regionDir = m_tmpDir / "region";
    std::filesystem::create_directories(regionDir);
    createFile(regionDir / "r.0.0.mca");

    SaveFormatInfo javaInfo;
    javaInfo.format = SaveFormat::JavaAnvil;
    javaInfo.formatName = "Java Test";
    javaInfo.dataVersion = 2500;
    javaInfo.readonly = true;

    JavaAnvilBackend javaBackend;
    auto javaOpenResult = javaBackend.open(m_tmpDir, javaInfo);
    ASSERT_TRUE(javaOpenResult.success()) << javaOpenResult.error().message();
    EXPECT_EQ(javaBackend.formatInfo().format, SaveFormat::JavaAnvil);
    EXPECT_EQ(javaBackend.formatInfo().formatName, "Java Test");
    javaBackend.close();

    SaveFormatInfo wrongInfo;
    wrongInfo.format = SaveFormat::BedrockLDB;
    wrongInfo.formatName = "Wrong";
    wrongInfo.readonly = true;

    auto wrongJavaOpenResult = javaBackend.open(m_tmpDir, wrongInfo);
    EXPECT_TRUE(wrongJavaOpenResult.failed());

    // Bedrock backend 的 open 还需要真实可打开的 LevelDB 目录，这里不伪造损坏 DB。
    // P0-4 关注的是“backend 不再自行 detect”，此处用 Java backend 的签名与错误路径锁定即可。
}

} // namespace
} // namespace mc::world::storage
