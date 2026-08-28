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

#include "item/crafting/RecipeLoader.hpp"
#include "common/core/Result.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/item/crafting/RecipeSerializers.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "resource/repository/DataPackRepository.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

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

Result<RecipeLoader::LoadResult> RecipeLoader::loadFromDataPackRepository(
    const mc::resource::DataPackRepository& dataPacks, ProgressCallback callback)
{
    LoadResult result;
    m_lastResult = LoadResult{};

    if (m_clearBeforeLoad) {
        crafting::RecipeManager::instance().clear();
    }

    auto listResult = dataPacks.listResources("", ".json");
    if (!listResult.success()) {
        return listResult.error();
    }

    // 过滤出配方资源。DataPack 资源路径形如 "<namespace>/<type_dir>/<path>.json"，
    // 配方必须位于命名空间下的 recipe/（MC 1.21+ 单数）或 recipes/（旧复数）类型目录。
    // 严格校验第二段为 recipe/recipes，避免误判 advancement/recipes/ 等路径中
    // 恰好含 "recipes/" 子串的非配方资源（如进度文件）。
    std::vector<std::string> recipeResources;
    for (const auto& path : listResult.value()) {
        if (!_isRecipeResourcePath(path)) {
            continue;
        }
        recipeResources.push_back(path);
    }

    size_t current = 0;
    const size_t total = recipeResources.size();

    for (const auto& resourcePath : recipeResources) {
        ResourceLocation id = pathToRecipeId(resourcePath);

        if (callback) {
            callback(current, total, id.toString());
        }

        auto readResult = dataPacks.readTextResource(resourcePath);
        if (!readResult.success()) {
            ++result.failedCount;
            result.errors.push_back(resourcePath + ": " + readResult.error().toString());
            ++current;
            continue;
        }

        auto loadResult = loadRecipeJson(id, readResult.value());
        if (loadResult.success()) {
            ++result.successCount;
        } else {
            ++result.failedCount;
            result.errors.push_back(resourcePath + ": " + loadResult.error().message());
        }
        ++current;
    }

    if (callback) {
        callback(total, total, "");
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

    // 获取配方类型
    if (!json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "Recipe missing 'type' field: " + id.toString());
    }

    std::string type = json["type"].get<std::string>();

    // 根据类型分发到对应的解析方法
    // 熔炼类配方
    if (type == "minecraft:smelting" || type == "minecraft:blasting" || type == "minecraft:smoking" ||
        type == "minecraft:campfire_cooking") {
        auto recipeResult = crafting::RecipeSerializers::fromSmeltingJson(id, json);
        if (recipeResult.failed()) {
            return recipeResult.error();
        }
        if (!crafting::RecipeManager::instance().registerSmeltingRecipe(recipeResult.value())) {
            return Error(ErrorCode::AlreadyExists, "Smelting recipe already registered: " + id.toString());
        }
        return id;
    }

    // 切石机配方
    if (type == "minecraft:stonecutting") {
        auto recipeResult = crafting::RecipeSerializers::parseStonecuttingRecipe(id, json);
        if (recipeResult.failed()) {
            return recipeResult.error();
        }
        // 切石机配方使用专门的注册方法
        if (!crafting::RecipeManager::instance().registerStonecuttingRecipe(recipeResult.value())) {
            return Error(ErrorCode::AlreadyExists, "Stonecutting recipe already registered: " + id.toString());
        }
        return id;
    }

    // 锻造台配方
    if (type == "minecraft:smithing") {
        auto recipeResult = crafting::RecipeSerializers::parseSmithingRecipe(id, json);
        if (recipeResult.failed()) {
            return recipeResult.error();
        }
        // 锻造台配方使用专门的注册方法
        if (!crafting::RecipeManager::instance().registerSmithingRecipe(recipeResult.value())) {
            return Error(ErrorCode::AlreadyExists, "Smithing recipe already registered: " + id.toString());
        }
        return id;
    }

    // 锻造升级配方（MC 1.21+ smithing_transform，如钻石→下界合金升级）
    if (type == "minecraft:smithing_transform") {
        auto recipeResult = crafting::RecipeSerializers::parseSmithingTransformRecipe(id, json);
        if (recipeResult.failed()) {
            return recipeResult.error();
        }
        if (!crafting::RecipeManager::instance().registerSmithingTransformRecipe(recipeResult.value())) {
            return Error(ErrorCode::AlreadyExists, "Smithing transform recipe already registered: " + id.toString());
        }
        return id;
    }

    // 盔甲纹饰配方（MC 1.21+ smithing_trim）
    if (type == "minecraft:smithing_trim") {
        auto recipeResult = crafting::RecipeSerializers::parseSmithingTrimRecipe(id, json);
        if (recipeResult.failed()) {
            return recipeResult.error();
        }
        if (!crafting::RecipeManager::instance().registerSmithingTrimRecipe(recipeResult.value())) {
            return Error(ErrorCode::AlreadyExists, "Smithing trim recipe already registered: " + id.toString());
        }
        return id;
    }

    // 特殊合成配方（crafting_special_* / crafting_decorated_pot）
    // 这些配方的 JSON 仅声明存在，行为由对应代码类实现，按 type 查工厂表创建实例。
    if (type.rfind("minecraft:crafting_special_", 0) == 0 || type == "minecraft:crafting_decorated_pot") {
        auto recipeResult = crafting::RecipeSerializers::parseSpecialRecipe(id, json);
        if (recipeResult.failed()) {
            return recipeResult.error();
        }
        // 特殊配方继承 CraftingRecipe，使用统一的合成配方注册接口
        if (!crafting::RecipeManager::instance().registerRecipe(recipeResult.value())) {
            return Error(ErrorCode::AlreadyExists, "Special recipe already registered: " + id.toString());
        }
        return id;
    }

    // 转化配方（crafting_transmute，如收纳袋染色）
    // 通过 fromJson 解析并使用 registerRecipe 注册到合成配方管理器
    // （TransmuteRecipe 继承 CraftingRecipe，使用统一的注册接口）

    // 合成配方（有序/无序/转化）
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

    // 查找 "recipes" 或 "recipe" 目录（MC 1.21+ 使用单数形式 recipe/）
    fs::path current = path.parent_path();
    while (!current.empty() && current.filename() != "recipes" && current.filename() != "recipe") {
        current = current.parent_path();
    }

    if (!current.empty()) {
        // 找到recipes/recipe目录，记录实际目录名
        fs::path recipesDirName = current.filename(); // "recipes" 或 "recipe"
        current = current.parent_path();              // 上一级
        if (!current.empty() && current.filename() != "data") {
            namespace_ = current.filename().string();
        }

        // 重建配方路径（包含子目录）
        fs::path recipesDir = current / namespace_ / recipesDirName;
        fs::path relative = fs::relative(path, recipesDir);
        recipePath = relative.stem().string();

        // 将路径分隔符替换为 /
        for (char& c : recipePath) {
            if (c == '\\') c = '/';
        }
    }

    return ResourceLocation(namespace_, recipePath);
}

bool RecipeLoader::_isRecipeResourcePath(const std::string& resourcePath)
{
    // DataPack 资源路径形如 "<namespace>/<type_dir>/<path>.json"。
    // 配方要求第二段为 recipe（1.21+ 单数）或 recipes（旧复数）。
    // 用 '/' 分割路径，校验至少 2 段且第 2 段为 recipe/recipes。
    // 同时兼容旧格式 data/<namespace>/recipes/...（第 3 段为 recipes）。
    size_t firstSlash = resourcePath.find('/');
    if (firstSlash == std::string::npos) {
        return false;
    }

    // 第一段不能为空（必须有命名空间）
    if (firstSlash == 0) {
        return false;
    }

    size_t secondStart = firstSlash + 1;
    size_t secondSlash = resourcePath.find('/', secondStart);
    size_t secondLen =
        (secondSlash == std::string::npos) ? (resourcePath.size() - secondStart) : (secondSlash - secondStart);
    std::string_view secondDir(resourcePath.data() + secondStart, secondLen);

    if (secondDir == "recipe" || secondDir == "recipes") {
        return true;
    }

    // 兼容旧格式 data/<namespace>/recipes/...：第一段为 data，第 3 段为 recipe/recipes
    if (secondDir == "data" && secondSlash != std::string::npos) {
        size_t thirdStart = secondSlash + 1;
        size_t thirdSlash = resourcePath.find('/', thirdStart);
        // data/<namespace> 之间不能再有更多段，namespace 后紧跟 recipe/recipes
        if (thirdSlash != std::string::npos) {
            size_t fourthStart = thirdSlash + 1;
            size_t fourthSlash = resourcePath.find('/', fourthStart);
            size_t fourthLen =
                (fourthSlash == std::string::npos) ? (resourcePath.size() - fourthStart) : (fourthSlash - fourthStart);
            std::string_view fourthDir(resourcePath.data() + fourthStart, fourthLen);
            if (fourthDir == "recipe" || fourthDir == "recipes") {
                return true;
            }
        }
    }

    return false;
}

} // namespace mc
