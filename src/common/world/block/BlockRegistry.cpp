#include "BlockRegistry.hpp"

namespace mc {

BlockRegistry& BlockRegistry::instance()
{
    static BlockRegistry instance;
    return instance;
}

} // namespace mc
