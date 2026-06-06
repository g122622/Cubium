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
 * copies of substantial portions of the Software.
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

#include "../../core/Item.hpp"

namespace mc {
namespace item {

/**
 * @brief 试炼密室旗帜图案物品和锻造模板物品注册声明
 *
 * 此文件集中声明试炼密室相关的特殊物品：
 * - 旋风旗帜图案 (Guster Banner Pattern)
 * - 涡流旗帜图案 (Flow Banner Pattern)
 * - 镶铆盔甲纹饰锻造模板 (Rib Armor Trim Smithing Template)
 * - 涡流盔甲纹饰锻造模板 (Flow Armor Trim Smithing Template)
 * - 涡流纹样陶片 (Flow Pottery Sherd)
 * - 旋风纹样陶片 (Guster Pottery Sherd)
 * - 刮削纹样陶片 (Scrape Pottery Sherd)
 * - 音乐唱片 (Creator / Creator Music Box / Precipice)
 *
 * 由于旗帜图案、锻造模板和陶片的基建可能尚未完全实现，
 * 这些物品暂时使用简单的 Item 基类，后续再扩展为专用类。
 */

// 旋风旗帜图案 - 命名空间ID: minecraft:guster_banner_pattern
// TODO(trial_chambers): 扩展 BannerPatternType 枚举添加 Guster 和 Flow

// 涡流旗帜图案 - 命名空间ID: minecraft:flow_banner_pattern

// 镶铆盔甲纹饰锻造模板 - 命名空间ID: minecraft:rib_armor_trim_smithing_template
// TODO(trial_chambers): 实现锻造模板系统后替换为专用类

// 涡流盔甲纹饰锻造模板 - 命名空间ID: minecraft:flow_armor_trim_smithing_template

// 涡流纹样陶片 - 命名空间ID: minecraft:flow_pottery_sherd
// TODO(trial_chambers): 实现陶片/饰纹陶罐系统后替换为专用类

// 旋风纹样陶片 - 命名空间ID: minecraft:guster_pottery_sherd

// 刮削纹样陶片 - 命名空间ID: minecraft:scrape_pottery_sherd

// 音乐唱片 (Creator) - 命名空间ID: minecraft:music_disc_creator
// TODO(trial_chambers): 实现音乐唱片播放逻辑

// 音乐唱片 (Creator 八音盒) - 命名空间ID: minecraft:music_disc_creator_music_box

// 音乐唱片 (Precipice) - 命空间ID: minecraft:music_disc_precipice

} // namespace item
} // namespace mc
