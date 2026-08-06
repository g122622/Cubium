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

#pragma once

#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/nbt/Nbt.hpp"
#include "Template.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

namespace resource {
class IResourcePack;
} // namespace resource

namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 模板 NBT 加载器
 *
 * 从 Minecraft 的 .nbt 文件格式加载结构模板。
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
        const resource::IResourcePack& pack, const ResourceLocation& location);

    /**
     * @brief 从原始 NBT 字节加载模板
     * @param data NBT 压缩数据（gzip）
     * @return 加载的模板，失败返回 nullptr
     */
    [[nodiscard]] static std::unique_ptr<Template> loadFromCompressedNbt(const std::vector<u8>& data);

    /**
     * @brief 从基岩版 .mcstructure 字节加载模板
     *
     * 基岩版 .mcstructure 是未压缩的小端序 NBT，schema 与 Java .nbt 不同：
     * 根含 format_version / size / structure{block_indices, palette{default{block_palette}}, entities}
     * / structure_world_origin。block_indices 是 List<List<Int>>（每层 palette 一个索引数组），
     * block_palette 每项是 {name, states, version} 复合标签。方块坐标由 size 三维索引线性推算。
     *
     * @param data .mcstructure 原始字节（未压缩）
     * @return 加载的模板，失败返回 nullptr
     */
    [[nodiscard]] static std::unique_ptr<Template> loadFromBedrockMcStructure(const std::vector<u8>& data);

private:
    /**
     * @brief 解包 NBT 根复合标签的空键嵌套层
     *
     * NBT 库的 `compound_tag::read`/`read_compound_bin` 不跳过根 tag 的 id+name 前缀，
     * 而是把根字节（0x0A Compound）+ 根 name 当成 body 的第一个子项读入。对于根 name 非空的文件
     * （如 level.dat 根 name="Data"），此 bug 使 root = `{"Data": <真内容>}`，恰好与现有 reader
     * 期望的键名一致而"歪打正着"工作；但对于根 name 为空的文件（Java 结构 .nbt 与基岩 .mcstructure
     * 根 name 均为空），root = `{"": <真内容>}`，直接取 "size"/"structure" 等键取不到，模板退化为
     * size=(0,0,0)。
     *
     * 本函数检测并解包此空键嵌套：若 root 仅含一个空键 `""` 且其值为 Compound，则返回该内层 Compound
     * 的裸指针（非拥有，调用方不得持久持有），否则返回入参 root 本身。仅作用于 TemplateLoader 的两个
     * 加载入口（结构文件根 name 恒为空），不触碰 NBT 库核心以避免影响 level.dat 等依赖"歪打正着"的 reader。
     *
     * @param root `compound_tag::read` 返回的根复合标签
     * @return 真正承载结构内容的复合标签（非拥有指针）
     */
    [[nodiscard]] static const nbt::CompoundTag* _unwrapRootCompound(const nbt::CompoundTag& root) noexcept;

    /**
     * @brief 从基岩版 NBT 根标签加载模板（schema 解析核心）
     * @param root 已解析的根复合标签（bedrock_disk 上下文）
     * @return 加载的模板
     */
    [[nodiscard]] static std::unique_ptr<Template> _loadFromBedrockNbt(const nbt::CompoundTag& root);

    /**
     * @brief 解析基岩版 block_palette 单项的方块状态 ID
     *
     * 基岩版 palette 项字段为小写 name / states / version（区别于 Java 的 Name / Properties）。
     * states 是 Compound，键为属性名、值为对应属性标签（String/Int/Byte 等）。
     *
     * @param paletteEntry block_palette 中的方块状态复合标签
     * @return 方块状态 ID
     */
    [[nodiscard]] static u32 _parseBedrockBlockStateId(const nbt::CompoundTag& paletteEntry);
    [[nodiscard]] static BlockPos _readBlockPos(const nbt::ListTag& list);
    [[nodiscard]] static std::unique_ptr<nbt::CompoundTag> _cloneNbt(const nbt::CompoundTag* source) noexcept;

    /**
     * @brief 解析方块状态 ID
     * @param paletteEntry palette 中的方块状态 NBT
     * @return 方块状态 ID
     */
    [[nodiscard]] static u32 _parseBlockStateId(const nbt::CompoundTag& paletteEntry);

    /**
     * @brief 解析 Jigsaw 方块信息
     * @param nbt 方块实体 NBT
     * @param pos 方块位置
     * @param blockStateId 方块状态 ID（用于读取 orientation 属性）
     * @return Jigsaw 信息，如果不是 Jigsaw 方块返回空
     */
    [[nodiscard]] static TemplateJigsawBlockInfo _parseJigsawBlock(
        const nbt::CompoundTag* nbt, const BlockPos& pos, u32 blockStateId);
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
