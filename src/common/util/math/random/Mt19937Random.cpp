#include "Mt19937Random.hpp"
#include <random>

namespace mc::math {

Mt19937Random::Mt19937Random(u64 seed)
    : m_engine(seed)
{}

Mt19937Random Mt19937Random::fromRandomDevice()
{
    std::random_device rd;
    return Mt19937Random(static_cast<u64>(rd()) << 32 | rd());
}

void Mt19937Random::setSeed(u64 seed)
{
    m_engine.seed(seed);
    m_hasGaussian = false;
}

u64 Mt19937Random::nextU64()
{
    return m_engine();
}

} // namespace mc::math
