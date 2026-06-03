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

#include "LootTableManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/DataPackList.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/ResourcePackList.hpp"
#include <functional>
#include <string>
#include <vector>

namespace mc {
namespace loot {

/**
 * @brief 掉落表加载器
 *
 * 从文件系统或资源包加载 JSON 格式的掉落表并注册到 LootTableManager。
 * 路径映射遵循 MC 数据包规范：
 *   data/<namespace>/loot_tables/<path>.json -> <namespace>:<path>
 */
class LootTableLoader {
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
     * @param currentId 当前正在处理的掉落表ID
     */
    using ProgressCallback = std::function<void(Size current, Size total, const std::string& currentId)>;

    /**
     * @brief 构造加载器
     * @param manager 掉落表管理器引用
     */
    explicit LootTableLoader(LootTableManager& manager);

    /**
     * @brief 从资源包列表加载所有掉落表
     *
     * 按资源包优先级从低到高加载，同名掉落表由高优先级资源包覆盖。
     *
     * @param packs 资源包列表
     * @param callback 进度回调（可选）
     * @return 加载结果
     */
    Result<LoadResult> loadFromResourcePacks(const ResourcePackList& packs, ProgressCallback callback = nullptr);

    /**
     * @brief 从数据包列表加载所有掉落表
     *
     * 使用 DataPackList 的 PackType::ServerData 限定接口从数据包加载掉落表。
     * 按数据包优先级从低到高加载，同名掉落表由高优先级数据包覆盖。
     *
     * @param dataPacks 数据包列表
     * @param callback 进度回调（可选）
     * @return 加载结果
     */
    Result<LoadResult> loadFromDataPackList(
        const mc::resource::DataPackList& dataPacks, ProgressCallback callback = nullptr);

    /**
     * @brief 从目录加载所有掉落表
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
     * @brief 从单个 JSON 文件加载掉落表
     *
     * @param filePath 文件路径
     * @return 掉落表ID（成功）或错误
     */
    Result<std::string> loadFile(const std::string& filePath);

    /**
     * @brief 从 JSON 字符串加载掉落表
     *
     * @param id 掉落表ID（如 "minecraft:blocks/stone"）
     * @param jsonString JSON 内容
     * @return 掉落表ID（成功）或错误
     */
    Result<std::string> loadJson(const std::string& id, const std::string& jsonString);

    /**
     * @brief 将文件路径转换为掉落表ID
     *
     * 遵循 MC 数据包规范：
     *   data/minecraft/loot_tables/blocks/stone.json -> minecraft:blocks/stone
     *   data/mod_id/loot_tables/entities/boss.json -> mod_id:entities/boss
     *
     * @param filePath 文件路径
     * @return 掉落表ID
     */
    [[nodiscard]] std::string pathToLootTableId(const std::string& filePath) const;

    /**
     * @brief 获取上次加载结果
     */
    [[nodiscard]] const LoadResult& getLastResult() const { return m_lastResult; }

    /**
     * @brief 重置加载结果
     */
    void resetResult();

    /**
     * @brief 设置加载前是否清空已有表
     * @param clear 是否清空（默认为 true）
     */
    void setClearBeforeLoad(bool clear) { m_clearBeforeLoad = clear; }

private:
    void _clearIfNeeded();

    LootTableManager& m_manager;
    LoadResult m_lastResult;
    bool m_clearBeforeLoad = true;
};

} // namespace loot
} // namespace mc
