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

#include "Material.hpp"
#include "common/core/Types.hpp"

namespace mc {

// ============================================================================
// MaterialBuilder
// ============================================================================

MaterialBuilder::MaterialBuilder() noexcept
    : m_blocksMovement(false)
    , m_flammable(false)
    , m_liquid(false)
    , m_solid(false)
    , m_replaceable(false)
    , m_opaque(false)
    , m_pushReaction(Material::PushReaction::Normal)
    , m_materialColor(0)
{}

MaterialBuilder& MaterialBuilder::liquid() noexcept
{
    m_liquid = true;
    m_solid = false;
    m_blocksMovement = false;
    return *this;
}

MaterialBuilder& MaterialBuilder::solid(bool solid) noexcept
{
    m_solid = solid;
    if (solid) {
        m_blocksMovement = true;
    }
    return *this;
}

MaterialBuilder& MaterialBuilder::blocksMovement(bool blocks) noexcept
{
    m_blocksMovement = blocks;
    return *this;
}

MaterialBuilder& MaterialBuilder::flammable(bool flammable) noexcept
{
    m_flammable = flammable;
    return *this;
}

MaterialBuilder& MaterialBuilder::replaceable(bool replaceable) noexcept
{
    m_replaceable = replaceable;
    return *this;
}

MaterialBuilder& MaterialBuilder::opaque(bool opaque) noexcept
{
    m_opaque = opaque;
    return *this;
}

MaterialBuilder& MaterialBuilder::pushReaction(Material::PushReaction reaction) noexcept
{
    m_pushReaction = reaction;
    return *this;
}

MaterialBuilder& MaterialBuilder::color(u8 color) noexcept
{
    m_materialColor = color;
    return *this;
}

Material MaterialBuilder::build() noexcept
{
    return Material(
        m_blocksMovement, m_flammable, m_liquid, m_solid, m_replaceable, m_opaque, m_pushReaction, m_materialColor);
}

// ============================================================================
// 预定义材质
// ============================================================================

namespace {
// 辅助函数创建材质
Material makeAirMaterial()
{
    return MaterialBuilder().solid(false).blocksMovement(false).replaceable().build();
}

Material makeRockMaterial()
{
    return MaterialBuilder().solid().opaque().build();
}

Material makeEarthMaterial()
{
    return MaterialBuilder().solid().opaque().build();
}

Material makeWoodMaterial()
{
    return MaterialBuilder().solid().flammable().opaque().build();
}

Material makePlantMaterial()
{
    return MaterialBuilder().solid(false).flammable().replaceable().build();
}

Material makeReplaceablePlantMaterial()
{
    return MaterialBuilder().solid(false).flammable().replaceable().blocksMovement(false).build();
}

Material makeWaterMaterial()
{
    return MaterialBuilder().liquid().replaceable().pushReaction(Material::PushReaction::Block).build();
}

Material makeLavaMaterial()
{
    return MaterialBuilder().liquid().replaceable().pushReaction(Material::PushReaction::Block).build();
}

Material makeLeavesMaterial()
{
    return MaterialBuilder().solid().flammable().opaque(false).pushReaction(Material::PushReaction::Destroy).build();
}

Material makeGlassMaterial()
{
    return MaterialBuilder().solid().opaque(false).pushReaction(Material::PushReaction::Destroy).build();
}

Material makeIceMaterial()
{
    return MaterialBuilder().solid().opaque(false).pushReaction(Material::PushReaction::Destroy).build();
}

Material makeWoolMaterial()
{
    return MaterialBuilder().solid().flammable().opaque().build();
}

Material makeSandMaterial()
{
    return MaterialBuilder().solid().opaque().build();
}

Material makeIronMaterial()
{
    return MaterialBuilder().solid().opaque().pushReaction(Material::PushReaction::Block).build();
}

Material makeSnowMaterial()
{
    return MaterialBuilder().solid().replaceable().opaque().build();
}

Material makeSlimeMaterial()
{
    return MaterialBuilder().solid().opaque().pushReaction(Material::PushReaction::Normal).build();
}

Material makeHoneyMaterial()
{
    // 蜂蜜块材质：固体但不完全阻挡移动，可粘住实体
    return MaterialBuilder().solid().opaque().pushReaction(Material::PushReaction::Normal).build();
}

Material makeTNTMaterial()
{
    return MaterialBuilder().solid().flammable().opaque().build();
}

Material makeSpongeMaterial()
{
    return MaterialBuilder().solid().opaque().build();
}

Material makeCoralMaterial()
{
    return MaterialBuilder().solid().opaque().build();
}

Material makeWebMaterial()
{
    return MaterialBuilder().solid(false).blocksMovement().build();
}

Material makeRedstoneLightMaterial()
{
    return MaterialBuilder().solid().opaque().build();
}

Material makePistonMaterial()
{
    return MaterialBuilder().solid().opaque().pushReaction(Material::PushReaction::Block).build();
}

Material makeDecorationMaterial()
{
    return MaterialBuilder().solid(false).pushReaction(Material::PushReaction::Destroy).build();
}

Material makeStructureVoidMaterial()
{
    return MaterialBuilder().solid(false).replaceable().build();
}

Material makePortalMaterial()
{
    return MaterialBuilder().solid(false).blocksMovement(false).pushReaction(Material::PushReaction::Block).build();
}

Material makeOceanPlantMaterial()
{
    return MaterialBuilder().solid(false).replaceable().build();
}

Material makeSeaGrassMaterial()
{
    return MaterialBuilder().solid(false).replaceable().build();
}

Material makeFireMaterial()
{
    return MaterialBuilder().solid(false).blocksMovement(false).replaceable().build();
}

Material makeAnvilMaterial()
{
    return MaterialBuilder().solid().opaque().pushReaction(Material::PushReaction::Block).build();
}

Material makeGourdMaterial()
{
    return MaterialBuilder().solid().opaque().pushReaction(Material::PushReaction::Destroy).build();
}

Material makeTallPlantsMaterial()
{
    return MaterialBuilder().solid(false).flammable().replaceable().blocksMovement(false).build();
}

Material makeBambooMaterial()
{
    return MaterialBuilder()
        .solid(false)
        .flammable()
        .blocksMovement()
        .pushReaction(Material::PushReaction::Destroy)
        .build();
}

Material makeNetherWoodMaterial()
{
    return MaterialBuilder().solid().flammable(false).opaque().build();
}

Material makeMossMaterial()
{
    return MaterialBuilder().solid(false).replaceable().opaque(false).build();
}

Material makeSculkMaterial()
{
    return MaterialBuilder()
        .solid(false)
        .replaceable()
        .opaque(false)
        .pushReaction(Material::PushReaction::Destroy)
        .build();
}

Material makePowderSnowMaterial()
{
    return MaterialBuilder().solid(false).replaceable().blocksMovement(false).opaque(false).build();
}

Material makeOrganicMaterial()
{
    // 有机材质：用于草方块、干草块、地狱疣块、诡异疣块等
    return MaterialBuilder().solid().opaque().build();
}
} // namespace

// 静态常量定义
const Material& Material::AIR = []() -> const Material& {
    static Material material = makeAirMaterial();
    return material;
}();

const Material& Material::STRUCTURE_VOID = []() -> const Material& {
    static Material material = makeStructureVoidMaterial();
    return material;
}();

const Material& Material::ROCK = []() -> const Material& {
    static Material material = makeRockMaterial();
    return material;
}();

const Material& Material::EARTH = []() -> const Material& {
    static Material material = makeEarthMaterial();
    return material;
}();

const Material& Material::WOOD = []() -> const Material& {
    static Material material = makeWoodMaterial();
    return material;
}();

const Material& Material::PLANT = []() -> const Material& {
    static Material material = makePlantMaterial();
    return material;
}();

const Material& Material::REPLACEABLE_PLANT = []() -> const Material& {
    static Material material = makeReplaceablePlantMaterial();
    return material;
}();

const Material& Material::WATER = []() -> const Material& {
    static Material material = makeWaterMaterial();
    return material;
}();

const Material& Material::LAVA = []() -> const Material& {
    static Material material = makeLavaMaterial();
    return material;
}();

const Material& Material::LEAVES = []() -> const Material& {
    static Material material = makeLeavesMaterial();
    return material;
}();

const Material& Material::GLASS = []() -> const Material& {
    static Material material = makeGlassMaterial();
    return material;
}();

const Material& Material::ICE = []() -> const Material& {
    static Material material = makeIceMaterial();
    return material;
}();

const Material& Material::WOOL = []() -> const Material& {
    static Material material = makeWoolMaterial();
    return material;
}();

const Material& Material::SAND = []() -> const Material& {
    static Material material = makeSandMaterial();
    return material;
}();

const Material& Material::IRON = []() -> const Material& {
    static Material material = makeIronMaterial();
    return material;
}();

const Material& Material::SNOW = []() -> const Material& {
    static Material material = makeSnowMaterial();
    return material;
}();

const Material& Material::SLIME = []() -> const Material& {
    static Material material = makeSlimeMaterial();
    return material;
}();

const Material& Material::HONEY = []() -> const Material& {
    static Material material = makeHoneyMaterial();
    return material;
}();

const Material& Material::TNT = []() -> const Material& {
    static Material material = makeTNTMaterial();
    return material;
}();

const Material& Material::SPONGE = []() -> const Material& {
    static Material material = makeSpongeMaterial();
    return material;
}();

const Material& Material::CORAL = []() -> const Material& {
    static Material material = makeCoralMaterial();
    return material;
}();

const Material& Material::WEB = []() -> const Material& {
    static Material material = makeWebMaterial();
    return material;
}();

const Material& Material::REDSTONE_LIGHT = []() -> const Material& {
    static Material material = makeRedstoneLightMaterial();
    return material;
}();

const Material& Material::PISTON = []() -> const Material& {
    static Material material = makePistonMaterial();
    return material;
}();

const Material& Material::DECORATION = []() -> const Material& {
    static Material material = makeDecorationMaterial();
    return material;
}();

const Material& Material::PORTAL = []() -> const Material& {
    static Material material = makePortalMaterial();
    return material;
}();

const Material& Material::OCEAN_PLANT = []() -> const Material& {
    static Material material = makeOceanPlantMaterial();
    return material;
}();

const Material& Material::SEA_GRASS = []() -> const Material& {
    static Material material = makeSeaGrassMaterial();
    return material;
}();

const Material& Material::FIRE = []() -> const Material& {
    static Material material = makeFireMaterial();
    return material;
}();

// ============================================================================
// 工具系统相关材质
// ============================================================================

const Material& Material::ANVIL = []() -> const Material& {
    static Material material = makeAnvilMaterial();
    return material;
}();

const Material& Material::GOURD = []() -> const Material& {
    static Material material = makeGourdMaterial();
    return material;
}();

const Material& Material::TALL_PLANTS = []() -> const Material& {
    static Material material = makeTallPlantsMaterial();
    return material;
}();

const Material& Material::BAMBOO = []() -> const Material& {
    static Material material = makeBambooMaterial();
    return material;
}();

const Material& Material::NETHER_WOOD = []() -> const Material& {
    static Material material = makeNetherWoodMaterial();
    return material;
}();

const Material& Material::MOSS = []() -> const Material& {
    static Material material = makeMossMaterial();
    return material;
}();

const Material& Material::ORGANIC = []() -> const Material& {
    static Material material = makeOrganicMaterial();
    return material;
}();

const Material& Material::CAKE = []() -> const Material& {
    static const Material material =
        MaterialBuilder().solid(false).pushReaction(Material::PushReaction::Destroy).build();
    return material;
}();

const Material& Material::BARRIER = []() -> const Material& {
    static const Material material =
        MaterialBuilder().solid().opaque().pushReaction(Material::PushReaction::Block).build();
    return material;
}();

const Material& Material::SCULK = []() -> const Material& {
    static Material material = makeSculkMaterial();
    return material;
}();

const Material& Material::POWDER_SNOW = []() -> const Material& {
    static Material material = makePowderSnowMaterial();
    return material;
}();

} // namespace mc
