#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "BlockModelLoader.hpp"
#include <memory>
#include <map>

namespace mc {

class IResourcePack;

/**
 * @brief 方块状态加载器
 *
 * 解析 blockstates 目录下的 JSON 文件，管理方块状态到模型的映射
 */
class BlockStateLoader {
public:
    BlockStateLoader() = default;

    // 从资源包加载方块状态
    [[nodiscard]] Result<void> loadFromResourcePack(IResourcePack& resourcePack);

    // 获取方块状态定义
    [[nodiscard]] const BlockStateDefinition* getBlockState(const ResourceLocation& blockId) const;

    // 获取方块状态的模型变体
    // stateStr格式: "axis=y,facing=north" 或 "normal"
    [[nodiscard]] const BlockStateVariant* getVariant(
        const ResourceLocation& blockId,
        StringView stateStr) const;

    // 根据方块属性获取模型变体
    [[nodiscard]] const BlockStateVariant* getVariant(
        const ResourceLocation& blockId,
        const std::map<String, String>& properties) const;

    // 清除缓存
    void clearCache();

    // 获取所有已加载的方块状态名称
    [[nodiscard]] std::vector<ResourceLocation> getLoadedBlockStates() const;

private:
    std::map<ResourceLocation, BlockStateDefinition> m_blockStates;
    IResourcePack* m_resourcePack = nullptr;

    // 将属性映射转换为状态字符串
    [[nodiscard]] static String propertiesToStateStr(const std::map<String, String>& properties);
};

} // namespace mc
