#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestErrorType.hpp"

#include <sstream>

namespace mc::test {

const char* gameTestErrorTypeName(GameTestErrorType type) noexcept
{
    switch (type) {
        case GameTestErrorType::Unknown:
            return "Unknown";
        case GameTestErrorType::Waiting:
            return "Waiting";
        case GameTestErrorType::ExhaustedAttempts:
            return "ExhaustedAttempts";
        case GameTestErrorType::AssertAtPosition:
            return "AssertAtPosition";
        case GameTestErrorType::MethodNotImplemented:
            return "MethodNotImplemented";
        case GameTestErrorType::ExecutionTimeout:
            return "ExecutionTimeout";
        case GameTestErrorType::LevelStateModificationFailed:
            return "LevelStateModificationFailed";
        case GameTestErrorType::FailConditionsMet:
            return "FailConditionsMet";
        case GameTestErrorType::Assert:
            return "Assert";
        case GameTestErrorType::SimulatedPlayerOutOfBounds:
            return "SimulatedPlayerOutOfBounds";
    }
    return "Unknown";
}

std::string GameTestError::formattedMessage() const
{
    // 逐字符扫描，遇到 {N} 形态的占位符用 params[N] 替换；非法占位符原样保留
    std::string result;
    result.reserve(m_message.size() + 16);
    const auto length = m_message.size();
    for (std::size_t i = 0; i < length;) {
        const char ch = m_message[i];
        if (ch != '{') {
            result.push_back(ch);
            ++i;
            continue;
        }
        // 寻找闭合 '}'
        std::size_t close = i + 1;
        while (close < length && m_message[close] != '}') {
            ++close;
        }
        if (close >= length) {
            // 未闭合，原样输出剩余
            result.append(m_message, i, std::string::npos);
            break;
        }
        // 解析 {N}
        const std::string idxStr = m_message.substr(i + 1, close - i - 1);
        bool isNumber = !idxStr.empty();
        for (char c : idxStr) {
            if (c < '0' || c > '9') {
                isNumber = false;
                break;
            }
        }
        if (!isNumber) {
            // 非法占位符，原样输出
            result.push_back('{');
            result.append(idxStr);
            result.push_back('}');
            i = close + 1;
            continue;
        }
        const auto idx = static_cast<std::size_t>(std::stoul(idxStr));
        if (idx < m_params.size()) {
            result.append(m_params[idx]);
        } else {
            // 越界，保留原占位符
            result.push_back('{');
            result.append(idxStr);
            result.push_back('}');
        }
        i = close + 1;
    }
    return result;
}

std::string GameTestError::getMessageToShowAtBlock() const
{
    if (!m_context.has_value()) {
        return formattedMessage();
    }
    const auto& ctx = *m_context;
    std::ostringstream oss;
    oss << "(" << ctx.relativePosition().x << "," << ctx.relativePosition().y << "," << ctx.relativePosition().z << ") "
        << formattedMessage();
    return oss.str();
}

} // namespace mc::test
