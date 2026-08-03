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

#include "../../feature/template/Template.hpp"
#include "../Structure.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
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

    [[nodiscard]] const std::string& name() const override { return s_name; }
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

private:
    /**
     * @brief 计算末地城生成高度
     */
    [[nodiscard]] static i32 _getYPosition(i32 chunkX, i32 chunkZ, IChunkGenerator& generator);

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
        math::Random& rng,
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

    [[nodiscard]] const std::string& templateName() const { return m_templateName; }
    [[nodiscard]] feature::template_::Rotation rotation() const { return m_rotation; }
    [[nodiscard]] bool overwrite() const { return m_overwrite; }
    [[nodiscard]] const BlockPos& templatePosition() const { return m_templatePosition; }
    [[nodiscard]] feature::template_::PlacementSettings& placementSettings() { return m_settings; }

    /// 唯一ID，用于碰撞检测
    i32 componentId = 0;

private:
    std::string m_templateName;
    BlockPos m_templatePosition;
    feature::template_::Rotation m_rotation;
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
        math::Random& rng) = 0;
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
        math::Random& rng) override;
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
        math::Random& rng) override;
};

/**
 * @brief 塔桥生成器
 *
 * 包含末地船生成逻辑
 */
class TowerBridgeGenerator : public IGenerator {
public:
    void init() override { m_shipCreated = false; }
    bool generate(feature::template_::TemplateManager& templateManager,
        i32 depth,
        CityTemplate& parent,
        const BlockPos& offset,
        std::vector<std::unique_ptr<StructurePiece>>& pieces,
        math::Random& rng) override;

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
        math::Random& rng) override;
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
    math::Random& rng);

/**
 * @brief 启动房屋塔生成
 */
void startHouseTower(feature::template_::TemplateManager& templateManager,
    const BlockPos& startPos,
    feature::template_::Rotation rotation,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng);

} // namespace end_city

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
