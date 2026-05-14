#pragma once

#include "core/Result.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "item/crafting/RecipeSerializers.hpp"
#include "resource/ResourceLocation.hpp"
#include <functional>
#include <string>
#include <vector>

namespace mc {

/**
 * @brief 配方加载器
 *
 * 从文件系统加载配方JSON文件，支持MC 1.16.5数据包格式。
 *
 * 数据包目录结构：
 * @code
 * data/
 * ├── minecraft/
 * │   └── recipes/
 * │       ├── crafting_table.json
 * │       ├── oak_planks.json
 * │       └── ...
 * └── mod_id/
 *     └── recipes/
 *         └── custom_item.json
 * @endcode
 *
 * 使用示例：
 * @code
 * RecipeLoader loader;
 * loader.loadFromDirectory("data/minecraft/recipes");
 * loader.loadFromDirectory("data/mod_id/recipes");
 *
 * // 获取所有配方
 * const auto& recipes = RecipeManager::instance().getAllRecipes();
 * @endcode
 */
class RecipeLoader {
public:
    /**
     * @brief 加载结果回调
     */
    struct LoadResult {
        size_t successCount = 0;         ///< 成功加载的配方数
        size_t failedCount = 0;          ///< 加载失败的配方数
        std::vector<std::string> errors; ///< 错误信息列表
    };

    /**
     * @brief 进度回调类型
     * @param current 当前处理的文件索引
     * @param total 总文件数
     * @param filename 当前文件名
     */
    using ProgressCallback = std::function<void(size_t current, size_t total, const std::string& filename)>;

    /**
     * @brief 从目录加载所有配方
     * @param directoryPath 配方目录路径
     * @param callback 进度回调（可选）
     * @return 加载结果
     *
     * 遍历目录下所有.json文件并尝试解析为配方。
     * 配方ID从文件路径推导：
     * - "data/minecraft/recipes/crafting_table.json" -> "minecraft:crafting_table"
     * - "data/mod_id/recipes/subdir/item.json" -> "mod_id:subdir/item"
     */
    Result<LoadResult> loadFromDirectory(const std::string& directoryPath, ProgressCallback callback = nullptr);

    /**
     * @brief 加载单个配方文件
     * @param filePath 配方文件路径
     * @return 配方ID（如果成功）
     */
    Result<ResourceLocation> loadRecipeFile(const std::string& filePath);

    /**
     * @brief 从JSON字符串加载配方
     * @param id 配方ID
     * @param jsonString JSON字符串
     * @return 配方ID（如果成功）
     */
    Result<ResourceLocation> loadRecipeJson(const ResourceLocation& id, const std::string& jsonString);

    /**
     * @brief 加载内置原版配方
     * @return 加载结果
     *
     * 加载一组基础的Minecraft原版配方（约50个）。
     * 包括：木板、木棍、工作台、工具等。
     */
    Result<LoadResult> loadVanillaRecipes();

    /**
     * @brief 获取最后加载的结果
     * @return 最后一次加载的结果
     */
    [[nodiscard]] const LoadResult& getLastResult() const { return m_lastResult; }

    /**
     * @brief 重置加载结果
     */
    void resetResult() { m_lastResult = LoadResult{}; }

    /**
     * @brief 设置是否在加载前清空配方管理器
     * @param clear 是否清空
     */
    void setClearBeforeLoad(bool clear) { m_clearBeforeLoad = clear; }

    /**
     * @brief 从文件路径推导配方ID
     * @param filePath 文件路径
     * @return 配方ID
     */
    [[nodiscard]] ResourceLocation pathToRecipeId(const std::string& filePath) const;

private:
    /**
     * @brief 注册内置原版配方
     */
    void registerBuiltinRecipes();

    LoadResult m_lastResult;
    bool m_clearBeforeLoad = true;
};

} // namespace mc
