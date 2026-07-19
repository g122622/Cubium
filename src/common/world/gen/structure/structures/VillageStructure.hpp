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

#include "../../chunk/IChunkGenerator.hpp"
#include "../JigsawStructure.hpp"
#include "../Structure.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 村庄类型
 *
 * 不同村庄风格
 */
enum class VillageType : u8 {
    Plains,  ///< 平原村庄
    Desert,  ///< 沙漠村庄
    Savanna, ///< 热带草原村庄
    Taiga,   ///< 针叶林村庄
    Snowy,   ///< 雪地村庄
    Zombie   ///< 僵尸村庄
};

/**
 * @brief 村庄配置
 */
struct VillageConfig {
    VillageType type = VillageType::Plains;
    i32 size = 6;           ///< 村庄大小
    i32 distance = 32;      ///< 村庄间距
    i32 separation = 8;     ///< 村庄分离距离
    i32 startPoolIndex = 0; ///< 起始模板池索引
    bool zombie = false;    ///< 是否为僵尸村庄
};

/**
 * @brief 村庄结构
 *
 * 使用 Jigsaw 系统生成的村庄结构。
 *
 * 村庄由多个建筑组成，通过 Jigsaw 连接点组装：
 * - 房屋（各种类型）
 * - 道路
 * - 中心广场
 * - 农田
 * - 铁匠铺
 * - 教堂
 * - 图书馆
 * 等等
 */
class VillageStructure : public Structure {
public:
    explicit VillageStructure(ResourceLocation id, VillageType type = VillageType::Plains);
    explicit VillageStructure(ResourceLocation id, const VillageConfig& config);

    [[nodiscard]] const std::string& name() const override { return m_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 村庄有多个变体，每个变体对应不同的生物群系标签。
     * 默认返回平原村庄标签，canGenerate() 中根据村庄类型进行详细检查。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    /**
     * @brief 检查是否可以生成
     */
    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    /**
     * @brief 生成村庄
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 获取村庄类型的起始模板池
     */
    [[nodiscard]] static ResourceLocation getStartPool(VillageType type);

    /**
     * @brief 获取村庄类型名称
     */
    [[nodiscard]] static const char* getVillageTypeName(VillageType type);

private:
    VillageConfig m_config;
    static const std::string m_name;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
