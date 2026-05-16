#include "BenchmarkRegistry.hpp"

namespace mc::benchmark {

BenchmarkRegistry& BenchmarkRegistry::instance()
{
    static BenchmarkRegistry registry;
    return registry;
}

void BenchmarkRegistry::registerCase(std::string name, Factory factory)
{
    m_factories.emplace(std::move(name), std::move(factory));
}

std::unique_ptr<IBenchmarkCase> BenchmarkRegistry::create(const std::string& name) const
{
    const auto iter = m_factories.find(name);
    if (iter == m_factories.end()) {
        return nullptr;
    }
    return iter->second();
}

} // namespace mc::benchmark
