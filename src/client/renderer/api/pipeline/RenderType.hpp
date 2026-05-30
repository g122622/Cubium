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

#include "RenderState.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <string>

namespace mc::client::renderer::api {

/**
 * @brief 渲染类型
 *
 * 命名渲染类型，用于分类和排序渲染调用。
 *
 * 渲染类型决定了：
 * 1. 渲染状态 (混合、深度、剔除)
 * 2. 着色器
 * 3. 纹理绑定
 * 4. 排序优先级
 */
class RenderType {
public:
    /**
     * @brief 预定义渲染类型名称
     */
    static constexpr const char* NAME_SOLID = "solid";
    static constexpr const char* NAME_CUTOUT = "cutout";
    static constexpr const char* NAME_CUTOUT_MIPPED = "cutout_mipped";
    static constexpr const char* NAME_TRANSLUCENT = "translucent";
    static constexpr const char* NAME_TRANSLUCENT_MOVING_BLOCK = "translucent_moving_block";
    static constexpr const char* NAME_TRIPWIRE = "tripwire";
    static constexpr const char* NAME_LINES = "lines";
    static constexpr const char* NAME_LINES_STRIP = "line_strip";
    static constexpr const char* NAME_LEASH = "leash";
    static constexpr const char* NAME_WATER_MASK = "water_mask";
    static constexpr const char* NAME_ARMOR_GLINT = "armor_entity_glint";
    static constexpr const char* NAME_ENTITY_SOLID = "entity_solid";
    static constexpr const char* NAME_ENTITY_CUTOUT = "entity_cutout";
    static constexpr const char* NAME_ENTITY_TRANSLUCENT = "entity_translucent";
    static constexpr const char* NAME_ENTITY_SMOOTH_CUTOUT = "entity_smooth_cutout";
    static constexpr const char* NAME_BEACON_BEAM = "beacon_beam";
    static constexpr const char* NAME_ENERGY_SWIRL = "energy_swirl";
    static constexpr const char* NAME_LIGHTNING = "lightning";
    static constexpr const char* NAME_PARTICLE = "particle";
    static constexpr const char* NAME_GUI = "gui";
    static constexpr const char* NAME_GUI_OVERLAY = "gui_overlay";
    static constexpr const char* NAME_GUI_TEXT = "gui_text";
    static constexpr const char* NAME_TEXT = "text";
    static constexpr const char* NAME_TEXT_SEE_THROUGH = "text_see_through";
    static constexpr const char* NAME_SKY = "sky";
    static constexpr const char* NAME_CLOUDS = "clouds";
    static constexpr const char* NAME_END_SKY = "end_sky";
    static constexpr const char* NAME_STARS = "stars";

    /**
     * @brief 默认构造
     */
    RenderType() = default;

    /**
     * @brief 构造渲染类型
     * @param name 类型名称
     * @param state 渲染状态
     * @param sortIndex 排序索引 (小值先渲染)
     */
    explicit RenderType(const std::string& name, const RenderState& state, i32 sortIndex = 0)
        : m_name(name)
        , m_state(state)
        , m_sortIndex(sortIndex)
        , m_valid(true)
    {}

    // 预定义的方块渲染类型

    /**
     * @brief 不透明方块
     */
    static RenderType solid() { return RenderType(NAME_SOLID, RenderState::solid(), 0); }

    /**
     * @brief 镂空方块 (无 mipmap)
     */
    static RenderType cutout() { return RenderType(NAME_CUTOUT, RenderState::cutout(), 1); }

    /**
     * @brief 镂空方块 (有 mipmap)
     */
    static RenderType cutoutMipped() { return RenderType(NAME_CUTOUT_MIPPED, RenderState::cutoutMipped(), 2); }

    /**
     * @brief 半透明方块
     */
    static RenderType translucent() { return RenderType(NAME_TRANSLUCENT, RenderState::translucent(), 100); }

    /**
     * @brief 线条
     */
    static RenderType lines() { return RenderType(NAME_LINES, RenderState::lines(), 200); }

    // 实体渲染类型

    /**
     * @brief 不透明实体
     */
    static RenderType entitySolid(const ResourceLocation& texture = ResourceLocation())
    {
        RenderType rt(NAME_ENTITY_SOLID, RenderState::solid(), 50);
        rt.m_texture = texture;
        return rt;
    }

    /**
     * @brief 镂空实体
     */
    static RenderType entityCutout(const ResourceLocation& texture = ResourceLocation())
    {
        RenderType rt(NAME_ENTITY_CUTOUT, RenderState::cutout(), 51);
        rt.m_texture = texture;
        return rt;
    }

    /**
     * @brief 半透明实体
     */
    static RenderType entityTranslucent(const ResourceLocation& texture = ResourceLocation())
    {
        RenderType rt(NAME_ENTITY_TRANSLUCENT, RenderState::translucent(), 150);
        rt.m_texture = texture;
        return rt;
    }

    // 特殊渲染类型

    /**
     * @brief 天空
     */
    static RenderType sky() { return RenderType(NAME_SKY, RenderState::solid(), -100); }

    /**
     * @brief 云
     */
    static RenderType clouds() { return RenderType(NAME_CLOUDS, RenderState::translucent(), 90); }

    /**
     * @brief 粒子
     */
    static RenderType particle(const ResourceLocation& texture = ResourceLocation())
    {
        RenderType rt(NAME_PARTICLE, RenderState::translucent(), 180);
        rt.m_texture = texture;
        return rt;
    }

    /**
     * @brief GUI
     */
    static RenderType gui() { return RenderType(NAME_GUI, RenderState::translucent(), 1000); }

    /**
     * @brief 闪电
     */
    static RenderType lightning() { return RenderType(NAME_LIGHTNING, RenderState::additive(), 190); }

    // 访问器

    /**
     * @brief 获取名称
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief 获取渲染状态
     */
    [[nodiscard]] const RenderState& state() const { return m_state; }

    /**
     * @brief 获取排序索引
     */
    [[nodiscard]] i32 sortIndex() const { return m_sortIndex; }

    /**
     * @brief 获取关联纹理
     */
    [[nodiscard]] const ResourceLocation& texture() const { return m_texture; }

    /**
     * @brief 检查是否有效
     */
    [[nodiscard]] bool isValid() const { return m_valid; }

    /**
     * @brief 检查是否需要排序
     */
    [[nodiscard]] bool needsSorting() const { return m_state.blend.enabled; }

    /**
     * @brief 比较排序顺序
     * @return 如果 this 应该先于 other 渲染则返回 true
     */
    [[nodiscard]] bool shouldRenderBefore(const RenderType& other) const { return m_sortIndex < other.m_sortIndex; }

    bool operator==(const RenderType& other) const { return m_name == other.m_name && m_state == other.m_state; }

    bool operator!=(const RenderType& other) const { return !(*this == other); }

    bool operator<(const RenderType& other) const { return m_sortIndex < other.m_sortIndex; }

private:
    std::string m_name;
    RenderState m_state;
    ResourceLocation m_texture;
    i32 m_sortIndex = 0;
    bool m_valid = false;
};

/**
 * @brief 渲染类型哈希
 */
struct RenderTypeHash {
    Size operator()(const RenderType& rt) const { return std::hash<std::string>{}(rt.name()); }
};

} // namespace mc::client::renderer::api
