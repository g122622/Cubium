#include "IResourcePack.hpp"

namespace mc {

Result<std::string> IResourcePack::readTextResource(std::string_view resourcePath) const {
    auto result = readResource(resourcePath);
    if (result.failed()) {
        return result.error();
    }

    const auto& data = result.value();
    return std::string(data.begin(), data.end());
}

} // namespace mc
