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

#include "LanguageManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include "pack/IResourcePack.hpp"
#include "repository/PackRepository.hpp"

#include <cctype>
#include <cstddef>
#include <exception>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::resource {

// ============================================================================
// 全局实例
// ============================================================================

LanguageManager& LanguageManager::instance()
{
    static LanguageManager instance;
    return instance;
}

// ============================================================================
// 语言加载
// ============================================================================

Result<void> LanguageManager::loadLanguage(const PackRepository& packList, const std::string& languageCode)
{
    return loadLanguage(packList, languageCode, "minecraft");
}

Result<void> LanguageManager::loadLanguage(
    const PackRepository& packList, const std::string& languageCode, const std::string& namespace_)
{
    // 清空现有翻译
    clear();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentLanguage = languageCode;

    // 获取启用的资源包（按优先级排序）
    auto packs = packList.getEnabledPacks();

    size_t totalLoaded = 0;

    // 首先加载默认语言 (en_us) 作为回退
    if (languageCode != DEFAULT_LANGUAGE) {
        for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
            auto result = loadLanguageFromPack(**it, DEFAULT_LANGUAGE, namespace_);
            if (result.success()) {
                totalLoaded += result.value();
            }
        }
    }

    // 然后加载目标语言（覆盖默认语言的翻译）
    for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
        auto result = loadLanguageFromPack(**it, languageCode, namespace_);
        if (result.success()) {
            totalLoaded += result.value();
        }
    }

    // 如果没有加载任何翻译，记录警告但不返回错误
    // 这允许使用翻译键作为回退
    if (totalLoaded == 0) {
        spdlog::warn(
            "[LanguageManager] No translations loaded for language '{}' (namespace: {})", languageCode, namespace_);
    }

    spdlog::info("[LanguageManager] Loaded {} translations for language: {}", m_translations.size(), languageCode);

    // 触发回调
    if (m_onLanguageChanged) {
        m_onLanguageChanged();
    }

    return Result<void>::ok();
}

Result<size_t> LanguageManager::loadLanguageFromPack(
    const IResourcePack& pack, const std::string& languageCode, const std::string& namespace_)
{
    // 构建语言文件路径: assets/<namespace>/lang/<lang_code>.json
    std::string filePath = namespace_ + "/lang/" + languageCode + ".json";

    // 检查资源是否存在
    if (!pack.hasResource(PackType::ClientResources, filePath)) {
        return Error(ErrorCode::ResourceNotFound, "Language file not found: " + filePath);
    }

    // 读取文件内容
    auto readResult = pack.readTextResource(PackType::ClientResources, filePath);
    if (readResult.failed()) {
        return readResult.error();
    }

    // 解析 JSON
    auto parseResult = loadFromJson(readResult.value());
    if (parseResult.failed()) {
        return parseResult.error();
    }

    return parseResult;
}

Result<size_t> LanguageManager::loadFromJson(const std::string& jsonContent)
{
    try {
        auto json = nlohmann::json::parse(jsonContent);

        if (!json.is_object()) {
            return Error(ErrorCode::InvalidData, "Language file must be a JSON object");
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        size_t count = 0;
        for (auto& [key, value] : json.items()) {
            if (value.is_string()) {
                m_translations[key] = value.get<std::string>();
                ++count;
            }
        }

        return count;
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::InvalidData, std::string("JSON parse error: ") + e.what());
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::InvalidData, std::string("Error parsing language JSON: ") + e.what());
    }
}

void LanguageManager::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_translations.clear();
    m_currentLanguage.clear();
}

// ============================================================================
// 翻译查询
// ============================================================================

std::string LanguageManager::get(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_translations.find(key);
    if (it != m_translations.end()) {
        return it->second;
    }

    // 返回键本身作为回退
    return key;
}

bool LanguageManager::has(const std::string& key) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_translations.find(key) != m_translations.end();
}

std::string LanguageManager::get(const std::string& key, const std::vector<std::string>& params) const
{
    std::string text = get(key);
    return _replacePlaceholders(text, params);
}

// ============================================================================
// 语言发现
// ============================================================================

std::vector<std::string> LanguageManager::getAvailableLanguages(
    const PackRepository& packList, const std::string& namespace_)
{
    std::set<std::string> languages;

    // 列出所有 lang 目录下的 .json 文件
    std::string directory = namespace_ + "/lang";
    auto listResult = packList.listResources(directory, ".json");

    if (listResult.success()) {
        for (const auto& path : listResult.value()) {
            // 从路径提取语言代码
            // 格式: minecraft/lang/en_us.json 或 en_us.json（无目录前缀）
            size_t lastSlash = path.find_last_of('/');
            size_t dotPos = path.find_last_of('.');
            if (dotPos != std::string::npos && (lastSlash == std::string::npos || dotPos > lastSlash)) {
                size_t nameStart = (lastSlash != std::string::npos) ? lastSlash + 1 : 0;
                std::string code = path.substr(nameStart, dotPos - nameStart);
                languages.insert(code);
            }
        }
    }

    return std::vector<std::string>(languages.begin(), languages.end());
}

std::vector<LanguageInfo> LanguageManager::getBuiltinLanguages()
{
    // Minecraft 1.16.5 支持的语言列表
    return {
        {"en_us", "English", "US", false},
        {"zh_cn", "简体中文", "CN", false},
        {"zh_tw", "繁體中文", "TW", false},
        {"ja_jp", "日本語", "JP", false},
        {"ko_kr", "한국어", "KR", false},
        {"de_de", "Deutsch", "DE", false},
        {"fr_fr", "Français", "FR", false},
        {"it_it", "Italiano", "IT", false},
        {"es_es", "Español", "ES", false},
        {"pt_br", "Português", "BR", false},
        {"ru_ru", "Русский", "RU", false},
        {"pl_pl", "Polski", "PL", false},
        {"nl_nl", "Nederlands", "NL", false},
        {"sv_se", "Svenska", "SE", false},
        {"da_dk", "Dansk", "DK", false},
        {"fi_fi", "Suomi", "FI", false},
        {"no_no", "Norsk", "NO", false},
        {"cs_cz", "Čeština", "CZ", false},
        {"hu_hu", "Magyar", "HU", false},
        {"tr_tr", "Türkçe", "TR", false},
        {"ar_sa", "العربية", "SA", true}, // RTL
        {"he_il", "עברית", "IL", true},   // RTL
    };
}

// ============================================================================
// 占位符替换
// ============================================================================

std::string LanguageManager::_replacePlaceholders(const std::string& text, const std::vector<std::string>& params)
{
    if (text.empty()) {
        return text;
    }

    std::string result;
    result.reserve(text.size() * 2);

    size_t i = 0;
    size_t sequentialIndex = 0;

    while (i < text.size()) {
        // 检查 %%
        if (i + 1 < text.size() && text[i] == '%' && text[i + 1] == '%') {
            result += '%';
            i += 2;
            continue;
        }

        // 检查占位符
        if (text[i] == '%' && i + 1 < text.size()) {
            size_t nextPercent = text.find('%', i + 1);

            // 检查是否是位置参数格式 %N$s
            if (i + 2 < text.size() && std::isdigit(text[i + 1])) {
                // 尝试解析位置参数
                size_t digitStart = i + 1;
                size_t digitEnd = digitStart;

                while (digitEnd < text.size() && std::isdigit(text[digitEnd])) {
                    ++digitEnd;
                }

                // 检查是否是 $s 格式
                if (digitEnd + 1 < text.size() && text[digitEnd] == '$' && text[digitEnd + 1] == 's') {
                    // 解析位置索引
                    i32 position = 0;
                    try {
                        position = std::stoi(text.substr(digitStart, digitEnd - digitStart));
                    }
                    catch (...) {
                        // 解析失败，保留原样
                        result += text.substr(i, digitEnd + 2 - i);
                        i = digitEnd + 2;
                        continue;
                    }

                    // 替换参数（位置从1开始）
                    if (position >= 1 && position <= static_cast<i32>(params.size())) {
                        result += params[position - 1];
                    } else {
                        // 参数不存在，保留原占位符
                        result += text.substr(i, digitEnd + 2 - i);
                    }

                    i = digitEnd + 2;
                    continue;
                }
            }

            // 检查是否是顺序参数 %s
            if (i + 1 < text.size() && text[i + 1] == 's') {
                if (sequentialIndex < params.size()) {
                    result += params[sequentialIndex];
                    ++sequentialIndex;
                } else {
                    // 参数不存在，保留占位符
                    result += "%s";
                }
                i += 2;
                continue;
            }

            // 检查 %d 或 %f（Minecraft 加载时转换为 %s，但这里也处理）
            if (i + 1 < text.size() && (text[i + 1] == 'd' || text[i + 1] == 'f')) {
                if (sequentialIndex < params.size()) {
                    result += params[sequentialIndex];
                    ++sequentialIndex;
                } else {
                    result += text.substr(i, 2);
                }
                i += 2;
                continue;
            }
        }

        // 普通字符
        result += text[i];
        ++i;
    }

    return result;
}

} // namespace mc::resource
