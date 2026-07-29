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

#include "common/network/buffer/NbtIo.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <sstream>

namespace mc::network::buffer::nbt_io {

namespace {

/**
 * @brief 把 ByteBuf 剩余可读字节拷成字符串（sstream 需连续内存）
 */
std::string remainingAsString(const ByteBuf& buf)
{
    return std::string(reinterpret_cast<const char*>(buf.bytes().data() + buf.readPosition()), buf.readableBytes());
}

} // namespace

Result<void> writeCompound(ByteBuf& buf, const mc::nbt::tags::compound_tag& tag)
{
    // 写时：tag → stringstream（带 java 上下文）→ 字节追加到 buf
    std::ostringstream out;
    out << mc::nbt::Contexts::java;
    tag.write(out); // compound_tag::write(ostream) 虚函数，内部按 java 上下文大端二进制序列化
    const std::string bytes = out.str();
    buf.writeBytes(reinterpret_cast<const u8*>(bytes.data()), bytes.size());
    return Result<void>::ok();
}

std::vector<u8> serializeRootCompoundToBytes(const mc::nbt::tags::compound_tag& tag)
{
    // 对齐 Java FriendlyByteBuf.writeNbt = NbtIo.writeAnyTag：
    //   writeByte(0x0A)        // TAG_Compound 类型字节
    //   writeUTF("")           // 空 root name（u16 长度 0x0000 + 0 字节）
    //   tag.write()            // entries + End（compound_tag::write 默认 is_root=false 已含 0x00 End）
    // 注意：is_root=true 会去掉 End（错向），此处须保持 is_root=false 默认。
    std::ostringstream out;
    out << mc::nbt::Contexts::java;
    out.put(static_cast<char>(mc::nbt::TagId::Compound)); // 0x0A
    out.put('\0');                                        // root name 长度高字节
    out.put('\0');                                        // root name 长度低字节（空 name）
    tag.write(out);                                       // entries + 0x00 End
    const std::string bytes = out.str();
    return std::vector<u8>(
        reinterpret_cast<const u8*>(bytes.data()), reinterpret_cast<const u8*>(bytes.data()) + bytes.size());
}

Result<void> writeRootCompound(ByteBuf& buf, const mc::nbt::tags::compound_tag& tag)
{
    const std::vector<u8> bytes = serializeRootCompoundToBytes(tag);
    buf.writeBytes(bytes.data(), bytes.size());
    return Result<void>::ok();
}

Result<std::unique_ptr<mc::nbt::tags::compound_tag>> readCompound(ByteBuf& buf)
{
    // 读时：剩余字节 → istringstream（带 java 上下文）→ compound_tag::read
    //       解析后按 tellg 差值推进 ByteBuf 游标。
    std::istringstream in(remainingAsString(buf));
    in >> mc::nbt::Contexts::java;

    const std::streampos before = in.tellg();
    std::unique_ptr<mc::nbt::tags::compound_tag> tag = mc::nbt::tags::compound_tag::read(in);
    if (tag == nullptr) {
        return Error(ErrorCode::InvalidData, "NBT compound tag parse failed", "nbt_io::readCompound");
    }
    const std::streampos after = in.tellg();
    const usize consumed = (after == std::streampos(-1) || before == std::streampos(-1))
        ? buf.readableBytes()
        : static_cast<usize>(after - before);

    // 前进游标到 NBT 实际消耗位置；剩余字节留给后续读取。
    buf.setReadPosition(buf.readPosition() + consumed);
    return tag;
}

Result<void> skipCompound(ByteBuf& buf)
{
    // 复用 readCompound 的定界逻辑推进游标，丢弃解析结果。
    auto tag = readCompound(buf);
    if (tag.failed()) {
        return tag.error();
    }
    return Result<void>::ok();
}

} // namespace mc::network::buffer::nbt_io
