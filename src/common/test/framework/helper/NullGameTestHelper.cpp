#include "common/test/framework/helper/NullGameTestHelper.hpp"

#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::test {

NullGameTestHelper::NullGameTestHelper() = default;

// 析构需完整类型 GameTestSequence，故在此 .cpp 定义（头里用前向声明 + unique_ptr 成员）
NullGameTestHelper::~NullGameTestHelper() = default;

GameTestSequence& NullGameTestHelper::startSequence()
{
    // 懒构造一个真实 GameTestSequence（持本 helper 引用），供状态机单测驱动
    if (!m_sequence) {
        m_sequence = std::make_unique<GameTestSequence>(*this);
    }
    return *m_sequence;
}

mc::IWorld& NullGameTestHelper::world() noexcept
{
    // NullGameTestHelper 不持有真实世界；调用方不应经 null helper 取 world。
    // 用断言暴露调用错误（对齐项目"不过度防御、用断言暴露问题"规范）。
    MC_ASSERT_RELEASE_MSG(false, "NullGameTestHelper has no world");
    // 不可达占位（编译需要返回值）
    static mc::IWorld* s_null = nullptr;
    return *s_null;
}

} // namespace mc::test
