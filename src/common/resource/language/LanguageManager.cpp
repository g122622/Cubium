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
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include "common/util/NumberParsing.hpp"

#include "common/util/json/JsonRead.hpp"
#include <cctype>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
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
    // 扫描资源包内出现的全部命名空间（minecraft 与模组/资源包自有命名空间）。
    // 某个命名空间没有该语言文件是正常情况，_mergeLanguageFile 会以 ResourceNotFound 跳过。
    std::vector<std::string> namespaces;
    auto nsResult = packList.getResourceNamespaces();
    if (nsResult.success()) {
        namespaces = nsResult.value();
    }
    if (namespaces.empty()) {
        namespaces.emplace_back("minecraft");
    }
    return _loadLanguage(packList, languageCode, namespaces);
}

Result<void> LanguageManager::loadLanguage(
    const PackRepository& packList, const std::string& languageCode, const std::string& namespace_)
{
    return _loadLanguage(packList, languageCode, {namespace_});
}

Result<void> LanguageManager::_loadLanguage(
    const PackRepository& packList, const std::string& languageCode, const std::vector<std::string>& namespaces)
{
    const auto packs = packList.getEnabledPacks();

    // 在锁外完成全部读文件与解析，最后一次性替换翻译表。持锁 I/O 会阻塞命令查询，
    // 而 loadFromJson 复用同一把锁时还会因非递归互斥量自死锁。
    std::unordered_map<std::string, std::string> merged;
    size_t loadedFiles = 0;

    const auto mergeFor = [&](const std::string& code) {
        // 资源包按优先级从低到高遍历：高优先级后写入，覆盖低优先级。
        for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
            for (const auto& ns : namespaces) {
                auto result = _mergeLanguageFile(merged, **it, code, ns);
                if (result.success()) {
                    ++loadedFiles;
                    continue;
                }
                if (result.error().code() != ErrorCode::ResourceNotFound) {
                    spdlog::warn("[LanguageManager] Failed to load '{}/lang/{}.json' from pack '{}': {}",
                        ns,
                        code,
                        (*it)->name(),
                        result.error().message());
                }
            }
        }
    };

    // 先加载 en_us 作为回退，再用目标语言覆盖。
    if (languageCode != DEFAULT_LANGUAGE) {
        mergeFor(DEFAULT_LANGUAGE);
    }
    mergeFor(languageCode);

    return _commitMerged(std::move(merged), languageCode, loadedFiles);
}

Result<size_t> LanguageManager::_mergeLanguageFile(std::unordered_map<std::string, std::string>& target,
    const IResourcePack& pack,
    const std::string& languageCode,
    const std::string& namespace_)
{
    // 资源路径相对于资源包根目录（资源包根下的 assets/ 由 IResourcePack 负责拼接）。
    const std::string filePath = namespace_ + "/lang/" + languageCode + ".json";

    if (!pack.hasResource(PackType::ClientResources, filePath)) {
        return Error(ErrorCode::ResourceNotFound, "Language file not found: " + filePath);
    }

    auto readResult = pack.readTextResource(PackType::ClientResources, filePath);
    if (readResult.failed()) {
        return readResult.error();
    }

    return _mergeJsonObject(target, readResult.value());
}

Result<size_t> LanguageManager::_mergeJsonObject(
    std::unordered_map<std::string, std::string>& target, const std::string& jsonContent)
{
    auto parsedJson = util::json::parse(jsonContent);
    if (parsedJson.failed()) {
        return Error(
            ErrorCode::InvalidData, std::string("Error parsing language JSON: ") + parsedJson.error().message());
    }
    const auto& json = parsedJson.value();

    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Language file must be a JSON object");
    }

    size_t count = 0;
    for (auto& [key, value] : json.items()) {
        const auto text = util::json::getString(value);
        if (text.success()) {
            target[key] = text.value();
            ++count;
        }
    }

    return count;
}

Result<void> LanguageManager::_commitMerged(
    std::unordered_map<std::string, std::string>&& merged, const std::string& languageCode, size_t loadedFiles)
{
    std::function<void()> callback;
    size_t total = 0;
    {
        std::unique_lock lock(m_mutex);
        m_translations = std::move(merged);
        m_currentLanguage = languageCode;
        total = m_translations.size();
        callback = m_onLanguageChanged;
    }

    if (total == 0) {
        // 没有任何语言文件时仍视为成功：查询回退为键本身。
        spdlog::warn("[LanguageManager] No translations loaded for language '{}' (no language file in enabled packs)",
            languageCode);
    } else {
        spdlog::info("[LanguageManager] Loaded {} translations for language '{}' from {} language file(s)",
            total,
            languageCode,
            loadedFiles);
    }

    // 回调在锁外执行：订阅方通常会在回调里查询翻译，持锁回调会自死锁。
    if (callback) {
        callback();
    }

    return Result<void>::ok();
}

Result<size_t> LanguageManager::loadFromJson(const std::string& jsonContent)
{
    std::unordered_map<std::string, std::string> parsed;
    auto mergeResult = _mergeJsonObject(parsed, jsonContent);
    if (mergeResult.failed()) {
        return mergeResult.error();
    }

    std::unique_lock lock(m_mutex);
    for (auto& [key, value] : parsed) {
        m_translations[key] = std::move(value);
    }

    return mergeResult.value();
}

void LanguageManager::clear()
{
    std::unique_lock lock(m_mutex);
    m_translations.clear();
    m_currentLanguage.clear();
}

// ============================================================================
// 翻译查询
// ============================================================================

std::string LanguageManager::get(const std::string& key) const
{
    {
        std::shared_lock lock(m_mutex);
        const auto it = m_translations.find(key);
        if (it != m_translations.end()) {
            return it->second;
        }
    }

    // 返回键本身作为回退
    return key;
}

bool LanguageManager::has(const std::string& key) const
{
    std::shared_lock lock(m_mutex);
    return m_translations.find(key) != m_translations.end();
}

std::string LanguageManager::get(const std::string& key, const std::vector<std::string>& params) const
{
    std::string text = get(key);
    return _replacePlaceholders(text, params);
}

std::string LanguageManager::currentLanguage() const
{
    std::shared_lock lock(m_mutex);
    return m_currentLanguage;
}

size_t LanguageManager::translationCount() const
{
    std::shared_lock lock(m_mutex);
    return m_translations.size();
}

void LanguageManager::setOnLanguageChanged(std::function<void()> callback)
{
    std::unique_lock lock(m_mutex);
    m_onLanguageChanged = std::move(callback);
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

std::vector<LanguageInfo> LanguageManager::scanLanguages(const PackRepository& packList)
{
    std::vector<LanguageInfo> result;

    const auto codes = getAvailableLanguages(packList);
    result.reserve(codes.size());

    for (const auto& code : codes) {
        LanguageInfo info;
        info.code = code;

        // 语言显示名/地区/书写方向来自语言文件自身的 language.* 键（与原版语言列表同源）。
        // 读取走最高优先级资源包：同一语言的多份文件里，高优先级包说了算。
        const std::string filePath = "minecraft/lang/" + code + ".json";
        auto textResult = packList.readTextResource(filePath);
        if (textResult.success()) {
            auto parsed = util::json::parse(textResult.value());
            if (parsed.failed()) {
                spdlog::warn("[LanguageManager] Failed to parse '{}' while scanning languages: {}",
                    filePath,
                    parsed.error().message());
            } else if (parsed.value().is_object()) {
                const auto& json = parsed.value();
                const auto name = util::json::getString(json, "language.name");
                if (name.success()) {
                    info.name = name.value();
                }
                const auto region = util::json::getString(json, "language.region");
                if (region.success()) {
                    info.region = region.value();
                }
                const auto bidirectional = util::json::getBool(json, "language.bidirectional");
                if (bidirectional.success()) {
                    info.bidirectional = bidirectional.value();
                }
            }
        }

        if (info.name.empty()) {
            info.name = code;
        }
        result.push_back(std::move(info));
    }

    return result;
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
                    // 解析位置索引；解析失败（如位数超出 i32）时保留原样
                    const auto positionResult =
                        util::parseInt32(std::string_view(text).substr(digitStart, digitEnd - digitStart));
                    if (positionResult.failed()) {
                        result += text.substr(i, digitEnd + 2 - i);
                        i = digitEnd + 2;
                        continue;
                    }
                    const i32 position = positionResult.value();

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
