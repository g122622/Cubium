#include "EntityTextureLoader.hpp"
#include "../../common/entity/core/EntityRegistry.hpp"
#include "../../common/entity/core/EntityClassification.hpp"
#include "../../common/resource/IResourcePack.hpp"
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace mc::client {

namespace {

/**
 * @brief 特殊实体的纹理路径映射
 *
 * 某些实体的纹理文件不遵循通用的 `textures/entity/<name>/<name>.png` 规律，
 * 这里必须按 vanilla 资源包的真实文件名显式指定，避免先尝试错误路径再回退。
 * 键为实体名称（不含命名空间），值为纹理路径列表（不含 textures/ 前缀和 .png 后缀）。
 */
const std::unordered_map<String, std::vector<String>> SPECIAL_TEXTURE_PATHS = {
    // 玩家 - 多种默认皮肤
    {"player", {"entity/steve", "entity/alex", "entity/player/wide/steve", "entity/player/slim/alex"}},

    // 基础家畜
    {"sheep", {"entity/sheep/sheep"}},
    {"chicken", {"entity/chicken"}},
    {"cow", {"entity/cow/cow"}},
    {"pig", {"entity/pig/pig"}},
    {"mooshroom", {"entity/cow/red_mooshroom"}},

    // 可驯服/常见动物
    {"rabbit", {"entity/rabbit/brown"}},
    {"wolf", {"entity/wolf/wolf"}},
    {"cat", {"entity/cat/tabby"}},
    {"ocelot", {"entity/cat/ocelot"}},
    {"parrot", {"entity/parrot/parrot_red_blue"}},
    {"fox", {"entity/fox/fox"}},
    {"panda", {"entity/panda/panda"}},
    {"polar_bear", {"entity/bear/polarbear"}},
    {"turtle", {"entity/turtle/big_sea_turtle"}},
    {"bee", {"entity/bee/bee"}},
    {"strider", {"entity/strider/strider"}},

    // 马类
    {"horse", {"entity/horse/horse_brown"}},
    {"donkey", {"entity/horse/donkey"}},
    {"mule", {"entity/horse/mule"}},
    {"llama", {"entity/llama/creamy"}},
    {"skeleton_horse", {"entity/horse/horse_skeleton"}},
    {"zombie_horse", {"entity/horse/horse_zombie"}},

    // 水生生物
    {"cod", {"entity/fish/cod"}},
    {"salmon", {"entity/fish/salmon"}},
    {"pufferfish", {"entity/fish/pufferfish"}},
    {"tropical_fish", {"entity/fish/tropical_a"}},
    {"squid", {"entity/squid"}},
    {"dolphin", {"entity/dolphin"}},

    // 环境生物 / 结构类实体
    {"bat", {"entity/bat"}},
    {"iron_golem", {"entity/iron_golem/iron_golem"}},
    {"snow_golem", {"entity/snow_golem"}},

    // 常见敌对生物
    {"zombie", {"entity/zombie/zombie"}},
    {"skeleton", {"entity/skeleton/skeleton"}},
    {"creeper", {"entity/creeper/creeper"}},
    {"spider", {"entity/spider/spider"}},
    {"enderman", {"entity/enderman/enderman"}},
    {"blaze", {"entity/blaze"}},
    {"witch", {"entity/witch"}},
    {"slime", {"entity/slime/slime"}},
    {"guardian", {"entity/guardian"}},
    {"elder_guardian", {"entity/guardian_elder"}},
    {"husk", {"entity/zombie/husk"}},
    {"drowned", {"entity/zombie/drowned"}},
    {"stray", {"entity/skeleton/stray"}},
    {"wither_skeleton", {"entity/skeleton/wither_skeleton"}},
    {"phantom", {"entity/phantom"}},
    {"giant", {"entity/zombie/zombie"}},

    // 村民和相关变种
    {"villager", {"entity/villager/villager"}},
    {"zombie_villager", {"entity/zombie_villager/zombie_villager"}},
    {"wandering_trader", {"entity/wandering_trader"}},

    // 下界相关
    {"zombified_piglin", {"entity/piglin/zombified_piglin"}},
    {"piglin", {"entity/piglin/piglin"}},
    {"piglin_brute", {"entity/piglin/piglin_brute"}},
    {"hoglin", {"entity/hoglin/hoglin"}},
    {"zoglin", {"entity/hoglin/zoglin"}},

    // 其他怪物
    {"cave_spider", {"entity/spider/cave_spider"}},
    {"silverfish", {"entity/silverfish"}},
    {"endermite", {"entity/endermite"}},
    {"shulker", {"entity/shulker/shulker"}},
    {"ghast", {"entity/ghast/ghast"}},
    {"magma_cube", {"entity/slime/magmacube"}},
    {"vindicator", {"entity/illager/vindicator"}},
    {"evoker", {"entity/illager/evoker"}},
    {"illusioner", {"entity/illager/illusioner"}},
    {"pillager", {"entity/illager/pillager"}},
    {"ravager", {"entity/illager/ravager"}},
    {"vex", {"entity/illager/vex"}},
    {"ender_dragon", {"entity/enderdragon/dragon"}},
    {"wither", {"entity/wither/wither"}},
};

/**
 * @brief 附加纹理映射（一个实体需要多个纹理文件）
 *
 * 键为实体名称，值为附加纹理路径列表。
 */
const std::unordered_map<String, std::vector<String>> ADDITIONAL_TEXTURES = {
    // 羊的毛皮层
    {"sheep", {"entity/sheep/sheep_fur"}},
};

} // namespace

bool EntityTextureLoader::needsTexture(entity::EntityClassification classification) {
    switch (classification) {
        case entity::EntityClassification::Creature:      // 动物
        case entity::EntityClassification::WaterCreature: // 水生生物
        case entity::EntityClassification::WaterAmbient:  // 水生环境生物（鱼）
        case entity::EntityClassification::Ambient:       // 环境生物（蝙蝠）
        case entity::EntityClassification::Monster:       // 怪物
            return true;
        case entity::EntityClassification::Misc:          // 物品、经验球等
        default:
            return false;
    }
}

Result<u32> EntityTextureLoader::loadAllEntityTextures(
    const std::vector<IResourcePack*>& packs,
    EntityTextureAtlas& atlas) {

    u32 loadedCount = 0;
    auto& registry = entity::EntityRegistry::instance();
    const auto& allTypes = registry.getAllTypes();

    spdlog::info("EntityTextureLoader: Loading textures for {} registered entity types", allTypes.size());

    for (const auto& type : allTypes) {
        // 只加载需要渲染的实体类型
        if (!needsTexture(type.classification())) {
            continue;
        }

        const String& entityName = type.name();

        // 获取纹理路径
        auto paths = getTexturePaths(entityName);

        // 尝试从资源包加载（后添加的优先级更高）
        bool loaded = false;
        for (auto it = packs.rbegin(); it != packs.rend() && !loaded; ++it) {
            auto* pack = *it;
            if (!pack) continue;

            for (const auto& loc : paths) {
                auto result = atlas.addTexture(*pack, loc);
                if (result.success()) {
                    loaded = true;
                    spdlog::info("EntityTextureLoader: Loaded texture '{}' for entity '{}'", loc.toString(), entityName);
                    break;
                }
            }
        }

        if (!loaded) {
            spdlog::error("EntityTextureLoader: No texture found for entity {}", entityName);
        } else {
            loadedCount++;
        }
    }

    // 加载附加纹理（如羊的毛皮层）
    loadedCount += loadAdditionalTextures(packs, atlas);

    return loadedCount;
}

Result<u32> EntityTextureLoader::loadDefaultTextures(mc::IResourcePack& pack, EntityTextureAtlas& atlas) {
    // 向后兼容：使用新的 loadAllEntityTextures
    std::vector<IResourcePack*> packs{&pack};
    return loadAllEntityTextures(packs, atlas);
}

Result<void> EntityTextureLoader::loadEntityTexture(mc::IResourcePack& pack,
                                                     EntityTextureAtlas& atlas,
                                                     const String& entityTypeId) {
    auto paths = getTexturePaths(entityTypeId);

    for (const auto& loc : paths) {
        auto result = atlas.addTexture(pack, loc);
        if (result.success()) {
            return Result<void>::ok();
        }
    }

    // 未找到纹理不算错误，继续加载其他纹理
    return Result<void>::ok();
}

u32 EntityTextureLoader::loadAdditionalTextures(
    const std::vector<IResourcePack*>& packs,
    EntityTextureAtlas& atlas) {

    u32 loadedCount = 0;

    for (const auto& [entityName, texturePaths] : ADDITIONAL_TEXTURES) {
        for (const auto& path : texturePaths) {
            ResourceLocation loc("minecraft:textures/" + path + ".png");

            // 尝试从资源包加载
            for (auto it = packs.rbegin(); it != packs.rend(); ++it) {
                auto* pack = *it;
                if (!pack) continue;

                auto result = atlas.addTexture(*pack, loc);
                if (result.success()) {
                    loadedCount++;
                    spdlog::debug("EntityTextureLoader: Loaded additional texture {} for entity {}",
                                 loc.toString(), entityName);
                    break;
                }
            }
        }
    }

    return loadedCount;
}

std::vector<ResourceLocation> EntityTextureLoader::getTexturePaths(const String& entityTypeId) {
    std::vector<ResourceLocation> paths;
    String name = parseEntityName(entityTypeId);

    // 检查特殊路径映射
    auto it = SPECIAL_TEXTURE_PATHS.find(name);
    if (it != SPECIAL_TEXTURE_PATHS.end()) {
        for (const auto& texturePath : it->second) {
            paths.emplace_back("minecraft:textures/" + texturePath + ".png");
        }
        return paths;
    }

    // 默认约定: textures/entity/<name>/<name>.png (MC 1.13+ 格式)
    paths.emplace_back("minecraft:textures/entity/" + name + "/" + name + ".png");

    // 备用: textures/entity/<name>.png (MC 1.12 格式)
    paths.emplace_back("minecraft:textures/entity/" + name + ".png");

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
