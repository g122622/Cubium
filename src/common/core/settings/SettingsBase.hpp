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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/core/settings/SettingsTypes.hpp"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

/**
 * @brief 设置基类
 *
 * 提供设置的加载、保存和管理功能。所有设置类（客户端设置、服务端设置）
 * 都应继承此类。
 *
 * 使用示例:
 * @code
 * class MySettings : public SettingsBase {
 * public:
 *     MySettings() {
 *         registerOption("fullscreen", &fullscreen);
 *         registerOption("renderDistance", &renderDistance);
 *     }
 *
 *     BooleanOption fullscreen{"fullscreen", fullscreenDefault};
 *     RangeOption renderDistance{"renderDistance", 2, 32, renderDistanceDefault};
 * };
 *
 * MySettings settings;
 * settings.load("settings.json");
 * settings.fullscreen.set(true);  // 自动保存
 * @endcode
 */
class SettingsBase {
public:
    SettingsBase() = default;
    virtual ~SettingsBase() = default;

    // 禁止拷贝
    SettingsBase(const SettingsBase&) = delete;
    SettingsBase& operator=(const SettingsBase&) = delete;

    // 允许移动
    SettingsBase(SettingsBase&&) = default;
    SettingsBase& operator=(SettingsBase&&) = default;

    /**
     * @brief 从文件加载设置，若文件不存在则生成默认配置后加载
     *
     * 如果指定路径的设置文件不存在，会调用 generateDefaultConfig() 生成默认配置文件，
     * 然后加载该文件。这确保用户首次启动时自动获得配置文件。
     *
     * @param path 设置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadOrGenerate(const std::filesystem::path& path);

    // ========================================================================
    // 文件操作
    // ========================================================================

    /**
     * @brief 从文件加载设置
     *
     * 加载 JSON 格式的设置文件。如果文件不存在，则使用当前值（默认值）。
     *
     * @param path 设置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> load(const std::filesystem::path& path);

    /**
     * @brief 保存设置到文件
     *
     * 将当前设置保存为 JSON 格式文件。如果目录不存在，会自动创建。
     *
     * @param path 设置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> save(const std::filesystem::path& path) const;

    /**
     * @brief 从 JSON 对象加载设置
     *
     * @param j JSON 对象
     */
    void loadFromJson(const nlohmann::json& j);

    /**
     * @brief 保存设置到 JSON 对象
     *
     * @param j JSON 对象引用
     */
    void saveToJson(nlohmann::json& j) const;

    // ========================================================================
    // 选项注册
    // ========================================================================

    /**
     * @brief 注册选项
     *
     * 将选项注册到设置系统中，使其能被自动序列化和反序列化。
     *
     * @param group 选项所属分组（如 "video", "audio", "control"）
     * @param option 选项指针
     */
    void registerOption(const std::string& group, IOption* option);

    /**
     * @brief 重置所有选项为默认值
     */
    void resetToDefaults();

    /**
     * @brief 重置指定分组的选项为默认值
     * @param group 分组名称
     */
    void resetGroupToDefaults(const std::string& group);

    // ========================================================================
    // 自动保存
    // ========================================================================

    /**
     * @brief 启用自动保存
     *
     * 启用后，任何设置变更都会自动保存到文件。
     *
     * @param path 设置文件路径
     */
    void enableAutoSave(std::filesystem::path path);

    /**
     * @brief 禁用自动保存
     */
    void disableAutoSave();

    /**
     * @brief 检查是否启用自动保存
     */
    [[nodiscard]] bool isAutoSaveEnabled() const { return m_autoSave; }

    /**
     * @brief 触发自动保存（如果有变更且启用了自动保存）
     */
    void triggerAutoSave() const;

protected:
    /**
     * @brief 生成默认配置文件
     *
     * 子类必须实现此方法以生成默认的配置文件内容。
     * 当 loadOrGenerate() 发现配置文件不存在时调用此方法。
     *
     * 实现应先将所有选项重置为默认值（resetToDefaults()），
     * 然后将当前设置保存到指定路径。
     *
     * @param path 配置文件路径
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> generateDefaultConfig(const std::filesystem::path& path) = 0;

    /**
     * @brief 通知设置变更
     *
     * 由子类调用，用于触发自动保存。
     */
    void onSettingChanged();

private:
    // 选项存储：分组 -> 选项列表
    std::map<std::string, std::vector<IOption*>> m_options;

    // 设置版本号（用于迁移）
    i32 m_version = 1;

    // 自动保存
    bool m_autoSave = false;
    std::filesystem::path m_autoSavePath;
    mutable bool m_dirty = false;
};

} // namespace mc
