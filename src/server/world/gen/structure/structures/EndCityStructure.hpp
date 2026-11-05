/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction restriction, including without limitation the rights
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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {
class TemplateManager;
}
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc

namespace mc {
namespace world {
namespace gen {
namespace structure {

// Forward declarations
namespace end_city {
class CityTemplate;
class IGenerator;
class HouseTowerGenerator;
class TowerGenerator;
class TowerBridgeGenerator;
class FatTowerGenerator;
struct CityGenerators;
} // namespace end_city

/**
 * @brief 末地城结构
 *
 * 在末地外岛生成的城市结构，由末地石砖和紫珀块构成。
 * 包含塔楼、房屋和末地船（有概率生成）。
 * 潜影贝会在末地城内生成。
 */
class EndCityStructure : public Structure {
public:
    explicit EndCityStructure(ResourceLocation id);
    ~EndCityStructure() override = default;

    [[nodiscard]] const std::string& name() const override
    {
        return s_name;
    }

    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const override;

private:
    /**
     * @brief 取 5x5 区域内四个角点的最低高度
     *
     * 四角偏移由旋转决定（±5），所以旋转必须由调用方传入——原版也是先抽旋转、
     * 再用它去取样。此前这里自己造了一个随机源抽旋转，既不消耗真实随机序列，
     * 判定用的旋转也与实际生成用的不一致。
     */
    [[nodiscard]] static i32 _getYPosition(
        i32 chunkX, i32 chunkZ, feature::template_::Rotation rotation, IChunkGenerator& generator);

    /// 最大递归深度
    static constexpr i32 MAX_DEPTH = 8;

    static const std::string s_name;
};

namespace end_city {

/**
 * @brief 末地城模板片段
 *
 * 使用模板系统生成末地城各部分。
 */
class CityTemplate : public StructurePiece {
public:
    CityTemplate(
        const std::string& templateName, const BlockPos& pos, feature::template_::Rotation rotation, bool overwrite);

    void generate(IWorldWriter& world,
        math::IRandom& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    /**
     * @brief 计算连接位置
     *
     * 计算从当前模板的某个位置连接到新模板的位置偏移
     */
    [[nodiscard]] BlockPos calculateConnectedPos(
        const BlockPos& localPos, feature::template_::Rotation newRotation) const;

    [[nodiscard]] const std::string& templateName() const
    {
        return m_templateName;
    }

    [[nodiscard]] feature::template_::Rotation rotation() const
    {
        return getRotation();
    }

    [[nodiscard]] bool overwrite() const
    {
        return m_overwrite;
    }

    [[nodiscard]] const BlockPos& templatePosition() const
    {
        return m_templatePosition;
    }

    [[nodiscard]] feature::template_::PlacementSettings& placementSettings()
    {
        return m_settings;
    }

    /**
     * @brief 平移片段
     *
     * `m_templatePosition` 必须与包围盒同步平移：它既是方块写入（`generate`）的原点，
     * 也是 `calculateConnectedPos` 的基准。只移动基类坐标会让后续所有子构件按旧基准
     * 计算连接点，整座结构逐渐偏移。
     */
    void offset(i32 dx, i32 dy, i32 dz) override
    {
        StructurePiece::offset(dx, dy, dz);
        m_templatePosition.x += dx;
        m_templatePosition.y += dy;
        m_templatePosition.z += dz;
    }

    /// 唯一ID，用于碰撞检测
    i32 componentId = 0;

    /// 挂载本座城的生成器（由 startHouseTower 在生成开始时调用）
    void setGenerators(std::shared_ptr<CityGenerators> generators);

    /// 从父片段继承生成器集合（同一座城的片段共享一组）
    void shareGeneratorsFrom(const CityTemplate& parent);
    [[nodiscard]] HouseTowerGenerator& houseTowerGen();
    [[nodiscard]] TowerGenerator& towerGen();
    [[nodiscard]] TowerBridgeGenerator& bridgeGen();
    [[nodiscard]] FatTowerGenerator& fatTowerGen();

    /// 片段来源的模板名（仅用于诊断与差分输出）
    [[nodiscard]] std::string_view templateLocation() const noexcept override
    {
        return m_templatePath;
    }

private:
    std::shared_ptr<CityGenerators> m_generators;
    std::string m_templateName;

    /// 完整模板路径（`minecraft:end_city/<name>`），仅供诊断输出
    std::string m_templatePath;
    BlockPos m_templatePosition;
    bool m_overwrite;
    feature::template_::PlacementSettings m_settings;
    BlockPos m_size;
};

/**
 * @brief 生成器接口
 */
class IGenerator {
public:
    virtual ~IGenerator() = default;

    virtual void init() = 0;
    virtual bool generate(feature::template_::TemplateManager& templateManager,
        i32 depth,
        CityTemplate& parent,
        const BlockPos& offset,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng) = 0;
};

/**
 * @brief 房屋塔生成器
 */
class HouseTowerGenerator : public IGenerator {
public:
    void init() override {}

    bool generate(feature::template_::TemplateManager& templateManager,
        i32 depth,
        CityTemplate& parent,
        const BlockPos& offset,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng) override;
};

/**
 * @brief 塔生成器
 */
class TowerGenerator : public IGenerator {
public:
    void init() override {}

    bool generate(feature::template_::TemplateManager& templateManager,
        i32 depth,
        CityTemplate& parent,
        const BlockPos& offset,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng) override;
};

/**
 * @brief 塔桥生成器
 *
 * 包含末地船生成逻辑
 */
class TowerBridgeGenerator : public IGenerator {
public:
    void init() override
    {
        m_shipCreated = false;
    }

    bool generate(feature::template_::TemplateManager& templateManager,
        i32 depth,
        CityTemplate& parent,
        const BlockPos& offset,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng) override;

private:
    bool m_shipCreated = false;
};

/**
 * @brief 胖塔生成器
 */
class FatTowerGenerator : public IGenerator {
public:
    void init() override {}

    bool generate(feature::template_::TemplateManager& templateManager,
        i32 depth,
        CityTemplate& parent,
        const BlockPos& offset,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::IRandom& rng) override;
};

/**
 * @brief 一座末地城所用的分段生成器集合
 *
 * 生成器带可变状态（桥生成器的「本座城是否已生成末地船」），原版每次 generatePieces
 * 都重新 init() 一遍。实例因此与城市同生命周期：每次生成新建一组并挂在起始片段上，
 * 不能用函数内 static——否则状态会残留到下一座城。
 */
struct CityGenerators {
    HouseTowerGenerator houseTower;
    TowerGenerator tower;
    TowerBridgeGenerator bridge;
    FatTowerGenerator fatTower;
};

/**
 * @brief 添加片段到列表并返回引用
 */
CityTemplate* addHelper(std::vector<std::unique_ptr<StructurePiece>>& pieces, std::unique_ptr<CityTemplate> piece);

/**
 * @brief 创建新片段并连接到父片段
 */
std::unique_ptr<CityTemplate> addPiece(feature::template_::TemplateManager& templateManager,
    CityTemplate& parent,
    const BlockPos& offset,
    const std::string& templateName,
    feature::template_::Rotation rotation,
    bool overwrite);

/**
 * @brief 递归生成子片段
 */
bool recursiveChildren(feature::template_::TemplateManager& templateManager,
    IGenerator& generator,
    i32 depth,
    CityTemplate& parent,
    const BlockPos& offset,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::IRandom& rng);

/**
 * @brief 启动房屋塔生成
 */
void startHouseTower(feature::template_::TemplateManager& templateManager,
    const BlockPos& startPos,
    feature::template_::Rotation rotation,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::IRandom& rng);

} // namespace end_city

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
