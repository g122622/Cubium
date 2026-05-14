#include "item/crafting/RecipeLoader.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace mc {

namespace fs = std::filesystem;

Result<RecipeLoader::LoadResult> RecipeLoader::loadFromDirectory(
    const std::string& directoryPath, ProgressCallback callback)
{
    LoadResult result;
    m_lastResult = LoadResult{};

    // 清空现有配方（如果设置）
    if (m_clearBeforeLoad) {
        crafting::RecipeManager::instance().clear();
    }

    // 检查目录是否存在
    if (!fs::exists(directoryPath)) {
        return Error(ErrorCode::FileNotFound, "Recipe directory not found: " + directoryPath);
    }

    // 收集所有JSON文件
    std::vector<fs::path> jsonFiles;
    for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            jsonFiles.push_back(entry.path());
        }
    }

    if (jsonFiles.empty()) {
        m_lastResult = result;
        return result;
    }

    // 加载每个文件
    for (size_t i = 0; i < jsonFiles.size(); ++i) {
        const auto& filePath = jsonFiles[i];

        if (callback) {
            callback(i + 1, jsonFiles.size(), filePath.filename().string());
        }

        auto loadResult = loadRecipeFile(filePath.string());
        if (loadResult.success()) {
            ++result.successCount;
        } else {
            ++result.failedCount;
            result.errors.push_back(filePath.filename().string() + ": " + loadResult.error().message());
        }
    }

    m_lastResult = result;
    return result;
}

Result<ResourceLocation> RecipeLoader::loadRecipeFile(const std::string& filePath)
{
    // 推导配方ID
    ResourceLocation id = pathToRecipeId(filePath);

    // 读取文件内容
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Failed to open recipe file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonString = buffer.str();
    file.close();

    return loadRecipeJson(id, jsonString);
}

Result<ResourceLocation> RecipeLoader::loadRecipeJson(const ResourceLocation& id, const std::string& jsonString)
{
    // 解析JSON
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonString);
    }
    catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, "JSON parse error in recipe " + id.toString() + ": " + e.what());
    }

    // 使用RecipeSerializers解析配方
    auto recipeResult = crafting::RecipeSerializers::fromJson(id, json);
    if (recipeResult.failed()) {
        return recipeResult.error();
    }

    // 注册到RecipeManager
    if (!crafting::RecipeManager::instance().registerRecipe(recipeResult.value())) {
        return Error(ErrorCode::AlreadyExists, "Recipe already registered: " + id.toString());
    }

    return id;
}

Result<RecipeLoader::LoadResult> RecipeLoader::loadVanillaRecipes()
{
    LoadResult result;
    m_lastResult = LoadResult{};

    // 清空现有配方
    if (m_clearBeforeLoad) {
        crafting::RecipeManager::instance().clear();
    }

    // 注册内置原版配方
    // 由于Items还未完全实现，这里先跳过实际的配方注册
    // 等待Items系统完善后再启用

    // 注册基础木制品配方（占位符 - 需要ItemRegistry支持）
    // 注册顺序：
    // 1. 原木 -> 木板 (4个)
    // 2. 木板 -> 木棍 (4个)
    // 3. 木板(2x2) -> 工作台
    // 4. 木棍 + 木板 -> 工具

    // 注：实际配方需要ItemRegistry中的物品才能创建Ingredient和结果ItemStack
    // 当前ItemRegistry为空，因此跳过实际注册
    // 这里只是示例代码，展示如何注册配方

    /*
    auto& items = ItemRegistry::instance();

    // 获取物品
    const Item* oakLog = items.get(ResourceLocation("minecraft", "oak_log"));
    const Item* oakPlanks = items.get(ResourceLocation("minecraft", "oak_planks"));
    const Item* stick = items.get(ResourceLocation("minecraft", "stick"));
    const Item* craftingTable = items.get(ResourceLocation("minecraft", "crafting_table"));

    if (oakLog && oakPlanks) {
        // 原木 -> 4木板
        auto recipe = std::make_unique<crafting::ShapelessRecipe>(
            ResourceLocation("minecraft", "oak_planks"),
            std::vector<crafting::Ingredient>{ crafting::Ingredient::fromItem(*oakLog) },
            ItemStack(*oakPlanks, 4)
        );
        crafting::RecipeManager::instance().registerRecipe(std::move(recipe));
        ++result.successCount;
    }

    if (oakPlanks && craftingTable) {
        // 2x2木板 -> 工作台
        std::vector<crafting::Ingredient> ingredients(4, crafting::Ingredient::fromItem(*oakPlanks));
        auto recipe = std::make_unique<crafting::ShapedRecipe>(
            ResourceLocation("minecraft", "crafting_table"),
            2, 2,
            std::move(ingredients),
            ItemStack(*craftingTable, 1)
        );
        crafting::RecipeManager::instance().registerRecipe(std::move(recipe));
        ++result.successCount;
    }
    */

    m_lastResult = result;
    return result;
}

ResourceLocation RecipeLoader::pathToRecipeId(const std::string& filePath) const
{
    // 将文件路径转换为配方ID
    // 例如: "data/minecraft/recipes/crafting_table.json" -> "minecraft:crafting_table"

    fs::path path(filePath);
    std::string filename = path.stem().string();

    // 尝试从路径中提取命名空间
    // 假设路径格式为 .../data/<namespace>/recipes/<path>.json
    std::string namespace_ = "minecraft"; // 默认命名空间
    std::string recipePath = filename;

    // 查找 "recipes" 目录
    fs::path current = path.parent_path();
    while (!current.empty() && current.filename() != "recipes") {
        current = current.parent_path();
    }

    if (!current.empty()) {
        // 找到recipes目录，检查上一级是否是data
        current = current.parent_path(); // 上一级
        if (!current.empty() && current.filename() != "data") {
            namespace_ = current.filename().string();
        }

        // 重建配方路径（包含子目录）
        fs::path recipesDir = current / namespace_ / "recipes";
        fs::path relative = fs::relative(path, recipesDir);
        recipePath = relative.stem().string();

        // 将路径分隔符替换为 /
        for (char& c : recipePath) {
            if (c == '\\') c = '/';
        }
    }

    return ResourceLocation(namespace_, recipePath);
}

} // namespace mc
