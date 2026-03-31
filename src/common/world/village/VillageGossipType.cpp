#include "VillageGossipType.hpp"

namespace mc {
namespace world {
namespace village {

const char* GossipTypeHelper::getName(VillageGossipType type) {
    switch (type) {
        case VillageGossipType::MajorNegative:   return "major_negative";
        case VillageGossipType::MinorNegative:   return "minor_negative";
        case VillageGossipType::Trading:         return "trading";
        case VillageGossipType::MinorPositive:   return "minor_positive";
        case VillageGossipType::MajorPositive:   return "major_positive";
        default: return "unknown";
    }
}

i32 GossipTypeHelper::getReputationImpact(VillageGossipType type) {
    switch (type) {
        case VillageGossipType::MajorNegative:   return -100;
        case VillageGossipType::MinorNegative:   return -20;
        case VillageGossipType::Trading:         return 2;
        case VillageGossipType::MinorPositive:   return 20;
        case VillageGossipType::MajorPositive:   return 100;
        default: return 0;
    }
}

i32 GossipTypeHelper::getMaxValue(VillageGossipType type) {
    switch (type) {
        case VillageGossipType::MajorNegative:   return 100;  // 最多累积100次
        case VillageGossipType::MinorNegative:   return 200;  // 最多累积200次
        case VillageGossipType::Trading:         return 100;  // 最多累积100次交易
        case VillageGossipType::MinorPositive:   return 200;  // 最多累积200次
        case VillageGossipType::MajorPositive:   return 20;   // 治愈最多累积20次
        default: return 100;
    }
}

i64 GossipTypeHelper::getDecayInterval(VillageGossipType type) {
    // 所有流言每24000 tick（1游戏日）衰减一次
    // 但负面流言衰减更快
    switch (type) {
        case VillageGossipType::MajorNegative:   return 12000;  // 半天
        case VillageGossipType::MinorNegative:   return 24000;  // 1天
        case VillageGossipType::Trading:         return 24000;  // 1天
        case VillageGossipType::MinorPositive:   return 24000;  // 1天
        case VillageGossipType::MajorPositive:   return 48000;  // 2天（治愈保持更久）
        default: return 24000;
    }
}

f32 GossipTypeHelper::getDecayRate(VillageGossipType type) {
    // 衰减率：每次衰减保留的比例
    switch (type) {
        case VillageGossipType::MajorNegative:   return 0.8f;   // 每次保留80%
        case VillageGossipType::MinorNegative:   return 0.9f;   // 每次保留90%
        case VillageGossipType::Trading:         return 0.9f;   // 每次保留90%
        case VillageGossipType::MinorPositive:   return 0.9f;   // 每次保留90%
        case VillageGossipType::MajorPositive:   return 0.95f;  // 每次保留95%
        default: return 0.9f;
    }
}

bool GossipTypeHelper::isNegative(VillageGossipType type) {
    return type == VillageGossipType::MajorNegative ||
           type == VillageGossipType::MinorNegative;
}

bool GossipTypeHelper::isPositive(VillageGossipType type) {
    return type == VillageGossipType::Trading ||
           type == VillageGossipType::MinorPositive ||
           type == VillageGossipType::MajorPositive;
}

} // namespace village
} // namespace world
} // namespace mc
