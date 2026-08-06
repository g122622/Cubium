/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "server/test/minecraft/structure/BehaviorPackStructureSource.hpp"

#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"

namespace mc::test {

BehaviorPackStructureSource::BehaviorPackStructureSource(mc::mod::bedrock::addon::BehaviorPackList& packList)
    : m_packList(packList)
{}

Result<std::vector<u8>> BehaviorPackStructureSource::readStructure(
    const std::string& namespaceId, const std::string& path) const
{
    // 基岩版语义：structures/<namespace>/<path>.mcstructure，遍历已启用行为包按优先级返回首个命中
    const std::string relativePath = "structures/" + namespaceId + "/" + path + ".mcstructure";

    for (const auto* pack : m_packList.getEnabledPacks()) {
        if (pack == nullptr) {
            continue;
        }
        auto result = pack->readResource(relativePath);
        if (result.success() && !result.value().empty()) {
            return result.value();
        }
    }

    return Error(
        ErrorCode::FileNotFound, "Structure resource not found in any behavior pack: " + namespaceId + ":" + path);
}

} // namespace mc::test
