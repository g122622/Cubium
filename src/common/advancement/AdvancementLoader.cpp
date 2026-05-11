#include "AdvancementLoader.hpp"
#include "AdvancementManager.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>

namespace mc::advancement {

Result<AdvancementLoader::LoadResult> AdvancementLoader::loadFromDirectory(
    const std::string& directoryPath,
    ProgressCallback callback) {

    LoadResult result;

    // 检查目录是否存在
    if (!std::filesystem::exists(directoryPath)) {
        return Error(ErrorCode::FileNotFound, "Directory not found: " + directoryPath);
    }

    // 如果设置，清空管理器
    if (m_clearBeforeLoad) {
        AdvancementManager::instance().clear();
    }

    // 查找所有JSON文件
    auto jsonFiles = findJsonFiles(directoryPath);
    result.errors.reserve(jsonFiles.size());

    spdlog::info("Loading advancements from: {} ({} files)", directoryPath, jsonFiles.size());

    size_t index = 0;
    for (const auto& filePath : jsonFiles) {
        if (callback) {
            callback(index, jsonFiles.size(), filePath.filename().string());
        }
        ++index;

        auto loadResult = loadFile(filePath.string());
        if (loadResult.success()) {
            ++result.successCount;
        } else {
            ++result.failedCount;
            result.errors.push_back(filePath.string() + ": " + loadResult.error().message());
            spdlog::warn("Failed to load advancement {}: {}", filePath.string(), loadResult.error().message());
        }
    }

    m_lastResult = result;
    spdlog::info("Loaded {} advancements, {} failed", result.successCount, result.failedCount);

    return result;
}

Result<ResourceLocation> AdvancementLoader::loadFile(const std::string& filePath) {
    // 读取文件
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, "Cannot open file: " + filePath);
    }

    // 解析JSON
    nlohmann::json json;
    try {
        file >> json;
    } catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, "JSON parse error: " + std::string(e.what()));
    }

    // 推导成就ID
    ResourceLocation id = pathToAdvancementId(filePath);

    // 解析成就
    auto result = loadJson(id, json);
    if (result.failed()) {
        return result.error();
    }

    // 注册到管理器
    auto advancement = std::make_shared<Advancement>(std::move(result.value()));
    if (!AdvancementManager::instance().registerAdvancement(advancement)) {
        return Error(ErrorCode::AlreadyExists, "Advancement already registered: " + id.toString());
    }

    return id;
}

Result<Advancement> AdvancementLoader::loadJson(const ResourceLocation& id, const std::string& jsonString) {
    // 解析JSON
    nlohmann::json json;
    try {
        json = nlohmann::json::parse(jsonString);
    } catch (const nlohmann::json::parse_error& e) {
        return Error(ErrorCode::ResourceParseError, "JSON parse error: " + std::string(e.what()));
    }

    return loadJson(id, json);
}

Result<Advancement> AdvancementLoader::loadJson(const ResourceLocation& id, const nlohmann::json& json) {
    return Advancement::fromJson(id, json);
}

ResourceLocation AdvancementLoader::pathToAdvancementId(const std::string& filePath) const {
    // 路径格式: "data/minecraft/advancements/story/mine_stone.json"
    // ID格式: "minecraft:story/mine_stone"

    std::filesystem::path path(filePath);

    // 查找 "advancements" 目录
    std::string namespaceName;
    std::string advancementPath;

    auto it = path.begin();
    bool foundAdvancements = false;
    bool foundData = false;

    for (; it != path.end(); ++it) {
        if (it->string() == "data") {
            foundData = true;
            ++it;
            if (it != path.end()) {
                namespaceName = it->string();
                ++it;
            }
            break;
        }
    }

    // 继续查找 "advancements" 目录
    for (; it != path.end(); ++it) {
        if (it->string() == "advancements") {
            foundAdvancements = true;
            ++it;
            break;
        }
    }

    // 收集剩余路径作为成就路径
    std::vector<std::string> pathParts;
    for (; it != path.end(); ++it) {
        std::string part = it->string();
        // 移除 .json 扩展名
        if (part.ends_with(".json")) {
            part = part.substr(0, part.length() - 5);
        }
        if (!part.empty()) {
            pathParts.push_back(part);
        }
    }

    if (!foundAdvancements || namespaceName.empty()) {
        // 如果找不到标准路径结构，使用文件名作为ID
        std::string filename = path.stem().string();
        return ResourceLocation(filename);
    }

    // 构建成就ID
    std::string advancementId;
    for (size_t i = 0; i < pathParts.size(); ++i) {
        if (i > 0) advancementId += "/";
        advancementId += pathParts[i];
    }

    return ResourceLocation(namespaceName, advancementId);
}

std::vector<std::filesystem::path> AdvancementLoader::findJsonFiles(const std::filesystem::path& directory) const {
    std::vector<std::filesystem::path> files;

    if (!std::filesystem::exists(directory)) {
        return files;
    }

    // 递归遍历目录
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }

    // 排序以保证加载顺序一致
    std::sort(files.begin(), files.end());

    return files;
}

} // namespace mc::advancement
