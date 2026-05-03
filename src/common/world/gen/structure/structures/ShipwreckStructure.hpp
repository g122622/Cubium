#pragma once

#include "../Structure.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../feature/template/Template.hpp"
#include "../../feature/template/TemplateManager.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::structure {

/**
 * @brief 沉船配置
 *
 * 参考 MC 1.16.5 ShipwreckConfig
 */
struct ShipwreckConfig {
    bool isBeached = false;  ///< 是否为搁浅沉船（在沙滩上）
};

/**
 * @brief 沉船结构片段
 *
 * 参考 MC 1.16.5 ShipwreckPieces.Piece
 * 使用模板系统生成沉船。
 */
class ShipwreckPiece : public StructurePiece {
public:
    /**
     * @brief 构造函数
     * @param templateName 模板名称（资源位置）
     * @param position 放置位置
     * @param rotation 旋转角度
     * @param isBeached 是否为搁浅沉船
     */
    ShipwreckPiece(
        const String& templateName,
        const BlockPos& position,
        Rotation rotation,
        bool isBeached);

    ~ShipwreckPiece() override = default;

    void generate(IWorldWriter& world, math::Random& rng,
                  i32 chunkX, i32 chunkZ,
                  const StructureBoundingBox& chunkBounds) override;

    /**
     * @brief 设置模板管理器
     *
     * 必须在 generate 之前调用
     */
    void setTemplateManager(feature::template_::TemplateManager* manager) { m_templateManager = manager; }

    [[nodiscard]] const String& templateName() const { return m_templateName; }
    [[nodiscard]] bool isBeached() const { return m_isBeached; }

    // 结构偏移（MC 1.16.5: BlockPos(4, 0, 15)）
    static const BlockPos STRUCTURE_OFFSET;

private:
    void loadTemplate();

    String m_templateName;
    Rotation m_rotation;
    bool m_isBeached;
    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_template = nullptr;
    BlockPos m_size;
};

/**
 * @brief 沉船结构
 *
 * 参考 MC 1.16.5 ShipwreckStructure
 * 使用模板系统生成沉船，支持水下和搁浅两种类型。
 */
class ShipwreckStructure : public Structure {
public:
    ShipwreckStructure();

    [[nodiscard]] const String& name() const override { return m_name; }
    [[nodiscard]] StructureSeparationSettings separationSettings() const override { return m_settings; }
    [[nodiscard]] const std::vector<BiomeId>& validBiomes() const override { return m_validBiomes; }

    [[nodiscard]] bool canGenerate(
        IWorld& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IWorldWriter& world,
        IChunkGenerator& generator,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ) const override;

    /**
     * @brief 设置模板管理器
     */
    void setTemplateManager(feature::template_::TemplateManager* manager) { m_templateManager = manager; }

    /**
     * @brief 设置配置
     */
    void setConfig(const ShipwreckConfig& config) { m_config = config; }

    [[nodiscard]] const ShipwreckConfig& config() const { return m_config; }

    // 模板名称常量（公开供测试访问）
    // 搁浅沉船变体（只有部分变体）
    static const std::vector<String> s_beachedTemplates;
    // 所有沉船变体（包括水下）
    static const std::vector<String> s_allTemplates;

private:
    void initializeBiomes();

    /**
     * @brief 获取随机沉船模板名称
     * @param rng 随机数生成器
     * @param isBeached 是否为搁浅沉船
     * @return 模板名称
     */
    [[nodiscard]] String getRandomTemplateName(math::Random& rng, bool isBeached) const;

    static constexpr StructureSeparationSettings m_settings{24, 4, 165745295};  // MC 1.16.5: 165745295
    static const String m_name;
    std::vector<BiomeId> m_validBiomes;
    ShipwreckConfig m_config;
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace mc::world::gen::structure
