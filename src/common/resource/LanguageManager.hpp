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
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::resource {

// 前向声明
class PackRepository;
class IResourcePack;

/**
 * @brief 语言信息结构
 *
 * 存储语言的元数据信息。
 */
struct LanguageInfo {
    std::string code;           ///< 语言代码 (如 "en_us", "zh_cn")
    std::string name;           ///< 语言名称 (如 "English", "简体中文")
    std::string region;         ///< 地区代码 (如 "US", "CN")
    bool bidirectional = false; ///< 是否为从右向左的语言 (如阿拉伯语)
};

/**
 * @brief 语言管理器
 *
 * 从资源包加载语言文件，提供翻译功能。
 * 支持多语言切换和资源包优先级。
 *
 * ## 语言文件格式
 *
 * 语言文件为 JSON 格式，位于 `assets/<namespace>/lang/<lang_code>.json`：
 * ```json
 * {
 *   "translation.key": "翻译文本",
 *   "chat.type.text": "%s: %s",
 *   "item.minecraft.diamond": "钻石"
 * }
 * ```
 *
 * ## 占位符格式
 *
 * 支持 Minecraft 1.16.5 风格的占位符：
 * - `%s` - 顺序参数，按出现顺序替换
 * - `%1$s`, `%2$s` - 位置参数，按索引指定参数位置
 * - `%%` - 转义的百分号，输出单个 `%`
 *
 * ## 使用示例
 *
 * @code
 * LanguageManager langManager;
 *
 * // 加载语言文件
 * auto result = langManager.loadLanguage(packList, "zh_cn");
 * if (result.success()) {
 *     // 获取翻译
 *     std::string text = langManager.get("item.minecraft.diamond");
 *     // 输出: "钻石"
 *
 *     // 带参数翻译
 *     std::string chat = langManager.get("chat.type.text", {"玩家", "你好"});
 *     // 输出: "玩家: 你好"
 * }
 * @endcode
 */
class LanguageManager {
public:
    /**
     * @brief 默认语言代码
     */
    static constexpr const char* DEFAULT_LANGUAGE = "en_us";

    /**
     * @brief 默认构造函数
     */
    LanguageManager() = default;

    /**
     * @brief 析构函数
     */
    ~LanguageManager() = default;

    // 禁止拷贝
    LanguageManager(const LanguageManager&) = delete;
    LanguageManager& operator=(const LanguageManager&) = delete;

    // 禁止移动（包含 std::mutex）
    LanguageManager(LanguageManager&&) = delete;
    LanguageManager& operator=(LanguageManager&&) = delete;

    // ========================================================================
    // 语言加载
    // ========================================================================

    /**
     * @brief 从资源包列表加载语言文件
     *
     * 按优先级加载，高优先级包的翻译会覆盖低优先级的。
     * 同时加载默认语言 (en_us) 作为回退。
     *
     * @param packList 资源包列表
     * @param languageCode 语言代码（如 "zh_cn", "en_us"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadLanguage(const PackRepository& packList, const std::string& languageCode);

    /**
     * @brief 从资源包列表加载语言文件（指定命名空间）
     *
     * @param packList 资源包列表
     * @param languageCode 语言代码
     * @param namespace_ 命名空间（如 "minecraft"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadLanguage(
        const PackRepository& packList, const std::string& languageCode, const std::string& namespace_);

    /**
     * @brief 清空当前加载的翻译
     */
    void clear();

    /**
     * @brief 从 JSON 内容加载翻译
     *
     * 主要用于测试场景，生产代码应使用 loadLanguage。
     *
     * @param jsonContent JSON 内容
     * @return 成功加载的翻译条目数量
     */
    [[nodiscard]] Result<size_t> loadFromJson(const std::string& jsonContent);

    // ========================================================================
    // 翻译查询
    // ========================================================================

    /**
     * @brief 获取翻译文本
     *
     * @param key 翻译键
     * @return 翻译后的文本，如果找不到返回键本身
     */
    [[nodiscard]] std::string get(const std::string& key) const;

    /**
     * @brief 检查翻译键是否存在
     *
     * @param key 翻译键
     * @return 是否存在
     */
    [[nodiscard]] bool has(const std::string& key) const;

    /**
     * @brief 获取翻译文本（带参数替换）
     *
     * 支持占位符：
     * - %s: 按顺序替换
     * - %1$s, %2$s: 按位置替换
     * - %%: 输出 %
     *
     * @param key 翻译键
     * @param params 参数列表
     * @return 翻译后的文本
     */
    [[nodiscard]] std::string get(const std::string& key, const std::vector<std::string>& params) const;

    /**
     * @brief 获取当前语言代码
     */
    [[nodiscard]] const std::string& currentLanguage() const noexcept { return m_currentLanguage; }

    /**
     * @brief 获取翻译条目数量
     */
    [[nodiscard]] size_t translationCount() const noexcept { return m_translations.size(); }

    // ========================================================================
    // 语言发现
    // ========================================================================

    /**
     * @brief 获取所有可用的语言代码
     *
     * 扫描资源包列表中的语言文件。
     *
     * @param packList 资源包列表
     * @param namespace_ 命名空间
     * @return 语言代码列表（已去重排序）
     */
    [[nodiscard]] static std::vector<std::string> getAvailableLanguages(
        const PackRepository& packList, const std::string& namespace_ = "minecraft");

    /**
     * @brief 获取内置语言列表
     *
     * 返回 Minecraft 1.16.5 支持的语言代码列表。
     *
     * @return 语言代码列表
     */
    [[nodiscard]] static std::vector<LanguageInfo> getBuiltinLanguages();

    // ========================================================================
    // 全局实例
    // ========================================================================

    /**
     * @brief 获取全局语言管理器实例
     *
     * 用于 TranslationTextComponent 等组件访问翻译服务。
     *
     * @return 全局实例引用
     */
    static LanguageManager& instance();

    // ========================================================================
    // 回调
    // ========================================================================

    /**
     * @brief 设置语言变更回调
     *
     * 当语言切换后调用。
     *
     * @param callback 回调函数
     */
    void setOnLanguageChanged(std::function<void()> callback) { m_onLanguageChanged = std::move(callback); }

private:
    /// 翻译映射表：翻译键 -> 翻译文本
    std::unordered_map<std::string, std::string> m_translations;

    /// 当前语言代码
    std::string m_currentLanguage;

    /// 语言变更回调
    std::function<void()> m_onLanguageChanged;

    /// 互斥锁（用于线程安全）
    mutable std::mutex m_mutex;

    /// 占位符替换（内部实现）
    static std::string _replacePlaceholders(const std::string& text, const std::vector<std::string>& params);

    /**
     * @brief 从单个资源包加载语言文件
     *
     * @param pack 资源包
     * @param languageCode 语言代码
     * @param namespace_ 命名空间
     * @return 成功加载的翻译条目数量
     */
    [[nodiscard]] Result<size_t> loadLanguageFromPack(
        const IResourcePack& pack, const std::string& languageCode, const std::string& namespace_ = "minecraft");
};

} // namespace mc::resource

namespace mc {
using LanguageManager = resource::LanguageManager;
using LanguageInfo = resource::LanguageInfo;
} // namespace mc
