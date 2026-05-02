#pragma once

#include "common/core/Types.hpp"
#include <vector>

namespace mc::util {

/**
 * @brief 解压 gzip 压缩数据
 *
 * @param compressed 压缩后的数据
 * @return 解压后的数据，失败时返回空向量
 */
[[nodiscard]] std::vector<u8> decompressGzip(const std::vector<u8>& compressed);

/**
 * @brief 使用 gzip 压缩数据
 *
 * @param data 原始数据
 * @return 压缩后的数据，失败时返回空向量
 */
[[nodiscard]] std::vector<u8> compressGzip(const std::vector<u8>& data);

} // namespace mc::util
