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

#include "../../feature/template/Template.hpp"
#include <memory>

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

using feature::template_::StructureProcessorList;

/**
 * @brief 预定义处理器列表
 *
 * 参考 MC 1.16.5: net.minecraft.world.gen.feature.template.ProcessorLists
 *
 * 所有处理器列表都是静态单例，在 initialize() 中初始化。
 * 使用方式：
 *   ProcessorLists::initialize();  // 启动时调用一次
 *   settings.setProcessors(&ProcessorLists::MOSSIFY_20_PERCENT);
 */
namespace ProcessorLists {

// ============================================================================
// 空处理器
// ============================================================================

/**
 * @brief 空处理器列表
 * MC 1.16.5: field_244101_a (empty)
 */
extern StructureProcessorList EMPTY;

// ============================================================================
// 僵尸村庄处理器
// ============================================================================

/**
 * @brief 平原僵尸村庄处理器
 * MC 1.16.5: field_244102_b (zombie_plains)
 */
extern StructureProcessorList ZOMBIE_PLAINS;

/**
 * @brief 沙漠僵尸村庄处理器
 * MC 1.16.5: field_244106_f (zombie_desert)
 */
extern StructureProcessorList ZOMBIE_DESERT;

/**
 * @brief 热带草原僵尸村庄处理器
 * MC 1.16.5: field_244103_c (zombie_savanna)
 */
extern StructureProcessorList ZOMBIE_SAVANNA;

/**
 * @brief 雪地僵尸村庄处理器
 * MC 1.16.5: field_244104_d (zombie_snowy)
 */
extern StructureProcessorList ZOMBIE_SNOWY;

/**
 * @brief 针叶林僵尸村庄处理器
 * MC 1.16.5: field_244105_e (zombie_taiga)
 */
extern StructureProcessorList ZOMBIE_TAIGA;

// ============================================================================
// 苔藓化处理器
// ============================================================================

/**
 * @brief 10% 苔藓化处理器
 * MC 1.16.5: field_244107_g (mossify_10_percent)
 */
extern StructureProcessorList MOSSIFY_10_PERCENT;

/**
 * @brief 20% 苔藓化处理器
 * MC 1.16.5: field_244108_h (mossify_20_percent)
 * 用于平原村庄中心建筑
 */
extern StructureProcessorList MOSSIFY_20_PERCENT;

/**
 * @brief 70% 苔藓化处理器
 * MC 1.16.5: field_244109_i (mossify_70_percent)
 */
extern StructureProcessorList MOSSIFY_70_PERCENT;

// ============================================================================
// 道路处理器
// ============================================================================

/**
 * @brief 平原村庄道路处理器
 * MC 1.16.5: field_244110_j (street_plains)
 */
extern StructureProcessorList STREET_PLAINS;

/**
 * @brief 热带草原村庄道路处理器
 * MC 1.16.5: field_244111_k (street_savanna)
 */
extern StructureProcessorList STREET_SAVANNA;

/**
 * @brief 雪地/针叶林村庄道路处理器
 * MC 1.16.5: field_244112_l (street_snowy_or_taiga)
 */
extern StructureProcessorList STREET_SNOWY_TAIGA;

// ============================================================================
// 农场处理器
// ============================================================================

/**
 * @brief 平原村庄农场处理器
 * MC 1.16.5: field_244113_m (farm_plains)
 */
extern StructureProcessorList FARM_PLAINS;

/**
 * @brief 热带草原村庄农场处理器
 * MC 1.16.5: field_244114_n (farm_savanna)
 */
extern StructureProcessorList FARM_SAVANNA;

/**
 * @brief 雪地村庄农场处理器
 * MC 1.16.5: field_244115_o (farm_snowy)
 */
extern StructureProcessorList FARM_SNOWY;

/**
 * @brief 针叶林村庄农场处理器
 * MC 1.16.5: field_244116_p (farm_taiga)
 */
extern StructureProcessorList FARM_TAIGA;

/**
 * @brief 沙漠村庄农场处理器
 * MC 1.16.5: field_244117_q (farm_desert)
 */
extern StructureProcessorList FARM_DESERT;

// ============================================================================
// 掠夺者前哨站处理器
// ============================================================================

/**
 * @brief 掠夺者前哨站腐朽处理器
 * MC 1.16.5: field_244118_r (outpost_rot)
 * 5% 完整度，大部分方块会被移除
 */
extern StructureProcessorList OUTPOST_ROT;

// ============================================================================
// 堡垒遗迹处理器
// ============================================================================

/**
 * @brief 堡垒遗迹底层城墙处理器
 * MC 1.16.5: field_244119_s (bottom_rampart)
 */
extern StructureProcessorList BASTION_BOTTOM_RAMPART;

/**
 * @brief 堡垒遗迹宝藏房间处理器
 * MC 1.16.5: field_244120_t (treasure_rooms)
 */
extern StructureProcessorList BASTION_TREASURE_ROOMS;

/**
 * @brief 堡垒遗迹住宅区域处理器
 * MC 1.16.5: field_244121_u (housing)
 */
extern StructureProcessorList BASTION_HOUSING;

/**
 * @brief 堡垒遗迹侧墙退化处理器
 * MC 1.16.5: field_244122_v (side_wall_degradation)
 */
extern StructureProcessorList BASTION_SIDE_WALL_DEGRADATION;

/**
 * @brief 堡垒遗迹马厩退化处理器
 * MC 1.16.5: field_244123_w (stable_degradation)
 */
extern StructureProcessorList BASTION_STABLE_DEGRADATION;

/**
 * @brief 堡垒遗迹通用退化处理器
 * MC 1.16.5: field_244124_x (bastion_generic_degradation)
 */
extern StructureProcessorList BASTION_GENERIC_DEGRADATION;

/**
 * @brief 堡垒遗迹城墙退化处理器
 * MC 1.16.5: field_244125_y (rampart_degradation)
 */
extern StructureProcessorList BASTION_RAMPART_DEGRADATION;

/**
 * @brief 堡垒遗迹入口替换处理器
 * MC 1.16.5: field_244126_z (entrance_replacement)
 */
extern StructureProcessorList BASTION_ENTRANCE_REPLACEMENT;

/**
 * @brief 堡垒遗迹桥梁处理器
 * MC 1.16.5: bridge
 */
extern StructureProcessorList BASTION_BRIDGE;

/**
 * @brief 堡垒遗迹屋顶处理器
 * MC 1.16.5: roof
 */
extern StructureProcessorList BASTION_ROOF;

/**
 * @brief 堡垒遗迹高墙处理器
 * MC 1.16.5: high_wall
 */
extern StructureProcessorList BASTION_HIGH_WALL;

/**
 * @brief 堡垒遗迹高城墙处理器
 * MC 1.16.5: high_rampart
 */
extern StructureProcessorList BASTION_HIGH_RAMPART;

// ============================================================================
// 初始化函数
// ============================================================================

/**
 * @brief 初始化所有处理器列表
 *
 * 必须在使用任何处理器列表之前调用。
 * 通常在服务器启动时调用一次。
 */
void initialize();

/**
 * @brief 检查是否已初始化
 */
bool isInitialized();

} // namespace ProcessorLists

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
