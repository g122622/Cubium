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

// world::storage::JavaAnvilBackend 读取真实存档的对外契约测试。
//
// 本套用例是唯一"整体装配"的层次，验证的是后端契约而非解码细节：
//   - 外来格式后端必须只读，且这一约束要有行为层面的证据（不落盘），而不只是 isReadonly() 的声明
//   - 格式信息由门面层检测后传入，后端不重复检测
//   - 未打开时的错误语义
//   - 本地玩家与按 UUID 读取玩家这两条路径
//
// 测试全程不实例化 SingleLevelStorageManager / GlobalStorageManager：那类门面会创建
// session.lock 与 db/ 目录，向素材目录写入内容。

#include "common/world/storage/JavaRealWorldFixture.hpp"

#include "common/core/Types.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "server/world/storage/core/SaveFormat.hpp"
#include "server/world/storage/player/PlayerSaveData.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace {

using test::JavaRealWorld;
using test::JavaRealWorldReaderFixture;

/// 目录递归快照：相对路径 → (文件大小, 最后写入时间)
using DirSnapshot = std::map<std::string, std::pair<std::uintmax_t, std::filesystem::file_time_type>>;

/**
 * @brief 对目录做递归快照
 *
 * 刻意只记录文件大小与最后写入时间，不记录访问时间：读取文件本身就会更新 atime，
 * 把它纳入比较会让"只读"断言必然失败。
 */
DirSnapshot snapshotDirectory(const std::filesystem::path& root)
{
    DirSnapshot snapshot;
    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto relative = std::filesystem::relative(entry.path(), root, ec).generic_string();
        snapshot[relative] = {entry.file_size(), entry.last_write_time()};
    }
    return snapshot;
}

// ============================================================================
// 只读性与格式标识
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, IsReadonlyAndFormatIdentity)
{
    EXPECT_TRUE(backend().isReadonly());
    EXPECT_TRUE(backend().isOpen());
    EXPECT_EQ(backend().format(), world::storage::SaveFormat::JavaAnvil);
    EXPECT_EQ(backend().worldPath(), m_worldDir);
    EXPECT_EQ(backend().formatInfo().format, world::storage::SaveFormat::JavaAnvil);
    EXPECT_EQ(backend().formatInfo().dataVersion, JavaRealWorld::kDataVersion);
    EXPECT_TRUE(backend().formatInfo().readonly);
}

TEST_F(JavaRealWorldReaderFixture, OpenRejectsNonJavaFormatInfo)
{
    world::storage::JavaAnvilBackend backend;

    world::storage::SaveFormatInfo nativeInfo;
    nativeInfo.format = world::storage::SaveFormat::Native;
    nativeInfo.formatName = "Native";
    nativeInfo.readonly = false;

    auto result = backend.open(m_worldDir, nativeInfo);
    ASSERT_TRUE(result.failed());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidState);
    EXPECT_FALSE(backend.isOpen());

    // 传入正确的格式信息后应能正常打开
    ASSERT_TRUE(backend.open(m_worldDir, JavaRealWorld::formatInfo()).success());
    EXPECT_TRUE(backend.isOpen());
    backend.close();
}

TEST_F(JavaRealWorldReaderFixture, OpenConsumesProvidedFormatInfo)
{
    // 后端的格式信息应原样来自外部检测结果（这里用真实检测器产出），而不是自行推断
    auto detected = world::storage::SaveFormatDetector::detect(m_worldDir);
    ASSERT_TRUE(detected.success()) << detected.error().message();

    world::storage::JavaAnvilBackend backend;
    auto result = backend.open(m_worldDir, detected.value());
    ASSERT_TRUE(result.success()) << result.error().message();

    EXPECT_EQ(backend.formatInfo().formatName, detected.value().formatName);
    EXPECT_EQ(backend.formatInfo().dataVersion, detected.value().dataVersion);
    EXPECT_EQ(backend.formatInfo().readonly, detected.value().readonly);
    backend.close();
}

TEST_F(JavaRealWorldReaderFixture, OperationsFailBeforeOpen)
{
    world::storage::JavaAnvilBackend backend;
    ASSERT_FALSE(backend.isOpen());

    EXPECT_EQ(backend.loadChunk(8, 3, 0).error().code(), ErrorCode::InvalidState);
    EXPECT_EQ(backend.listChunks(0).error().code(), ErrorCode::InvalidState);
    EXPECT_EQ(backend.loadPlayer("~local_player").error().code(), ErrorCode::InvalidState);
    EXPECT_EQ(backend.listPlayerUuids().error().code(), ErrorCode::InvalidState);
    EXPECT_EQ(backend.loadLevelData().error().code(), ErrorCode::InvalidState);

    // 未打开时 close 必须安全且幂等
    backend.close();
    backend.close();
    EXPECT_FALSE(backend.isOpen());
}

// ============================================================================
// 世界元数据
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, LoadLevelDataMatchesReader)
{
    auto result = backend().loadLevelData();
    ASSERT_TRUE(result.success()) << result.error().message();
    const auto& runtime = result.value();

    EXPECT_EQ(runtime.summary.displayName, std::string(JavaRealWorld::kLevelNameUtf8));
    EXPECT_EQ(runtime.summary.version.dataVersion, JavaRealWorld::kDataVersion);
    EXPECT_EQ(runtime.summary.version.versionName, "Java 1.21.11");
    EXPECT_EQ(runtime.summary.gameMode, GameMode::Creative);
    EXPECT_EQ(runtime.summary.difficulty, Difficulty::Normal);
    EXPECT_FALSE(runtime.summary.hardcore);
    EXPECT_TRUE(runtime.summary.allowCommands);
    EXPECT_EQ(runtime.summary.seed, static_cast<u64>(JavaRealWorld::kWorldGenSeed));

    EXPECT_EQ(runtime.gameTime, 135);
    EXPECT_EQ(runtime.dayTime, 135);
    EXPECT_EQ(runtime.rainTime, 75714);
    EXPECT_FALSE(runtime.raining);
    EXPECT_FALSE(runtime.thundering);
    EXPECT_TRUE(runtime.initialized);
}

// ============================================================================
// 玩家数据
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, LoadLocalPlayerViaLevelDat)
{
    // "~local_player" 是本地玩家的约定标识，应从 level.dat 的 Data.Player 读取，
    // 而不是去拼 playerdata/~local_player.dat
    auto byConvention = backend().loadPlayer("~local_player");
    ASSERT_TRUE(byConvention.success()) << byConvention.error().message();
    ASSERT_TRUE(byConvention.value().has_value());

    // 空字符串同样归入本地玩家路径
    auto byEmpty = backend().loadPlayer("");
    ASSERT_TRUE(byEmpty.success()) << byEmpty.error().message();
    ASSERT_TRUE(byEmpty.value().has_value());

    const auto& player = byConvention.value().value();
    EXPECT_EQ(player.uuid, "~local_player");
    EXPECT_DOUBLE_EQ(player.posX, -51.5);
    EXPECT_DOUBLE_EQ(player.posY, 66.0);
    EXPECT_DOUBLE_EQ(player.posZ, 70.5);
    EXPECT_FLOAT_EQ(player.health, 20.0f);
    EXPECT_EQ(player.gameMode, GameMode::Creative);

    // 两条路径读到的应是同一份数据
    EXPECT_EQ(byEmpty.value().value().posX, player.posX);
    EXPECT_EQ(byEmpty.value().value().uuid, player.uuid);
}

TEST_F(JavaRealWorldReaderFixture, LoadPlayerByUuidFromPlayerdata)
{
    auto result = backend().loadPlayer(JavaRealWorld::kPlayerUuid);
    ASSERT_TRUE(result.success()) << result.error().message();
    ASSERT_TRUE(result.value().has_value());
    const auto& player = result.value().value();

    // playerdata 文件内的 UUID 是整型数组，PlayerSaveData::fromNbt 读不出字符串，
    // 后端会用请求参数补齐
    EXPECT_EQ(player.uuid, JavaRealWorld::kPlayerUuid);
    EXPECT_DOUBLE_EQ(player.posX, -51.5);
    EXPECT_DOUBLE_EQ(player.posY, 66.0);
    EXPECT_DOUBLE_EQ(player.posZ, 70.5);
    EXPECT_FLOAT_EQ(player.health, 20.0f);
    EXPECT_EQ(player.experienceLevel, 0);
    EXPECT_EQ(player.foodLevel, 20);
    EXPECT_EQ(player.airSupply, 300);
    EXPECT_EQ(player.gameMode, GameMode::Creative);
}

TEST_F(JavaRealWorldReaderFixture, LoadUnknownPlayerReturnsEmpty)
{
    // 未知玩家应表现为"没有这份数据"，而不是错误
    auto result = backend().loadPlayer("00000000-0000-0000-0000-000000000000");
    ASSERT_TRUE(result.success()) << result.error().message();
    EXPECT_FALSE(result.value().has_value());
}

TEST_F(JavaRealWorldReaderFixture, ListPlayerUuidsContentAndOrder)
{
    auto result = backend().listPlayerUuids();
    ASSERT_TRUE(result.success()) << result.error().message();
    const auto& uuids = result.value();

    // 实现先放入 "~local_player" 再扫描 playerdata/，但目录遍历顺序不保证，
    // 因此只断言集合内容，不按下标断言
    EXPECT_EQ(uuids.size(), 2u);
    EXPECT_NE(std::find(uuids.begin(), uuids.end(), "~local_player"), uuids.end());
    EXPECT_NE(std::find(uuids.begin(), uuids.end(), JavaRealWorld::kPlayerUuid), uuids.end());

    // playerdata/ 下的 *.dat 才被收录
    for (const auto& uuid : uuids) {
        EXPECT_EQ(uuid.find(".dat"), std::string::npos) << uuid;
    }
}

// ============================================================================
// 区块读取
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, LoadChunkThroughBackend)
{
    auto visited = backend().loadChunk(-4, 7, 0);
    ASSERT_TRUE(visited.success()) << visited.error().message();
    ASSERT_TRUE(visited.value().has_value());
    EXPECT_EQ(visited.value().value().blockEntityCount(), 4u);
    EXPECT_TRUE(visited.value().value().hasLoadedEntityNbt());

    auto trialChamber = backend().loadChunk(8, 3, 0);
    ASSERT_TRUE(trialChamber.success()) << trialChamber.error().message();
    ASSERT_TRUE(trialChamber.value().has_value());
    EXPECT_EQ(trialChamber.value().value().blockEntityCount(), 19u);
    EXPECT_FALSE(trialChamber.value().value().hasLoadedEntityNbt());

    // 未完成生成的列经后端读取同样应表现为"无此区块"
    auto unfinished = backend().loadChunk(-13, 19, 0);
    ASSERT_TRUE(unfinished.success()) << unfinished.error().message();
    EXPECT_FALSE(unfinished.value().has_value());
}

TEST_F(JavaRealWorldReaderFixture, ListChunksThroughBackend)
{
    auto overworld = backend().listChunks(0);
    ASSERT_TRUE(overworld.success()) << overworld.error().message();
    EXPECT_EQ(overworld.value().size(), static_cast<size_t>(JavaRealWorld::kColumnsTotal));

    for (const DimensionId dimension : {static_cast<DimensionId>(-1), static_cast<DimensionId>(1)}) {
        auto result = backend().listChunks(dimension);
        ASSERT_TRUE(result.success()) << "dimension " << dimension;
        EXPECT_TRUE(result.value().empty()) << "dimension " << dimension;
    }
}

// ============================================================================
// 只读的行为证据：读取过程不得改动存档文件
// ============================================================================

TEST_F(JavaRealWorldReaderFixture, ReadsDoNotModifyWorldFilesOnDisk)
{
    const DirSnapshot before = snapshotDirectory(m_worldDir);
    ASSERT_FALSE(before.empty());

    // 跑一轮覆盖各读取路径的操作
    EXPECT_TRUE(backend().listChunks(0).success());
    EXPECT_TRUE(backend().loadChunk(8, 3, 0).success());
    EXPECT_TRUE(backend().loadChunk(-4, 7, 0).success());
    EXPECT_TRUE(backend().loadLevelData().success());
    EXPECT_TRUE(backend().loadPlayer("~local_player").success());
    EXPECT_TRUE(backend().loadPlayer(JavaRealWorld::kPlayerUuid).success());
    EXPECT_TRUE(backend().listPlayerUuids().success());

    const DirSnapshot after = snapshotDirectory(m_worldDir);

    ASSERT_EQ(before.size(), after.size()) << "读取过程改变了存档目录中的文件数量";
    for (const auto& [relative, state] : before) {
        const auto it = after.find(relative);
        ASSERT_NE(it, after.end()) << "读取过程删除了文件：" << relative;
        EXPECT_EQ(it->second.first, state.first) << "文件大小被改动：" << relative;
        EXPECT_EQ(it->second.second, state.second) << "文件内容被改动：" << relative;
    }
}

} // namespace
} // namespace mc
