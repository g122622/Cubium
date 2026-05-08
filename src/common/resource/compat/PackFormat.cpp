#include "PackFormat.hpp"

namespace mc {
namespace resource {
namespace compat {

PackFormat detectPackFormat(i32 packFormat) {
    switch (packFormat) {
        case 1:  return PackFormat::V1_6_to_1_8;
        case 2:  return PackFormat::V1_9_to_1_10;
        case 3:  return PackFormat::V1_11_to_1_12;
        case 4:  return PackFormat::V1_13_to_1_14;
        case 5:  return PackFormat::V1_15_to_1_16_1;
        case 6:  return PackFormat::V1_16_2_to_1_16_5;
        case 7:  return PackFormat::V1_17;
        case 8:  return PackFormat::V1_18;
        case 9:  return PackFormat::V1_19;
        default: return PackFormat::Unknown;
    }
}

bool usesOldTexturePaths(PackFormat format) {
    // MC 1.12 及更早版本使用 textures/blocks/
    return format == PackFormat::V1_6_to_1_8 ||
           format == PackFormat::V1_9_to_1_10 ||
           format == PackFormat::V1_11_to_1_12;
}

bool usesNewTexturePaths(PackFormat format) {
    // MC 1.13+ 使用 textures/block/
    return format == PackFormat::V1_13_to_1_14 ||
           format == PackFormat::V1_15_to_1_16_1 ||
           format == PackFormat::V1_16_2_to_1_16_5 ||
           format == PackFormat::V1_17 ||
           format == PackFormat::V1_18 ||
           format == PackFormat::V1_19;
}

bool usesOldItemPaths(PackFormat format) {
    // MC 1.12 及更早版本使用 textures/items/
    return format == PackFormat::V1_6_to_1_8 ||
           format == PackFormat::V1_9_to_1_10 ||
           format == PackFormat::V1_11_to_1_12;
}

std::string_view packFormatToString(PackFormat format) {
    switch (format) {
        case PackFormat::Unknown:        return "Unknown";
        case PackFormat::V1_6_to_1_8:    return "1.6-1.8";
        case PackFormat::V1_9_to_1_10:   return "1.9-1.10";
        case PackFormat::V1_11_to_1_12:  return "1.11-1.12";
        case PackFormat::V1_13_to_1_14:  return "1.13-1.14";
        case PackFormat::V1_15_to_1_16_1: return "1.15-1.16.1";
        case PackFormat::V1_16_2_to_1_16_5: return "1.16.2-1.16.5";
        case PackFormat::V1_17:          return "1.17";
        case PackFormat::V1_18:          return "1.18";
        case PackFormat::V1_19:          return "1.19";
        default:                         return "Unknown";
    }
}

bool requiresTextureNameMapping(PackFormat format) {
    // MC 1.12 及更早版本使用旧版命名约定
    return usesOldTexturePaths(format);
}

} // namespace compat
} // namespace resource
} // namespace mc
