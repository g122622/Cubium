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

#include "BossInfo.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace mc {
namespace server {

namespace {
// 颜色名称映射
const std::unordered_map<std::string, BossInfoColor> s_colorNameMap = {
    {"pink", BossInfoColor::Pink},
    {"blue", BossInfoColor::Blue},
    {"red", BossInfoColor::Red},
    {"green", BossInfoColor::Green},
    {"yellow", BossInfoColor::Yellow},
    {"purple", BossInfoColor::Purple},
    {"white", BossInfoColor::White},
};

// 样式名称映射
const std::unordered_map<std::string, BossInfoOverlay> s_overlayNameMap = {
    {"progress", BossInfoOverlay::Progress},
    {"notched_6", BossInfoOverlay::Notched6},
    {"notched_10", BossInfoOverlay::Notched10},
    {"notched_12", BossInfoOverlay::Notched12},
    {"notched_20", BossInfoOverlay::Notched20},
};
} // namespace

BossInfoColor bossInfoColorFromName(const std::string& name)
{
    auto it = s_colorNameMap.find(name);
    if (it != s_colorNameMap.end()) {
        return it->second;
    }
    return BossInfoColor::White;
}

std::string bossInfoColorToName(BossInfoColor color)
{
    switch (color) {
        case BossInfoColor::Pink:
            return "pink";
        case BossInfoColor::Blue:
            return "blue";
        case BossInfoColor::Red:
            return "red";
        case BossInfoColor::Green:
            return "green";
        case BossInfoColor::Yellow:
            return "yellow";
        case BossInfoColor::Purple:
            return "purple";
        case BossInfoColor::White:
        default:
            return "white";
    }
}

BossInfoOverlay bossInfoOverlayFromName(const std::string& name)
{
    auto it = s_overlayNameMap.find(name);
    if (it != s_overlayNameMap.end()) {
        return it->second;
    }
    return BossInfoOverlay::Progress;
}

std::string bossInfoOverlayToName(BossInfoOverlay overlay)
{
    switch (overlay) {
        case BossInfoOverlay::Notched6:
            return "notched_6";
        case BossInfoOverlay::Notched10:
            return "notched_10";
        case BossInfoOverlay::Notched12:
            return "notched_12";
        case BossInfoOverlay::Notched20:
            return "notched_20";
        case BossInfoOverlay::Progress:
        default:
            return "progress";
    }
}

BossInfo::BossInfo(Uuid uuid, std::unique_ptr<text::ITextComponent> name, BossInfoColor color, BossInfoOverlay overlay)
    : m_uuid(std::move(uuid))
    , m_name(name ? std::move(name) : std::make_unique<text::StringTextComponent>(""))
    , m_color(color)
    , m_overlay(overlay)
{}

void BossInfo::setName(std::unique_ptr<text::ITextComponent> name)
{
    if (name) {
        m_name = std::move(name);
    }
}

void BossInfo::setPercent(f32 percent)
{
    m_percent = math::clamp(percent, 0.0f, 1.0f);
}

void BossInfo::setColor(BossInfoColor color)
{
    m_color = color;
}

void BossInfo::setOverlay(BossInfoOverlay overlay)
{
    m_overlay = overlay;
}

void BossInfo::setDarkenSky(bool darken)
{
    m_darkenSky = darken;
}

void BossInfo::setPlayEndBossMusic(bool play)
{
    m_playEndBossMusic = play;
}

void BossInfo::setCreateFog(bool create)
{
    m_createFog = create;
}

void BossInfo::setVisible(bool visible)
{
    m_visible = visible;
}

} // namespace server
} // namespace mc
