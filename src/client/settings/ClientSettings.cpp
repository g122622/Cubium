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

#include "ClientSettings.hpp"

#include <fstream>
#include <spdlog/spdlog.h>

namespace mc::client {

// 静态成员初始化
std::vector<std::unique_ptr<KeyBinding>> ClientSettings::s_keyBindings;

ClientSettings::ClientSettings()
    // 视频设置
    : renderDistance("renderDistance", 2, 32, 12)
    , framerateLimit("framerateLimit", 0, 260, 120)
    , guiScale("guiScale", 0, 4, 0)
    , fullscreen("fullscreen", false)
    , vsync("vsync", true)
    , graphics("graphics",
          {static_cast<u8>(GraphicsMode::Fast), static_cast<u8>(GraphicsMode::Fancy)},
          static_cast<u8>(GraphicsMode::Fancy),
          {"fast", "fancy"})
    , clouds("clouds",
          {static_cast<u8>(CloudMode::Off), static_cast<u8>(CloudMode::Fast), static_cast<u8>(CloudMode::Fancy)},
          static_cast<u8>(CloudMode::Fancy),
          {"off", "fast", "fancy"})
    , mipmapLevels("mipmapLevels", 0, 4, 4)
    , fovEffectScale("fovEffectScale", 0.0f, 1.0f, 1.0f)
    , screenShakeScale("screenShakeScale", 0.0f, 1.0f, 1.0f)
    , fogDensity("fogDensity", 0.0f, 2.0f, 1.0f)
    , ambientOcclusion("ambientOcclusion",
          {static_cast<u8>(AmbientOcclusionMode::Off),
              static_cast<u8>(AmbientOcclusionMode::Min),
              static_cast<u8>(AmbientOcclusionMode::Max)},
          static_cast<u8>(AmbientOcclusionMode::Max),
          {"off", "min", "max"})
    , biomeBlendRadius("biomeBlendRadius", 0, 7, 2) // MC 默认 2 (5x5 混合区域)
    , antiAliasing("antiAliasing", true)

    // 音频设置
    , masterVolume("masterVolume", 0.0f, 1.0f, 1.0f)
    , musicVolume("musicVolume", 0.0f, 1.0f, 0.5f)
    , recordVolume("recordVolume", 0.0f, 1.0f, 1.0f)
    , weatherVolume("weatherVolume", 0.0f, 1.0f, 1.0f)
    , blockVolume("blockVolume", 0.0f, 1.0f, 1.0f)
    , hostileVolume("hostileVolume", 0.0f, 1.0f, 1.0f)
    , neutralVolume("neutralVolume", 0.0f, 1.0f, 1.0f)
    , playerVolume("playerVolume", 0.0f, 1.0f, 1.0f)
    , ambientVolume("ambientVolume", 0.0f, 1.0f, 1.0f)
    , voiceVolume("voiceVolume", 0.0f, 1.0f, 1.0f)

    // 控制设置
    , mouseSensitivity("mouseSensitivity", 0.0f, 1.0f, 0.5f)
    , invertMouse("invertMouse", false)
    , rawMouseInput("rawMouseInput", true)
    , mouseWheelSensitivity("mouseWheelSensitivity", 0.0f, 1.0f, 1.0f)
    , autoJump("autoJump", false)

    // 游戏设置
    , viewBobbing("viewBobbing", true)
    , fov("fov", 30.0f, 110.0f, 70.0f)
    , showFps("showFps", false)
    , showDebug("showDebug", false)
    , language("language", "zh_cn")

    // 网络设置
    , serverAddress("serverAddress", "127.0.0.1")
    , serverPort("serverPort", 1, 65535, 19132)
    , username("username", "Player")

    // 日志设置
    , logLevel("logLevel", "info")

    // 资源包设置
    , resourcePacks("resourcePacks")
    , resourcePackDir("resourcePackDir", "resourcepacks")
{
    // 注册视频设置
    registerOption("video", &renderDistance);
    registerOption("video", &framerateLimit);
    registerOption("video", &guiScale);
    registerOption("video", &fullscreen);
    registerOption("video", &vsync);
    registerOption("video", &graphics);
    registerOption("video", &clouds);
    registerOption("video", &mipmapLevels);
    registerOption("video", &fovEffectScale);
    registerOption("video", &screenShakeScale);
    registerOption("video", &fogDensity);
    registerOption("video", &ambientOcclusion);
    registerOption("video", &biomeBlendRadius);
    registerOption("video", &antiAliasing);

    // 注册音频设置
    registerOption("audio", &masterVolume);
    registerOption("audio", &musicVolume);
    registerOption("audio", &recordVolume);
    registerOption("audio", &weatherVolume);
    registerOption("audio", &blockVolume);
    registerOption("audio", &hostileVolume);
    registerOption("audio", &neutralVolume);
    registerOption("audio", &playerVolume);
    registerOption("audio", &ambientVolume);
    registerOption("audio", &voiceVolume);

    // 注册控制设置
    registerOption("control", &mouseSensitivity);
    registerOption("control", &invertMouse);
    registerOption("control", &rawMouseInput);
    registerOption("control", &mouseWheelSensitivity);
    registerOption("control", &autoJump);

    // 注册游戏设置
    registerOption("game", &viewBobbing);
    registerOption("game", &fov);
    registerOption("game", &showFps);
    registerOption("game", &showDebug);
    registerOption("game", &language);

    // 注册网络设置
    registerOption("network", &serverAddress);
    registerOption("network", &serverPort);
    registerOption("network", &username);

    // 注册日志设置
    registerOption("log", &logLevel);

    // 注册资源包设置
    registerOption("resourcePacks", &resourcePacks);
    registerOption("resourcePacks", &resourcePackDir);
}

void ClientSettings::initializeKeyBindings()
{
    // 清空现有绑定
    s_keyBindings.clear();

    // 移动控制
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.forward", Keys::W, "key.categories.movement"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.left", Keys::A, "key.categories.movement"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.back", Keys::S, "key.categories.movement"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.right", Keys::D, "key.categories.movement"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.jump", Keys::Space, "key.categories.movement"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.sneak", Keys::LeftShift, "key.categories.movement"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.sprint", Keys::LeftControl, "key.categories.movement"));

    // 游戏控制
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.inventory", Keys::E, "key.categories.gameplay"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.use", Keys::Mouse::Right, "key.categories.gameplay"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.attack", Keys::Mouse::Left, "key.categories.gameplay"));
    s_keyBindings.push_back(
        std::make_unique<KeyBinding>("key.pickItem", Keys::Mouse::Middle, "key.categories.gameplay"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.chat", Keys::T, "key.categories.gameplay"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.playerlist", Keys::Tab, "key.categories.gameplay"));

    // 物品栏
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.1", Keys::D1, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.2", Keys::D2, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.3", Keys::D3, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.4", Keys::D4, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.5", Keys::D5, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.6", Keys::D6, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.7", Keys::D7, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.8", Keys::D8, "key.categories.inventory"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.hotbar.9", Keys::D9, "key.categories.inventory"));

    // 功能键
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.screenshot", Keys::F2, "key.categories.misc"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.toggleDebug", Keys::F3, "key.categories.misc"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.fullscreen", Keys::F11, "key.categories.misc"));
    s_keyBindings.push_back(std::make_unique<KeyBinding>("key.smoothCamera", Keys::F8, "key.categories.misc"));

    spdlog::info("Initialized {} key bindings", s_keyBindings.size());
}

KeyBinding* ClientSettings::getKeyBinding(const std::string& id)
{
    return KeyBinding::find(id);
}

const std::vector<std::unique_ptr<KeyBinding>>& ClientSettings::getAllKeyBindings()
{
    return s_keyBindings;
}

Result<void> ClientSettings::loadSettings(const std::filesystem::path& path)
{
    // 加载基本设置
    auto result = load(path);
    if (result.failed()) {
        return result;
    }

    // 加载按键绑定
    // 尝试读取按键绑定部分
    if (std::filesystem::exists(path)) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;
                file.close();

                if (j.contains("keyBindings") && j["keyBindings"].is_object()) {
                    KeyBinding::deserializeAll(j["keyBindings"]);
                }
            }
            catch (const std::exception& e) {
                spdlog::warn("Failed to load key bindings: {}", e.what());
            }
        }
    }

    spdlog::info("Client settings loaded from: {}", path.string());
    return Result<void>::ok();
}

Result<void> ClientSettings::saveSettings(const std::filesystem::path& path)
{
    // 先保存基本设置
    auto result = save(path);
    if (result.failed()) {
        return result;
    }

    // 读取现有 JSON 并添加按键绑定
    try {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;
            file.close();

            // 添加按键绑定
            nlohmann::json keyBindingsJson;
            KeyBinding::serializeAll(keyBindingsJson);
            j["keyBindings"] = keyBindingsJson;

            // 写回文件
            std::ofstream outFile(path, std::ios::binary | std::ios::trunc);
            if (outFile.is_open()) {
                outFile << j.dump(4);
                outFile.close();
            }
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("Failed to save key bindings: {}", e.what());
    }

    spdlog::info("Client settings saved to: {}", path.string());
    return Result<void>::ok();
}

f32 ClientSettings::getVolumeForCategory(sound::SoundCategory category) const
{
    using namespace sound;

    switch (category) {
        case SoundCategory::Master:
            return masterVolume.get();
        case SoundCategory::Music:
            return musicVolume.get();
        case SoundCategory::Records:
            return recordVolume.get();
        case SoundCategory::Weather:
            return weatherVolume.get();
        case SoundCategory::Blocks:
            return blockVolume.get();
        case SoundCategory::Hostile:
            return hostileVolume.get();
        case SoundCategory::Neutral:
            return neutralVolume.get();
        case SoundCategory::Players:
            return playerVolume.get();
        case SoundCategory::Ambient:
            return ambientVolume.get();
        case SoundCategory::Voice:
            return voiceVolume.get();
        default:
            return 1.0f;
    }
}

void ClientSettings::setVolumeForCategory(sound::SoundCategory category, f32 volume)
{
    using namespace sound;

    volume = std::clamp(volume, 0.0f, 1.0f);

    switch (category) {
        case SoundCategory::Master:
            masterVolume.set(volume);
            break;
        case SoundCategory::Music:
            musicVolume.set(volume);
            break;
        case SoundCategory::Records:
            recordVolume.set(volume);
            break;
        case SoundCategory::Weather:
            weatherVolume.set(volume);
            break;
        case SoundCategory::Blocks:
            blockVolume.set(volume);
            break;
        case SoundCategory::Hostile:
            hostileVolume.set(volume);
            break;
        case SoundCategory::Neutral:
            neutralVolume.set(volume);
            break;
        case SoundCategory::Players:
            playerVolume.set(volume);
            break;
        case SoundCategory::Ambient:
            ambientVolume.set(volume);
            break;
        case SoundCategory::Voice:
            voiceVolume.set(volume);
            break;
        default:
            break;
    }
}

} // namespace mc::client
