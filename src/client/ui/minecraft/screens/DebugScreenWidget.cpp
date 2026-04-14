#include "DebugScreenWidget.hpp"
#include "client/renderer/util/GpuInfo.hpp"
#include "common/util/PlatformInfo.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/time/GameTime.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/util/math/MathUtils.hpp"
#include "client/network/NetworkClient.hpp"
#include "client/renderer/Camera.hpp"
#include "client/world/ClientWorld.hpp"
#include "client/world/entity/ClientEntityManager.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "client/renderer/trident/sky/CelestialCalculations.hpp"
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>

namespace mc::client::ui::minecraft {

DebugScreenWidget::DebugScreenWidget()
    : Screen("debug_screen")
{
    setVisible(true);
    setActive(true);
}

void DebugScreenWidget::setGpuInfo(const DebugGpuInfo& info) {
    m_gpuVendor = std::string(info.vendor.begin(), info.vendor.end());
    m_gpuName = std::string(info.name.begin(), info.name.end());
    m_gpuDriverVersion = std::string(info.driverVersion.begin(), info.driverVersion.end());
    m_dedicatedVideoMB = info.dedicatedVideoMB;
    m_sharedSystemMB = info.sharedSystemMB;
    m_apiMajorVersion = info.apiMajorVersion;
    m_apiMinorVersion = info.apiMinorVersion;
    m_gpuInfoSet = true;
}

void DebugScreenWidget::tick(f32 dt) {
    if (!isVisible()) return;

    updateFps(dt);
    updateSystemInfo();
    buildLeftDebugText();
    buildRightDebugText();
    measureTexts();
}

void DebugScreenWidget::updateFps(f32 dt) {
    m_frameTime = dt;
    m_fpsAccumulator += dt;
    m_frameCount++;

    m_fpsUpdateTimer += dt;
    if (m_fpsUpdateTimer >= FPS_UPDATE_INTERVAL) {
        m_fps = static_cast<f32>(m_frameCount) / m_fpsAccumulator;
        m_fpsAccumulator = 0.0f;
        m_frameCount = 0;
        m_fpsUpdateTimer = 0.0f;
    }
}

void DebugScreenWidget::updateSystemInfo() {
    m_systemInfoTimer += m_frameTime;
    if (m_systemInfoTimer >= SYSTEM_INFO_UPDATE_INTERVAL) {
        m_systemInfoTimer = 0.0f;
        auto memInfo = util::PlatformInfo::getMemoryInfo();
        m_processMemoryMB = memInfo.processUsedMB;
        m_memoryPercent = memInfo.usagePercent;
        m_totalMemoryMB = memInfo.totalPhysicalMB;
    }
}

void DebugScreenWidget::buildLeftDebugText() {
    m_leftLines.clear();

    std::ostringstream oss;

    // F3 显示玩家真实坐标，不要把视角摇晃的相机偏移混进来。
    const bool hasPlayer = m_player != nullptr;
    const bool hasCamera = m_camera != nullptr;

    // 版本信息
    oss.str("");
    oss << m_version << " (" << m_version << "/" << m_rendererInfo << ")";
    m_leftLines.push_back(oss.str());

    // FPS 信息
    oss.str("");
    oss << std::fixed << std::setprecision(0) << m_fps << " fps"
        << " T: " << std::setprecision(1) << (m_frameTime * 1000.0f) << " ms";
    m_leftLines.push_back(oss.str());

    // 服务器信息
    if (m_networkClient != nullptr) {
        oss.str("");
        oss << "Integrated server @ " << std::fixed << std::setprecision(0)
            << m_serverTickTimeMs << " ms ticks, "
            << m_networkClient->packetsSent() << " tx, "
            << m_networkClient->packetsReceived() << " rx";
        m_leftLines.push_back(oss.str());
    } else {
        m_leftLines.push_back("Server: local (integrated)");
    }

    // 渲染统计
    if (m_world != nullptr) {
        oss.str("");
        oss << "ChunkCount: 0/" << m_world->chunkCount()
            << ", RenderDistance: " << m_renderDistance
            << ", LightUpdates: 0"
            << ", EntityCount: " << (m_entityManager ? m_entityManager->entityCount() : 0);
        m_leftLines.push_back(oss.str());
    }

    // 粒子统计
    oss.str("");
    oss << "Particles: 0, Entities: " << (m_entityManager ? m_entityManager->entityCount() : 0);
    m_leftLines.push_back(oss.str());

    // 维度信息
    m_leftLines.push_back("Dimension: minecraft:overworld");
    m_leftLines.push_back("minecraft:overworld ForcedChunks: 0");
    m_leftLines.push_back("");

    // 位置信息
    if (hasPlayer || hasCamera) {
        f64 posX = 0.0;
        f64 posY = 0.0;
        f64 posZ = 0.0;
        f32 yaw = 0.0f;
        f32 pitch = 0.0f;

        if (hasPlayer) {
            const Vector3 playerPos = m_player->position();
            posX = static_cast<f64>(playerPos.x);
            posY = static_cast<f64>(playerPos.y);
            posZ = static_cast<f64>(playerPos.z);
            yaw = m_player->yaw();
            pitch = m_player->pitch();
        } else {
            const auto& cameraPos = m_camera->position();
            posX = cameraPos.x;
            posY = cameraPos.y;
            posZ = cameraPos.z;

            const auto& cameraRot = m_camera->rotation();
            yaw = static_cast<f32>(cameraRot.y);
            pitch = static_cast<f32>(cameraRot.x);
        }

        oss.str("");
        oss << "XYZ: " << std::fixed << std::setprecision(3)
            << posX << " / " << posY << " / " << posZ;
        m_leftLines.push_back(oss.str());

        i32 blockX = static_cast<i32>(std::floor(posX));
        i32 blockY = static_cast<i32>(std::floor(posY));
        i32 blockZ = static_cast<i32>(std::floor(posZ));
        oss.str("");
        oss << "Block: " << blockX << " " << blockY << " " << blockZ;
        m_leftLines.push_back(oss.str());

        i32 chunkX = blockX >> 4;
        i32 chunkY = blockY >> 4;
        i32 chunkZ = blockZ >> 4;
        i32 relX = blockX & 15;
        i32 relY = blockY & 15;
        i32 relZ = blockZ & 15;
        oss.str("");
        oss << "Chunk: " << relX << " " << relY << " " << relZ
            << " in " << chunkX << " " << chunkY << " " << chunkZ;
        m_leftLines.push_back(oss.str());

        auto [dirName, dirDesc] = getFacingDirection(yaw);
        oss.str("");
        oss << "Facing: " << dirName << " (" << dirDesc << ")"
            << " (" << std::fixed << std::setprecision(1)
            << yaw << " / " << pitch << ")";
        m_leftLines.push_back(oss.str());

        // 光照信息
        if (m_world != nullptr) {
            bool blockLoaded = m_world->getChunkAt(chunkX, chunkZ) != nullptr;

            if (blockLoaded) {
                u8 skyLight = m_world->getSkyLight(blockX, blockY, blockZ);
                u8 blockLight = m_world->getBlockLight(blockX, blockY, blockZ);
                u8 totalLight = std::max(skyLight, blockLight);
                oss.str("");
                oss << "Client Light: " << static_cast<i32>(totalLight)
                    << " (" << static_cast<i32>(skyLight) << " sky, "
                    << static_cast<i32>(blockLight) << " block)";
                m_leftLines.push_back(oss.str());
                m_leftLines.push_back("Server Light: (?? sky, ?? block)");
            } else {
                m_leftLines.push_back("Outside of world...");
            }

            // 生物群系
            if (blockLoaded && blockY >= 0 && blockY < 256) {
                if (const Biome* biome = m_world->getBiomeAtBlock(blockX, blockY, blockZ)) {
                    oss.str("");
                    oss << "Biome: minecraft:" << biome->name();
                    m_leftLines.push_back(oss.str());

                    oss.str("");
                    oss << "Climate: T=" << std::fixed << std::setprecision(2)
                        << biome->temperature()
                        << " H=" << biome->humidity()
                        << " C=" << biome->continentalness();
                    m_leftLines.push_back(oss.str());
                }

                // 时间信息
                i64 gameTime = m_world->gameTime();
                i64 dayCount = gameTime / 24000;
                i32 moonPhase = CelestialCalculations::calculateMoonPhase(gameTime);
                oss.str("");
                oss << "Local Difficulty: 0.00 // 0.00 (Day " << dayCount << ", Moon " << moonPhase << ")";
                m_leftLines.push_back(oss.str());
            }
        } else {
            m_leftLines.push_back("Outside of world...");
        }
    } else {
        m_leftLines.push_back("No camera");
    }

    m_leftLines.push_back("");
    m_leftLines.push_back("Debug: Pie [shift]: hidden FPS + TPS [alt]: hidden");
    m_leftLines.push_back("For help: press F3 + Q");

    // 种子信息
    if (m_world != nullptr) {
        std::ostringstream seedOss;
        seedOss << "Seed: " << m_world->seed();
        m_leftLines.push_back(seedOss.str());
    }
}

void DebugScreenWidget::buildRightDebugText() {
    m_rightLines.clear();

    std::ostringstream oss;

    // 语言/编译器信息
    oss.str("");
    oss << "C++17 " << (util::PlatformInfo::is64BitSystem() ? "64bit" : "32bit");
    m_rightLines.push_back(oss.str());

    // 内存信息
    oss.str("");
    oss << "Mem: " << std::setw(3) << m_memoryPercent << "% "
        << std::setw(4) << m_processMemoryMB << "/"
        << std::setw(4) << m_totalMemoryMB << "MB";
    m_rightLines.push_back(oss.str());

    oss.str("");
    f32 allocatedPercent = m_totalMemoryMB > 0
        ? (static_cast<f32>(m_processMemoryMB) / m_totalMemoryMB) * 100.0f
        : 0.0f;
    oss << "Allocated: " << std::setw(3) << static_cast<i32>(allocatedPercent) << "%";
    m_rightLines.push_back(oss.str());

    m_rightLines.push_back("");

    // CPU 信息
    auto cpuInfo = util::PlatformInfo::getCpuInfo();
    oss.str("");
    if (!cpuInfo.brand.empty()) {
        std::string cpuName = cpuInfo.brand;
        if (cpuName.length() > 40) {
            cpuName = cpuName.substr(0, 37) + "...";
        }
        oss << "CPU: " << cpuName;
    } else if (!cpuInfo.vendor.empty()) {
        oss << "CPU: " << cpuInfo.vendor << " " << cpuInfo.coreCount << " cores";
    } else {
        oss << "CPU: " << cpuInfo.coreCount << " cores";
    }
    m_rightLines.push_back(oss.str());

    m_rightLines.push_back("");

    // 显示器和GPU信息
    oss.str("");
    oss << "Display: " << static_cast<i32>(width()) << "x" << static_cast<i32>(height());
    if (m_gpuInfoSet && !m_gpuVendor.empty()) {
        oss << " (" << m_gpuVendor << ")";
    }
    m_rightLines.push_back(oss.str());

    if (m_gpuInfoSet && !m_gpuName.empty()) {
        m_rightLines.push_back(m_gpuName);
    } else {
        m_rightLines.push_back(m_rendererInfo);
    }

    if (m_gpuInfoSet) {
        oss.str("");
        oss << "Vulkan " << m_apiMajorVersion << "." << m_apiMinorVersion;
        if (!m_gpuDriverVersion.empty()) {
            oss << " (Driver: " << m_gpuDriverVersion << ")";
        }
        m_rightLines.push_back(oss.str());
    } else {
        m_rightLines.push_back("Vulkan 1.x");
    }

    // 目标方块信息
    if (m_targetBlock != nullptr && !m_targetBlock->isMiss()) {
        const auto& blockPos = m_targetBlock->blockPos();

        m_rightLines.push_back("");
        oss.str("");
        oss << "Targeted Block: " << blockPos.x << ", " << blockPos.y << ", " << blockPos.z;
        m_rightLines.push_back(oss.str());

        if (m_world != nullptr) {
            const BlockState* state = m_world->getBlockState(blockPos.x, blockPos.y, blockPos.z);
            if (state != nullptr) {
                const ResourceLocation& loc = state->blockLocation();
                oss.str("");
                oss << loc.toString();
                m_rightLines.push_back(oss.str());
            }
        }
    }
}

std::pair<std::string, std::string> DebugScreenWidget::getFacingDirection(f32 yaw) const {
    yaw = math::wrapDegrees(yaw);

    if (yaw >= -45.0f && yaw < 45.0f) {
        return {"South", "Towards positive Z"};
    } else if (yaw >= 45.0f && yaw < 135.0f) {
        return {"West", "Towards negative X"};
    } else if (yaw >= 135.0f || yaw < -135.0f) {
        return {"North", "Towards negative Z"};
    } else {
        return {"East", "Towards positive X"};
    }
}

void DebugScreenWidget::measureTexts() {
    m_leftMaxWidth = 0.0f;
    m_rightMaxWidth = 0.0f;

    for (const auto& line : m_leftLines) {
        if (!line.empty() && m_textWidthCallback) {
            f32 width = m_textWidthCallback(line);
            m_leftMaxWidth = std::max(m_leftMaxWidth, width);
        }
    }

    for (const auto& line : m_rightLines) {
        if (!line.empty() && m_textWidthCallback) {
            f32 width = m_textWidthCallback(line);
            m_rightMaxWidth = std::max(m_rightMaxWidth, width);
        }
    }
}

void DebugScreenWidget::paint(kagero::widget::PaintContext& ctx) {
    if (!isVisible()) return;
    if (m_leftLines.empty() && m_rightLines.empty()) return;

    const f32 screenWidth = static_cast<f32>(width());
    const f32 screenHeight = static_cast<f32>(height());

    // 绘制左侧面板
    if (!m_leftLines.empty()) {
        i32 leftBgWidth = static_cast<i32>(m_leftMaxWidth + 10.0f);
        i32 leftBgHeight = static_cast<i32>(m_leftLines.size() * m_lineHeight + 4);

        ctx.drawFilledRect(kagero::Rect(1, 1, leftBgWidth, leftBgHeight), BG_COLOR);

        i32 y = 2;
        for (const auto& line : m_leftLines) {
            if (!line.empty()) {
                ctx.drawText(line, 3, y + 1, SHADOW_COLOR);
                ctx.drawText(line, 2, y, TEXT_COLOR);
            }
            y += m_lineHeight;
        }
    }

    // 绘制右侧面板
    if (!m_rightLines.empty()) {
        i32 rightBgWidth = static_cast<i32>(m_rightMaxWidth + 10.0f);
        i32 rightBgHeight = static_cast<i32>(m_rightLines.size() * m_lineHeight + 4);
        i32 rightBgX = static_cast<i32>(screenWidth - m_rightMaxWidth - 12.0f);

        ctx.drawFilledRect(kagero::Rect(rightBgX, 1, rightBgWidth, rightBgHeight), BG_COLOR);

        i32 y = 2;
        for (const auto& line : m_rightLines) {
            if (!line.empty()) {
                f32 textWidth = 0.0f;
                if (m_textWidthCallback) {
                    textWidth = m_textWidthCallback(line);
                }
                i32 x = static_cast<i32>(screenWidth - textWidth - 4.0f);

                ctx.drawText(line, x + 1, y + 1, SHADOW_COLOR);
                ctx.drawText(line, x, y, TEXT_COLOR);
            }
            y += m_lineHeight;
        }
    }
}

void DebugScreenWidget::setTextWidthCallback(TextWidthCallback callback) {
    m_textWidthCallback = std::move(callback);
}

} // namespace mc::client::ui::minecraft
