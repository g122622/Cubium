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

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::i32;
using mc::u8;

/**
 * @brief 实体分类枚举
 *
 * 用于生物生成、刷怪笼和刷怪规则等系统。
 * 每种分类有不同的生成限制和生成规则。
 */
enum class EntityClassification : u8 {
    Monster = 0,                  // 怪物（僵尸、骷髅等）- 每区块最多70个，不和平生成
    Creature = 1,                 // 生物（猪、牛、羊等）- 每区块最多10个，和平生成
    Ambient = 2,                  // 环境生物（蝙蝠）- 每区块最多15个
    Axolotls = 3,                 // 美西螈 - 每区块最多5个（独立分类，避免与其他水生生物竞争名额）
    UndergroundWaterCreature = 4, // 地下水生生物（发光鱿鱼）- 每区块最多5个
    WaterCreature = 5,            // 水生生物（鱿鱼、海豚）- 每区块最多5个
    WaterAmbient = 6,             // 水生环境生物（鱼）- 每区块最多20个
    Misc = 7                      // 其他（物品、经验球、箭等）- 无限制
};

/**
 * @brief 实体分类信息
 *
 * 存储每个分类的生成参数
 */
struct EntityClassificationInfo {
    EntityClassification classification;
    std::string name;
    i32 maxCount;                   // 每区块最大数量
    bool isPeaceful;                // 是否为和平生物
    bool isAnimal;                  // 是否为动物
    i32 despawnDistance;            // 立即消失距离
    i32 randomDespawnDistance = 32; // 随机消失距离

    static EntityClassificationInfo get(EntityClassification classification);
};

/**
 * @brief 获取实体分类的最大数量
 * @param classification 实体分类
 * @return 每区块最大数量
 */
inline i32 getMaxCount(EntityClassification classification)
{
    switch (classification) {
        case EntityClassification::Monster:
            return 70;
        case EntityClassification::Creature:
            return 10;
        case EntityClassification::Ambient:
            return 15;
        case EntityClassification::Axolotls:
            return 5;
        case EntityClassification::UndergroundWaterCreature:
            return 5;
        case EntityClassification::WaterCreature:
            return 5;
        case EntityClassification::WaterAmbient:
            return 20;
        case EntityClassification::Misc:
            return -1; // 无限制
    }
    return -1;
}

/**
 * @brief 获取实体分类是否为和平生物
 * @param classification 实体分类
 * @return 是否为和平生物
 */
inline bool isPeaceful(EntityClassification classification)
{
    return classification != EntityClassification::Monster;
}

/**
 * @brief 获取实体分类是否为动物
 * @param classification 实体分类
 * @return 是否为动物
 */
inline bool isAnimal(EntityClassification classification)
{
    return classification == EntityClassification::Creature;
}

/**
 * @brief 获取实体分类是否为持久化分类（控制生成节流与消失行为）
 *
 * MobCategory.isPersistent：CREATURE/MISC 为 true，其余为 false。
 * 持久化分类每 400tick 才参与一次生成（getFilteredSpawningCategories 第三条件）。
 * @param classification 实体分类
 * @return 是否为持久化分类
 */
inline bool isPersistent(EntityClassification classification)
{
    return classification == EntityClassification::Creature || classification == EntityClassification::Misc;
}

/**
 * @brief 获取实体分类的立即消失距离
 * @param classification 实体分类
 * @return 立即消失距离（方块）
 */
inline i32 getDespawnDistance(EntityClassification classification)
{
    switch (classification) {
        case EntityClassification::Monster:
            return 128;
        case EntityClassification::Creature:
            return 128;
        case EntityClassification::Ambient:
            return 128;
        case EntityClassification::Axolotls:
            return 128;
        case EntityClassification::UndergroundWaterCreature:
            return 128;
        case EntityClassification::WaterCreature:
            return 128;
        case EntityClassification::WaterAmbient:
            return 64;
        case EntityClassification::Misc:
            return 128;
    }
    return 128;
}

} // namespace mc::entity
