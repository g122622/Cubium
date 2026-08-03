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

#include "Screen.hpp"
#include "client/renderer/util/GpuInfo.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace mc {
class Player;
class BlockRaycastResult;
class ClientDimensionManager;
} // namespace mc

namespace mc::client {
class Camera;
class ClientWorld;
class ClientEntityManager;
namespace net {
class ClientNetwork;
} // namespace net
} // namespace mc::client

namespace mc::client::ui::minecraft {

/**
 * @brief 调试屏幕Widget
 *
 * 使用 kagero UI 引擎渲染 Minecraft F3 调试屏幕。
 * 显示左侧和右侧两个面板，包含游戏状态、系统信息等调试数据。
 */
class DebugScreenWidget : public Screen {
public:
    /**
     * @brief 文本宽度测量回调类型
     * @param text 要测量宽度的文本
     * @return 文本宽度（像素）
     */
    using TextWidthCallback = std::function<f32(const std::string& text)>;

    DebugScreenWidget();
    ~DebugScreenWidget() override = default;

    // ========== 数据提供者设置 ==========

    /**
     * @brief 设置相机
     */
    void setCamera(const mc::client::Camera* camera) { m_camera = camera; }

    /**
     * @brief 设置世界
     */
    void setWorld(const mc::client::ClientWorld* world) { m_world = world; }

    /**
     * @brief 设置实体管理器
     */
    void setEntityManager(const mc::client::ClientEntityManager* manager) { m_entityManager = manager; }

    /**
     * @brief 设置网络客户端
     */
    void setClientNetwork(const mc::client::net::ClientNetwork* client) { m_clientNetwork = client; }

    /**
     * @brief 设置目标方块
     */
    void setTargetBlock(const mc::BlockRaycastResult* result) { m_targetBlock = result; }

    /**
     * @brief 设置玩家
     */
    void setPlayer(const mc::Player* player) { m_player = player; }

    /**
     * @brief 设置维度管理器
     */
    void setDimensionManager(const mc::ClientDimensionManager* dm) { m_dimensionManager = dm; }

    /**
     * @brief 设置GPU信息
     */
    void setGpuInfo(const DebugGpuInfo& info);

    /**
     * @brief 设置渲染距离
     */
    void setRenderDistance(i32 distance) { m_renderDistance = distance; }

    /**
     * @brief 设置版本字符串
     */
    void setVersion(const std::string& version) { m_version = version; }

    /**
     * @brief 设置渲染器信息
     */
    void setRendererInfo(const std::string& info) { m_rendererInfo = info; }

    /**
     * @brief 设置服务端tick耗时
     */
    void setServerTickTimeMs(f32 ms) { m_serverTickTimeMs = ms; }

    /**
     * @brief 设置服务端目标每tick毫秒数
     */
    void setServerTargetMsPerTick(f32 ms) { m_serverTargetMsPerTick = ms; }

    /**
     * @brief 设置强制加载区块计数
     */
    void setForcedChunkCount(i32 count) { m_forcedChunkCount = count; }

    // ========== Widget接口 ==========

    /**
     * @brief 每帧更新（收集数据）
     */
    void tick(f32 dt) override;

    /**
     * @brief 绘制调试屏幕
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 设置文本宽度测量回调
     */
    void setTextWidthCallback(TextWidthCallback callback);

    /**
     * @brief 设置字体高度（行高）
     */
    void setLineHeight(i32 lineHeight) { m_lineHeight = lineHeight; }

private:
    /**
     * @brief 更新FPS统计
     */
    void _updateFps(f32 dt);

    /**
     * @brief 更新系统信息（每秒更新一次）
     */
    void _updateSystemInfo();

    /**
     * @brief 构建左侧调试文本
     */
    void _buildLeftDebugText();

    /**
     * @brief 构建右侧调试文本
     */
    void _buildRightDebugText();

    /**
     * @brief 获取方向名称
     */
    std::pair<std::string, std::string> _getFacingDirection(f32 yaw) const;

    /**
     * @brief 计算文本最大宽度
     */
    void _measureTexts();

    // ========== 数据提供者 ==========
    const mc::client::Camera* m_camera = nullptr;
    const mc::client::ClientWorld* m_world = nullptr;
    const mc::client::ClientEntityManager* m_entityManager = nullptr;
    const mc::client::net::ClientNetwork* m_clientNetwork = nullptr;
    const mc::BlockRaycastResult* m_targetBlock = nullptr;
    const mc::Player* m_player = nullptr;
    const mc::ClientDimensionManager* m_dimensionManager = nullptr;

    // ========== 调试数据 ==========
    std::string m_version = "Cubium 0.1.0";
    std::string m_rendererInfo = "Vulkan";
    i32 m_renderDistance = 12;

    // FPS统计
    f32 m_fps = 0.0f;
    f32 m_frameTime = 0.0f;
    f32 m_fpsAccumulator = 0.0f;
    u32 m_frameCount = 0;
    f32 m_fpsUpdateTimer = 0.0f;
    static constexpr f32 FPS_UPDATE_INTERVAL = 0.5f;

    // 系统信息更新定时器
    f32 m_systemInfoTimer = 0.0f;
    static constexpr f32 SYSTEM_INFO_UPDATE_INTERVAL = 1.0f;

    // GPU信息
    std::string m_gpuVendor;
    std::string m_gpuName;
    std::string m_gpuDriverVersion;
    u64 m_dedicatedVideoMB = 0;
    u64 m_sharedSystemMB = 0;
    u32 m_apiMajorVersion = 0;
    u32 m_apiMinorVersion = 0;
    bool m_gpuInfoSet = false;

    // 服务端信息
    f32 m_serverTickTimeMs = 0.0f;
    f32 m_serverTargetMsPerTick = 50.0f;
    i32 m_forcedChunkCount = 0;

    // 内存信息
    u64 m_processMemoryMB = 0;
    u64 m_processPeakMemoryMB = 0;
    u32 m_memoryPercent = 0;
    u64 m_totalMemoryMB = 0;

    // ========== 渲染数据 ==========
    std::vector<std::string> m_leftLines;
    std::vector<std::string> m_rightLines;
    f32 m_leftMaxWidth = 0.0f;
    f32 m_rightMaxWidth = 0.0f;
    i32 m_lineHeight = 11;
    TextWidthCallback m_textWidthCallback;

    // 颜色常量
    static constexpr u32 BG_COLOR = 0xA0303030;
    static constexpr u32 TEXT_COLOR = 0xFFFFFFFF;
    static constexpr u32 SHADOW_COLOR = 0xFF3F3F3F;
};

} // namespace mc::client::ui::minecraft
