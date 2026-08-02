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

#pragma once

#include "common/core/Types.hpp"

namespace mc::defaults {

// ============================================================================
// 游戏目录默认值
// ============================================================================
namespace game {
inline constexpr const char* gameDirectoryName = "minecraft_reborn";
inline constexpr const char* clientOptionsFile = "client_options.json";
inline constexpr const char* serverOptionsFile = "server_options.json";
inline constexpr const char* resourcePacksDirName = "resourcepacks";
inline constexpr const char* dataPacksDirName = "datapacks";
inline constexpr const char* savesDirName = "saves";
inline constexpr const char* backupsDirName = "backups";
inline constexpr const char* logsDirName = "logs";
inline constexpr const char* cacheDirName = "cache";
inline constexpr const char* builtinDirName = "builtin";
} // namespace game

// ============================================================================
// 客户端默认值
// ============================================================================
namespace client {
// 视频
inline constexpr i32 renderDistance = 12;
inline constexpr i32 framerateLimit = 120;
inline constexpr i32 guiScale = 0;
inline constexpr bool fullscreen = false;
inline constexpr bool vsync = true;
inline constexpr i32 mipmapLevels = 4;
inline constexpr f32 fovEffectScale = 1.0f;
inline constexpr f32 screenShakeScale = 1.0f;
inline constexpr f32 fogDensity = 1.0f;
inline constexpr i32 biomeBlendRadius = 2;
inline constexpr bool antiAliasing = true;
inline constexpr f32 damageTiltStrength = 1.0f;

// 音频
inline constexpr f32 masterVolume = 1.0f;
inline constexpr f32 musicVolume = 0.5f;
inline constexpr f32 recordVolume = 1.0f;
inline constexpr f32 weatherVolume = 1.0f;
inline constexpr f32 blockVolume = 1.0f;
inline constexpr f32 hostileVolume = 1.0f;
inline constexpr f32 neutralVolume = 1.0f;
inline constexpr f32 playerVolume = 1.0f;
inline constexpr f32 ambientVolume = 1.0f;
inline constexpr f32 voiceVolume = 1.0f;
inline constexpr f32 uiVolume = 1.0f;

// 控制
inline constexpr f32 mouseSensitivity = 0.5f;
inline constexpr bool invertMouse = false;
inline constexpr bool rawMouseInput = true;
inline constexpr f32 mouseWheelSensitivity = 1.0f;
inline constexpr bool autoJump = false;
inline constexpr bool viewBobbing = true;

// 游戏
inline constexpr f32 fov = 70.0f;
inline constexpr bool showFps = false;
inline constexpr bool showDebug = false;
inline constexpr const char* language = "zh_cn";

// 网络
inline constexpr const char* serverAddress = "127.0.0.1";
inline constexpr u16 serverPort = 25565;
inline constexpr const char* username = "Player";

// 窗口
inline constexpr i32 windowWidth = 1280;
inline constexpr i32 windowHeight = 720;
inline constexpr const char* windowTitle = "Cubium";
inline constexpr i32 windowedX = 100;
inline constexpr i32 windowedY = 100;

// 日志
inline constexpr const char* logLevel = "info";
} // namespace client

// ============================================================================
// 服务端默认值
// ============================================================================
namespace server {
// 网络
inline constexpr u16 serverPort = 25565;
inline constexpr const char* bindAddress = "0.0.0.0";
inline constexpr i32 maxPlayers = 20;
inline constexpr bool onlineMode = false;
inline constexpr const char* motd = "A Cubium Server";
inline constexpr bool p2pEnabled = false;

// 世界
inline constexpr const char* worldName = "world";
inline constexpr const char* levelName = "Cubium Server";
inline constexpr const char* levelSeed = "";
inline constexpr bool generateStructures = true;
inline constexpr bool enableCommandBlock = false;

// 游戏
inline constexpr const char* defaultGameMode = "survival";
inline constexpr const char* difficulty = "normal";
inline constexpr bool hardcore = false;
inline constexpr bool pvpEnabled = true;
inline constexpr bool allowFlight = false;
inline constexpr i32 playerIdleTimeout = 0;

// 性能
inline constexpr i32 tickRate = 20;
inline constexpr i32 viewDistance = 10;
inline constexpr i32 simulationDistance = 10;
inline constexpr i32 maxEntitiesPerChunk = 128;
inline constexpr i32 chunkLoadRate = 16;

// 安全
inline constexpr bool whiteList = false;
inline constexpr bool blackList = true;
inline constexpr i32 maxTickTime = 60000;
inline constexpr i32 maxPacketSize = 2097152;

// 日志
inline constexpr const char* serverLogLevel = "info";
inline constexpr bool logToFile = false;
inline constexpr const char* logFile = "server.log";
inline constexpr bool debugLogging = false;
} // namespace server

// ============================================================================
// 服务器核心默认值
// ============================================================================
namespace serverCore {
inline constexpr u64 tickDurationMs = 50;
inline constexpr u64 cleanupIntervalTicks = 100;
} // namespace serverCore

// ============================================================================
// 集成服务器默认值
// ============================================================================
namespace integratedServer {
inline constexpr const char* worldName = "singleplayer";
inline constexpr i64 seed = 0;
inline constexpr const char* defaultGameMode = "survival";
inline constexpr i32 viewDistance = 6;
inline constexpr i32 tickRate = 20;
inline constexpr const char* worldType = "default";
} // namespace integratedServer

} // namespace mc::defaults
