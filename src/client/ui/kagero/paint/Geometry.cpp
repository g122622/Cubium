#include "Geometry.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::ui::kagero::paint {

namespace {

f32 radians(f32 degrees) {
    return degrees * mc::math::PI / 180.0f;
}
} // namespace

Matrix Matrix::makeTranslate(f32 dx, f32 dy) {
    Matrix result;
    result.m[2] = dx;
    result.m[5] = dy;
    return result;
}

Matrix Matrix::makeScale(f32 sx, f32 sy) {
    Matrix result;
    result.m[0] = sx;
    result.m[4] = sy;
    return result;
}

Matrix Matrix::makeRotate(f32 degrees) {
    Matrix result;
    const f32 angle = radians(degrees);
    const f32 c = std::cos(angle);
    const f32 s = std::sin(angle);
    result.m[0] = c;
    result.m[1] = -s;
    result.m[3] = s;
    result.m[4] = c;
    return result;
}

Matrix Matrix::operator*(const Matrix& other) const {
    Matrix result;
    // 3x3 矩阵乘法
    // result[i][j] = sum(this[i][k] * other[k][j])
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            f32 sum = 0.0f;
            for (int k = 0; k < 3; ++k) {
                sum += m[row * 3 + k] * other.m[k * 3 + col];
            }
            result.m[row * 3 + col] = sum;
        }
    }
    return result;
}

void Matrix::translate(f32 dx, f32 dy) {
    *this = *this * makeTranslate(dx, dy);
}

void Matrix::scale(f32 sx, f32 sy) {
    *this = *this * makeScale(sx, sy);
}

void Matrix::rotate(f32 degrees) {
    *this = *this * makeRotate(degrees);
}

void Matrix::transformPoint(f32& x, f32& y) const {
    const f32 nx = m[0] * x + m[1] * y + m[2];
    const f32 ny = m[3] * x + m[4] * y + m[5];
    x = nx;
    y = ny;
}

f32 Matrix::determinant() const {
    // 对于仿射矩阵，最后一行是 [0, 0, 1]
    // det = m[0] * m[4] - m[1] * m[3]
    return m[0] * m[4] - m[1] * m[3];
}

Matrix Matrix::inverse() const {
    const f32 det = determinant();
    if (std::abs(det) < 1e-6f) {
        // 不可逆，返回单位矩阵
        return identity();
    }

    const f32 invDet = 1.0f / det;
    Matrix result;

    // 仿射矩阵求逆
    // | a  b  tx |         | d  -b  (b*ty - d*tx) |
    // | c  d  ty | inverse | -c  a  (c*tx - a*ty) |
    // | 0  0  1  |         | 0   0       1        |
    const f32 a = m[0];
    const f32 b = m[1];
    const f32 tx = m[2];
    const f32 c = m[3];
    const f32 d = m[4];
    const f32 ty = m[5];

    result.m[0] = d * invDet;
    result.m[1] = -b * invDet;
    result.m[2] = (b * ty - d * tx) * invDet;
    result.m[3] = -c * invDet;
    result.m[4] = a * invDet;
    result.m[5] = (c * tx - a * ty) * invDet;

    return result;
}

} // namespace mc::client::ui::kagero::paint
