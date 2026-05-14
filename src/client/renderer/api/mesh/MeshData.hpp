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

#pragma once

#include "../Types.hpp"
#include <vector>

namespace mc::client::renderer::api {

/**
 * @brief 网格数据
 *
 * 存储顶点和索引数据，用于渲染。
 */
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    /**
     * @brief 清空网格数据
     */
    void clear()
    {
        vertices.clear();
        indices.clear();
    }

    /**
     * @brief 预分配空间
     */
    void reserve(size_t vertexCount, size_t indexCount)
    {
        vertices.reserve(vertexCount);
        indices.reserve(indexCount);
    }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const { return vertices.empty(); }

    /**
     * @brief 获取顶点数量
     */
    [[nodiscard]] size_t vertexCount() const { return vertices.size(); }

    /**
     * @brief 获取索引数量
     */
    [[nodiscard]] size_t indexCount() const { return indices.size(); }

    /**
     * @brief 添加一个面 (4个顶点 + 6个索引)
     * @param faceVertices 4个顶点
     * @param baseIndex 基础索引偏移
     */
    void addFace(const std::array<Vertex, 4>& faceVertices, u32 baseIndex)
    {
        // 添加顶点
        for (const auto& v : faceVertices) {
            vertices.push_back(v);
        }
        // 添加索引 (两个三角形)
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 0);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }

    /**
     * @brief 获取顶点数据大小（字节）
     */
    [[nodiscard]] size_t vertexDataSize() const { return vertices.size() * sizeof(Vertex); }

    /**
     * @brief 获取索引数据大小（字节）
     */
    [[nodiscard]] size_t indexDataSize() const { return indices.size() * sizeof(u32); }
};

/**
 * @brief 区块网格数据
 *
 * 存储单个区块的网格数据，分离不透明和透明网格。
 */
struct ChunkMeshData {
    MeshData solidMesh;       // 不透明网格
    MeshData translucentMesh; // 半透明网格

    /**
     * @brief 清空所有网格数据
     */
    void clear()
    {
        solidMesh.clear();
        translucentMesh.clear();
    }

    /**
     * @brief 检查是否为空
     */
    [[nodiscard]] bool empty() const { return solidMesh.empty() && translucentMesh.empty(); }

    /**
     * @brief 获取总顶点数
     */
    [[nodiscard]] size_t totalVertexCount() const { return solidMesh.vertexCount() + translucentMesh.vertexCount(); }

    /**
     * @brief 获取总索引数
     */
    [[nodiscard]] size_t totalIndexCount() const { return solidMesh.indexCount() + translucentMesh.indexCount(); }
};

} // namespace mc::client::renderer::api
