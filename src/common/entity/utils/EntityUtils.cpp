#include "EntityUtils.hpp"

namespace mc {

namespace EntityUtils {

[[nodiscard]] const char* legacyTypeToTypeId(LegacyEntityType type)
{
    switch (type) {
        case LegacyEntityType::Player:
            return "minecraft:player";
        case LegacyEntityType::Item:
            return "minecraft:item";
        case LegacyEntityType::ExperienceOrb:
            return "minecraft:experience_orb";

        case LegacyEntityType::Pig:
            return "minecraft:pig";
        case LegacyEntityType::Cow:
            return "minecraft:cow";
        case LegacyEntityType::Sheep:
            return "minecraft:sheep";
        case LegacyEntityType::Chicken:
            return "minecraft:chicken";
        case LegacyEntityType::Rabbit:
            return "minecraft:rabbit";
        case LegacyEntityType::Mooshroom:
            return "minecraft:mooshroom";
        case LegacyEntityType::Wolf:
            return "minecraft:wolf";
        case LegacyEntityType::Cat:
            return "minecraft:cat";
        case LegacyEntityType::Ocelot:
            return "minecraft:ocelot";
        case LegacyEntityType::Parrot:
            return "minecraft:parrot";
        case LegacyEntityType::Fox:
            return "minecraft:fox";
        case LegacyEntityType::Panda:
            return "minecraft:panda";
        case LegacyEntityType::PolarBear:
            return "minecraft:polar_bear";
        case LegacyEntityType::Turtle:
            return "minecraft:turtle";
        case LegacyEntityType::Bee:
            return "minecraft:bee";
        case LegacyEntityType::Strider:
            return "minecraft:strider";
        case LegacyEntityType::Squid:
            return "minecraft:squid";
        case LegacyEntityType::Dolphin:
            return "minecraft:dolphin";
        case LegacyEntityType::Cod:
            return "minecraft:cod";
        case LegacyEntityType::Salmon:
            return "minecraft:salmon";
        case LegacyEntityType::Pufferfish:
            return "minecraft:pufferfish";
        case LegacyEntityType::TropicalFish:
            return "minecraft:tropical_fish";
        case LegacyEntityType::Bat:
            return "minecraft:bat";
        case LegacyEntityType::IronGolem:
            return "minecraft:iron_golem";
        case LegacyEntityType::SnowGolem:
            return "minecraft:snow_golem";
        case LegacyEntityType::Horse:
            return "minecraft:horse";
        case LegacyEntityType::Donkey:
            return "minecraft:donkey";
        case LegacyEntityType::Mule:
            return "minecraft:mule";
        case LegacyEntityType::SkeletonHorse:
            return "minecraft:skeleton_horse";
        case LegacyEntityType::ZombieHorse:
            return "minecraft:zombie_horse";
        case LegacyEntityType::Llama:
            return "minecraft:llama";
        case LegacyEntityType::TraderLlama:
            return "minecraft:trader_llama";

        case LegacyEntityType::Zombie:
            return "minecraft:zombie";
        case LegacyEntityType::Skeleton:
            return "minecraft:skeleton";
        case LegacyEntityType::Husk:
            return "minecraft:husk";
        case LegacyEntityType::Drowned:
            return "minecraft:drowned";
        case LegacyEntityType::Stray:
            return "minecraft:stray";
        case LegacyEntityType::WitherSkeleton:
            return "minecraft:wither_skeleton";
        case LegacyEntityType::Phantom:
            return "minecraft:phantom";
        case LegacyEntityType::Spider:
            return "minecraft:spider";
        case LegacyEntityType::CaveSpider:
            return "minecraft:cave_spider";
        case LegacyEntityType::Endermite:
            return "minecraft:endermite";
        case LegacyEntityType::Silverfish:
            return "minecraft:silverfish";
        case LegacyEntityType::Creeper:
            return "minecraft:creeper";
        case LegacyEntityType::Slime:
            return "minecraft:slime";
        case LegacyEntityType::Giant:
            return "minecraft:giant";
        case LegacyEntityType::Enderman:
            return "minecraft:enderman";
        case LegacyEntityType::Shulker:
            return "minecraft:shulker";
        case LegacyEntityType::Ghast:
            return "minecraft:ghast";
        case LegacyEntityType::MagmaCube:
            return "minecraft:magma_cube";
        case LegacyEntityType::Piglin:
            return "minecraft:piglin";
        case LegacyEntityType::PiglinBrute:
            return "minecraft:piglin_brute";
        case LegacyEntityType::Hoglin:
            return "minecraft:hoglin";
        case LegacyEntityType::Zoglin:
            return "minecraft:zoglin";
        case LegacyEntityType::Vindicator:
            return "minecraft:vindicator";
        case LegacyEntityType::Evoker:
            return "minecraft:evoker";
        case LegacyEntityType::Illusioner:
            return "minecraft:illusioner";
        case LegacyEntityType::Pillager:
            return "minecraft:pillager";
        case LegacyEntityType::Guardian:
            return "minecraft:guardian";
        case LegacyEntityType::ElderGuardian:
            return "minecraft:elder_guardian";
        case LegacyEntityType::Witch:
            return "minecraft:witch";
        case LegacyEntityType::Ravager:
            return "minecraft:ravager";
        case LegacyEntityType::Blaze:
            return "minecraft:blaze";

        case LegacyEntityType::Wither:
            return "minecraft:wither";
        case LegacyEntityType::EnderDragon:
            return "minecraft:ender_dragon";

        case LegacyEntityType::Boat:
            return "minecraft:boat";
        case LegacyEntityType::Minecart:
            return "minecraft:minecart";

        case LegacyEntityType::Villager:
            return "minecraft:villager";
        case LegacyEntityType::Unknown:
        default:
            return "minecraft:unknown";
    }
}

} // namespace EntityUtils

} // namespace mc
