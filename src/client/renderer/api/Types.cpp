/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#include "Types.hpp"

namespace mc::client::renderer::api {

namespace BlockGeometry {

std::array<f64, 3> getFaceNormal(Face face)
{
    switch (face) {
        case Face::Bottom:
            return {0.0f, -1.0f, 0.0f};
        case Face::Top:
            return {0.0f, 1.0f, 0.0f};
        case Face::North:
            return {0.0f, 0.0f, -1.0f};
        case Face::South:
            return {0.0f, 0.0f, 1.0f};
        case Face::West:
            return {-1.0f, 0.0f, 0.0f};
        case Face::East:
            return {1.0f, 0.0f, 0.0f};
        default:
            return {0.0f, 0.0f, 0.0f};
    }
}

std::array<f64, 12> getFaceVertices(Face face)
{
    // 方块顶点 (单位立方体 0-1)
    // v0 = (0,0,0), v1 = (1,0,0), v2 = (1,1,0), v3 = (0,1,0)
    // v4 = (0,0,1), v5 = (1,0,1), v6 = (1,1,1), v7 = (0,1,1)

    switch (face) {
        case Face::Bottom: // Y- (v0, v4, v5, v1)
            return {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f};
        case Face::Top: // Y+ (v3, v2, v6, v7)
            return {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};
        case Face::North: // Z- (v0, v3, v2, v1)
            return {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f};
        case Face::South: // Z+ (v5, v6, v7, v4)
            return {1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
        case Face::West: // X- (v4, v7, v3, v0)
            return {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        case Face::East: // X+ (v1, v2, v6, v5)
            return {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};
        default:
            return {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    }
}

std::array<u32, 6> getFaceIndices()
{
    // 两个三角形：0-1-2, 0-2-3
    return {0, 1, 2, 0, 2, 3};
}

std::array<i32, 3> getFaceDirection(Face face)
{
    switch (face) {
        case Face::Bottom:
            return {0, -1, 0};
        case Face::Top:
            return {0, 1, 0};
        case Face::North:
            return {0, 0, -1};
        case Face::South:
            return {0, 0, 1};
        case Face::West:
            return {-1, 0, 0};
        case Face::East:
            return {1, 0, 0};
        default:
            return {0, 0, 0};
    }
}

bool shouldRenderFace(Face face, bool neighborOpaque)
{
    // 如果邻居不透明，则不渲染该面（面剔除）
    return !neighborOpaque;
}

} // namespace BlockGeometry

} // namespace mc::client::renderer::api
