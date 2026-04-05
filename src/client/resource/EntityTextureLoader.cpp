#include "EntityTextureLoader.hpp"
#include "../../common/resource/IResourcePack.hpp"
#include <spdlog/spdlog.h>

namespace mc::client {

const std::vector<String>& EntityTextureLoader::getDefaultEntityTypes() {
    static const std::vector<String> defaultTypes = {
        "minecraft:player",
        "minecraft:pig",
        "minecraft:cow",
        "minecraft:sheep",
        "minecraft:chicken"
    };
    return defaultTypes;
}

Result<u32> EntityTextureLoader::loadDefaultTextures(mc::IResourcePack& pack, EntityTextureAtlas& atlas) {
    u32 loadedCount = 0;

    for (const auto& typeId : getDefaultEntityTypes()) {
        auto result = loadEntityTexture(pack, atlas, typeId);
        if (result.success()) {
            loadedCount++;
        }
    }

    // 额外加载羊的羊毛纹理
    auto result = atlas.addTexture(pack, ResourceLocation("minecraft:textures/entity/sheep/sheep_fur.png"));
    if (result.success()) {
        loadedCount++;
    }

    spdlog::info("Loaded {} entity textures", loadedCount);
    return loadedCount;  // 隐式转换为Result<u32>
}

Result<void> EntityTextureLoader::loadEntityTexture(mc::IResourcePack& pack,
                                                     EntityTextureAtlas& atlas,
                                                     const String& entityTypeId) {
    auto paths = getTexturePaths(entityTypeId);
    bool loaded = false;

    for (const auto& location : paths) {
        auto result = atlas.addTexture(pack, location);
        if (result.success()) {
            loaded = true;
            spdlog::debug("Loaded entity texture: {} for {}", location.toString(), entityTypeId);
            break;
        }
    }

    if (!loaded) {
        spdlog::warn("Failed to load texture for entity: {}", entityTypeId);
        // 不返回错误，继续加载其他纹理
    }

    return Result<void>::ok();
}

std::vector<ResourceLocation> EntityTextureLoader::getTexturePaths(const String& entityTypeId) {
    std::vector<ResourceLocation> paths;
    String name = parseEntityName(entityTypeId);

    if (name == "player") {
        // 兼容不同版本资源路径的玩家默认皮肤。
        paths.emplace_back("minecraft:textures/entity/steve.png");
        paths.emplace_back("minecraft:textures/entity/alex.png");
        paths.emplace_back("minecraft:textures/entity/player/wide/steve.png");
        paths.emplace_back("minecraft:textures/entity/player/slim/alex.png");
        return paths;
    }

    // MC 1.13+ 格式: textures/entity/<name>/<name>.png
    paths.emplace_back("minecraft:textures/entity/" + name + "/" + name + ".png");

    // MC 1.12 格式: textures/entity/<name>.png
    paths.emplace_back("minecraft:textures/entity/" + name + ".png");

    // 羊的特殊处理
    if (name == "sheep") {
        paths.emplace_back("minecraft:textures/entity/sheep/sheep.png");
    }

    // 鸡的特殊处理
    if (name == "chicken") {
        paths.emplace_back("minecraft:textures/entity/chicken.png");
    }

    return paths;
}

String EntityTextureLoader::parseEntityName(const String& entityTypeId) {
    // 解析 "minecraft:pig" -> "pig"
    size_t colonPos = entityTypeId.find(':');
    if (colonPos != String::npos && colonPos + 1 < entityTypeId.size()) {
        return entityTypeId.substr(colonPos + 1);
    }
    return entityTypeId;
}

} // namespace mc::client
