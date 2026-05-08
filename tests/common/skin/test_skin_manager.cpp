#include <gtest/gtest.h>
#include "common/skin/manager/SkinManager.hpp"
#include "common/skin/manager/DefaultSkinProvider.hpp"
#include "common/skin/core/GameProfile.hpp"
#include <filesystem>
#include <fstream>

using namespace mc::skin;

class SkinManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir_ = "./test_skin_manager_" + std::to_string(std::time(nullptr));
        manager_ = std::make_unique<SkinManager>(testDir_);
    }

    void TearDown() override {
        manager_.reset();

        // 清理临时目录
        std::error_code ec;
        std::filesystem::remove_all(testDir_, ec);
    }

    mc::std::string testDir_;
    std::unique_ptr<SkinManager> manager_;
};

TEST_F(SkinManagerTest, Initialize) {
    auto result = manager_->initialize();
    EXPECT_TRUE(result.success());
}

TEST_F(SkinManagerTest, DefaultSkinSteve) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 零 UUID 应该对应 Steve 或 Alex
    std::array<mc::u8, 16> zeroUUID = {};
    mc::ResourceLocation skin = manager_->getDefaultSkin(zeroUUID);

    // 应该是 steve 或 alex
    mc::std::string skinStr = skin.toString();
    EXPECT_TRUE(skinStr.find("steve") != mc::std::string::npos ||
                skinStr.find("alex") != mc::std::string::npos);
}

TEST_F(SkinManagerTest, DefaultSkinTypeForUUID) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 相同的 UUID 应该返回相同的皮肤类型
    std::array<mc::u8, 16> uuid1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };
    std::array<mc::u8, 16> uuid2 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01
    };

    SkinType type1 = manager_->getDefaultSkinType(uuid1);
    SkinType type2 = manager_->getDefaultSkinType(uuid2);

    // 类型应该是 Default 或 Slim
    EXPECT_TRUE(type1 == SkinType::Default || type1 == SkinType::Slim);
    EXPECT_TRUE(type2 == SkinType::Default || type2 == SkinType::Slim);
}

TEST_F(SkinManagerTest, GetOrCreatePlayerInfo) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "TestPlayer");

    auto info1 = manager_->getOrCreatePlayerInfo(profile);
    ASSERT_NE(nullptr, info1);
    EXPECT_EQ("TestPlayer", info1->name());

    // 再次获取应该返回同一个对象
    auto info2 = manager_->getOrCreatePlayerInfo(profile);
    EXPECT_EQ(info1.get(), info2.get());
}

TEST_F(SkinManagerTest, GetPlayerInfo) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

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

TEST_F(SkinManagerTest, RemovePlayerInfo) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };

    GameProfile profile(uuid, "TestPlayer");
    manager_->getOrCreatePlayerInfo(profile);

    auto info1 = manager_->getPlayerInfo(uuid);
    ASSERT_NE(nullptr, info1);

    manager_->removePlayerInfo(uuid);

    auto info2 = manager_->getPlayerInfo(uuid);
    EXPECT_EQ(nullptr, info2);
}

TEST_F(SkinManagerTest, ClearAllPlayerInfos) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    std::array<mc::u8, 16> uuid1 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x00
    };
    std::array<mc::u8, 16> uuid2 = {
        0x55, 0x0e, 0x84, 0x00, 0xe2, 0x9b, 0x41, 0xd4,
        0xa7, 0x16, 0x44, 0x66, 0x55, 0x44, 0x00, 0x01
    };

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

TEST_F(SkinManagerTest, CacheAccess) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 应该能够访问缓存
    EXPECT_NO_THROW(manager_->cache());
}

TEST_F(SkinManagerTest, DefaultSkinProviderAccess) {
    auto initResult = manager_->initialize();
    ASSERT_TRUE(initResult.success());

    // 应该能够访问默认皮肤提供者
    EXPECT_NO_THROW(manager_->defaultSkinProvider());
}
