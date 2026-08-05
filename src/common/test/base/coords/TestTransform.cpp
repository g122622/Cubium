#include "common/test/base/coords/TestTransform.hpp"

#include "common/util/math/Vector3.hpp"

namespace mc::test {

math::Vector3d TestTransform::relativeToWorldF(math::Vector3d relativePos) const noexcept
{
    // 浮点旋转：连续坐标 [0,size]，不取 size-1 偏移
    math::Vector3d rotated{relativePos};
    switch (m_rotation) {
        case Rotation::None:
            break;
        case Rotation::Clockwise90:
            rotated = math::Vector3d{static_cast<f64>(m_size.z) - relativePos.z, relativePos.y, relativePos.x};
            break;
        case Rotation::Clockwise180:
            rotated = math::Vector3d{
                static_cast<f64>(m_size.x) - relativePos.x, relativePos.y, static_cast<f64>(m_size.z) - relativePos.z};
            break;
        case Rotation::CounterClockwise90:
            rotated = math::Vector3d{relativePos.z, relativePos.y, static_cast<f64>(m_size.x) - relativePos.x};
            break;
    }
    return math::Vector3d{static_cast<f64>(m_origin.x) + rotated.x,
        static_cast<f64>(m_origin.y) + rotated.y,
        static_cast<f64>(m_origin.z) + rotated.z};
}

math::Vector3d TestTransform::worldToRelativeF(math::Vector3d worldPos) const noexcept
{
    const math::Vector3d local{worldPos.x - static_cast<f64>(m_origin.x),
        worldPos.y - static_cast<f64>(m_origin.y),
        worldPos.z - static_cast<f64>(m_origin.z)};
    math::Vector3d relative{local};
    switch (m_rotation) {
        case Rotation::None:
            break;
        case Rotation::Clockwise90:
            relative = math::Vector3d{local.z, local.y, static_cast<f64>(m_size.z) - local.x};
            break;
        case Rotation::Clockwise180:
            relative =
                math::Vector3d{static_cast<f64>(m_size.x) - local.x, local.y, static_cast<f64>(m_size.z) - local.z};
            break;
        case Rotation::CounterClockwise90:
            relative = math::Vector3d{static_cast<f64>(m_size.x) - local.z, local.y, local.x};
            break;
    }
    return relative;
}

} // namespace mc::test
