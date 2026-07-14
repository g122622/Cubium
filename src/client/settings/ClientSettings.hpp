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

#include "common/core/settings/ResourcePackListOption.hpp"
#include "common/core/settings/SettingsBase.hpp"
#include "common/core/settings/SettingsTypes.hpp"
#include "common/input/KeyBinding.hpp"
#include "common/sound/SoundCategory.hpp"

#include <memory>
#include <vector>

namespace mc::client {

/**
 * @brief 图形质量模式
 */
enum class GraphicsMode : u8 {
    Fast = 0, // 快速模式（简化渲染）
    Fancy = 1 // 精致模式（完整渲染）
};

/**
 * @brief 云渲染模式
 */
enum class CloudMode : u8 {
    Off = 0,  // 关闭
    Fast = 1, // 快速（2D）
    Fancy = 2 // 精致（3D）
};

/**
 * @brief 粒子效果模式
 *
 * 控制客户端粒子渲染质量：
 * - Minimal: 仅显示重要粒子（overrideLimiter=true 的粒子），ambient 粒子被完全跳过
 * - Decreased: 约 2/3 的普通粒子通过（每帧 1/3 概率降级为 Minimal 行为）
 * - All: 显示所有粒子
 */
enum class ParticleMode : u8 {
    Minimal = 0,   // 最小
    Decreased = 1, // 减少
    All = 2        // 全部
};

/**
 * @brief 环境光遮蔽（AO）模式
 */
enum class AmbientOcclusionMode : u8 {
    Off = 0, // 关闭 - 使用平面光照
    Min = 1, // 最小 - 较低质量的 AO
    Max = 2  // 最大 - 最高质量的 AO（默认）
};

/**
 * @brief 客户端设置类
 *
 * 管理客户端所有设置项，包括视频、音频、控制和游戏设置。
 * 设置自动保存到文件，支持热重载。
 *
 * 使用示例:
 * @code
 * ClientSettings settings;
 * settings.load("options.json");
 *
 * // 访问设置
 * int distance = settings.renderDistance.get();
 * settings.fullscreen.set(true);
 *
 * // 设置变更回调
 * settings.renderDistance.onChange([](int value) {
 *     spdlog::info("Render distance: {}", value);
 * });
 *
 * // 按键绑定
 * KeyBinding* forward = ClientSettings::getKeyBinding("key.forward");
 * if (forward && forward->isPressed()) {
 *     player.moveForward();
 * }
 * @endcode
 */
class ClientSettings : public SettingsBase {
public:
    ClientSettings();
    ~ClientSettings() override = default;

    // 禁止拷贝
    ClientSettings(const ClientSettings&) = delete;
    ClientSettings& operator=(const ClientSettings&) = delete;

    // 允许移动
    ClientSettings(ClientSettings&&) = default;
    ClientSettings& operator=(ClientSettings&&) = default;

    // ========================================================================
    // 视频设置
    // ========================================================================

    /// 渲染距离（区块数），范围 2-32
    RangeOption renderDistance;

    /// 帧率限制，0 表示无限制
    RangeOption framerateLimit;

    /// GUI 缩放，0 表示自动
    RangeOption guiScale;

    /// 全屏模式
    BooleanOption fullscreen;

    /// 垂直同步
    BooleanOption vsync;

    /// 图形模式
    EnumOption<u8> graphics;

    /// 云渲染模式
    EnumOption<u8> clouds;

    /// Mipmap 等级，范围 0-4
    RangeOption mipmapLevels;

    /// FOV（视野）效果强度
    FloatOption fovEffectScale;

    /// 屏幕抖动强度
    FloatOption screenShakeScale;

    /// 受伤屏幕倾斜强度 (0.0-1.0, 默认 1.0)。options.damageTiltStrength，
    /// 控制 damageTilt（bobHurt）的 Z 轴旋转幅度。
    FloatOption damageTiltStrength;

    /// 雾效果密度倍率 (0.0-2.0, 默认 1.0)
    /// 1.0 = 标准雾效果, 0.0 = 禁用雾, >1.0 = 更浓的雾
    FloatOption fogDensity;

    /// 环境光遮蔽（AO）模式
    /// Off: 平面光照（每个面使用统一光照）
    /// Min: 最小 AO（较低质量）
    /// Max: 最大 AO（最高质量，平滑光照）
    EnumOption<u8> ambientOcclusion;

    /// 粒子效果模式
    /// Minimal: 仅显示重要粒子
    /// Decreased: 减少普通粒子密度（约 2/3 通过）
    /// All: 显示所有粒子
    EnumOption<u8> particles;

    /// 生物群系颜色混合半径（0-7）
    /// 0: 无混合（直接使用当前生物群系颜色）
    /// 2: 5x5 混合区域（默认，MC 默认值）
    /// 7: 15x15 混合区域（最大）
    /// 更大的值会产生更平滑的生物群系边界，但性能消耗更高
    RangeOption biomeBlendRadius;

    /// 抗锯齿（MSAA）开关
    BooleanOption antiAliasing;

    // ========================================================================
    // 音频设置
    // ========================================================================

    /// 主音量
    FloatOption masterVolume;

    /// 音乐音量
    FloatOption musicVolume;

    /// 唱片机音量
    FloatOption recordVolume;

    /// 天气音量
    FloatOption weatherVolume;

    /// 方块音量
    FloatOption blockVolume;

    /// 敌对生物音量
    FloatOption hostileVolume;

    /// 中立生物音量
    FloatOption neutralVolume;

    /// 玩家音量
    FloatOption playerVolume;

    /// 环境音效音量
    FloatOption ambientVolume;

    /// 语音音量
    FloatOption voiceVolume;

    /// UI 界面音量（按钮点击、菜单操作等）
    FloatOption uiVolume;

    /**
     * @brief 获取指定类别的音量
     *
     * @param category 声音类别
     * @return 音量值 (0.0-1.0)
     */
    [[nodiscard]] f32 getVolumeForCategory(sound::SoundCategory category) const;

    /**
     * @brief 设置指定类别的音量
     *
     * @param category 声音类别
     * @param volume 音量值 (0.0-1.0)
     */
    void setVolumeForCategory(sound::SoundCategory category, f32 volume);

    // ========================================================================
    // 控制设置
    // ========================================================================

    /// 鼠标灵敏度
    FloatOption mouseSensitivity;

    /// 反转鼠标 Y 轴
    BooleanOption invertMouse;

    /// 原始鼠标输入
    BooleanOption rawMouseInput;

    /// 鼠标滚轮灵敏度
    FloatOption mouseWheelSensitivity;

    /// 自动跳跃
    BooleanOption autoJump;

    // ========================================================================
    // 游戏设置
    // ========================================================================

    /// 视角摇晃
    BooleanOption viewBobbing;

    /// 视野（FOV），范围 30-110
    FloatOption fov;

    /// 显示 FPS
    BooleanOption showFps;

    /// 显示调试屏幕
    BooleanOption showDebug;

    /// 语言代码
    StringOption language;

    // ========================================================================
    // 网络设置
    // ========================================================================

    /// 服务器地址
    StringOption serverAddress;

    /// 服务器端口
    RangeOption serverPort;

    /// 玩家名称
    StringOption username;

    // ========================================================================
    // 日志设置
    // ========================================================================

    /// 日志级别
    StringOption logLevel;

    // ========================================================================
    // 资源包设置
    // ========================================================================

    /// 资源包列表（按优先级排序，高优先级在前）
    ResourcePackListOption resourcePacks;

    // ========================================================================
    // 按键绑定
    // ========================================================================

    /**
     * @brief 初始化按键绑定
     *
     * 创建所有默认按键绑定。需要在使用按键绑定前调用。
     */
    void initializeKeyBindings();

    /**
     * @brief 获取按键绑定
     * @param id 绑定 ID（如 "key.forward"）
     * @return 按键绑定指针，找不到返回 nullptr
     */
    [[nodiscard]] static KeyBinding* getKeyBinding(const std::string& id);

    /**
     * @brief 获取所有按键绑定
     */
    [[nodiscard]] static const std::vector<std::unique_ptr<KeyBinding>>& getAllKeyBindings();

    /**
     * @brief 生成默认客户端配置文件
     *
     * 将所有选项重置为默认值并保存到指定路径。
     * 当配置文件不存在时由 loadOrGenerate() 调用。
     *
     * @param path 配置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> generateDefaultConfig(const std::filesystem::path& path) override;

    // ========================================================================
    // 加载/保存
    // ========================================================================

    /**
     * @brief 加载设置（包括按键绑定）
     * @param path 设置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadSettings(const std::filesystem::path& path);

    /**
     * @brief 保存设置（包括按键绑定）
     * @param path 设置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> saveSettings(const std::filesystem::path& path);

private:
    // 按键绑定存储
    static std::vector<std::unique_ptr<KeyBinding>> s_keyBindings;
};

} // namespace mc::client
