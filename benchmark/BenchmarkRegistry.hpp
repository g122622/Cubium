#pragma once

#include "BenchmarkCase.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc::benchmark {

class BenchmarkRegistry {
public:
    using Factory = std::function<std::unique_ptr<IBenchmarkCase>()>;

    static BenchmarkRegistry& instance();

    void registerCase(std::string name, Factory factory);
    [[nodiscard]] std::unique_ptr<IBenchmarkCase> create(const std::string& name) const;

private:
    std::unordered_map<std::string, Factory> m_factories;
};

} // namespace mc::benchmark
