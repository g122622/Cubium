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

#include "../../core/Types.hpp"
#include <string>

namespace mc {

/**
 * @brief 材质类
 *
 * 定义方块的基础材质属性，如是否固体、是否液体等。
 * 参考: net.minecraft.block.material.Material
 *
 * 注意: 材质是不可变的，应该通过静态常量访问预定义材质。
 */
class Material {
public:
    /**
     * @brief 推动反应类型
     */
    enum class PushReaction {
        Normal,  // 正常推动
        Destroy, // 被推动时销毁
        Block,   // 阻止推动
        PushOnly // 只能被推动，不能被拉回
    };

    /**
     * @brief 获取材质的颜色索引（用于地图渲染）
     */
    [[nodiscard]] u8 materialColor() const noexcept { return m_materialColor; }

    /**
     * @brief 是否阻挡移动
     */
    [[nodiscard]] bool blocksMovement() const noexcept { return m_blocksMovement; }

    /**
     * @brief 是否可燃
     */
    [[nodiscard]] bool isFlammable() const noexcept { return m_flammable; }

    /**
     * @brief 是否为液体
     */
    [[nodiscard]] bool isLiquid() const noexcept { return m_liquid; }

    /**
     * @brief 是否为固体
     */
    [[nodiscard]] bool isSolid() const noexcept { return m_solid; }

    /**
     * @brief 是否可替换
     */
    [[nodiscard]] bool isReplaceable() const noexcept { return m_replaceable; }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque() const noexcept { return m_opaque; }

    /**
     * @brief 获取推动反应
     */
    [[nodiscard]] PushReaction getPushReaction() const noexcept { return m_pushReaction; }

    // ========================================================================
    // 预定义材质
    // ========================================================================

    /** 空气材质 */
    static const Material& AIR;

    /** 结构空位材质 */
    static const Material& STRUCTURE_VOID;

    /** 石头材质 */
    static const Material& ROCK;

    /** 泥土材质 */
    static const Material& EARTH;

    /** 木头材质 */
    static const Material& WOOD;

    /** 植物材质 */
    static const Material& PLANT;

    /** 可替换植物材质（草、蕨等） */
    static const Material& REPLACEABLE_PLANT;

    /** 水材质 */
    static const Material& WATER;

    /** 岩浆材质 */
    static const Material& LAVA;

    /** 树叶材质 */
    static const Material& LEAVES;

    /** 玻璃材质 */
    static const Material& GLASS;

    /** 冰材质 */
    static const Material& ICE;

    /** 羊毛材质 */
    static const Material& WOOL;

    /** 沙子材质 */
    static const Material& SAND;

    /** 金属材质 */
    static const Material& IRON;

    /** 雪材质 */
    static const Material& SNOW;

    /** 粘液块材质 */
    static const Material& SLIME;

    /** 蜂蜜块材质 */
    static const Material& HONEY;

    /** TNT材质 */
    static const Material& TNT;

    /** 海绵材质 */
    static const Material& SPONGE;

    /** 珊瑚材质 */
    static const Material& CORAL;

    /** 蛛网材质 */
    static const Material& WEB;

    /** 红石灯材质 */
    static const Material& REDSTONE_LIGHT;

    /** 粘性活塞材质 */
    static const Material& PISTON;

    /** 装饰材质（不可移动） */
    static const Material& DECORATION;

    /** 传送门材质 */
    static const Material& PORTAL;

    /** 海洋植物材质 */
    static const Material& OCEAN_PLANT;

    /** 海草材质 */
    static const Material& SEA_GRASS;

    /** 火材质 */
    static const Material& FIRE;

    // ========================================================================
    // 工具系统相关材质
    // ========================================================================

    /** 铁砧材质 - 用于铁砧类方块，需要镐采集 */
    static const Material& ANVIL;

    /** 葫芦材质 - 用于南瓜、西瓜等，斧有效 */
    static const Material& GOURD;

    /** 高植物材质 - 用于高草、藤蔓等 */
    static const Material& TALL_PLANTS;

    /** 竹子材质 - 用于竹子相关方块 */
    static const Material& BAMBOO;

    /** 下界木材质 - 用于下界木相关方块，斧有效 */
    static const Material& NETHER_WOOD;

    /** 蕨类/地衣材质 - 用于苔藓、地衣等 */
    static const Material& MOSS;

    /** 有机材质 - 用于草方块、干草块、疣块等 */
    static const Material& ORGANIC;

    // ========================================================================
    // 比较运算符
    // ========================================================================

    /**
     * @brief 比较两个材质是否相同
     *
     * 通过比较内存地址判断是否为同一材质实例。
     */
    [[nodiscard]] bool operator==(const Material& other) const noexcept { return this == &other; }

    /**
     * @brief 比较两个材质是否不同
     */
    [[nodiscard]] bool operator!=(const Material& other) const noexcept { return this != &other; }

private:
    friend class MaterialBuilder;

    bool m_blocksMovement;
    bool m_flammable;
    bool m_liquid;
    bool m_solid;
    bool m_replaceable;
    bool m_opaque;
    PushReaction m_pushReaction;
    u8 m_materialColor;

    Material(bool blocksMovement,
        bool flammable,
        bool liquid,
        bool solid,
        bool replaceable,
        bool opaque,
        PushReaction pushReaction,
        u8 materialColor)
        : m_blocksMovement(blocksMovement)
        , m_flammable(flammable)
        , m_liquid(liquid)
        , m_solid(solid)
        , m_replaceable(replaceable)
        , m_opaque(opaque)
        , m_pushReaction(pushReaction)
        , m_materialColor(materialColor)
    {}
};

/**
 * @brief 材质构建器
 *
 * 用于创建自定义材质。
 *
 * 用法示例:
 * @code
 * Material customMaterial = MaterialBuilder()
 *     .solid()
 *     .flammable()
 *     .build();
 * @endcode
 */
class MaterialBuilder {
public:
    MaterialBuilder();

    /**
     * @brief 设置为液体
     */
    MaterialBuilder& liquid();

    /**
     * @brief 设置为固体
     */
    MaterialBuilder& solid(bool solid = true);

    /**
     * @brief 设置为阻挡移动
     */
    MaterialBuilder& blocksMovement(bool blocks = true);

    /**
     * @brief 设置为可燃
     */
    MaterialBuilder& flammable(bool flammable = true);

    /**
     * @brief 设置为可替换
     */
    MaterialBuilder& replaceable(bool replaceable = true);

    /**
     * @brief 设置为不透明
     */
    MaterialBuilder& opaque(bool opaque = true);

    /**
     * @brief 设置推动反应
     */
    MaterialBuilder& pushReaction(Material::PushReaction reaction);

    /**
     * @brief 设置材质颜色
     */
    MaterialBuilder& color(u8 color);

    /**
     * @brief 构建材质
     */
    Material build();

private:
    bool m_blocksMovement;
    bool m_flammable;
    bool m_liquid;
    bool m_solid;
    bool m_replaceable;
    bool m_opaque;
    Material::PushReaction m_pushReaction;
    u8 m_materialColor;
};

} // namespace mc
