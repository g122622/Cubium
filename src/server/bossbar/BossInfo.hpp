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
#include "common/util/text/ITextComponentFwd.hpp"
#include <memory>
#include <string>

namespace mc {
namespace server {

/**
 * @brief Boss 信息颜色枚举
 *
 * 定义 Boss 栏的颜色选项。
 * 参考 MC 1.16.5: net.minecraft.world.BossInfo.Color
 */
enum class BossInfoColor : u8 {
    Pink = 0,   // 粉色
    Blue = 1,   // 蓝色
    Red = 2,    // 红色
    Green = 3,  // 绿色
    Yellow = 4, // 黄色
    Purple = 5, // 紫色
    White = 6,  // 白色
};

/**
 * @brief Boss 信息样式枚举
 *
 * 定义 Boss 栏的样式选项（进度条外观）。
 * 参考 MC 1.16.5: net.minecraft.world.BossInfo.Overlay
 */
enum class BossInfoOverlay : u8 {
    Progress = 0,  // 平滑进度条
    Notched6 = 1,  // 6 分割
    Notched10 = 2, // 10 分割
    Notched12 = 3, // 12 分割
    Notched20 = 4, // 20 分割
};

/**
 * @brief 从名称获取颜色
 *
 * @param name 颜色名称（如 "pink", "blue" 等）
 * @return 对应的颜色枚举，默认返回 White
 */
BossInfoColor bossInfoColorFromName(const std::string& name);

/**
 * @brief 获取颜色名称
 *
 * @param color 颜色枚举
 * @return 颜色名称字符串
 */
std::string bossInfoColorToName(BossInfoColor color);

/**
 * @brief 从名称获取样式
 *
 * @param name 样式名称（如 "progress", "notched_6" 等）
 * @return 对应的样式枚举，默认返回 Progress
 */
BossInfoOverlay bossInfoOverlayFromName(const std::string& name);

/**
 * @brief 获取样式名称
 *
 * @param overlay 样式枚举
 * @return 样式名称字符串
 */
std::string bossInfoOverlayToName(BossInfoOverlay overlay);

/**
 * @brief Boss 信息基类
 *
 * 定义 Boss 栏的核心属性：唯一ID、名称、百分比、颜色、样式、标志位。
 * 参考 MC 1.16.5: net.minecraft.world.BossInfo
 */
class BossInfo {
public:
    /**
     * @brief 构造函数
     *
     * @param uuid 唯一标识符
     * @param name 显示名称（文本组件）
     * @param color 颜色
     * @param overlay 样式
     */
    BossInfo(u64 uuid, std::unique_ptr<text::ITextComponent> name, BossInfoColor color, BossInfoOverlay overlay);

    /**
     * @brief 虚析构函数
     */
    virtual ~BossInfo() = default;

    // 禁止拷贝
    BossInfo(const BossInfo&) = delete;
    BossInfo& operator=(const BossInfo&) = delete;

    // 允许移动
    BossInfo(BossInfo&&) noexcept = default;
    BossInfo& operator=(BossInfo&&) noexcept = default;

    // ========== 属性访问器 ==========

    /**
     * @brief 获取唯一标识符
     */
    [[nodiscard]] u64 uuid() const noexcept { return m_uuid; }

    /**
     * @brief 获取显示名称
     */
    [[nodiscard]] const text::ITextComponent& name() const noexcept { return *m_name; }

    /**
     * @brief 设置显示名称
     *
     * @param name 新的显示名称
     */
    virtual void setName(std::unique_ptr<text::ITextComponent> name);

    /**
     * @brief 获取生命值百分比 (0.0 ~ 1.0)
     */
    [[nodiscard]] f32 percent() const noexcept { return m_percent; }

    /**
     * @brief 设置生命值百分比
     *
     * @param percent 新的百分比 (会被 clamp 到 0.0 ~ 1.0)
     */
    virtual void setPercent(f32 percent);

    /**
     * @brief 获取颜色
     */
    [[nodiscard]] BossInfoColor color() const noexcept { return m_color; }

    /**
     * @brief 设置颜色
     *
     * @param color 新的颜色
     */
    virtual void setColor(BossInfoColor color);

    /**
     * @brief 获取样式
     */
    [[nodiscard]] BossInfoOverlay overlay() const noexcept { return m_overlay; }

    /**
     * @brief 设置样式
     *
     * @param overlay 新的样式
     */
    virtual void setOverlay(BossInfoOverlay overlay);

    /**
     * @brief 是否变暗天空
     */
    [[nodiscard]] bool darkenSky() const noexcept { return m_darkenSky; }

    /**
     * @brief 设置是否变暗天空
     *
     * @param darken 是否变暗
     */
    virtual void setDarkenSky(bool darken);

    /**
     * @brief 是否播放末影龙 Boss 音乐
     */
    [[nodiscard]] bool playEndBossMusic() const noexcept { return m_playEndBossMusic; }

    /**
     * @brief 设置是否播放末影龙 Boss 音乐
     *
     * @param play 是否播放
     */
    virtual void setPlayEndBossMusic(bool play);

    /**
     * @brief 是否创建迷雾
     */
    [[nodiscard]] bool createFog() const noexcept { return m_createFog; }

    /**
     * @brief 设置是否创建迷雾
     *
     * @param create 是否创建
     */
    virtual void setCreateFog(bool create);

    /**
     * @brief 是否可见
     */
    [[nodiscard]] bool visible() const noexcept { return m_visible; }

    /**
     * @brief 设置是否可见
     *
     * @param visible 是否可见
     */
    virtual void setVisible(bool visible);

protected:
    u64 m_uuid;
    std::unique_ptr<text::ITextComponent> m_name;
    f32 m_percent = 1.0f;
    BossInfoColor m_color;
    BossInfoOverlay m_overlay;
    bool m_darkenSky = false;
    bool m_playEndBossMusic = false;
    bool m_createFog = false;
    bool m_visible = true;
};

} // namespace server
} // namespace mc
