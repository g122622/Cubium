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

#include "ITextComponent.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

namespace resource {
class LanguageManager;
} // namespace resource

namespace text {

/**
 * @brief 翻译键文本组件
 *
 * 表示一个翻译键，在渲染时根据语言设置替换为对应的翻译文本。
 * 参考: net.minecraft.util.text.TranslationTextComponent
 *
 * ## 使用示例
 *
 * ```cpp
 * // 简单翻译
 * auto text = std::make_unique<TranslationTextComponent>("chat.type.text");
 *
 * // 带参数翻译
 * auto text = std::make_unique<TranslationTextComponent>("chat.type.announcement");
 * text->addParam(std::make_unique<StringTextComponent>("Server"));
 * text->addParam(std::make_unique<StringTextComponent>("Hello!"));
 * ```
 *
 * ## 翻译格式
 *
 * 翻译文本支持以下占位符：
 * - `%s`: 顺序参数
 * - `%1$s`, `%2$s`: 位置参数
 * - `%%`: 转义的百分号
 *
 * ## JSON 格式
 *
 * ```json
 * {
 *   "translate": "chat.type.announcement",
 *   "with": [
 *     {"text": "Server"},
 *     {"text": "Hello!"}
 *   ]
 * }
 * ```
 */
class TranslationTextComponent : public BaseTextComponent {
public:
    /**
     * @brief 默认构造函数
     */
    TranslationTextComponent() = default;

    /**
     * @brief 构造翻译组件
     * @param key 翻译键
     */
    explicit TranslationTextComponent(std::string key)
        : m_key(std::move(key))
    {}

    /**
     * @brief 构造带参数的翻译组件
     * @param key 翻译键
     * @param params 参数列表
     */
    TranslationTextComponent(std::string key, std::vector<std::unique_ptr<ITextComponent>> params)
        : m_key(std::move(key))
        , m_params(std::move(params))
    {}

    // ========== ITextComponent 接口 ==========

    [[nodiscard]] std::string getUnformattedText() const override;

    [[nodiscard]] std::string getFormattedText() const override;

    [[nodiscard]] std::unique_ptr<ITextComponent> deepCopy() const override
    {
        auto copy = std::make_unique<TranslationTextComponent>(m_key);
        for (const auto& param : m_params) {
            copy->m_params.push_back(param->deepCopy());
        }
        copyBaseTo(*copy);
        return copy;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> shallowCopy() const override
    {
        auto copy = std::make_unique<TranslationTextComponent>(m_key);
        copy->setStyle(m_style);
        return copy;
    }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json = m_style.toJson();
        json["translate"] = m_key;

        if (!m_params.empty()) {
            nlohmann::json with = nlohmann::json::array();
            for (const auto& param : m_params) {
                with.push_back(param->toJson());
            }
            json["with"] = std::move(with);
        }

        if (!m_siblings.empty()) {
            nlohmann::json extra = nlohmann::json::array();
            for (const auto& sibling : m_siblings) {
                extra.push_back(sibling->toJson());
            }
            json["extra"] = std::move(extra);
        }

        return json;
    }

    // ========== TranslationTextComponent 特有方法 ==========

    /**
     * @brief 获取翻译键
     * @return 翻译键
     */
    [[nodiscard]] const std::string& getKey() const noexcept { return m_key; }

    /**
     * @brief 设置翻译键
     * @param key 翻译键
     */
    void setKey(std::string key) { m_key = std::move(key); }

    /**
     * @brief 获取参数列表
     * @return 参数列表的常量引用
     */
    [[nodiscard]] const std::vector<std::unique_ptr<ITextComponent>>& getParams() const noexcept { return m_params; }

    /**
     * @brief 添加翻译参数
     * @param param 参数组件
     */
    void addParam(std::unique_ptr<ITextComponent> param)
    {
        if (param) {
            m_params.push_back(std::move(param));
        }
    }

    /**
     * @brief 清空参数列表
     */
    void clearParams() { m_params.clear(); }

    /**
     * @brief 设置全局语言管理器
     *
     * TranslationTextComponent 会使用这个管理器进行翻译。
     * 如果未设置，将使用 LanguageManager::instance() 单例。
     *
     * @param manager 语言管理器指针（生命周期由调用者管理）
     */
    static void setLanguageManager(resource::LanguageManager* manager) { s_languageManager = manager; }

    /**
     * @brief 获取全局语言管理器
     * @return 语言管理器指针，可能为 nullptr
     */
    static resource::LanguageManager* getLanguageManager() { return s_languageManager; }

private:
    std::string m_key;
    std::vector<std::unique_ptr<ITextComponent>> m_params;

    /// 全局语言管理器指针（生命周期由外部管理）
    static resource::LanguageManager* s_languageManager;

    /**
     * @brief 获取翻译后的文本
     * @return 翻译后的文本，如果找不到翻译则返回翻译键
     */
    [[nodiscard]] std::string getTranslatedText() const;

    /**
     * @brief 本地占位符替换（当无语言管理器时使用）
     */
    [[nodiscard]] static std::string replacePlaceholdersLocal(
        const std::string& text, const std::vector<std::string>& params);
};

} // namespace text
} // namespace mc
