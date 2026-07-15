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

#include "../JigsawStructure.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 试炼密室结构
 *
 * 在主世界深板岩层生成的地下结构，包含试炼刷怪笼和宝库。
 * 使用 Jigsaw 模板池系统生成，由柱廊、过道、交叉口和决斗室组成。
 *
 * 生成参数：
 * - 起始池: minecraft:trial_chambers/chamber/end
 * - 深度: 20
 * - 起始高度: Y=-40 到 Y=-20（均匀分布）
 * - 间距: 34 区块，分离: 12 区块
 * - 盐值: 94251327
 * - 地形适配: ENCAPSULATE（用凝灰岩砖完全包裹）
 * - 最大距离: 116
 * - 维度填充: 10
 */
class TrialChambersStructure : public JigsawStructure {
public:
    TrialChambersStructure();
    ~TrialChambersStructure() override = default;

    [[nodiscard]] const std::string& name() const override { return s_name; }

    /**
     * @brief 获取试炼密室关联的生物群系标签
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] DecorationStage defaultDecorationStage() const override
    {
        return DecorationStage::UndergroundStructures;
    }

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 创建试炼密室的池别名绑定
     *
     * 试炼密室使用池别名来随机化刷怪笼类型：
     * - melee → zombie / husk / spider
     * - small_melee → slime / cave_spider / silverfish / baby_zombie
     * - ranged → skeleton / stray / poison_skeleton
     * - slow_ranged → skeleton / stray / poison_skeleton（低频率变体）
     *
     * @return 配置好的池别名绑定集合
     */
    static jigsaw::PoolAliasBindings createPoolAliases();

private:
    static const std::string s_name;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
