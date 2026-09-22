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

// world::storage::SaveFormatDetector 对真实外来 Java 存档的判定测试。
//
// 这是"打开世界"的前置门：判定错了会选错后端，而更严重的后果是只读性判错——
// 把外来存档当成可写格式会污染原存档，把本项目新世界当成只读格式会让数据静默不落盘。
//
// 素材本身的目录结构里就有 region/，因此对素材根调用 detect 只会命中"优先级 1"分支。
// 为了覆盖到"由 level.dat 判定 + 是否本项目写出"这条守卫，另有两个只放 level.dat
// 的临时目录用例。

#include "common/world/storage/JavaRealWorldFixture.hpp"

#include "common/TempDirHelper.hpp"
#include "common/util/CompressionUtils.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "server/world/storage/core/SaveFormat.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace mc {
namespace {

using test::JavaRealWorld;
using test::JavaRealWorldFixture;

/**
 * @brief 在指定目录写出一份最小的 Java 版 level.dat
 *
 * @param dir 目标目录
 * @param versionName Data.Version.Name 的取值（"Cubium" 表示本项目写出的世界）
 */
void writeMinimalLevelDat(const std::filesystem::path& dir, const std::string& versionName)
{
    nbt::tags::compound_tag root(true);

    auto version = std::make_unique<nbt::tags::compound_tag>();
    version->value["Name"] = std::make_unique<nbt::tags::string_tag>(versionName);
    version->value["Id"] = std::make_unique<nbt::tags::int_tag>(1);
    version->value["Snapshot"] = std::make_unique<nbt::tags::byte_tag>(0);

    auto data = std::make_unique<nbt::tags::compound_tag>();
    data->value["Version"] = std::move(version);
    data->value["DataVersion"] = std::make_unique<nbt::tags::int_tag>(1);
    data->value["LevelName"] = std::make_unique<nbt::tags::string_tag>("Test");
    root.value["Data"] = std::move(data);

    std::ostringstream stream(std::ios::binary);
    stream << nbt::contexts::java;
    root.write(stream);
    const std::string serialized = stream.str();

    const std::vector<u8> compressed = util::compressGzip(std::vector<u8>(serialized.begin(), serialized.end()));

    std::ofstream out(dir / "level.dat", std::ios::binary);
    out.write(reinterpret_cast<const char*>(compressed.data()), static_cast<std::streamsize>(compressed.size()));
}

/**
 * @brief 读回 level.dat 并取出 Data.Version.Name，用于确认 fixture 造出的素材确实有效
 *
 * 缺少这一步，构造失败会被 world::storage::SaveFormatDetector 的"默认按 Native 处理"兜底掩盖，
 * 使本文件的用例变成永真的空测试。
 */
std::string readLevelDatVersionName(const std::filesystem::path& dir)
{
    std::ifstream in(dir / "level.dat", std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    const std::vector<u8> compressed((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    const std::vector<u8> decompressed = util::decompressGzip(compressed);
    if (decompressed.empty()) {
        return {};
    }
    std::istringstream stream(std::string(decompressed.begin(), decompressed.end()));
    stream >> nbt::contexts::java;
    auto root = nbt::tags::compound_tag::read(stream);
    if (!root) {
        return {};
    }
    // Java 写出的 level.dat 根标签带 id+name 前缀，读取时会多出一层空键包装
    const auto& levelData = nbt::unwrapRootCompound(*root);
    if (levelData.value.count("Data") == 0) {
        return {};
    }
    // get<compound_tag> 返回的是复合标签内部的键值映射，而非标签本身
    const auto& data = levelData.get<nbt::tags::compound_tag>("Data");
    if (data.count("Version") == 0) {
        return {};
    }
    const auto* version = dynamic_cast<const nbt::tags::compound_tag*>(data.at("Version").get());
    if (version == nullptr || version->value.count("Name") == 0) {
        return {};
    }
    return version->get<nbt::tags::string_tag>("Name");
}

// ============================================================================
// 真实存档
// ============================================================================

TEST_F(JavaRealWorldFixture, DetectsRealJavaWorldAsReadonlyAnvil)
{
    auto result = world::storage::SaveFormatDetector::detect(m_worldDir);
    ASSERT_TRUE(result.success()) << result.error().message();
    const auto& info = result.value();

    EXPECT_EQ(info.format, world::storage::SaveFormat::JavaAnvil);
    EXPECT_TRUE(info.readonly) << "外来 Java 存档必须以只读方式打开";
    EXPECT_EQ(info.formatName, "Java 1.21.11");
    EXPECT_EQ(info.dataVersion, JavaRealWorld::kDataVersion);
}

TEST_F(JavaRealWorldFixture, NotMisdetectedAsNativeOrBedrock)
{
    auto result = world::storage::SaveFormatDetector::detect(m_worldDir);
    ASSERT_TRUE(result.success()) << result.error().message();

    // 存档内没有 db/ 目录，存在 entities/ 与 poi/ 也不应影响判定
    EXPECT_NE(result.value().format, world::storage::SaveFormat::Native);
    EXPECT_NE(result.value().format, world::storage::SaveFormat::BedrockLDB);
    EXPECT_EQ(result.value().format, world::storage::SaveFormat::JavaAnvil);
}

// ============================================================================
// 仅凭 level.dat 判定：外来存档必须保持只读
// ============================================================================

TEST_F(JavaRealWorldFixture, ForeignLevelDatWithoutRegionIsNotCubiumAuthored)
{
    // 复制真实存档的 level.dat 到只含该文件的目录：既没有 region/ 也没有 db/，
    // 检测只能依赖 level.dat 内容。Version.Name 为 "1.21.11"，必须被判为外来 Java 存档。
    const auto tempDir = test::makeUniqueTestDir("mc_java_real_world_detect_foreign");
    std::filesystem::copy_file(
        m_worldDir / "level.dat", tempDir / "level.dat", std::filesystem::copy_options::overwrite_existing);

    ASSERT_EQ(readLevelDatVersionName(tempDir), std::string(JavaRealWorld::kVersionName));

    auto result = world::storage::SaveFormatDetector::detect(tempDir);
    ASSERT_TRUE(result.success()) << result.error().message();

    EXPECT_NE(result.value().format, world::storage::SaveFormat::Native)
        << "外来 Java 存档不得被当作可写的 Native 格式";
    EXPECT_EQ(result.value().format, world::storage::SaveFormat::JavaAnvil);
    EXPECT_TRUE(result.value().readonly);
    EXPECT_EQ(result.value().dataVersion, JavaRealWorld::kDataVersion);

    test::removeTestDir(tempDir);
}

// ============================================================================
// 仅凭 level.dat 判定：本项目写出的新世界必须保持可写
// ============================================================================

TEST_F(JavaRealWorldFixture, CubiumAuthoredLevelDatWithoutRegionStaysWritable)
{
    // 本项目新建、尚未首次落盘的世界同样没有 region/ 与 db/，但其 level.dat 的
    // Version.Name 为 "Cubium"。若被误判为只读的外来格式，这个世界将什么都不落盘。
    const auto tempDir = test::makeUniqueTestDir("mc_java_real_world_detect_cubium");
    writeMinimalLevelDat(tempDir, "Cubium");

    // 先确认造出的 level.dat 确实可被解析且带上了预期标记；
    // 否则检测会落到"默认按 Native 处理"的兜底分支，使本用例变成永真断言。
    ASSERT_EQ(readLevelDatVersionName(tempDir), "Cubium");

    auto result = world::storage::SaveFormatDetector::detect(tempDir);
    ASSERT_TRUE(result.success()) << result.error().message();

    EXPECT_EQ(result.value().format, world::storage::SaveFormat::Native);
    EXPECT_FALSE(result.value().readonly) << "本项目新建的世界必须保持可写";
    EXPECT_EQ(result.value().formatName, "Native");

    test::removeTestDir(tempDir);
}

// ============================================================================
// 失败路径
// ============================================================================

TEST_F(JavaRealWorldFixture, MissingDirectoryFails)
{
    auto result = world::storage::SaveFormatDetector::detect(m_worldDir / "__not_a_world__");
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::WorldNotFound);
}

} // namespace
} // namespace mc
