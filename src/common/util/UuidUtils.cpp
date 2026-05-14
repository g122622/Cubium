/**
 * @file UuidUtils.cpp
 * @brief UUID 工具函数实现
 */

#include "UuidUtils.hpp"
#include "crypto/Md5.hpp"

namespace mc {
namespace util {

std::array<u8, 16> computeMd5Hash(const std::string& data)
{
    return crypto::Md5::hash(data);
}

} // namespace util
} // namespace mc
