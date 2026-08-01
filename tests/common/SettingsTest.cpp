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

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

#include "client/settings/ClientSettings.hpp"
#include "common/TempDirHelper.hpp"
#include "common/core/settings/SettingsBase.hpp"
#include "common/core/settings/SettingsTypes.hpp"
#include "common/input/KeyBinding.hpp"
#include "server/settings/ServerSettings.hpp"

using namespace mc;
using namespace mc::client;
using namespace mc::server;

// ============================================================================
// SettingsTypes 测试
// ============================================================================

class SettingsTypesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 助手以 PID 组合 token 保证跨进程唯一，避免 CTest -j16 并发覆盖
        testDir = mc::test::makeUniqueTestDir("mc_settings_test");
    }

    void TearDown() override
    {
        // 清理临时目录
        mc::test::removeTestDir(testDir);
    }

    std::filesystem::path testDir;
};

// BooleanOption 测试
TEST_F(SettingsTypesTest, BooleanOption_DefaultValue)
{
    BooleanOption option("test_bool", true);
    EXPECT_TRUE(option.get());
    EXPECT_TRUE(option.isDefault());
}

TEST_F(SettingsTypesTest, BooleanOption_SetValue)
{
    BooleanOption option("test_bool", false);
    option.set(true);
    EXPECT_TRUE(option.get());
    EXPECT_FALSE(option.isDefault());
}

TEST_F(SettingsTypesTest, BooleanOption_Reset)
{
    BooleanOption option("test_bool", false);
    option.set(true);
    option.reset();
    EXPECT_FALSE(option.get());
    EXPECT_TRUE(option.isDefault());
}

TEST_F(SettingsTypesTest, BooleanOption_Callback)
{
    BooleanOption option("test_bool", false);
    bool callbackCalled = false;
    bool callbackValue = false;

    option.onChange([&](bool value) {
        callbackCalled = true;
        callbackValue = value;
    });

    option.set(true);
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(callbackValue);
}

TEST_F(SettingsTypesTest, BooleanOption_CallbackNotCalledOnSameValue)
{
    BooleanOption option("test_bool", false);
    int callCount = 0;

    option.onChange([&](bool /*value*/) { callCount++; });

    option.set(false); // 相同值
    EXPECT_EQ(callCount, 0);
}

TEST_F(SettingsTypesTest, BooleanOption_Serialize)
{
    BooleanOption option("test_bool", true);
    nlohmann::json j;
    option.serialize(j);

    EXPECT_TRUE(j.contains("test_bool"));
    EXPECT_TRUE(j["test_bool"].get<bool>());
}

TEST_F(SettingsTypesTest, BooleanOption_Deserialize)
{
    BooleanOption option("test_bool", false);
    nlohmann::json j = {{"test_bool", true}};
    option.deserialize(j);

    EXPECT_TRUE(option.get());
}

// RangeOption 测试
TEST_F(SettingsTypesTest, RangeOption_DefaultValue)
{
    RangeOption option("test_range", 0, 100, 50);
    EXPECT_EQ(option.get(), 50);
    EXPECT_TRUE(option.isDefault());
}

TEST_F(SettingsTypesTest, RangeOption_ClampValue)
{
    RangeOption option("test_range", 0, 100, 50);

    option.set(150);
    EXPECT_EQ(option.get(), 100); // 应该 clamp 到最大值

    option.set(-50);
    EXPECT_EQ(option.get(), 0); // 应该 clamp 到最小值
}

TEST_F(SettingsTypesTest, RangeOption_SetValue)
{
    RangeOption option("test_range", 0, 100, 50);
    option.set(75);
    EXPECT_EQ(option.get(), 75);
    EXPECT_FALSE(option.isDefault());
}

TEST_F(SettingsTypesTest, RangeOption_Callback)
{
    RangeOption option("test_range", 0, 100, 50);
    int callbackValue = 0;

    option.onChange([&](i32 value) { callbackValue = value; });

    option.set(75);
    EXPECT_EQ(callbackValue, 75);
}

TEST_F(SettingsTypesTest, RangeOption_Serialize)
{
    RangeOption option("test_range", 0, 100, 50);
    option.set(75);
    nlohmann::json j;
    option.serialize(j);

    EXPECT_TRUE(j.contains("test_range"));
    EXPECT_EQ(j["test_range"].get<i32>(), 75);
}

TEST_F(SettingsTypesTest, RangeOption_Deserialize)
{
    RangeOption option("test_range", 0, 100, 50);
    nlohmann::json j = {{"test_range", 75}};
    option.deserialize(j);

    EXPECT_EQ(option.get(), 75);
}

// FloatOption 测试
TEST_F(SettingsTypesTest, FloatOption_DefaultValue)
{
    FloatOption option("test_float", 0.0f, 1.0f, 0.5f);
    EXPECT_FLOAT_EQ(option.get(), 0.5f);
    EXPECT_TRUE(option.isDefault());
}

TEST_F(SettingsTypesTest, FloatOption_ClampValue)
{
    FloatOption option("test_float", 0.0f, 1.0f, 0.5f);

    option.set(1.5f);
    EXPECT_FLOAT_EQ(option.get(), 1.0f);

    option.set(-0.5f);
    EXPECT_FLOAT_EQ(option.get(), 0.0f);
}

TEST_F(SettingsTypesTest, FloatOption_Callback)
{
    FloatOption option("test_float", 0.0f, 1.0f, 0.5f);
    float callbackValue = 0.0f;

    option.onChange([&](f32 value) { callbackValue = value; });

    option.set(0.75f);
    EXPECT_FLOAT_EQ(callbackValue, 0.75f);
}

TEST_F(SettingsTypesTest, FloatOption_Serialize)
{
    FloatOption option("test_float", 0.0f, 1.0f, 0.5f);
    option.set(0.75f);
    nlohmann::json j;
    option.serialize(j);

    EXPECT_TRUE(j.contains("test_float"));
    EXPECT_FLOAT_EQ(j["test_float"].get<f32>(), 0.75f);
}

// EnumOption 测试
TEST_F(SettingsTypesTest, EnumOption_DefaultValue)
{
    EnumOption<u8> option("test_enum", {0, 1, 2}, 1, {"low", "medium", "high"});
    EXPECT_EQ(option.get(), 1);
    EXPECT_TRUE(option.isDefault());
}

TEST_F(SettingsTypesTest, EnumOption_SetValue)
{
    EnumOption<u8> option("test_enum", {0, 1, 2}, 1, {"low", "medium", "high"});
    option.set(2);
    EXPECT_EQ(option.get(), 2);
    EXPECT_EQ(option.getName(), "high");
}

TEST_F(SettingsTypesTest, EnumOption_SetByName)
{
    EnumOption<u8> option("test_enum", {0, 1, 2}, 1, {"low", "medium", "high"});
    EXPECT_TRUE(option.setByName("low"));
    EXPECT_EQ(option.get(), 0);
}

TEST_F(SettingsTypesTest, EnumOption_InvalidValue)
{
    EnumOption<u8> option("test_enum", {0, 1, 2}, 1, {"low", "medium", "high"});
    option.set(5);              // 无效值
    EXPECT_EQ(option.get(), 1); // 应该保持原值
}

TEST_F(SettingsTypesTest, EnumOption_Serialize)
{
    EnumOption<u8> option("test_enum", {0, 1, 2}, 1, {"low", "medium", "high"});
    option.set(2);
    nlohmann::json j;
    option.serialize(j);

    EXPECT_TRUE(j.contains("test_enum"));
    EXPECT_EQ(j["test_enum"].get<std::string>(), "high");
}

TEST_F(SettingsTypesTest, EnumOption_Deserialize)
{
    EnumOption<u8> option("test_enum", {0, 1, 2}, 1, {"low", "medium", "high"});
    nlohmann::json j = {{"test_enum", "low"}};
    option.deserialize(j);

    EXPECT_EQ(option.get(), 0);
}

// StringOption 测试
TEST_F(SettingsTypesTest, StringOption_DefaultValue)
{
    StringOption option("test_string", "default");
    EXPECT_EQ(option.get(), "default");
    EXPECT_TRUE(option.isDefault());
}

TEST_F(SettingsTypesTest, StringOption_SetValue)
{
    StringOption option("test_string", "default");
    option.set("new value");
    EXPECT_EQ(option.get(), "new value");
    EXPECT_FALSE(option.isDefault());
}

TEST_F(SettingsTypesTest, StringOption_Serialize)
{
    StringOption option("test_string", "default");
    option.set("custom");
    nlohmann::json j;
    option.serialize(j);

    EXPECT_TRUE(j.contains("test_string"));
    EXPECT_EQ(j["test_string"].get<std::string>(), "custom");
}

// ============================================================================
// SettingsBase 测试
// ============================================================================

class TestSettings : public SettingsBase {
public:
    TestSettings()
    {
        registerOption("video", &resolution);
        registerOption("video", &fullscreen);
        registerOption("audio", &volume);
    }

    RangeOption resolution{"resolution", 640, 3840, 1920};
    BooleanOption fullscreen{"fullscreen", false};
    FloatOption volume{"volume", 0.0f, 1.0f, 0.8f};

protected:
    [[nodiscard]] Result<void> generateDefaultConfig(const std::filesystem::path& path) override
    {
        resetToDefaults();
        return save(path);
    }
};

class SettingsBaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 助手以 PID 组合 token 保证跨进程唯一，避免 CTest -j16 并发覆盖
        testDir = mc::test::makeUniqueTestDir("mc_settings_settings_test");
    }

    void TearDown() override { mc::test::removeTestDir(testDir); }

    std::filesystem::path testDir;
};

TEST_F(SettingsBaseTest, SaveAndLoad)
{
    TestSettings settings;
    settings.resolution.set(2560);
    settings.fullscreen.set(true);
    settings.volume.set(0.5f);

    auto path = testDir / "test_settings.json";
    auto saveResult = settings.save(path);
    ASSERT_TRUE(saveResult.success());

    // 创建新设置并加载
    TestSettings loadedSettings;
    auto loadResult = loadedSettings.load(path);
    ASSERT_TRUE(loadResult.success());

    EXPECT_EQ(loadedSettings.resolution.get(), 2560);
    EXPECT_TRUE(loadedSettings.fullscreen.get());
    EXPECT_FLOAT_EQ(loadedSettings.volume.get(), 0.5f);
}

TEST_F(SettingsBaseTest, LoadNonExistentFile)
{
    TestSettings settings;
    auto path = testDir / "nonexistent.json";

    // 加载不存在的文件应该成功（使用默认值）
    auto result = settings.load(path);
    EXPECT_TRUE(result.success());

    // 应该保持默认值
    EXPECT_EQ(settings.resolution.get(), 1920);
    EXPECT_FALSE(settings.fullscreen.get());
    EXPECT_FLOAT_EQ(settings.volume.get(), 0.8f);
}

TEST_F(SettingsBaseTest, ResetToDefaults)
{
    TestSettings settings;
    settings.resolution.set(2560);
    settings.fullscreen.set(true);
    settings.volume.set(0.5f);

    settings.resetToDefaults();

    EXPECT_EQ(settings.resolution.get(), 1920);
    EXPECT_FALSE(settings.fullscreen.get());
    EXPECT_FLOAT_EQ(settings.volume.get(), 0.8f);
}

TEST_F(SettingsBaseTest, ResetGroupToDefaults)
{
    TestSettings settings;
    settings.resolution.set(2560);
    settings.fullscreen.set(true);
    settings.volume.set(0.5f);

    // 只重置 video 组
    settings.resetGroupToDefaults("video");

    EXPECT_EQ(settings.resolution.get(), 1920);
    EXPECT_FALSE(settings.fullscreen.get());
    // audio 组应该保持不变
    EXPECT_FLOAT_EQ(settings.volume.get(), 0.5f);
}

TEST_F(SettingsBaseTest, LoadOrGenerateCreatesFile)
{
    TestSettings settings;
    auto path = testDir / "generated_settings.json";
    ASSERT_FALSE(std::filesystem::exists(path));

    auto result = settings.loadOrGenerate(path);
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(std::filesystem::exists(path));
}

// ============================================================================
// KeyBinding 测试
// ============================================================================

class KeyBindingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 清除之前的绑定
        KeyBinding::resetAllToDefault();
    }
};

TEST_F(KeyBindingTest, CreateBinding)
{
    KeyBinding binding("test.key", Keys::W, "test.category");

    EXPECT_EQ(binding.id(), "test.key");
    EXPECT_EQ(binding.key(), Keys::W);
    EXPECT_EQ(binding.defaultKey(), Keys::W);
    EXPECT_EQ(binding.category(), "test.category");
    EXPECT_TRUE(binding.isDefault());
}

TEST_F(KeyBindingTest, SetKey)
{
    KeyBinding binding("test.key", Keys::W, "test.category");
    binding.setKey(Keys::A);

    EXPECT_EQ(binding.key(), Keys::A);
    EXPECT_FALSE(binding.isDefault());
}

TEST_F(KeyBindingTest, ResetToDefault)
{
    KeyBinding binding("test.key", Keys::W, "test.category");
    binding.setKey(Keys::A);
    binding.resetToDefault();

    EXPECT_EQ(binding.key(), Keys::W);
    EXPECT_TRUE(binding.isDefault());
}

TEST_F(KeyBindingTest, FindBinding)
{
    KeyBinding binding("test.find", Keys::W, "test.category");

    auto* found = KeyBinding::find("test.find");
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->key(), Keys::W);

    auto* notFound = KeyBinding::find("nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(KeyBindingTest, UpdateAll)
{
    KeyBinding binding("test.update", Keys::W, "test.category");

    std::vector<i32> pressed = {Keys::W};
    std::vector<i32> justPressed = {Keys::W};
    std::vector<i32> justReleased;

    KeyBinding::updateAll(pressed, justPressed, justReleased);

    EXPECT_TRUE(binding.isPressed());
    EXPECT_TRUE(binding.isJustPressed());
    EXPECT_FALSE(binding.isJustReleased());
}

TEST_F(KeyBindingTest, SerializeAndDeserialize)
{
    {
        KeyBinding binding1("test.serialize1", Keys::W, "test.category");
        KeyBinding binding2("test.serialize2", Keys::A, "test.category");
        binding1.setKey(Keys::E);
        binding2.setKey(Keys::Q);

        nlohmann::json j;
        KeyBinding::serializeAll(j);

        // 只有非默认值会被序列化
        EXPECT_TRUE(j.contains("test.serialize1"));
        EXPECT_TRUE(j.contains("test.serialize2"));
    }

    {
        KeyBinding binding1("test.serialize1", Keys::W, "test.category");
        KeyBinding binding2("test.serialize2", Keys::A, "test.category");

        nlohmann::json j = {{"test.serialize1", Keys::E}, {"test.serialize2", Keys::Q}};

        KeyBinding::deserializeAll(j);

        EXPECT_EQ(binding1.key(), Keys::E);
        EXPECT_EQ(binding2.key(), Keys::Q);
    }
}

// ============================================================================
// ClientSettings 测试
// ============================================================================

class ClientSettingsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 助手以 PID 组合 token 保证跨进程唯一，避免 CTest -j16 并发覆盖
        testDir = mc::test::makeUniqueTestDir("mc_settings_client_test");
    }

    void TearDown() override { mc::test::removeTestDir(testDir); }

    std::filesystem::path testDir;
};

TEST_F(ClientSettingsTest, DefaultValues)
{
    ClientSettings settings;

    EXPECT_EQ(settings.renderDistance.get(), 12);
    EXPECT_FALSE(settings.fullscreen.get());
    EXPECT_TRUE(settings.vsync.get());
    EXPECT_FLOAT_EQ(settings.mouseSensitivity.get(), 0.5f);
    EXPECT_EQ(settings.serverPort.get(), 25565);
}

TEST_F(ClientSettingsTest, InitializeKeyBindings)
{
    ClientSettings settings;
    settings.initializeKeyBindings();

    // 检查一些关键绑定
    auto* forward = ClientSettings::getKeyBinding("key.forward");
    EXPECT_NE(forward, nullptr);
    EXPECT_EQ(forward->key(), Keys::W);

    auto* jump = ClientSettings::getKeyBinding("key.jump");
    EXPECT_NE(jump, nullptr);
    EXPECT_EQ(jump->key(), Keys::Space);

    auto* inventory = ClientSettings::getKeyBinding("key.inventory");
    EXPECT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->key(), Keys::E);
}

TEST_F(ClientSettingsTest, SaveAndLoadSettings)
{
    {
        ClientSettings settings;
        settings.renderDistance.set(20);
        settings.fullscreen.set(true);
        settings.mouseSensitivity.set(0.75f);
        settings.initializeKeyBindings();

        // 修改一个按键绑定
        auto* forward = ClientSettings::getKeyBinding("key.forward");
        if (forward) {
            forward->setKey(Keys::D);
        }

        auto path = testDir / "client_settings.json";
        auto result = settings.saveSettings(path);
        ASSERT_TRUE(result.success());
    }

    {
        ClientSettings settings;
        settings.initializeKeyBindings();
        auto path = testDir / "client_settings.json";
        auto result = settings.loadSettings(path);
        ASSERT_TRUE(result.success());

        EXPECT_EQ(settings.renderDistance.get(), 20);
        EXPECT_TRUE(settings.fullscreen.get());
        EXPECT_FLOAT_EQ(settings.mouseSensitivity.get(), 0.75f);

        // 检查按键绑定是否被加载
        auto* forward = ClientSettings::getKeyBinding("key.forward");
        if (forward) {
            EXPECT_EQ(forward->key(), Keys::D);
        }
    }
}

// ============================================================================
// ServerSettings 测试
// ============================================================================

class ServerSettingsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 助手以 PID 组合 token 保证跨进程唯一，避免 CTest -j16 并发覆盖
        testDir = mc::test::makeUniqueTestDir("mc_settings_server_test");
    }

    void TearDown() override { mc::test::removeTestDir(testDir); }

    std::filesystem::path testDir;
};

TEST_F(ServerSettingsTest, DefaultValues)
{
    ServerSettings settings;

    EXPECT_EQ(settings.serverPort.get(), 25565);
    EXPECT_EQ(settings.maxPlayers.get(), 20);
    EXPECT_FALSE(settings.onlineMode.get());
    EXPECT_EQ(settings.viewDistance.get(), 10);
    EXPECT_EQ(settings.tickRate.get(), 20);
}

TEST_F(ServerSettingsTest, SaveAndLoadSettings)
{
    {
        ServerSettings settings;
        settings.serverPort.set(25565);
        settings.maxPlayers.set(50);
        settings.onlineMode.set(false);
        settings.viewDistance.set(16);
        settings.difficulty.set(DifficultyValue::Hard);

        auto path = testDir / "server_settings.json";
        auto result = settings.saveSettings(path);
        ASSERT_TRUE(result.success());
    }

    {
        ServerSettings settings;
        auto path = testDir / "server_settings.json";
        auto result = settings.loadSettings(path);
        ASSERT_TRUE(result.success());

        EXPECT_EQ(settings.serverPort.get(), 25565);
        EXPECT_EQ(settings.maxPlayers.get(), 50);
        EXPECT_FALSE(settings.onlineMode.get());
        EXPECT_EQ(settings.viewDistance.get(), 16);
        EXPECT_EQ(settings.difficulty.get(), DifficultyValue::Hard);
    }
}

TEST_F(ServerSettingsTest, EnumOptionSerialize)
{
    ServerSettings settings;
    settings.difficulty.set(DifficultyValue::Hard);

    nlohmann::json j;
    settings.difficulty.serialize(j);

    EXPECT_TRUE(j.contains("difficulty"));
    EXPECT_EQ(j["difficulty"].get<std::string>(), "hard");
}

TEST_F(ServerSettingsTest, EnumOptionDeserialize)
{
    ServerSettings settings;
    nlohmann::json j = {{"difficulty", "peaceful"}};
    settings.difficulty.deserialize(j);

    EXPECT_EQ(settings.difficulty.get(), DifficultyValue::Peaceful);
}
