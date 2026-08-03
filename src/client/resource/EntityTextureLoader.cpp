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

#include "EntityTextureLoader.hpp"
#include "client/resource/atlas/TexturePathVariant.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::client {

namespace {

bool shouldSuppressMissingTextureWarning(const ResourceLocation& location)
{
    // 目前不需要抑制任何纹理缺失告警
    // 之前曾抑制 villager/zombie_villager 的 none.png，
    // 但已从 ADDITIONAL_TEXTURES 中移除该路径
    MC_UNUSED(location);
    return false;
}

/**
 * @brief 特殊实体的纹理路径映射
 *
 * 某些实体的纹理文件不遵循通用的 `textures/entity/<name>/<name>.png` 规律，
 * 这里必须按 vanilla 资源包的真实文件名显式指定，避免先尝试错误路径再回退。
 * 键为实体名称（不含命名空间），值为纹理路径列表（不含 textures/ 前缀和 .png 后缀）。
 */
const std::unordered_map<std::string, std::vector<std::string>> SPECIAL_TEXTURE_PATHS = {
    // 玩家 - 新版资源包只有 player/wide 和 player/slim 子目录下的皮肤
    {"player", {"entity/player/wide/steve", "entity/player/slim/alex", "entity/steve", "entity/alex"}},

    // 基础家畜
    // MC 1.21+ 资源包中 pig/cow/chicken 的纹理已按气候变体重命名，
    // 不再存在 <name>/<name>.png 格式，需使用气候变体作为默认纹理
    {"sheep", {"entity/sheep/sheep"}},
    {"chicken", {"entity/chicken/temperate_chicken", "entity/chicken/cold_chicken", "entity/chicken/warm_chicken"}},
    {"cow", {"entity/cow/temperate_cow", "entity/cow/cold_cow", "entity/cow/warm_cow"}},
    {"pig", {"entity/pig/temperate_pig", "entity/pig/cold_pig", "entity/pig/warm_pig"}},
    {"mooshroom", {"entity/cow/red_mooshroom", "entity/cow/brown_mooshroom"}},

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
    // MC 1.21+ 资源包中 squid 纹理移至子目录 textures/entity/squid/squid.png
    {"squid", {"entity/squid/squid"}},
    {"glow_squid", {"entity/squid/glow_squid"}},
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
    // 沼骸骨（bogged）与普通骷髅同族，纹理位于 entity/skeleton/ 下而非 entity/bogged/
    {"bogged", {"entity/skeleton/bogged"}},
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

    // 投掷物和特殊实体
    // MC 1.21+ 资源包中多数投掷物没有 entity/ 下的纹理，
    // 需要回退到 item/ 目录下的纹理
    {"snowball", {"item/snowball"}},
    {"egg", {"item/egg"}},
    {"ender_pearl", {"item/ender_pearl"}},
    {"potion", {"item/potion"}},
    {"experience_bottle", {"item/experience_bottle"}},
    {"fireball", {"entity/fireball"}},
    {"fishing_bobber", {"entity/fishing_hook"}},
    {"eye_of_ender", {"item/ender_pearl"}},
    {"experience_orb", {"entity/experience_orb"}},

    // MC 1.21+ 新增气候变体实体 - 需要在 SPECIAL_TEXTURE_PATHS 中注册
    // 以避免回退到不存在的默认路径
    {"axolotl",
        {"entity/axolotl/axolotl_lucy",
            "entity/axolotl/axolotl_wild",
            "entity/axolotl/axolotl_gold",
            "entity/axolotl/axolotl_cyan",
            "entity/axolotl/axolotl_blue"}},
    {"trader_llama", {"entity/llama/creamy"}},

    // MC 1.21+ 新增的其他实体
    {"allay", {"entity/allay/allay"}},
    {"armadillo", {"entity/armadillo"}},
    {"breeze", {"entity/breeze/breeze"}},
    {"camel", {"entity/camel/camel"}},
    {"copper_golem", {"entity/copper_golem/copper_golem"}},
    {"creaking", {"entity/creaking/creaking"}},
    {"frog", {"entity/frog/temperate_frog", "entity/frog/warm_frog", "entity/frog/cold_frog"}},
    {"goat", {"entity/goat/goat"}},
    {"nautilus", {"entity/nautilus/nautilus"}},
    {"zombie_nautilus", {"entity/nautilus/zombie_nautilus", "entity/nautilus/zombie_nautilus_coral"}},
    {"sniffer", {"entity/sniffer/sniffer"}},
    {"tadpole", {"entity/tadpole/tadpole"}},
    {"warden", {"entity/warden/warden"}},
};

/**
 * @brief 附加纹理映射（一个实体需要多个纹理文件）
 *
 * 键为实体名称，值为附加纹理路径列表。
 */
const std::unordered_map<std::string, std::vector<std::string>> ADDITIONAL_TEXTURES = {
    // 羊的毛皮层 - MC 1.21+ 资源包中已从 sheep_fur 重命名为 sheep_wool
    {"sheep", {"entity/sheep/sheep_wool"}},

    // 村民多层纹理
    // 类型层 - 根据生物群系
    {"villager",
        {"entity/villager/type/desert",
            "entity/villager/type/jungle",
            "entity/villager/type/plains",
            "entity/villager/type/savanna",
            "entity/villager/type/snow",
            "entity/villager/type/swamp",
            "entity/villager/type/taiga",
            // 职业层 - 根据职业
            // MC 1.21+ 资源包中没有 none.png，无职业时不渲染职业层
            "entity/villager/profession/armorer",
            "entity/villager/profession/butcher",
            "entity/villager/profession/cartographer",
            "entity/villager/profession/cleric",
            "entity/villager/profession/farmer",
            "entity/villager/profession/fisherman",
            "entity/villager/profession/fletcher",
            "entity/villager/profession/leatherworker",
            "entity/villager/profession/librarian",
            "entity/villager/profession/mason",
            "entity/villager/profession/nitwit",
            "entity/villager/profession/shepherd",
            "entity/villager/profession/toolsmith",
            "entity/villager/profession/weaponsmith",
            // 等级徽章层
            "entity/villager/profession_level/stone",
            "entity/villager/profession_level/iron",
            "entity/villager/profession_level/gold",
            "entity/villager/profession_level/emerald",
            "entity/villager/profession_level/diamond"}},

    // 僵尸村民多层纹理（与村民相同的层结构）
    {"zombie_villager",
        {"entity/zombie_villager/type/desert",
            "entity/zombie_villager/type/jungle",
            "entity/zombie_villager/type/plains",
            "entity/zombie_villager/type/savanna",
            "entity/zombie_villager/type/snow",
            "entity/zombie_villager/type/swamp",
            "entity/zombie_villager/type/taiga",
            // 职业层
            // MC 1.21+ 资源包中没有 none.png，无职业时不渲染职业层
            "entity/zombie_villager/profession/armorer",
            "entity/zombie_villager/profession/butcher",
            "entity/zombie_villager/profession/cartographer",
            "entity/zombie_villager/profession/cleric",
            "entity/zombie_villager/profession/farmer",
            "entity/zombie_villager/profession/fisherman",
            "entity/zombie_villager/profession/fletcher",
            "entity/zombie_villager/profession/leatherworker",
            "entity/zombie_villager/profession/librarian",
            "entity/zombie_villager/profession/mason",
            "entity/zombie_villager/profession/nitwit",
            "entity/zombie_villager/profession/shepherd",
            "entity/zombie_villager/profession/toolsmith",
            "entity/zombie_villager/profession/weaponsmith",
            // 等级徽章层
            "entity/zombie_villager/profession_level/stone",
            "entity/zombie_villager/profession_level/iron",
            "entity/zombie_villager/profession_level/gold",
            "entity/zombie_villager/profession_level/emerald",
            "entity/zombie_villager/profession_level/diamond"}},
};

} // namespace

bool EntityTextureLoader::needsTexture(entity::EntityClassification classification)
{
    switch (classification) {
        case entity::EntityClassification::Creature:                 // 动物
        case entity::EntityClassification::WaterCreature:            // 水生生物
        case entity::EntityClassification::WaterAmbient:             // 水生环境生物（鱼）
        case entity::EntityClassification::Ambient:                  // 环境生物（蝙蝠）
        case entity::EntityClassification::Monster:                  // 怪物
        case entity::EntityClassification::Axolotls:                 // 美西螈
        case entity::EntityClassification::UndergroundWaterCreature: // 地下水生生物（发光鱿鱼）
            return true;
        case entity::EntityClassification::Misc: // 物品、经验球等
            // Misc 类别的实体通常不加载纹理，但经验球等需要纹理
            // 通过 SPECIAL_TEXTURE_PATHS 的存在来判断是否需要纹理
            return false;
        default:
            return false;
    }
}

Result<u32> EntityTextureLoader::loadAllEntityTextures(
    const std::vector<IResourcePack*>& packs, EntityTextureAtlas& atlas)
{

    u32 loadedCount = 0;
    auto& registry = entity::EntityRegistry::instance();
    const auto& allTypes = registry.getAllTypes();

    spdlog::info("EntityTextureLoader: Loading textures for {} registered entity types", allTypes.size());

    for (const auto& type : allTypes) {
        // 只加载需要渲染的实体类型
        if (!needsTexture(type.classification())) {
            continue;
        }

        const std::string& entityName = type.name();

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
                    spdlog::info(
                        "EntityTextureLoader: Loaded texture '{}' for entity '{}'", loc.toString(), entityName);
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
    loadedCount += _loadAdditionalTextures(packs, atlas);

    // 加载 Misc 类别中需要纹理的实体（经验球等）
    // 这些实体在主循环中因 needsTexture(Misc) = false 被跳过
    loadedCount += _loadMiscEntityTextures(packs, atlas);

    return loadedCount;
}

Result<u32> EntityTextureLoader::loadDefaultTextures(mc::IResourcePack& pack, EntityTextureAtlas& atlas)
{
    // 向后兼容：使用新的 loadAllEntityTextures
    std::vector<IResourcePack*> packs{&pack};
    return loadAllEntityTextures(packs, atlas);
}

Result<void> EntityTextureLoader::loadEntityTexture(
    mc::IResourcePack& pack, EntityTextureAtlas& atlas, const std::string& entityTypeId)
{
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

u32 EntityTextureLoader::_loadAdditionalTextures(const std::vector<IResourcePack*>& packs, EntityTextureAtlas& atlas)
{

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
                    break;
                } else if (!shouldSuppressMissingTextureWarning(loc)) {
                    spdlog::warn("Failed to load entity texture: {} - {}", loc.toString(), result.error().toString());
                }
            }
        }
    }

    return loadedCount;
}

u32 EntityTextureLoader::_loadMiscEntityTextures(const std::vector<IResourcePack*>& packs, EntityTextureAtlas& atlas)
{
    // Misc 类别的实体在主循环中被 needsTexture() 跳过，
    // 但 SPECIAL_TEXTURE_PATHS 中存在的实体需要加载纹理
    static const std::vector<std::string> MISC_TEXTURE_ENTITIES = {
        "experience_orb",
    };

    u32 loadedCount = 0;
    auto& registry = entity::EntityRegistry::instance();
    const auto& allTypes = registry.getAllTypes();

    for (const auto& type : allTypes) {
        // 只处理 Misc 类别
        if (type.classification() != entity::EntityClassification::Misc) {
            continue;
        }

        const std::string& entityName = type.name();

        // 检查是否在 Misc 纹理实体列表中
        bool isMiscTextureEntity = false;
        for (const auto& name : MISC_TEXTURE_ENTITIES) {
            if (entityName == name) {
                isMiscTextureEntity = true;
                break;
            }
        }
        if (!isMiscTextureEntity) {
            continue;
        }

        // 获取纹理路径
        auto paths = getTexturePaths(entityName);

        // 尝试从资源包加载
        bool loaded = false;
        for (auto it = packs.rbegin(); it != packs.rend() && !loaded; ++it) {
            auto* pack = *it;
            if (!pack) continue;

            for (const auto& loc : paths) {
                auto result = atlas.addTexture(*pack, loc);
                if (result.success()) {
                    loaded = true;
                    spdlog::info("EntityTextureLoader: Loaded Misc entity texture '{}' for entity '{}'",
                        loc.toString(),
                        entityName);
                    break;
                }
            }
        }

        if (!loaded) {
            spdlog::warn("EntityTextureLoader: No texture found for Misc entity {}", entityName);
        } else {
            loadedCount++;
        }
    }

    return loadedCount;
}

std::vector<ResourceLocation> EntityTextureLoader::getTexturePaths(const std::string& entityTypeId)
{
    std::vector<ResourceLocation> paths;
    std::string name = _parseEntityName(entityTypeId);

    // 检查特殊路径映射
    auto it = SPECIAL_TEXTURE_PATHS.find(name);
    if (it != SPECIAL_TEXTURE_PATHS.end()) {
        for (const auto& texturePath : it->second) {
            paths.emplace_back("minecraft:textures/" + texturePath + ".png");
        }
        return paths;
    }

    // 默认约定: textures/entity/<name>/<name>.png (MC 1.13+ 格式)
    ResourceLocation modernLoc("minecraft:textures/entity/" + name + "/" + name + ".png");
    paths.push_back(modernLoc);

    // 使用 getAltTexturePath() 自动计算 MC 1.12 扁平格式变体
    // 例如：textures/entity/pig/pig -> textures/entity/pig
    std::string altPath = resource::atlas::TexturePathVariant::getAltTexturePath(modernLoc.path());
    if (!altPath.empty()) {
        paths.emplace_back("minecraft", std::move(altPath));
    }

    return paths;
}

std::string EntityTextureLoader::_parseEntityName(const std::string& entityTypeId)
{
    // 解析 "minecraft:pig" -> "pig"
    size_t colonPos = entityTypeId.find(':');
    if (colonPos != std::string::npos && colonPos + 1 < entityTypeId.size()) {
        return entityTypeId.substr(colonPos + 1);
    }
    return entityTypeId;
}

} // namespace mc::client
