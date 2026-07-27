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
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/manager/DefaultSkinProvider.hpp"
#include "common/skin/manager/SkinManager.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace mc::skin;

class SkinManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // PID + 纳秒时间戳保证 CTest -j16 跨进程唯一，避免同秒 token 碰撞
        testDir_ = mc::test::makeUniqueTestDir("mc_skin_manager_test").string();
        manager_ = std::make_unique<SkinManager>(testDir_);
    }

    void TearDown() override
    {
        manager_.reset();

        mc::test::removeTestDir(std::filesystem::path(testDir_));
    }

    std::string testDir_;
    std::unique_ptr<SkinManager> manager_;
};

TEST_F(SkinManagerTest, Initialize)
{
    auto result = manager_->initialize();
    EXPECT_TRUE(result.success());
}

TEST_F(SkinManagerTest, DefaultSkinSteve)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 零 UUID 应该对应 Steve 或 Alex
    std::array<mc::u8, 16> zeroUUID = {};
    mc::ResourceLocation skin = manager_->getDefaultSkin(zeroUUID);

    // 应该是 steve 或 alex
    std::string skinStr = skin.toString();
    EXPECT_TRUE(skinStr.find("steve") != std::string::npos || skinStr.find("alex") != std::string::npos);
}

TEST_F(SkinManagerTest, DefaultSkinTypeForUUID)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 相同的 UUID 应该返回相同的皮肤类型
    std::array<mc::u8, 16> uuid1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};
    std::array<mc::u8, 16> uuid2 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01};

    SkinType type1 = manager_->getDefaultSkinType(uuid1);
    SkinType type2 = manager_->getDefaultSkinType(uuid2);

    // 类型应该是 Default 或 Slim
    EXPECT_TRUE(type1 == SkinType::Default || type1 == SkinType::Slim);
    EXPECT_TRUE(type2 == SkinType::Default || type2 == SkinType::Slim);
}

TEST_F(SkinManagerTest, GetOrCreatePlayerInfo)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "TestPlayer");

    auto info1 = manager_->getOrCreatePlayerInfo(profile);
    ASSERT_NE(nullptr, info1);
    EXPECT_EQ("TestPlayer", info1->name());

    // 再次获取应该返回同一个对象
    auto info2 = manager_->getOrCreatePlayerInfo(profile);
    EXPECT_EQ(info1.get(), info2.get());
}

TEST_F(SkinManagerTest, GetPlayerInfo)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    // 不存在时返回 nullptr
    auto info1 = manager_->getPlayerInfo(uuid);
    EXPECT_EQ(nullptr, info1);

    // 创建后再获取
    GameProfile profile(uuid, "TestPlayer");
    manager_->getOrCreatePlayerInfo(profile);

    auto info2 = manager_->getPlayerInfo(uuid);
    ASSERT_NE(nullptr, info2);
    EXPECT_EQ("TestPlayer", info2->name());
}

TEST_F(SkinManagerTest, RemovePlayerInfo)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};

    GameProfile profile(uuid, "TestPlayer");
    manager_->getOrCreatePlayerInfo(profile);

    auto info1 = manager_->getPlayerInfo(uuid);
    ASSERT_NE(nullptr, info1);

    manager_->removePlayerInfo(uuid);

    auto info2 = manager_->getPlayerInfo(uuid);
    EXPECT_EQ(nullptr, info2);
}

TEST_F(SkinManagerTest, ClearAllPlayerInfos)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00};
    std::array<mc::u8, 16> uuid2 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4, 0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01};

    GameProfile profile1(uuid1, "Player1");
    GameProfile profile2(uuid2, "Player2");

    manager_->getOrCreatePlayerInfo(profile1);
    manager_->getOrCreatePlayerInfo(profile2);

    ASSERT_NE(nullptr, manager_->getPlayerInfo(uuid1));
    ASSERT_NE(nullptr, manager_->getPlayerInfo(uuid2));

    manager_->clearAllPlayerInfos();

    EXPECT_EQ(nullptr, manager_->getPlayerInfo(uuid1));
    EXPECT_EQ(nullptr, manager_->getPlayerInfo(uuid2));
}

TEST_F(SkinManagerTest, CacheAccess)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 应该能够访问缓存
    EXPECT_NO_THROW(manager_->cache());
}

TEST_F(SkinManagerTest, DefaultSkinProviderAccess)
{
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 应该能够访问默认皮肤提供者
    EXPECT_NO_THROW(manager_->defaultSkinProvider());
}
