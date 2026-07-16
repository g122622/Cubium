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

#include "common/core/DefaultValues.hpp"

#include <algorithm>
#include <fstream>
#include <spdlog/spdlog.h>

namespace mc::client {

// 静态成员初始化
std::vector<std::unique_ptr<KeyBinding>> ClientSettings::s_keyBindings;

ClientSettings::ClientSettings()
    // 视频设置
    : renderDistance("renderDistance", 2, 32, defaults::client::renderDistance)
    , framerateLimit("framerateLimit", 0, 260, defaults::client::framerateLimit)
    , guiScale("guiScale", 0, 4, defaults::client::guiScale)
    , fullscreen("fullscreen", defaults::client::fullscreen)
    , vsync("vsync", defaults::client::vsync)
    , graphics("graphics",
          {static_cast<u8>(GraphicsMode::Fast), static_cast<u8>(GraphicsMode::Fancy)},
          static_cast<u8>(GraphicsMode::Fancy),
          {"fast", "fancy"})
    , clouds("clouds",
          {static_cast<u8>(CloudMode::Off), static_cast<u8>(CloudMode::Fast), static_cast<u8>(CloudMode::Fancy)},
          static_cast<u8>(CloudMode::Fancy),
          {"off", "fast", "fancy"})
    , mipmapLevels("mipmapLevels", 0, 4, defaults::client::mipmapLevels)
    , fovEffectScale("fovEffectScale", 0.0f, 1.0f, defaults::client::fovEffectScale)
    , screenShakeScale("screenShakeScale", 0.0f, 1.0f, defaults::client::screenShakeScale)
    , damageTiltStrength("damageTiltStrength", 0.0f, 1.0f, defaults::client::damageTiltStrength)
    , fogDensity("fogDensity", 0.0f, 2.0f, defaults::client::fogDensity)
    , ambientOcclusion("ambientOcclusion",
          {static_cast<u8>(AmbientOcclusionMode::Off),
              static_cast<u8>(AmbientOcclusionMode::Min),
              static_cast<u8>(AmbientOcclusionMode::Max)},
          static_cast<u8>(AmbientOcclusionMode::Max),
          {"off", "min", "max"})
    , particles("particles",
          {static_cast<u8>(ParticleMode::Minimal),
              static_cast<u8>(ParticleMode::Decreased),
              static_cast<u8>(ParticleMode::All)},
          static_cast<u8>(ParticleMode::All),
          {"minimal", "decreased", "all"})
    , biomeBlendRadius("biomeBlendRadius", 0, 7, defaults::client::biomeBlendRadius)
    , antiAliasing("antiAliasing", defaults::client::antiAliasing)

    // 音频设置
    , masterVolume("masterVolume", 0.0f, 1.0f, defaults::client::masterVolume)
    , musicVolume("musicVolume", 0.0f, 1.0f, defaults::client::musicVolume)
    , recordVolume("recordVolume", 0.0f, 1.0f, defaults::client::recordVolume)
    , weatherVolume("weatherVolume", 0.0f, 1.0f, defaults::client::weatherVolume)
    , blockVolume("blockVolume", 0.0f, 1.0f, defaults::client::blockVolume)
    , hostileVolume("hostileVolume", 0.0f, 1.0f, defaults::client::hostileVolume)
    , neutralVolume("neutralVolume", 0.0f, 1.0f, defaults::client::neutralVolume)
    , playerVolume("playerVolume", 0.0f, 1.0f, defaults::client::playerVolume)
    , ambientVolume("ambientVolume", 0.0f, 1.0f, defaults::client::ambientVolume)
    , voiceVolume("voiceVolume", 0.0f, 1.0f, defaults::client::voiceVolume)
    , uiVolume("uiVolume", 0.0f, 1.0f, defaults::client::uiVolume)

    // 控制设置
    , mouseSensitivity("mouseSensitivity", 0.0f, 1.0f, defaults::client::mouseSensitivity)
    , invertMouse("invertMouse", defaults::client::invertMouse)
    , rawMouseInput("rawMouseInput", defaults::client::rawMouseInput)
    , mouseWheelSensitivity("mouseWheelSensitivity", 0.0f, 1.0f, defaults::client::mouseWheelSensitivity)
    , autoJump("autoJump", defaults::client::autoJump)

    // 游戏设置
    , viewBobbing("viewBobbing", defaults::client::viewBobbing)
    , fov("fov", 30.0f, 110.0f, defaults::client::fov)
    , showFps("showFps", defaults::client::showFps)
    , showDebug("showDebug", defaults::client::showDebug)
    , language("language", defaults::client::language)

    // 网络设置
    , serverAddress("serverAddress", defaults::client::serverAddress)
    , serverPort("serverPort", 1, 65535, defaults::server::serverPort)
    , username("username", defaults::client::username)

    // 日志设置
    , logLevel("logLevel", defaults::client::logLevel)

    // 资源包设置
    , resourcePacks("resourcePacks")
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
    registerOption("video", &damageTiltStrength);
    registerOption("video", &fogDensity);
    registerOption("video", &ambientOcclusion);
    registerOption("video", &particles);
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
    registerOption("audio", &uiVolume);

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

Result<void> ClientSettings::generateDefaultConfig(const std::filesystem::path& path)
{
    resetToDefaults();
    return save(path);
}

Result<void> ClientSettings::loadSettings(const std::filesystem::path& path)
{
    // 加载基本设置
    auto result = load(path);
    if (result.failed()) {
        return result;
    }

    // 加载按键绑定
    try {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;
            file.close();

            if (j.contains("keyBindings") && j["keyBindings"].is_object()) {
                KeyBinding::deserializeAll(j["keyBindings"]);
            }
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("Failed to load key bindings: {}", e.what());
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
        case SoundCategory::UI:
            return uiVolume.get();
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
        case SoundCategory::UI:
            uiVolume.set(volume);
            break;
        default:
            break;
    }
}

} // namespace mc::client
