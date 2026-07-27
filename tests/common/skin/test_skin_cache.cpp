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

#include "common/TempDirHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/manager/SkinCache.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include "common/util/TimeUtils.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <gtest/gtest.h>

using namespace mc::skin;

class SkinCacheTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // PID + 纳秒时间戳保证 CTest -j16 跨进程唯一，避免同秒 token 碰撞
        testDir_ = mc::test::makeUniqueTestDir("mc_skin_cache_test").string();
        cache_ = std::make_unique<SkinCache>(testDir_);
    }

    void TearDown() override
    {
        cache_.reset();

        mc::test::removeTestDir(std::filesystem::path(testDir_));
    }

    std::string testDir_;
    std::unique_ptr<SkinCache> cache_;
};

TEST_F(SkinCacheTest, Initialize)
{
    auto result = cache_->initialize();
    EXPECT_TRUE(result.success());
}

TEST_F(SkinCacheTest, SaveAndReadSkin)
{
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    std::string hash = "abc123def456";
    std::vector<mc::u8> testData(64 * 64 * 4, 0xAB); // 64x64 RGBA

    // 保存
    auto saveResult = cache_->saveSkin(hash, testData);
    EXPECT_TRUE(saveResult.success());

    // 检查存在
    EXPECT_TRUE(cache_->hasSkin(hash));

    // 读取
    auto readResult = cache_->readSkin(hash);
    EXPECT_TRUE(readResult.success());
    EXPECT_EQ(testData.size(), readResult.value().size());
    EXPECT_EQ(0xAB, readResult.value()[0]);

    // 清理
    EXPECT_TRUE(cache_->removeSkin(hash));
    EXPECT_FALSE(cache_->hasSkin(hash));
}

TEST_F(SkinCacheTest, GenerateSkinLocation)
{
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    std::string hash = "abc123def456";
    mc::ResourceLocation location = cache_->generateSkinLocation(hash);

    EXPECT_EQ("minecraft:skins/ab/abc123def456", location.toString());
}

TEST_F(SkinCacheTest, CacheCount)
{
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    EXPECT_EQ(0u, cache_->cacheCount());

    std::vector<mc::u8> testData(64 * 64 * 4, 0);

    cache_->saveSkin("hash1", testData);
    EXPECT_EQ(1u, cache_->cacheCount());

    cache_->saveSkin("hash2", testData);
    EXPECT_EQ(2u, cache_->cacheCount());

    cache_->removeSkin("hash1");
    EXPECT_EQ(1u, cache_->cacheCount());
}

TEST_F(SkinCacheTest, ClearAll)
{
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    std::vector<mc::u8> testData(64 * 64 * 4, 0);

    cache_->saveSkin("hash1", testData);
    cache_->saveSkin("hash2", testData);
    EXPECT_GT(cache_->cacheCount(), 0u);

    cache_->clearAll();
    EXPECT_EQ(0u, cache_->cacheCount());
}

TEST_F(SkinCacheTest, MetadataPersistence)
{
    // 初始化并保存皮肤数据
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    std::string skinHash = "sha1_abcdef1234567890";
    std::string capeHash = "sha1_cape0987654321ab";
    std::vector<mc::u8> skinData(1024, 0xCD);
    std::vector<mc::u8> capeData(512, 0xEF);

    cache_->saveSkin(skinHash, skinData);
    cache_->saveCape(capeHash, capeData);
    EXPECT_EQ(2u, cache_->cacheCount());

    // 关闭缓存（会保存元数据）
    cache_->shutdown();
    cache_.reset();

    // 重新创建缓存实例并初始化（会从 metadata.json 加载）
    cache_ = std::make_unique<SkinCache>(testDir_);
    auto reinitResult = cache_->initialize();
    ASSERT_TRUE(reinitResult.success());

    // 验证缓存条目数量恢复
    EXPECT_EQ(2u, cache_->cacheCount());

    // 验证皮肤数据可读取
    EXPECT_TRUE(cache_->hasSkin(skinHash));
    auto readSkin = cache_->readSkin(skinHash);
    EXPECT_TRUE(readSkin.success());
    EXPECT_EQ(skinData.size(), readSkin.value().size());

    // 验证披风数据可读取
    EXPECT_TRUE(cache_->hasCape(capeHash));
    auto readCape = cache_->readCape(capeHash);
    EXPECT_TRUE(readCape.success());
    EXPECT_EQ(capeData.size(), readCape.value().size());
}

TEST_F(SkinCacheTest, MetadataPersistenceWithEmptyCache)
{
    // 初始化空缓存
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());
    EXPECT_EQ(0u, cache_->cacheCount());

    // 关闭并重新打开
    cache_->shutdown();
    cache_.reset();

    cache_ = std::make_unique<SkinCache>(testDir_);
    auto reinitResult = cache_->initialize();
    ASSERT_TRUE(reinitResult.success());
    EXPECT_EQ(0u, cache_->cacheCount());
}

TEST_F(SkinCacheTest, MetadataPersistenceWithCorruptJson)
{
    // 初始化并添加一个皮肤
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    cache_->saveSkin("hash1", std::vector<mc::u8>(100, 0xAA));
    cache_->shutdown();
    cache_.reset();

    // 破坏 metadata.json 文件
    std::string metadataPath = testDir_ + "/metadata.json";
    {
        std::ofstream file(metadataPath, std::ios::trunc);
        file << "{ this is not valid JSON }}}";
    }

    // 重新初始化，应能从损坏的元数据中恢复（不崩溃）
    cache_ = std::make_unique<SkinCache>(testDir_);
    auto reinitResult = cache_->initialize();
    ASSERT_TRUE(reinitResult.success());

    // 文件仍然存在于磁盘，扫描应能恢复
    EXPECT_TRUE(cache_->hasSkin("hash1"));
}

TEST_F(SkinCacheTest, CleanExpiredRemovesEntry)
{
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    // 保存一个皮肤
    cache_->saveSkin("expired_hash", std::vector<mc::u8>(100, 0xBB));
    EXPECT_TRUE(cache_->hasSkin("expired_hash"));

    // 清理超过 0 秒的缓存（立即过期）
    cache_->cleanExpired(std::chrono::seconds(0));

    // 皮肤应被清理
    EXPECT_FALSE(cache_->hasSkin("expired_hash"));
    EXPECT_EQ(0u, cache_->cacheCount());
}

TEST_F(SkinCacheTest, MetadataTimestampRoundTrip)
{
    // 验证时间戳在序列化/反序列化后保持正确
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    std::string skinHash = "timestamp_test_hash";
    std::vector<mc::u8> skinData(256, 0x42);
    auto saveResult = cache_->saveSkin(skinHash, skinData);
    ASSERT_TRUE(saveResult.success());

    // 关闭缓存（触发元数据保存）
    cache_->shutdown();
    cache_.reset();

    // 重新打开缓存（从 metadata.json 加载）
    cache_ = std::make_unique<SkinCache>(testDir_);
    auto reinitResult = cache_->initialize();
    ASSERT_TRUE(reinitResult.success());

    // 皮肤数据应该仍然可用
    EXPECT_TRUE(cache_->hasSkin(skinHash));
    auto readResult = cache_->readSkin(skinHash);
    EXPECT_TRUE(readResult.success());
    EXPECT_EQ(skinData.size(), readResult.value().size());

    // 清理过期缓存应该不会移除刚加载的条目（它们刚被访问）
    cache_->cleanExpired(std::chrono::hours(1));
    EXPECT_TRUE(cache_->hasSkin(skinHash));
}

TEST_F(SkinCacheTest, CleanExpiredPreservesRecentEntry)
{
    auto initResult = cache_->initialize();
    ASSERT_TRUE(initResult.success());

    std::string skinHash = "recent_hash";
    std::vector<mc::u8> skinData(100, 0x55);
    cache_->saveSkin(skinHash, skinData);

    // 清理超过 1 小时的缓存——刚保存的条目不应被清理
    cache_->cleanExpired(std::chrono::hours(1));
    EXPECT_TRUE(cache_->hasSkin(skinHash));
    EXPECT_EQ(1u, cache_->cacheCount());
}

// ============================================================================
// TimeUtils 跨平台时间戳转换测试
// ============================================================================

TEST(TimeUtilsFileTimeTest, FileTimeToUnixSecondsRoundTrip)
{
    // 使用当前时间进行往返测试
    auto now = std::filesystem::file_time_type::clock::now();
    auto unixSeconds = mc::util::TimeUtils::fileTimeToUnixSeconds(now);
    auto restored = mc::util::TimeUtils::unixSecondsToFileTime(unixSeconds);

    // 由于秒级精度截断，往返后的时间与原始时间差应在 1 秒以内
    auto diff = std::chrono::duration_cast<std::chrono::seconds>((now > restored) ? (now - restored) : (restored - now))
                    .count();
    EXPECT_LE(diff, 1);
}

TEST(TimeUtilsFileTimeTest, FileTimeToUnixSecondsKnownEpoch)
{
    // Unix 纪元（1970-01-01 00:00:00 UTC）对应的 file_time_type 转换后应为 0
    auto fileTimeAtEpoch = mc::util::TimeUtils::unixSecondsToFileTime(0);
    auto seconds = mc::util::TimeUtils::fileTimeToUnixSeconds(fileTimeAtEpoch);
    EXPECT_EQ(0, seconds);
}

TEST(TimeUtilsFileTimeTest, UnixSecondsToFileTimeKnownValue)
{
    // 2024-01-01 00:00:00 UTC ≈ 1704067200 秒
    constexpr mc::i64 testTimestamp = 1704067200;
    auto fileTime = mc::util::TimeUtils::unixSecondsToFileTime(testTimestamp);
    auto backToSeconds = mc::util::TimeUtils::fileTimeToUnixSeconds(fileTime);
    EXPECT_EQ(testTimestamp, backToSeconds);
}

// ============================================================================
// PlayerSkinInfo 测试
// ============================================================================

class PlayerSkinInfoTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        uuid_ = GameProfile::parseUUID("550e8400-e29b-41d4-a716-446655440000");
        profile_ = std::make_unique<GameProfile>(uuid_, "TestPlayer");
        info_ = std::make_unique<PlayerSkinInfo>(*profile_);
    }

    std::array<mc::u8, 16> uuid_;
    std::unique_ptr<GameProfile> profile_;
    std::unique_ptr<PlayerSkinInfo> info_;
};

TEST_F(PlayerSkinInfoTest, BasicProperties)
{
    EXPECT_EQ("TestPlayer", info_->name());

    const auto& infoUuid = info_->uuid();
    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(uuid_[i], infoUuid[i]) << "UUID mismatch at index " << i;
    }
}

TEST_F(PlayerSkinInfoTest, LoadState)
{
    EXPECT_EQ(SkinLoadState::NotLoaded, info_->loadState());

    info_->setLoadState(SkinLoadState::Loading);
    EXPECT_EQ(SkinLoadState::Loading, info_->loadState());
    EXPECT_TRUE(info_->isLoading());

    info_->setLoadState(SkinLoadState::Loaded);
    EXPECT_EQ(SkinLoadState::Loaded, info_->loadState());
    EXPECT_TRUE(info_->isLoaded());

    info_->setLoadState(SkinLoadState::Failed);
    EXPECT_TRUE(info_->isFailed());

    info_->setLoadState(SkinLoadState::UsingDefault);
    EXPECT_TRUE(info_->isUsingDefault());
}

TEST_F(PlayerSkinInfoTest, SkinTextures)
{
    mc::ResourceLocation skinLoc("minecraft:skins/test");
    info_->setSkinLocation(skinLoc);

    EXPECT_TRUE(info_->getSkinLocation() == skinLoc);

    mc::ResourceLocation capeLoc("minecraft:capes/test");
    info_->setCapeLocation(capeLoc);

    auto cape = info_->getCapeLocation();
    EXPECT_TRUE(cape.has_value());
    EXPECT_EQ(capeLoc, cape.value());
}

TEST_F(PlayerSkinInfoTest, SkinType)
{
    // 默认根据 UUID 确定
    SkinType type = info_->getSkinType();
    EXPECT_TRUE(type == SkinType::Default || type == SkinType::Slim);

    // 设置皮肤纹理后，使用纹理的类型。
    // 注意：hasSkin() 以皮肤 URL 为存在判定依据（SkinMetadataParser 解析后设置 URL，
    // ResourceLocation 由 SkinManager 在缓存/下载完成后再填充），因此这里必须
    // 设置 skinUrl 才能让 getSkinType() 走"已加载皮肤"分支返回纹理自带的类型。
    SkinTextures textures;
    textures.setSkinUrl("http://textures.minecraft.net/texture/slimtest");
    textures.setSkinType(SkinType::Slim);
    info_->setSkinTextures(textures);

    EXPECT_EQ(SkinType::Slim, info_->getSkinType());
}

TEST_F(PlayerSkinInfoTest, ModelParts)
{
    mc::u8 parts = info_->modelParts();
    EXPECT_EQ(0x7F, parts); // 默认显示所有部件（除披风外）

    // 设置特定部件
    info_->setModelPartEnabled(mc::PlayerModelPart::Cape, true);
    EXPECT_TRUE(info_->isWearing(mc::PlayerModelPart::Cape));

    info_->setModelPartEnabled(mc::PlayerModelPart::Cape, false);
    EXPECT_FALSE(info_->isWearing(mc::PlayerModelPart::Cape));

    // 批量设置
    info_->setModelParts(0x00);
    EXPECT_EQ(0x00, info_->modelParts());
}

TEST_F(PlayerSkinInfoTest, DefaultSkinLocation)
{
    mc::ResourceLocation defaultSkin = info_->getDefaultSkinLocation();
    // MC 1.21.1 有 18 种默认皮肤（9 slim + 9 wide），9 个名称：
    // alex, ari, efe, kai, makena, noor, steve, sunny, zuri。
    // 根据 UUID 哈希选择，因此路径中应包含其中任意一个名称。
    const std::string path = defaultSkin.toString();
    const std::array<std::string, 9> validNames = {
        "alex", "ari", "efe", "kai", "makena", "noor", "steve", "sunny", "zuri"};
    bool matched = false;
    for (const auto& name : validNames) {
        if (path.find(name) != std::string::npos) {
            matched = true;
            break;
        }
    }
    EXPECT_TRUE(matched) << "Default skin path does not match any known default name: " << path;
}
