#pragma once

#include "Template.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/nbt/Nbt.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

class IResourcePack;

namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 模板 NBT 加载器
 *
 * 从 Minecraft 的 .nbt 文件格式加载结构模板。
 * 文件格式参考 MC 1.16.5 的 Template 类
 *
 * NBT 结构:
 * - size: [x, y, z] 模板大小
 * - blocks: 列表，每个元素包含:
 *   - pos: [x, y, z] 位置
 *   - state: 索引到 palette
 *   - nbt: 方块实体数据（可选）
 * - palette: 方块状态列表
 * - entities: 实体列表（可选）
 */
class TemplateLoader {
public:
    /**
     * @brief 从 NBT 数据加载模板
     * @param nbt NBT 复合标签
     * @return 加载的模板，失败返回 nullptr
     */
    [[nodiscard]] static std::unique_ptr<Template> loadFromNbt(const nbt::CompoundTag& nbt);

    /**
     * @brief 从资源包加载模板
     * @param pack 资源包
     * @param location 模板资源位置
     * @return 加载的模板，失败返回 nullptr
     */
    [[nodiscard]] static std::unique_ptr<Template> loadFromResourcePack(
        const IResourcePack& pack,
        const ResourceLocation& location);

    /**
     * @brief 从原始 NBT 字节加载模板
     * @param data NBT 压缩数据（gzip）
     * @return 加载的模板，失败返回 nullptr
     */
    [[nodiscard]] static std::unique_ptr<Template> loadFromCompressedNbt(const std::vector<u8>& data);

private:
    [[nodiscard]] static BlockPos readBlockPos(const nbt::ListTag& list);
    [[nodiscard]] static std::unique_ptr<nbt::CompoundTag> cloneNbt(const nbt::CompoundTag* source);

    /**
     * @brief 解析方块状态 ID
     * @param paletteEntry palette 中的方块状态 NBT
     * @return 方块状态 ID
     */
    [[nodiscard]] static u32 parseBlockStateId(const nbt::CompoundTag& paletteEntry);

    /**
     * @brief 解析 Jigsaw 方块信息
     * @param nbt 方块实体 NBT
     * @param pos 方块位置
     * @param blockStateId 方块状态 ID（用于读取 orientation 属性）
     * @return Jigsaw 信息，如果不是 Jigsaw 方块返回空
     */
    [[nodiscard]] static TemplateJigsawBlockInfo parseJigsawBlock(
        const nbt::CompoundTag* nbt,
        const BlockPos& pos,
        u32 blockStateId);
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
