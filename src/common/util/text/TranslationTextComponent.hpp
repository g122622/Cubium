#pragma once

#include "ITextComponent.hpp"
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::text {

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

private:
    std::string m_key;
    std::vector<std::unique_ptr<ITextComponent>> m_params;
};

// ========== 内联实现 ==========

inline std::string TranslationTextComponent::getUnformattedText() const
{
    // 在翻译系统实现前，返回翻译键作为占位符
    // TODO: 集成翻译系统后，从翻译表获取翻译文本
    std::string result = "[" + m_key + "]";

    // 添加参数信息
    for (size_t i = 0; i < m_params.size(); ++i) {
        if (i == 0) {
            result += "(";
        } else {
            result += ", ";
        }
        result += m_params[i]->getUnformattedText();
    }
    if (!m_params.empty()) {
        result += ")";
    }

    // 添加子组件
    for (const auto& sibling : m_siblings) {
        result += sibling->getUnformattedText();
    }

    return result;
}

inline std::string TranslationTextComponent::getFormattedText() const
{
    // 在翻译系统实现前，返回翻译键作为占位符
    std::string result = getStyleCodes(m_style);
    result += "[" + m_key + "]";

    // 添加参数信息
    for (size_t i = 0; i < m_params.size(); ++i) {
        if (i == 0) {
            result += "(";
        } else {
            result += ", ";
        }
        result += m_params[i]->getFormattedText();
    }
    if (!m_params.empty()) {
        result += ")";
    }

    // 添加子组件
    for (const auto& sibling : m_siblings) {
        result += sibling->getFormattedText();
    }

    return result;
}

} // namespace mc::text
