/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the conditions:
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

#include "TemplateScreen.hpp"
#include "common/core/Types.hpp"
#include <functional>

namespace mc::client::ui::minecraft {

/**
 * @brief 游戏选项设置界面
 *
 * 提供游戏设置选项的骨架界面，使用模板驱动布局。
 * ESC 键或 Done 按钮可关闭界面返回上一级。
 *
 * 难度循环按钮与锁定按钮仅集成服可见（对齐 Java hasSingleplayerServer 门控），
 * 联机服由 setDifficultyControlsEnabled(false) 禁用。
 */
class OptionsScreen : public TemplateScreen {
public:
    OptionsScreen();

    /**
     * @brief 设置关闭回调
     * @param callback 关闭时调用的回调函数
     */
    void setOnClose(std::function<void()> callback) { m_onClose = std::move(callback); }

    /**
     * @brief 设置难度循环切换回调（点击难度按钮时触发，参数为切换后的新难度，由调用方发 sb:3）。
     */
    void setOnCycleDifficulty(std::function<void(Difficulty)> callback) { m_onCycleDifficulty = std::move(callback); }

    /**
     * @brief 设置难度锁定回调（点击锁定按钮时触发，由调用方发 sb:28）。
     */
    void setOnLockDifficulty(std::function<void()> callback) { m_onLockDifficulty = std::move(callback); }

    /**
     * @brief 设置当前难度并刷新按钮文案。
     */
    void setDifficulty(Difficulty difficulty);

    /**
     * @brief 设置难度锁定状态：锁定后禁用难度按钮与锁定按钮。
     */
    void setDifficultyLocked(bool locked);

    /**
     * @brief 启用/禁用难度控件（集成服启用，联机服禁用）。
     */
    void setDifficultyControlsEnabled(bool enabled);

    /**
     * @brief 处理键盘事件，ESC 键关闭界面
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

private:
    void _registerCallbacks();
    /// 刷新难度按钮文案与启用状态（依据 m_difficulty / m_difficultyLocked）
    void _refreshDifficultyButton();

    std::function<void()> m_onClose;
    std::function<void(Difficulty)> m_onCycleDifficulty;
    std::function<void()> m_onLockDifficulty;

    Difficulty m_difficulty = Difficulty::Normal;
    bool m_difficultyLocked = false;
    bool m_difficultyControlsEnabled = true;
};

} // namespace mc::client::ui::minecraft
