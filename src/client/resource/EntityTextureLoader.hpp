#pragma once

#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
class IResourcePack;

namespace entity {
enum class EntityClassification : u8;
}
} // namespace mc

namespace mc::client {

// 导入 EntityTextureAtlas 类型
using renderer::entity::pipeline::EntityTextureAtlas;

/**
 * @brief 实体纹理加载器
 *
 * 从资源包加载实体纹理并构建纹理图集。
 * 支持 MC 1.12 和 MC 1.13+ 的纹理路径格式。
 * 自动从 EntityRegistry 获取需要纹理的实体列表。
 *
 * 参考 MC 1.16.5 实体纹理加载
 */
class EntityTextureLoader {
public:
    EntityTextureLoader() = default;
    ~EntityTextureLoader() = default;

    // 禁止拷贝
    EntityTextureLoader(const EntityTextureLoader&) = delete;
    EntityTextureLoader& operator=(const EntityTextureLoader&) = delete;

    /**
     * @brief 从所有资源包加载所有实体纹理
     *
     * 遍历 EntityRegistry 中所有需要纹理的实体类型，
     * 并从资源包中加载对应的纹理。
     *
     * @param packs 资源包列表（按优先级从低到高排列）
     * @param atlas 纹理图集（需要已初始化）
     * @return 加载的纹理数量
     */
    [[nodiscard]] Result<u32> loadAllEntityTextures(
        const std::vector<IResourcePack*>& packs, EntityTextureAtlas& atlas);

    /**
     * @brief 加载默认实体纹理（向后兼容）
     *
     * @param pack 单个资源包
     * @param atlas 纹理图集
     * @return 加载的纹理数量
     */
    [[nodiscard]] Result<u32> loadDefaultTextures(mc::IResourcePack& pack, EntityTextureAtlas& atlas);

    /**
     * @brief 加载指定实体类型的纹理
     * @param pack 资源包
     * @param atlas 纹理图集
     * @param entityTypeId 实体类型ID（如 "minecraft:pig"）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadEntityTexture(
        mc::IResourcePack& pack, EntityTextureAtlas& atlas, const std::string& entityTypeId);

    /**
     * @brief 获取实体纹理路径
     * @param entityTypeId 实体类型ID
     * @return 纹理资源位置列表（尝试多个路径）
     */
    [[nodiscard]] static std::vector<ResourceLocation> getTexturePaths(const std::string& entityTypeId);

    /**
     * @brief 判断实体分类是否需要纹理
     * @param classification 实体分类
     * @return 是否需要加载纹理
     */
    [[nodiscard]] static bool needsTexture(entity::EntityClassification classification);

private:
    /**
     * @brief 解析实体类型名称
     * @param entityTypeId 实体类型ID（如 "minecraft:pig"）
     * @return 实体名称（如 "pig"）
     */
    [[nodiscard]] static std::string parseEntityName(const std::string& entityTypeId);

    /**
     * @brief 加载附加纹理（如羊的毛皮层）
     * @param packs 资源包列表
     * @param atlas 纹理图集
     * @return 加载的纹理数量
     */
    [[nodiscard]] u32 loadAdditionalTextures(const std::vector<IResourcePack*>& packs, EntityTextureAtlas& atlas);
};

} // namespace mc::client
