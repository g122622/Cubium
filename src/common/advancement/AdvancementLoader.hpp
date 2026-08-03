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

#include "Advancement.hpp"
#include "AdvancementManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

/**
 * @brief 成就加载器
 *
 * 从文件系统加载成就JSON文件，支持MC 1.16.5数据包格式。
 *
 * 数据包目录结构：
 * @code
 * data/
 * ├── minecraft/
 * │   └── advancements/
 * │       ├── story/
 * │       │   ├── root.json
 * │       │   ├── mine_stone.json
 * │       │   └── ...
 * │       ├── nether/
 * │       ├── end/
 * │       ├── adventure/
 * │       └── husbandry/
 * └── mod_id/
 *     └── advancements/
 *         └── ...
 * @endcode
 *
 * 使用示例：
 * @code
 * AdvancementLoader loader;
 * auto result = loader.loadFromDirectory("data/minecraft/advancements");
 * if (result.success()) {
 *     spdlog::info("Loaded {} advancements, {} failed",
 *                  result.value().successCount, result.value().failedCount);
 * }
 * @endcode
 */
class AdvancementLoader {
public:
    /**
     * @brief 加载结果
     */
    struct LoadResult {
        Size successCount = 0;           ///< 成功加载的成就数
        Size failedCount = 0;            ///< 加载失败的成就数
        std::vector<std::string> errors; ///< 错误信息列表
    };

    /**
     * @brief 进度回调类型
     * @param current 当前处理的文件索引
     * @param total 总文件数
     * @param filename 当前文件名
     */
    using ProgressCallback = std::function<void(Size current, Size total, const std::string& filename)>;

    /**
     * @brief 从数据包列表加载所有成就
     *
     * 使用 DataPackRepository 的 PackType::ServerData 限定接口从数据包加载成就。
     * 按数据包优先级从低到高加载，同名成就由高优先级数据包覆盖。
     *
     * @param dataPacks 数据包列表
     * @param callback 进度回调（可选）
     * @return 加载结果
     */
    Result<LoadResult> loadFromDataPackRepository(
        const mc::resource::DataPackRepository& dataPacks, ProgressCallback callback = nullptr);

    /**
     * @brief 从目录加载所有成就
     * @param directoryPath 成就目录路径
     * @param callback 进度回调（可选）
     * @return 加载结果
     *
     * 遍历目录下所有.json文件并尝试解析为成就。
     * 成就ID从文件路径推导：
     * - "data/minecraft/advancements/story/mine_stone.json" -> "minecraft:story/mine_stone"
     * - "data/mod_id/advancements/custom/item.json" -> "mod_id:custom/item"
     */
    Result<LoadResult> loadFromDirectory(const std::string& directoryPath, ProgressCallback callback = nullptr);

    /**
     * @brief 从文件系统路径加载成就文件
     * @param filePath 成就文件路径
     * @return 成就ID（如果成功）
     */
    Result<ResourceLocation> loadFile(const std::string& filePath);

    /**
     * @brief 从JSON字符串加载成就
     * @param id 成就ID
     * @param jsonString JSON字符串
     * @return 成就（如果成功）
     */
    Result<Advancement> loadJson(const ResourceLocation& id, const std::string& jsonString);

    /**
     * @brief 从JSON对象加载成就
     * @param id 成就ID
     * @param json JSON对象
     * @return 成就（如果成功）
     */
    Result<Advancement> loadJson(const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 获取最后加载的结果
     */
    [[nodiscard]] const LoadResult& getLastResult() const { return m_lastResult; }

    /**
     * @brief 重置加载结果
     */
    void resetResult() { m_lastResult = LoadResult{}; }

    /**
     * @brief 设置是否在加载前清空管理器
     * @param clear 是否清空
     */
    void setClearBeforeLoad(bool clear) { m_clearBeforeLoad = clear; }

    /**
     * @brief 从文件路径推导成就ID
     * @param filePath 文件路径
     * @return 成就ID
     *
     * 路径格式："data/minecraft/advancements/story/mine_stone.json"
     * ID格式："minecraft:story/mine_stone"
     */
    [[nodiscard]] ResourceLocation pathToAdvancementId(const std::string& filePath) const;

private:
    LoadResult m_lastResult;
    bool m_clearBeforeLoad = true;

    /**
     * @brief 如果设置了清空标志，则清空管理器
     */
    void _clearIfNeeded();

    /**
     * @brief 递归遍历目录查找JSON文件
     */
    std::vector<std::filesystem::path> _findJsonFiles(const std::filesystem::path& directory) const;
};

} // namespace mc::advancement
