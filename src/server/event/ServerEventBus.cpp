#include "ServerEventBus.hpp"

namespace mc::server::event {

ServerEventBus& ServerEventBus::instance()
{
    static ServerEventBus instance;
    return instance;
}

} // namespace mc::server::event
