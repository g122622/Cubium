#include "FolderResourcePack.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace mc {

namespace {
std::string getDirectoryName(const std::string& path)
{
    fs::path p(path);
    return p.filename().string();
}
} // namespace

FolderResourcePack::FolderResourcePack(std::string rootPath)
    : m_rootPath(std::move(rootPath))
    , m_name(getDirectoryName(m_rootPath))
{}

Result<void> FolderResourcePack::initialize()
{
    // 检查根目录是否存在
    if (!fs::exists(m_rootPath)) {
        return Error(ErrorCode::ResourcePackNotFound, std::string("Resource pack not found: ") + m_rootPath);
    }

    if (!fs::is_directory(m_rootPath)) {
        return Error(
            ErrorCode::ResourcePackInvalid, std::string("Resource pack path is not a directory: ") + m_rootPath);
    }

    // 读取pack.mcmeta
    std::string mcmetaPath = m_rootPath + "/pack.mcmeta";

    if (fs::exists(mcmetaPath)) {
        auto result = PackMetadata::parseFile(mcmetaPath);
        if (result.success()) {
            m_metadata = result.value();
        } else {
            // pack.mcmeta存在但解析失败，继续但记录警告
            m_metadata = PackMetadata();
        }
    }

    return Result<void>::ok();
}

bool FolderResourcePack::hasResource(std::string_view resourcePath) const
{
    std::string fullPath = normalizePath(resourcePath);
    return fs::exists(fullPath) && fs::is_regular_file(fullPath);
}

Result<std::vector<u8>> FolderResourcePack::readResource(std::string_view resourcePath) const
{
    std::string fullPath = normalizePath(resourcePath);

    if (!fs::exists(fullPath)) {
        return Error(ErrorCode::ResourceNotFound,
            std::string("Resource not found in folder pack: ") + std::string(m_name) +
                ", name: " + std::string(resourcePath));
    }

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        return Error(ErrorCode::FileOpenFailed, std::string("Cannot open resource: ") + fullPath);
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<u8> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file) {
        return Error(ErrorCode::FileReadFailed, std::string("Failed to read resource: ") + fullPath);
    }

    return data;
}

Result<std::vector<std::string>> FolderResourcePack::listResources(
    std::string_view directory, std::string_view extension) const
{
    std::string fullPath = m_rootPath + "/" + std::string(directory);

    if (!fs::exists(fullPath) || !fs::is_directory(fullPath)) {
        return Error(ErrorCode::NotFound,
            std::string("Directory not found in folder pack: ") + std::string(m_name) + ", path: " + fullPath);
    }

    std::vector<std::string> resources;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(fullPath)) {
            if (!entry.is_regular_file()) continue;

            std::string filename = entry.path().filename().string();

            if (!extension.empty()) {
                std::string ext = entry.path().extension().string();
                if (extension[0] != '.') {
                    ext = ext.substr(1); // 移除前导点
                }

                std::string checkExt(extension);
                if (checkExt[0] == '.') {
                    checkExt = checkExt.substr(1);
                }

                if (ext != checkExt) continue;
            }

            // 返回相对于资源包根目录的路径
            std::string relativePath = fs::relative(entry.path(), m_rootPath).string();
            // 将反斜杠转换为正斜杠
            for (char& c : relativePath) {
                if (c == '\\') c = '/';
            }
            resources.push_back(relativePath);
        }
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::OperationFailed, std::string("Failed to list directory: ") + e.what());
    }

    return resources;
}

std::string FolderResourcePack::normalizePath(std::string_view resourcePath) const
{
    std::string path(resourcePath);

    // 确保使用正斜杠
    for (char& c : path) {
        if (c == '\\') c = '/';
    }

    // 移除前导斜杠
    while (!path.empty() && path[0] == '/') {
        path = path.substr(1);
    }

    return m_rootPath + "/" + path;
}

} // namespace mc
