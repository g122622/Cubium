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

#include "world/blockentity/interactive/BannerEntity.hpp"
#include "item/core/ItemStack.hpp"
#include "text/ITextComponent.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockState.hpp"
#include "world/block/blocks/decorative/BannerBlock.hpp"

namespace mc {
namespace blockentity {

// ========== BannerEntity 实现 ==========

BannerEntity::BannerEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Banner, pos)
{}

BannerEntity::~BannerEntity() noexcept = default;

bool BannerEntity::addPattern(const BannerPattern& pattern)
{
    if (static_cast<i32>(m_patterns.size()) >= MAX_PATTERNS) {
        return false;
    }

    m_patterns.push_back(pattern);
    setChanged();
    return true;
}

void BannerEntity::setPatterns(const std::vector<BannerPattern>& patterns)
{
    m_patterns.clear();
    for (const auto& pattern : patterns) {
        if (static_cast<i32>(m_patterns.size()) < MAX_PATTERNS) {
            m_patterns.push_back(pattern);
        }
    }
    setChanged();
}

bool BannerEntity::removeTopPattern()
{
    if (m_patterns.empty()) {
        return false;
    }

    m_patterns.pop_back();
    setChanged();
    return true;
}

void BannerEntity::clearPatterns()
{
    m_patterns.clear();
    setChanged();
}

void BannerEntity::setBaseColor(DyeColor color)
{
    if (m_baseColor != color) {
        m_baseColor = color;
        setChanged();
    }
}

std::string BannerEntity::getTextureName() const
{
    // 生成纹理名称，用于渲染
    // 格式: banner_<base_color>[_<pattern_hash>_<color>]*
    // 每个图案追加 "_<hash>_<color>"
    std::string name = "banner_" + std::to_string(static_cast<i32>(m_baseColor));

    // 添加图案信息
    for (const auto& pattern : m_patterns) {
        name +=
            "_" + BannerPatterns::getHashName(pattern.pattern) + "_" + std::to_string(static_cast<i32>(pattern.color));
    }

    return name;
}

void BannerEntity::tick(IWorld& world)
{
    MC_UNUSED(world);
    // 旗帜不需要tick更新
}

bool BannerEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载底色
    if (data.contains("base_color")) {
        m_baseColor = static_cast<DyeColor>(data["base_color"].get<i32>());
    }

    // 加载图案
    m_patterns.clear();
    if (data.contains("patterns")) {
        const auto& patternsJson = data["patterns"];
        if (patternsJson.is_array()) {
            for (const auto& patternJson : patternsJson) {
                BannerPattern pattern;
                if (patternJson.contains("pattern")) {
                    // 从哈希名解析图案类型
                    std::string hashName = patternJson["pattern"].get<std::string>();
                    pattern.pattern = BannerPatterns::byHash(hashName);
                }
                if (patternJson.contains("color")) {
                    pattern.color = static_cast<DyeColor>(patternJson["color"].get<i32>());
                }
                m_patterns.push_back(pattern);
            }
        }
    }

    return true;
}

void BannerEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // 保存底色
    data["base_color"] = static_cast<i32>(m_baseColor);

    // 保存图案
    nlohmann::json patternsJson = nlohmann::json::array();
    for (const auto& pattern : m_patterns) {
        nlohmann::json patternJson;
        patternJson["pattern"] = BannerPatterns::getHashName(pattern.pattern);
        patternJson["color"] = static_cast<i32>(pattern.color);
        patternsJson.push_back(patternJson);
    }
    data["patterns"] = patternsJson;
}

void BannerEntity::setCustomDisplayName(std::unique_ptr<text::ITextComponent> name)
{
    m_customName = std::move(name);
    setChanged();
}

const text::ITextComponent* BannerEntity::getCustomDisplayName() const
{
    return m_customName.get();
}

void BannerEntity::loadFromItemStack(const item::ItemStack& stack, DyeColor baseColor)
{
    m_baseColor = baseColor;

    // 从BlockEntityTag.Patterns读取图案
    const nlohmann::json* tag = stack.getChildTag("BlockEntityTag");
    if (tag != nullptr && tag->contains("Patterns")) {
        const auto& patternsJson = (*tag)["Patterns"];
        if (patternsJson.is_array()) {
            m_patterns.clear();
            for (const auto& patternJson : patternsJson) {
                if (static_cast<i32>(m_patterns.size()) >= MAX_PATTERNS) {
                    break;
                }
                BannerPattern pattern;
                if (patternJson.contains("Pattern")) {
                    std::string hashName = patternJson["Pattern"].get<std::string>();
                    pattern.pattern = BannerPatterns::byHash(hashName);
                }
                if (patternJson.contains("Color")) {
                    pattern.color = static_cast<DyeColor>(patternJson["Color"].get<i32>());
                }
                m_patterns.push_back(pattern);
            }
        }
    }

    // 从物品的自定义名称设置
    if (stack.hasCustomName()) {
        m_customName = stack.getDisplayName();
    }

    setChanged();
}

item::ItemStack BannerEntity::getItem(const BlockState& state) const
{
    // 根据底色查找对应的旗帜物品
    // 墙壁旗帜和站立旗帜共享同一个物品
    const auto* block = &state.getBlock();
    auto& itemRegistry = item::ItemRegistry::instance();

    // 通过方块位置查找对应物品
    const item::Item* bannerItem = itemRegistry.getItem(block->blockLocation());

    // 如果是墙壁旗帜，方块位置是 xxx_wall_banner，物品位置是 xxx_banner
    if (bannerItem == nullptr) {
        std::string blockPath = block->blockLocation().path();
        auto wallPos = blockPath.find("_wall_banner");
        if (wallPos != std::string::npos) {
            std::string itemPath = blockPath.substr(0, wallPos) + "_banner";
            bannerItem = itemRegistry.getItem(ResourceLocation(block->blockLocation().namespace_(), itemPath));
        }
    }

    // 最终回退：使用底色查找白色旗帜
    if (bannerItem == nullptr) {
        bannerItem = itemRegistry.getItem(ResourceLocation("minecraft", "white_banner"));
    }

    if (bannerItem == nullptr) {
        return item::ItemStack();
    }

    item::ItemStack result(*bannerItem, 1);

    // 将图案数据写入BlockEntityTag
    if (!m_patterns.empty()) {
        nlohmann::json& tag = result.getOrCreateChildTag("BlockEntityTag");
        nlohmann::json patternsJson = nlohmann::json::array();
        for (const auto& pattern : m_patterns) {
            nlohmann::json patternJson;
            patternJson["Pattern"] = BannerPatterns::getHashName(pattern.pattern);
            patternJson["Color"] = static_cast<i32>(pattern.color);
            patternsJson.push_back(patternJson);
        }
        tag["Patterns"] = patternsJson;
    }

    // 设置自定义名称
    if (m_customName != nullptr) {
        result.setCustomName(m_customName->getString());
    }

    return result;
}

// ========== 静态工具方法 ==========

std::vector<BannerPattern> BannerEntity::getPatternsFromItemStack(const item::ItemStack& stack)
{
    std::vector<BannerPattern> patterns;

    const nlohmann::json* tag = stack.getChildTag("BlockEntityTag");
    if (tag != nullptr && tag->contains("Patterns")) {
        const auto& patternsJson = (*tag)["Patterns"];
        if (patternsJson.is_array()) {
            for (const auto& patternJson : patternsJson) {
                BannerPattern pattern;
                if (patternJson.contains("Pattern")) {
                    std::string hashName = patternJson["Pattern"].get<std::string>();
                    pattern.pattern = BannerPatterns::byHash(hashName);
                }
                if (patternJson.contains("Color")) {
                    pattern.color = static_cast<DyeColor>(patternJson["Color"].get<i32>());
                }
                patterns.push_back(pattern);
            }
        }
    }

    return patterns;
}

i32 BannerEntity::getPatternCount(const item::ItemStack& stack)
{
    const nlohmann::json* tag = stack.getChildTag("BlockEntityTag");
    if (tag != nullptr && tag->contains("Patterns")) {
        const auto& patternsJson = (*tag)["Patterns"];
        if (patternsJson.is_array()) {
            return static_cast<i32>(patternsJson.size());
        }
    }
    return 0;
}

void BannerEntity::removeBannerData(item::ItemStack& stack)
{
    nlohmann::json* tag = stack.getChildTag();
    if (tag == nullptr) {
        return;
    }

    auto it = tag->find("BlockEntityTag");
    if (it == tag->end()) {
        return;
    }

    auto& blockEntityTag = it.value();
    if (!blockEntityTag.contains("Patterns")) {
        return;
    }

    auto& patterns = blockEntityTag["Patterns"];
    if (!patterns.is_array() || patterns.empty()) {
        return;
    }

    // 移除最顶层图案
    patterns.erase(patterns.size() - 1);

    // 如果图案列表为空，移除整个BlockEntityTag
    if (patterns.empty()) {
        tag->erase("BlockEntityTag");
    }
}

std::vector<std::pair<BannerPatternType, DyeColor>> BannerEntity::composePatterns(
    DyeColor baseColor, const std::vector<BannerPattern>& patterns)
{
    std::vector<std::pair<BannerPatternType, DyeColor>> result;

    // 第一层是底色
    result.emplace_back(BannerPatternType::Base, baseColor);

    // 追加所有图案层
    for (const auto& pattern : patterns) {
        result.emplace_back(pattern.pattern, pattern.color);
    }

    return result;
}

// ========== 序列化 ==========

bool BannerEntity::loadFromNBT(const nbt::tags::compound_tag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    // 加载自定义名称
    // TODO: 从NBT读取ITextComponent

    // 加载图案
    m_patterns.clear();
    // NBT序列化将在NBT系统完善后实现

    setChanged();
    return true;
}

void BannerEntity::saveToNBT(nbt::tags::compound_tag& tag) const
{
    BlockEntity::saveToNBT(tag);

    // 保存自定义名称
    // TODO: 将ITextComponent写入NBT

    // 保存图案
    // NBT序列化将在NBT系统完善后实现
}

std::unique_ptr<BlockEntity> BannerEntity::clone() const
{
    auto clone = std::make_unique<BannerEntity>(m_pos);
    clone->m_patterns = m_patterns;
    clone->m_baseColor = m_baseColor;
    return clone;
}

} // namespace blockentity
} // namespace mc
