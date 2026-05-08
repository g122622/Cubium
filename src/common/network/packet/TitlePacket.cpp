#include "TitlePacket.hpp"
#include "util/text/ITextComponent.hpp"
#include <nlohmann/json.hpp>

namespace mc::network {

TitlePacket::TitlePacket()
    : Packet(PacketType::Title)
{
}

TitlePacket::TitlePacket(TitleAction action)
    : Packet(PacketType::Title)
    , m_action(action)
{
}

// static
TitlePacket TitlePacket::createTitle(const std::string& text) {
    TitlePacket packet(TitleAction::Title);
    packet.m_text = text;
    return packet;
}

// static
TitlePacket TitlePacket::createTitle(const text::ITextComponent& text) {
    return createTitle(serializeText(text));
}

// static
TitlePacket TitlePacket::createSubtitle(const std::string& text) {
    TitlePacket packet(TitleAction::Subtitle);
    packet.m_text = text;
    return packet;
}

// static
TitlePacket TitlePacket::createSubtitle(const text::ITextComponent& text) {
    return createSubtitle(serializeText(text));
}

// static
TitlePacket TitlePacket::createActionbar(const std::string& text) {
    TitlePacket packet(TitleAction::Actionbar);
    packet.m_text = text;
    return packet;
}

// static
TitlePacket TitlePacket::createActionbar(const text::ITextComponent& text) {
    return createActionbar(serializeText(text));
}

// static
TitlePacket TitlePacket::createTimes(i32 fadeIn, i32 stay, i32 fadeOut) {
    TitlePacket packet(TitleAction::Times);
    packet.m_fadeIn = fadeIn;
    packet.m_stay = stay;
    packet.m_fadeOut = fadeOut;
    return packet;
}

// static
TitlePacket TitlePacket::createClear() {
    return TitlePacket(TitleAction::Clear);
}

// static
TitlePacket TitlePacket::createReset() {
    return TitlePacket(TitleAction::Reset);
}

void TitlePacket::setTimes(i32 fadeIn, i32 stay, i32 fadeOut) {
    m_fadeIn = fadeIn;
    m_stay = stay;
    m_fadeOut = fadeOut;
}

// static
std::string TitlePacket::serializeText(const text::ITextComponent& text) {
    return text.toJson().dump();
}

size_t TitlePacket::expectedSize() const {
    // 基础：VarInt(action)
    size_t size = 5;  // VarInt最多5字节

    // 根据动作类型添加额外数据
    switch (m_action) {
        case TitleAction::Title:
        case TitleAction::Subtitle:
        case TitleAction::Actionbar:
            // 文本：VarInt(长度) + UTF-8数据
            if (m_text.has_value()) {
                size += 5 + m_text->size();  // VarInt长度 + 文本
            }
            break;

        case TitleAction::Times:
            // 3个i32
            size += 12;
            break;

        case TitleAction::Clear:
        case TitleAction::Reset:
            // 无额外数据
            break;
    }

    return size;
}

Result<std::vector<u8>> TitlePacket::serialize() const {
    PacketSerializer serializer(expectedSize());

    // 写入动作类型
    serializer.writeVarInt(static_cast<i32>(m_action));

    // 根据动作类型写入额外数据
    switch (m_action) {
        case TitleAction::Title:
        case TitleAction::Subtitle:
        case TitleAction::Actionbar:
            // 写入文本JSON
            if (m_text.has_value()) {
                serializer.writeString(m_text.value());
            } else {
                // 空字符串表示清除
                serializer.writeString("");
            }
            break;

        case TitleAction::Times:
            // 写入时间参数（tick）
            serializer.writeI32(m_fadeIn);
            serializer.writeI32(m_stay);
            serializer.writeI32(m_fadeOut);
            break;

        case TitleAction::Clear:
        case TitleAction::Reset:
            // 无额外数据
            break;
    }

    std::vector<u8> result;
    result.insert(result.end(), serializer.data(), serializer.data() + serializer.size());
    return result;
}

Result<void> TitlePacket::deserialize(const u8* data, size_t size) {
    if (size == 0) {
        return Error(ErrorCode::InvalidData, "TitlePacket: empty data");
    }

    PacketDeserializer deserializer(data, size);

    // 读取动作类型
    auto actionResult = deserializer.readVarInt();
    if (!actionResult.success()) {
        return actionResult.error();
    }

    // 验证动作类型
    i32 actionValue = actionResult.value();
    if (actionValue < 0 || actionValue > 5) {
        return Error(ErrorCode::InvalidData, "TitlePacket: invalid action type");
    }
    m_action = static_cast<TitleAction>(actionValue);

    // 根据动作类型读取额外数据
    switch (m_action) {
        case TitleAction::Title:
        case TitleAction::Subtitle:
        case TitleAction::Actionbar: {
            // 读取文本JSON
            auto textResult = deserializer.readString();
            if (!textResult.success()) {
                return textResult.error();
            }
            m_text = textResult.value();
            break;
        }

        case TitleAction::Times: {
            // 读取时间参数
            auto fadeInResult = deserializer.readI32();
            if (!fadeInResult.success()) {
                return fadeInResult.error();
            }
            m_fadeIn = fadeInResult.value();

            auto stayResult = deserializer.readI32();
            if (!stayResult.success()) {
                return stayResult.error();
            }
            m_stay = stayResult.value();

            auto fadeOutResult = deserializer.readI32();
            if (!fadeOutResult.success()) {
                return fadeOutResult.error();
            }
            m_fadeOut = fadeOutResult.value();
            break;
        }

        case TitleAction::Clear:
        case TitleAction::Reset:
            // 无额外数据
            m_text = std::nullopt;
            break;
    }

    return {};
}

} // namespace mc::network
