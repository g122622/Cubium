/**
 * @file EntityClassification.cpp
 * @brief 实体分类实现
 */

#include "EntityClassification.hpp"

namespace mc::entity {

EntityClassificationInfo EntityClassificationInfo::get(EntityClassification classification) {
    switch (classification) {
        case EntityClassification::Monster:
            return {
                EntityClassification::Monster,
                "monster",
                70,     // maxCount
                false,  // isPeaceful
                false,  // isAnimal
                128,    // despawnDistance
                32      // randomDespawnDistance
            };
        case EntityClassification::Creature:
            return {
                EntityClassification::Creature,
                "creature",
                10,     // maxCount
                true,   // isPeaceful
                true,   // isAnimal
                128,    // despawnDistance
                32      // randomDespawnDistance
            };
        case EntityClassification::Ambient:
            return {
                EntityClassification::Ambient,
                "ambient",
                15,     // maxCount
                true,   // isPeaceful
                false,  // isAnimal
                128,    // despawnDistance
                32      // randomDespawnDistance
            };
        case EntityClassification::WaterCreature:
            return {
                EntityClassification::WaterCreature,
                "water_creature",
                5,      // maxCount
                true,   // isPeaceful
                false,  // isAnimal
                128,    // despawnDistance
                32      // randomDespawnDistance
            };
        case EntityClassification::WaterAmbient:
            return {
                EntityClassification::WaterAmbient,
                "water_ambient",
                20,     // maxCount
                true,   // isPeaceful
                false,  // isAnimal
                64,     // despawnDistance
                32      // randomDespawnDistance
            };
        case EntityClassification::Misc:
        default:
            return {
                EntityClassification::Misc,
                "misc",
                -1,     // maxCount (无限制)
                true,   // isPeaceful
                false,  // isAnimal
                128,    // despawnDistance
                32      // randomDespawnDistance
            };
    }
}

} // namespace mc::entity
