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

#include "client/application/ClientApplication.hpp"

#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/renderer/trident/cloud/CloudMode.hpp"
#include "client/renderer/trident/core/TridentEngine.hpp"
#include "client/renderer/trident/particle/ParticleManager.hpp"
#include "client/settings/ClientSettings.hpp"
#include "client/ui/GuiScale.hpp"
#include "client/ui/minecraft/screens/DebugScreenWidget.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"

#include <filesystem>
#include <string>
#include <system_error>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

Result<void> ClientApplication::loadSettings(const std::string& path)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "LoadSettings", "path", path);

    m_settingsPath = std::filesystem::path(path);

    auto result = m_settings.loadSettings(path);
    if (result.failed()) {
        return result;
    }

    // 确保设置目录存在（使用当前实际设置路径，避免写到默认目录）
    const auto settingsDir = m_settingsPath.parent_path();
    if (!settingsDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(settingsDir, ec);
        if (ec) {
            spdlog::warn("Failed to create settings directory: {}", settingsDir.string());
        }
    }

    // 启用自动保存
    m_settings.enableAutoSave(m_settingsPath);

    spdlog::info("Client settings path: {}", m_settingsPath.string());

    return Result<void>::ok();
}

void ClientApplication::applySettings()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ApplySettings");

    // 设置变更回调在 setupSettingCallbacks 中设置
    // 这里应用初始设置值
    // 同步渲染器运行时参数
    if (m_renderer) {
        m_renderer->setRenderDistanceChunks(m_settings.renderDistance.get());
        m_renderer->setLandFogDensity(m_settings.fogDensity.get());
        m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(m_settings.clouds.get()));
    }

    // 应用光照模式（环境光遮蔽）
    ChunkMesher::syncFromSettings(static_cast<client::AmbientOcclusionMode>(m_settings.ambientOcclusion.get()));

    // 应用生物群系颜色混合半径
    ChunkMesher::setBiomeBlendRadius(m_settings.biomeBlendRadius.get());

    // 应用粒子效果模式
    if (m_renderer && m_renderer->isParticleManagerInitialized()) {
        m_renderer->particleManager().setParticleMode(static_cast<client::ParticleMode>(m_settings.particles.get()));
    }
}

void ClientApplication::setupSettingCallbacks()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "SetupSettingCallbacks");

    // 渲染距离变更
    m_settings.renderDistance.onChange([this](i32 value) {
        spdlog::info("Render distance changed to: {}", value);
        if (m_renderer) {
            m_renderer->setRenderDistanceChunks(value);
        }
        auto* debugWidget = m_kageroEngine
            ? static_cast<ui::minecraft::DebugScreenWidget*>(m_kageroEngine->getLayer(m_debugScreenLayerId))
            : nullptr;
        if (debugWidget) {
            debugWidget->setRenderDistance(value);
        }
        // 世界更新时会使用新值
    });

    // 全屏模式变更
    m_settings.fullscreen.onChange([this](bool value) {
        spdlog::info("Fullscreen changed to: {}", value);
        m_window.setFullscreen(value);
    });

    m_settings.guiScale.onChange([this](i32 value) {
        spdlog::info("GUI scale changed to: {}", value);
        applyGuiScale();
    });

    // VSync 变更
    m_settings.vsync.onChange([this](bool value) {
        spdlog::info("VSync changed to: {}", value);
        m_window.setVSync(value);
        if (m_renderer) {
            auto result = m_renderer->setVSyncEnabled(value);
            if (result.failed()) {
                spdlog::warn("Failed to apply VSync change: {}", result.error().toString());
            }
        }
    });

    // 云模式变更
    m_settings.clouds.onChange([this](u8 value) {
        spdlog::info("Cloud mode changed to: {}", static_cast<i32>(value));
        if (m_renderer) {
            m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(value));
        }
    });

    // 鼠标灵敏度变更
    m_settings.mouseSensitivity.onChange([this](f32 value) {
        spdlog::info("Mouse sensitivity changed to: {}", value);
        // 鼠标灵敏度在 handleEvents 中应用
    });

    // FOV 变更
    m_settings.fov.onChange([this](f32 value) {
        spdlog::info("FOV changed to: {}", value);
        m_camera.setFOV(value);
    });

    // 雾气密度变更
    m_settings.fogDensity.onChange([this](f32 value) {
        spdlog::info("Fog density changed to: {}", value);
        if (m_renderer) {
            m_renderer->setLandFogDensity(value);
        }
    });

    // 光照模式（环境光遮蔽）变更
    m_settings.ambientOcclusion.onChange([this](u8 value) {
        auto mode = static_cast<client::AmbientOcclusionMode>(value);
        spdlog::info("Ambient occlusion changed to: {}", static_cast<i32>(mode));
        ChunkMesher::syncFromSettings(mode);
    });

    // 生物群系颜色混合半径变更
    m_settings.biomeBlendRadius.onChange([this](i32 value) {
        spdlog::info("Biome blend radius changed to: {} ({}x{} area)", value, value * 2 + 1, value * 2 + 1);
        ChunkMesher::setBiomeBlendRadius(value);
    });

    // 粒子效果模式变更
    m_settings.particles.onChange([this](u8 value) {
        auto mode = static_cast<client::ParticleMode>(value);
        spdlog::info("Particle mode changed to: {}", static_cast<i32>(mode));
        if (m_renderer && m_renderer->isParticleManagerInitialized()) {
            m_renderer->particleManager().setParticleMode(mode);
        }
    });

    m_settings.antiAliasing.onChange(
        [](bool enabled) { spdlog::info("Anti-aliasing changed to: {} (restart required)", enabled); });
}

void ClientApplication::applyGuiScale()
{
    m_guiScaleState = ui::calculateGuiScale(
        m_settings.guiScale.get(), static_cast<i32>(m_window.width()), static_cast<i32>(m_window.height()));

    if (m_renderer) {
        m_renderer->setGuiScaleFactor(static_cast<f64>(m_guiScaleState.scaleFactor));
    }

    if (m_canvas) {
        m_canvas->resize(m_guiScaleState.width, m_guiScaleState.height);
    }

    if (m_kageroEngine) {
        m_kageroEngine->resize(m_guiScaleState.width, m_guiScaleState.height);
    }
}

} // namespace mc::client
