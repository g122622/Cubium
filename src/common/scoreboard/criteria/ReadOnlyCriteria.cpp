#include "ReadOnlyCriteria.hpp"

namespace mc::scoreboard {

ReadOnlyCriteria::ReadOnlyCriteria(const std::string& name, RenderType renderType)
    : m_name(name)
    , m_renderType(renderType)
{
}

HealthCriteria::HealthCriteria()
    : ReadOnlyCriteria(NAME, RenderType::Hearts)
{
}

FoodCriteria::FoodCriteria()
    : ReadOnlyCriteria(NAME, RenderType::Integer)
{
}

AirCriteria::AirCriteria()
    : ReadOnlyCriteria(NAME, RenderType::Integer)
{
}

ArmorCriteria::ArmorCriteria()
    : ReadOnlyCriteria(NAME, RenderType::Integer)
{
}

XpCriteria::XpCriteria()
    : ReadOnlyCriteria(NAME, RenderType::Integer)
{
}

LevelCriteria::LevelCriteria()
    : ReadOnlyCriteria(NAME, RenderType::Integer)
{
}

} // namespace mc::scoreboard
