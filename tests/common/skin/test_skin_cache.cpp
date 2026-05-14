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

#include "common/core/Types.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include "common/network/packet/PacketSerializer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/manager/SkinCache.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"
#include "common/skin/network/SkinPackets.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace mc::skin;

class SkinCacheTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 使用临时目录
        testDir_ = "./test_skin_cache_" + std::to_string(std::time(nullptr));
        cache_ = std::make_unique<SkinCache>(testDir_);
    }

    void TearDown() override
    {
        cache_.reset();

        // 清理临时目录
        std::error_code ec;
        std::filesystem::remove_all(testDir_, ec);
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

    // 设置皮肤纹理后，使用纹理的类型
    SkinTextures textures;
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
    EXPECT_TRUE(defaultSkin.toString().find("steve") != std::string::npos ||
        defaultSkin.toString().find("alex") != std::string::npos);
}
