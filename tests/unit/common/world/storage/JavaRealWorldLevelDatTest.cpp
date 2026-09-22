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

// JavaLevelDatReader 解析真实 level.dat 的测试。
//
// level.dat 是 gzip 压缩的大端 NBT，与区块数据完全无关，因此本套用例不涉及任何区块逻辑。
// 期望值来自对素材 level.dat 的独立解析（见 JavaRealWorldFixture.hpp 的说明）。

#include "common/world/storage/JavaRealWorldFixture.hpp"

#include "common/TempDirHelper.hpp"
#include "common/core/Types.hpp"
#include "server/world/storage/core/LevelDatCodec.hpp"
#include "server/world/storage/reader/java/JavaLevelDatReader.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace mc {
namespace {

using test::JavaRealWorld;
using test::JavaRealWorldFixture;
using world::storage::reader::java::JavaLevelDatReader;

// ============================================================================
// readSummary：世界列表所需的摘要信息
// ============================================================================

TEST_F(JavaRealWorldFixture, ReadSummaryVersionAndName)
{
    auto result = JavaLevelDatReader::readSummary(m_worldDir);
    ASSERT_TRUE(result.success()) << result.error().message();
    const auto& summary = result.value();

    // 素材由原版 1.21.11 生成，版本名会被加上 "Java " 前缀以区别于本项目写出的存档
    EXPECT_EQ(summary.version.dataVersion, JavaRealWorld::kDataVersion);
    EXPECT_EQ(summary.version.versionName, "Java 1.21.11");
    EXPECT_FALSE(summary.version.snapshot);
    EXPECT_EQ(summary.dataVersion, JavaRealWorld::kDataVersion);

    // 世界名是中文，验证 NBT 字符串按 UTF-8 解码
    EXPECT_EQ(summary.displayName, std::string(JavaRealWorld::kLevelNameUtf8));

    // 1.21.11 的 level.dat 已不再写 formatVersion 字段，读取器据此回落到 0。
    // 该断言用于钉住现状：若将来补充了 version 字段的解析，此处应一并更新。
    EXPECT_EQ(summary.storageVersion, 0);
}

TEST_F(JavaRealWorldFixture, ReadSummaryGameSettings)
{
    auto result = JavaLevelDatReader::readSummary(m_worldDir);
    ASSERT_TRUE(result.success()) << result.error().message();
    const auto& summary = result.value();

    EXPECT_EQ(summary.gameMode, GameMode::Creative);
    EXPECT_EQ(summary.difficulty, Difficulty::Normal);
    EXPECT_FALSE(summary.hardcore);
    EXPECT_TRUE(summary.allowCommands);

    // 种子取自 WorldGenSettings.seed（1.21 起已无 RandomSeed 字段），可能为负数
    EXPECT_EQ(summary.seed, static_cast<u64>(JavaRealWorld::kWorldGenSeed));

    // 素材最近一次游玩的时间戳
    EXPECT_EQ(summary.lastPlayedMs, 1790070726430LL);
}

// ============================================================================
// readRuntimeData：世界运行时状态
// ============================================================================

TEST_F(JavaRealWorldFixture, ReadRuntimeDataWeatherAndTime)
{
    auto result = JavaLevelDatReader::readRuntimeData(m_worldDir);
    ASSERT_TRUE(result.success()) << result.error().message();
    const auto& runtime = result.value();

    EXPECT_EQ(runtime.gameTime, 135);
    EXPECT_EQ(runtime.dayTime, 135);
    EXPECT_EQ(runtime.clearWeatherTime, 0);
    EXPECT_EQ(runtime.rainTime, 75714);
    EXPECT_FALSE(runtime.raining);
    EXPECT_EQ(runtime.thunderTime, 84248);
    EXPECT_FALSE(runtime.thundering);

    // 该世界已初始化过出生点，重开时不应重复计算
    EXPECT_TRUE(runtime.initialized);
    EXPECT_FALSE(runtime.difficultyLocked);
}

TEST_F(JavaRealWorldFixture, SpawnPointFallsBackToDefaults)
{
    auto result = JavaLevelDatReader::readRuntimeData(m_worldDir);
    ASSERT_TRUE(result.success()) << result.error().message();

    // 1.21.11 已把出生点从 Data.SpawnX/Y/Z 迁到 Data.spawn.pos（素材中为 [-48, 63, 64]），
    // 而 JavaLevelDatReader 目前只读旧字段，故这里拿到的是代码中的默认值。
    // 该断言有意钉住这一已知缺口：将来补齐 Data.spawn 解析后此用例会失败，
    // 提示把期望值更新为 -48 / 63 / 64。
    // TODO: 支持 1.21.11 的 Data.spawn 复合标签，并更新本用例期望值。
    EXPECT_EQ(result.value().spawnX, 0);
    EXPECT_EQ(result.value().spawnY, 64);
    EXPECT_EQ(result.value().spawnZ, 0);
}

// ============================================================================
// readLocalPlayer：level.dat 内嵌的本地玩家
// ============================================================================

TEST_F(JavaRealWorldFixture, ReadLocalPlayerFromLevelDat)
{
    auto result = JavaLevelDatReader::readLocalPlayer(m_worldDir);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());
    const auto& player = result.value().value();

    EXPECT_DOUBLE_EQ(player.posX, -51.5);
    EXPECT_DOUBLE_EQ(player.posY, 66.0);
    EXPECT_DOUBLE_EQ(player.posZ, 70.5);
    EXPECT_FLOAT_EQ(player.health, 20.0f);
    EXPECT_EQ(player.experienceLevel, 0);
    EXPECT_EQ(player.totalExperience, 0);
    EXPECT_FLOAT_EQ(player.experienceProgress, 0.0f);
    EXPECT_EQ(player.foodLevel, 20);
    EXPECT_EQ(player.airSupply, 300);
    EXPECT_EQ(player.gameMode, GameMode::Creative);

    // level.dat 里的 UUID 是整型数组、且没有 Name 字段，读取器按字符串读取均落空，
    // 于是回落到本地玩家的约定标识
    EXPECT_EQ(player.uuid, "~local_player");
    EXPECT_EQ(player.username, "~local_player");

    // Dimension 是字符串形式的资源位置，不是整型维度 ID，读取器回落为主世界
    EXPECT_EQ(player.dimension, 0);

    // 背包槽位固定预分配 41 个（0-8 快捷栏、9-35 主背包、36-39 护甲、40 副手）
    EXPECT_EQ(player.inventoryItems.size(), 41u);
}

// ============================================================================
// 失败路径
// ============================================================================

TEST_F(JavaRealWorldFixture, MissingLevelDatFails)
{
    const auto emptyDir = test::makeUniqueTestDir("mc_java_real_world_leveldat");
    ASSERT_TRUE(std::filesystem::exists(emptyDir));

    EXPECT_TRUE(JavaLevelDatReader::readSummary(emptyDir).failed());
    EXPECT_TRUE(JavaLevelDatReader::readRuntimeData(emptyDir).failed());
    EXPECT_TRUE(JavaLevelDatReader::readLocalPlayer(emptyDir).failed());

    test::removeTestDir(emptyDir);
}

} // namespace
} // namespace mc
