/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/ sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// 真实 Java 版存档的测试夹具。
//
// 素材：tests/unit/testdata/worlds/java-anvil-1.21.11/
//   来源   HMCL 启动器导出的原版存档（源世界名「新的世界」，仓库内目录名改为 ASCII）
//   版本   Java 1.21.11 / DataVersion 4671
//   内容   主世界 4 个 region 文件共 2378 列、entities 97 列 205 个实体、poi 30 列、
//          下界与末地只有 data/ 没有 region/（玩家未去过）
//   取舍   已剔除 .DS_Store / session.lock / *_old / icon.png / 空 datapacks 目录；
//          刻意保留了 0 字节的 poi/r.0.-1.mca（RegionFile 打开失败路径的唯一样本）
//
// 本文件中的常量是**独立**得出的 ground truth：由一份不依赖项目代码的 Python Anvil 解析器
// 直接解出，用于交叉验证 C++ 读取器。改动素材时必须同步复核这些常量，否则测试会以
// "期望值过时"的形式失败，而那不一定意味着读取器有 bug。
//
// 与 WorldGenRegistryFixture 的区别：那套夹具读的是机器相关的外部路径，缺失即
// GTEST_SKIP；本素材随仓库分发，缺失属故障，因此夹具直接断言失败而不跳过。

#pragma once

#include "common/TestDataDir.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/storage/backend/JavaAnvilBackend.hpp"
#include "server/world/storage/core/SaveFormat.hpp"
#include "server/world/storage/reader/java/JavaBiomeMapper.hpp"
#include "server/world/storage/reader/java/JavaBlockStateMapper.hpp"
#include "server/world/storage/reader/java/JavaChunkReader.hpp"
#include "server/world/storage/reader/java/JavaColumnReader.hpp"
#include "server/world/storage/reader/java/JavaWorldReader.hpp"
#include "server/world/storage/reader/java/RegionFile.hpp"

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace mc {
namespace test {

/**
 * @brief 真实 Java 1.21.11 存档素材的常量与访问入口
 *
 * 常量集中在此，使"期望值"只有一处定义，各测试文件引用同一份 ground truth。
 */
struct JavaRealWorld {
    /// level.dat 的 Data.DataVersion
    static constexpr i32 kDataVersion = 4671;

    /// level.dat 的 Data.Version.Name
    static constexpr const char* kVersionName = "1.21.11";

    /// level.dat 的 Data.LevelName，UTF-8 字节：「新的世界」
    /// 写成转义字节而非源文件中文字面量，避免 MSVC 未开 /utf-8 时按 ANSI 代码页解释。
    static constexpr const char* kLevelNameUtf8 = "\xe6\x96\xb0\xe7\x9a\x84\xe4\xb8\x96\xe7\x95\x8c";

    /// level.dat 的 Data.WorldGenSettings.seed（Long，可能为负）
    static constexpr i64 kWorldGenSeed = -6671478382168981129LL;

    /// 存档内唯一的玩家 UUID（playerdata/ 文件名，也是 level.dat Data.Player.UUID 的十进制形式）
    static constexpr const char* kPlayerUuid = "4869b771-32de-3100-8be4-e31ef9760495";

    /// 主世界四个 region 文件的列数
    static constexpr i32 kColumnsR00 = 604;   // r.0.0.mca
    static constexpr i32 kColumnsRm10 = 800;  // r.-1.0.mca
    static constexpr i32 kColumnsR0m1 = 414;  // r.0.-1.mca
    static constexpr i32 kColumnsRm1m1 = 560; // r.-1.-1.mca
    static constexpr i32 kColumnsTotal = kColumnsR00 + kColumnsRm10 + kColumnsR0m1 + kColumnsRm1m1;

    /// entities/ 四个 region 文件的列数与实体数
    static constexpr i32 kEntityColumns = 97;
    static constexpr i32 kEntityCount = 205;

    /// poi/ 各文件列数（r.0.-1.mca 为 0 字节，列数 0）
    static constexpr i32 kPoiColumnsR00 = 23;
    static constexpr i32 kPoiColumnsRm10 = 4;
    static constexpr i32 kPoiColumnsRm1m1 = 3;

    /// 素材根下的世界目录名
    static constexpr const char* kWorldRelPath = "worlds/java-anvil-1.21.11";

    /**
     * @brief 真实存档的世界目录
     *
     * @return <testdata>/worlds/java-anvil-1.21.11
     */
    static std::filesystem::path worldDir() { return testDataPath(kWorldRelPath); }

    /**
     * @brief 打开该存档时应使用的格式信息
     *
     * 与 world::storage::SaveFormatDetector 对同一目录的检测结果一致（formatName/dataVersion/readonly）。
     *
     * @return Java Anvil 格式信息
     */
    static world::storage::SaveFormatInfo formatInfo()
    {
        world::storage::SaveFormatInfo info;
        info.format = world::storage::SaveFormat::JavaAnvil;
        info.formatName = "Java 1.21.11";
        info.dataVersion = kDataVersion;
        info.readonly = true;
        return info;
    }

    /**
     * @brief 取某个 region 文件中某一列的原始（已解压）NBT 字节流
     *
     * RegionFile 层与 Column 层测试共用：前者直接断言字节流，后者把它喂给
     * JavaColumnReader::readColumn。
     *
     * @param regionFileName 世界目录下 region/ 内的文件名，如 "r.0.0.mca"
     * @param localX region 内局部 X（0-31）
     * @param localZ region 内局部 Z（0-31）
     * @return 列 NBT 字节流，或打开/读取失败的 Error
     */
    static Result<std::vector<u8>> rawColumnNbt(const std::string& regionFileName, i32 localX, i32 localZ)
    {
        world::storage::reader::java::RegionFile region(worldDir() / "region" / regionFileName);
        auto openResult = region.open();
        if (openResult.failed()) {
            return openResult.error();
        }
        return region.readChunkData(localX, localZ);
    }

    /**
     * @brief 把列 NBT 字节流解析为根复合标签
     *
     * 现代（1.13+ 扁平化后）Java 存档的列数据无 Level 包装，根即区块复合标签本身。
     *
     * @param nbtData 已解压的 Java 大端 NBT 字节流
     * @return 根复合标签；解析失败返回 nullptr
     */
    static std::unique_ptr<nbt::tags::compound_tag> parseRoot(const std::vector<u8>& nbtData)
    {
        std::istringstream stream(std::string(nbtData.begin(), nbtData.end()));
        stream >> nbt::contexts::java;
        auto root = nbt::tags::compound_tag::read(stream);
        if (!root) {
            return nullptr;
        }
        // Java 写出的 NBT 根标签带 id+name 前缀，读取时会多出一层空键包装
        const nbt::tags::compound_tag& unwrapped = nbt::unwrapRootCompound(*root);
        if (&unwrapped != root.get()) {
            return std::make_unique<nbt::tags::compound_tag>(unwrapped);
        }
        return root;
    }
};

/**
 * @brief 真实存档素材的基础夹具
 *
 * 只校验素材存在，不构造任何读取器，因此不需要方块注册表。
 * 适用于 RegionFile 层、level.dat 层、格式检测层的测试。
 */
class JavaRealWorldFixture : public ::testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(testDataExists(JavaRealWorld::kWorldRelPath))
            << "测试素材缺失：" << JavaRealWorld::worldDir().string()
            << "（请检查是否被 .gitignore 规则或 sparse-checkout 过滤掉）";
        m_worldDir = JavaRealWorld::worldDir();
        ASSERT_TRUE(std::filesystem::exists(m_worldDir / "level.dat")) << "素材损坏：缺少 level.dat";
        ASSERT_TRUE(std::filesystem::exists(m_worldDir / "region")) << "素材损坏：缺少 region/ 目录";
    }

    std::filesystem::path m_worldDir;
};

/**
 * @brief 装配完整 Java 读取链的夹具
 *
 * 额外做两件事：
 * 1. 显式初始化方块注册表。JavaBlockStateMapper 未命中 BlockRegistry 时把方块映射为
 *    stateId 0（空气）并**永久缓存**该结果，因此必须在构造 mapper 之前初始化。
 *    tests/unit/main.cpp 的全局环境只在数据包目录存在时才初始化，不可依赖。
 * 2. 构造并打开 JavaWorldReader 与 JavaAnvilBackend，供各层测试直接取用。
 *
 * 两者都指向**同一个素材目录**，且后端是只读的，不会向素材写入任何内容。
 */
class JavaRealWorldReaderFixture : public JavaRealWorldFixture {
public:
    /// 已打开的真实存档世界级读取器
    [[nodiscard]] world::storage::reader::java::JavaWorldReader& worldReader() { return *m_worldReader; }

    /// 已打开的真实存档列级读取器（用于直接喂原始 NBT）
    [[nodiscard]] world::storage::reader::java::JavaColumnReader& columnReader() { return *m_columnReader; }

    /// 已打开的真实存档只读后端
    [[nodiscard]] world::storage::JavaAnvilBackend& backend() { return *m_backend; }

protected:
    void SetUp() override
    {
        JavaRealWorldFixture::SetUp();
        if (::testing::Test::HasFatalFailure()) {
            // 素材缺失，后续构造无意义
            return;
        }

        mc::VanillaBlocks::initialize();

        m_blockMapper = std::make_unique<world::storage::reader::java::JavaBlockStateMapper>();
        m_biomeMapper = std::make_unique<world::storage::reader::java::JavaBiomeMapper>();
        m_chunkReader = std::make_unique<world::storage::reader::java::JavaChunkReader>(*m_blockMapper, *m_biomeMapper);
        m_columnReader = std::make_unique<world::storage::reader::java::JavaColumnReader>(*m_chunkReader);
        m_worldReader = std::make_unique<world::storage::reader::java::JavaWorldReader>(*m_columnReader);

        auto openResult = m_worldReader->open(m_worldDir, JavaRealWorld::formatInfo());
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        m_backend = std::make_unique<world::storage::JavaAnvilBackend>();
        auto backendResult = m_backend->open(m_worldDir, JavaRealWorld::formatInfo());
        ASSERT_TRUE(backendResult.success()) << backendResult.error().message();
    }

    void TearDown() override
    {
        if (m_backend) {
            m_backend->close();
        }
        if (m_worldReader) {
            m_worldReader->close();
        }
    }

private:
    std::unique_ptr<world::storage::reader::java::JavaBlockStateMapper> m_blockMapper;
    std::unique_ptr<world::storage::reader::java::JavaBiomeMapper> m_biomeMapper;
    std::unique_ptr<world::storage::reader::java::JavaChunkReader> m_chunkReader;
    std::unique_ptr<world::storage::reader::java::JavaColumnReader> m_columnReader;
    std::unique_ptr<world::storage::reader::java::JavaWorldReader> m_worldReader;
    std::unique_ptr<world::storage::JavaAnvilBackend> m_backend;
};

} // namespace test
} // namespace mc
