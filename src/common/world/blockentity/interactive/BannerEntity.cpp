#include "world/blockentity/interactive/BannerEntity.hpp"
#include "world/IWorld.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== BannerEntity 实现 ==========

BannerEntity::BannerEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Banner, pos) {
}

BannerEntity::~BannerEntity() = default;

bool BannerEntity::addPattern(const BannerPattern& pattern) {
    if (static_cast<i32>(m_patterns.size()) >= MAX_PATTERNS) {
        return false;
    }

    m_patterns.push_back(pattern);
    setChanged();
    return true;
}

void BannerEntity::setPatterns(const std::vector<BannerPattern>& patterns) {
    m_patterns.clear();
    for (const auto& pattern : patterns) {
        if (static_cast<i32>(m_patterns.size()) < MAX_PATTERNS) {
            m_patterns.push_back(pattern);
        }
    }
    setChanged();
}

bool BannerEntity::removeTopPattern() {
    if (m_patterns.empty()) {
        return false;
    }

    m_patterns.pop_back();
    setChanged();
    return true;
}

void BannerEntity::clearPatterns() {
    m_patterns.clear();
    setChanged();
}

void BannerEntity::setBaseColor(i32 color) {
    if (m_baseColor != color) {
        m_baseColor = color;
        setChanged();
    }
}

String BannerEntity::getTextureName() const {
    // 生成纹理名称，用于渲染
    // 格式: banner_<base_color>_<pattern_hash>
    String name = "banner_" + std::to_string(m_baseColor);

    // 添加图案信息
    for (const auto& pattern : m_patterns) {
        name += "_" + pattern.pattern + "_" + std::to_string(pattern.color);
    }

    return name;
}

void BannerEntity::tick(IWorld& world) {
    MC_UNUSED(world);
    // 旗帜不需要tick更新
}

bool BannerEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载底色
    if (data.contains("base_color")) {
        m_baseColor = data["base_color"].get<i32>();
    }

    // 加载图案
    m_patterns.clear();
    if (data.contains("patterns")) {
        const auto& patternsJson = data["patterns"];
        if (patternsJson.is_array()) {
            for (const auto& patternJson : patternsJson) {
                BannerPattern pattern;
                if (patternJson.contains("pattern")) {
                    pattern.pattern = patternJson["pattern"].get<String>();
                }
                if (patternJson.contains("color")) {
                    pattern.color = patternJson["color"].get<i32>();
                }
                m_patterns.push_back(pattern);
            }
        }
    }

    return true;
}

void BannerEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    // 保存底色
    data["base_color"] = m_baseColor;

    // 保存图案
    nlohmann::json patternsJson = nlohmann::json::array();
    for (const auto& pattern : m_patterns) {
        nlohmann::json patternJson;
        patternJson["pattern"] = pattern.pattern;
        patternJson["color"] = pattern.color;
        patternsJson.push_back(patternJson);
    }
    data["patterns"] = patternsJson;
}

std::unique_ptr<BlockEntity> BannerEntity::clone() const {
    auto clone = std::make_unique<BannerEntity>(m_pos);
    clone->m_patterns = m_patterns;
    clone->m_baseColor = m_baseColor;
    return clone;
}

} // namespace blockentity
} // namespace mc
