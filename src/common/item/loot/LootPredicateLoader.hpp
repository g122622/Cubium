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
 * The above copyright notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "LootPredicateManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/DataPackRepository.hpp"
#include "common/resource/PackRepository.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 战利品谓词加载器
 *
 * 从文件系统或资源包加载 JSON 格式的战利品谓词并注册到 LootPredicateManager。
 * 路径映射遵循 MC 数据包规范：
 *   data/<namespace>/predicates/<path>.json -> <namespace>:<path>
 */
class LootPredicateLoader {
public:
    /**
     * @brief 加载结果
     */
    struct LoadResult {
        Size successCount = 0;
        Size failedCount = 0;
        std::vector<std::string> errors;
    };

    /**
     * @brief 加载进度回调
     * @param current 当前已处理文件数
     * @param total 总文件数
     * @param currentId 当前正在处理的谓词ID
     */
    using ProgressCallback = std::function<void(Size current, Size total, const std::string& currentId)>;

    /**
     * @brief 构造加载器
     * @param manager 谓词管理器引用
     */
    explicit LootPredicateLoader(LootPredicateManager& manager);

    /**
     * @brief 从资源包列表加载所有谓词
     *
     * 按资源包优先级从低到高加载，同名谓词由高优先级资源包覆盖。
     *
     * @param packs 资源包列表
     * @param callback 进度回调（可选）
     * @return 加载结果
     */
    Result<LoadResult> loadFromResourcePacks(const PackRepository& packs, ProgressCallback callback = nullptr);

    /**
     * @brief 从数据包列表加载所有谓词
     *
     * 使用 DataPackRepository 的 PackType::ServerData 限定接口从数据包加载谓词。
     * 按数据包优先级从低到高加载，同名谓词由高优先级数据包覆盖。
     *
     * @param dataPacks 数据包列表
     * @param callback 进度回调（可选）
     * @return 加载结果
     */
    Result<LoadResult> loadFromDataPackRepository(
        const mc::resource::DataPackRepository& dataPacks, ProgressCallback callback = nullptr);

    /**
     * @brief 从目录加载所有谓词
     *
     * 递归扫描目录下所有 .json 文件，解析并注册到管理器。
     * 单个文件解析失败不影响其他文件加载。
     *
     * @param directoryPath 目录路径
     * @param callback 进度回调（可选）
     * @return 加载结果
     */
    Result<LoadResult> loadFromDirectory(const std::string& directoryPath, ProgressCallback callback = nullptr);

    /**
     * @brief 从单个 JSON 文件加载谓词
     *
     * @param filePath 文件路径
     * @return 谓词ID（成功）或错误
     */
    Result<std::string> loadFile(const std::string& filePath);

    /**
     * @brief 从 JSON 字符串加载谓词
     *
     * @param id 谓词ID（如 "minecraft:gameplay/raid"）
     * @param jsonString JSON 内容
     * @return 谓词ID（成功）或错误
     */
    Result<std::string> loadJson(const std::string& id, const std::string& jsonString);

    /**
     * @brief 将文件路径转换为谓词ID
     *
     * 遵循 MC 数据包规范：
     *   data/minecraft/predicates/gameplay/raid.json -> minecraft:gameplay/raid
     *   data/mod_id/predicates/custom/predicate.json -> mod_id:custom/predicate
     *
     * @param filePath 文件路径
     * @return 谓词ID
     */
    [[nodiscard]] std::string pathToPredicateId(const std::string& filePath) const;

    /**
     * @brief 获取上次加载结果
     */
    [[nodiscard]] const LoadResult& getLastResult() const { return m_lastResult; }

    /**
     * @brief 重置加载结果
     */
    void resetResult();

    /**
     * @brief 设置加载前是否清空已有谓词
     * @param clear 是否清空（默认为 true）
     */
    void setClearBeforeLoad(bool clear) { m_clearBeforeLoad = clear; }

private:
    void _clearIfNeeded();

    LootPredicateManager& m_manager;
    LoadResult m_lastResult;
    bool m_clearBeforeLoad = true;
};

} // namespace loot
} // namespace mc
