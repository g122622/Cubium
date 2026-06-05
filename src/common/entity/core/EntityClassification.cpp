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

/**
 * @file EntityClassification.cpp
 * @brief 实体分类实现
 */

#include "EntityClassification.hpp"

namespace mc::entity {

EntityClassificationInfo EntityClassificationInfo::get(EntityClassification classification)
{
    switch (classification) {
        case EntityClassification::Monster:
            return {
                EntityClassification::Monster,
                "monster",
                70,    // maxCount
                false, // isPeaceful
                false, // isAnimal
                128,   // despawnDistance
                32     // randomDespawnDistance
            };
        case EntityClassification::Creature:
            return {
                EntityClassification::Creature,
                "creature",
                10,   // maxCount
                true, // isPeaceful
                true, // isAnimal
                128,  // despawnDistance
                32    // randomDespawnDistance
            };
        case EntityClassification::Ambient:
            return {
                EntityClassification::Ambient,
                "ambient",
                15,    // maxCount
                true,  // isPeaceful
                false, // isAnimal
                128,   // despawnDistance
                32     // randomDespawnDistance
            };
        case EntityClassification::WaterCreature:
            return {
                EntityClassification::WaterCreature,
                "water_creature",
                5,     // maxCount
                true,  // isPeaceful
                false, // isAnimal
                128,   // despawnDistance
                32     // randomDespawnDistance
            };
        case EntityClassification::WaterAmbient:
            return {
                EntityClassification::WaterAmbient,
                "water_ambient",
                20,    // maxCount
                true,  // isPeaceful
                false, // isAnimal
                64,    // despawnDistance
                32     // randomDespawnDistance
            };
        case EntityClassification::Misc:
        default:
            return {
                EntityClassification::Misc,
                "misc",
                -1,    // maxCount (无限制)
                true,  // isPeaceful
                false, // isAnimal
                128,   // despawnDistance
                32     // randomDespawnDistance
            };
    }
}

} // namespace mc::entity
