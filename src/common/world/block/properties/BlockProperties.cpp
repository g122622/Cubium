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

#include "BlockProperties.hpp"

namespace mc {
namespace {
/**
 * @brief 根据材质获取默认的声音类型
 */
const BlockSoundType& _getDefaultSoundType(const Material& material) noexcept
{
    // 木头材质 -> 木头声音
    if (&material == &Material::WOOD || &material == &Material::NETHER_WOOD) {
        return BlockSoundTypes::WOOD;
    }
    // 泥土材质 -> 泥土声音
    if (&material == &Material::EARTH) {
        return BlockSoundTypes::DIRT;
    }
    // 草材质 -> 草声音
    if (&material == &Material::PLANT || &material == &Material::REPLACEABLE_PLANT || &material == &Material::LEAVES ||
        &material == &Material::TALL_PLANTS || &material == &Material::OCEAN_PLANT ||
        &material == &Material::SEA_GRASS || &material == &Material::MOSS) {
        return BlockSoundTypes::GRASS;
    }
    // 沙子材质 -> 沙子声音
    if (&material == &Material::SAND) {
        return BlockSoundTypes::SAND;
    }
    // 玻璃材质 -> 玻璃声音
    if (&material == &Material::GLASS || &material == &Material::ICE) {
        return BlockSoundTypes::GLASS;
    }
    // 金属材质 -> 金属声音
    if (&material == &Material::IRON) {
        return BlockSoundTypes::METAL;
    }
    // 雪材质 -> 雪声音
    if (&material == &Material::SNOW) {
        return BlockSoundTypes::SNOW;
    }
    // 羊毛材质 -> 羊毛声音
    if (&material == &Material::WOOL) {
        return BlockSoundTypes::WOOL;
    }
    // 水材质 -> 水声音
    if (&material == &Material::WATER) {
        return BlockSoundTypes::WATER;
    }
    // 岩浆材质 -> 岩浆声音
    if (&material == &Material::LAVA) {
        return BlockSoundTypes::LAVA;
    }
    // 空气和其他材质 -> 默认石头声音
    return BlockSoundTypes::STONE;
}
} // namespace

BlockProperties::BlockProperties(const Material& material)
    : m_material(&material)
    , m_hardness(0.0f)
    , m_resistance(0.0f)
    , m_lightLevel(0)
    , m_hasCollision(material.blocksMovement())
    , m_isSolid(material.isSolid())
    , m_isFlammable(material.isFlammable())
    , m_requiresTool(false)
    // 可替换性**不从材质推导**：原版 `BlockBehaviour.Properties#replaceable()` 是个显式开关，
    // 默认 false，只有 29 个方块显式开启（空气、水、草、藤蔓、雪层等）。
    // 先前按材质推导会让整片借用植物材质的方块（花、树苗、作物、蘑菇）全部变成可替换，
    // 玩家能把这些方块直接用手上的方块覆盖掉，而原版是「放不进去」。
    , m_isReplaceable(false)
    , m_soundType(&_getDefaultSoundType(material))
{}

} // namespace mc
