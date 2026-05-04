# NBT (Named Binary Tag) 模块

Minecraft NBT 格式序列化库，已集成到 Minecraft Reborn 项目。

## 目录结构

```
src/common/util/nbt/
├── LICENSE           # MIT 许可证（原作者 Ktlo）
├── Nbt.hpp           # 主头文件 - 公共 API
├── NbtInternal.hpp   # 内部头文件 - 实现细节
├── Nbt.cpp           # 实现文件
└── README.md         # 本文档
```

## 文件说明

### Nbt.hpp

**职责**: 主头文件，定义所有公共 API。

**主要内容**:
- **TagId 枚举**: 定义 12 种 NBT 标签类型
  - `End` (0x00) - 结束标记
  - `Byte` (0x01) - 8 位有符号整数
  - `Short` (0x02) - 16 位有符号整数
  - `Int` (0x03) - 32 位有符号整数
  - `Long` (0x04) - 64 位有符号整数
  - `Float` (0x05) - 单精度浮点
  - `Double` (0x06) - 双精度浮点
  - `ByteArray` (0x07) - 字节数组
  - `String` (0x08) - UTF-8 字符串
  - `List` (0x09) - 同类型元素列表
  - `Compound` (0x0A) - 键值对映射
  - `IntArray` (0x0B) - 整型数组
  - `LongArray` (0x0C) - 长整型数组

- **Context 结构**: 控制序列化格式
  - `Order::BigEndian` / `Order::LittleEndian` - 字节序
  - `Format::Bin` / `Format::Zigzag` / `Format::Mojangson` / `Format::Zint` - 数据格式

- **预定义上下文** (`mc::nbt::Contexts` 命名空间):
  - `java` - Java Edition 格式（大端序二进制）
  - `bedrock_net` - Bedrock 网络格式（小端序 + Zigzag）
  - `bedrock_disk` - Bedrock 磁盘格式（小端序二进制）
  - `kbt` - KBT 格式（大端序 + Zint）
  - `mojangson` - Mojangson 文本格式

- **标签类型** (`mc::nbt::tags` 命名空间):
  - 数值标签: `byte_tag`, `short_tag`, `int_tag`, `long_tag`, `float_tag`, `double_tag`
  - 数组标签: `bytearray_tag`, `intarray_tag`, `longarray_tag`
  - 字符串标签: `string_tag`
  - 列表标签: `list_tag` (基类), `tag_list_tag`, `end_list_tag`, `number_list_tag`, `array_list_tag`, `string_list_tag`, `list_list_tag`, `compound_list_tag`
  - 复合标签: `compound_tag`

- **类型别名** (`mc::nbt` 命名空间):
  - `Tag`, `CompoundTag`, `ListTag`, `StringTag`, `IntTag`, `LongTag`, `DoubleTag`, `FloatTag`, `ByteTag`, `ShortTag`, `ByteArrayTag`, `IntArrayTag`, `LongArrayTag`

- **工具函数**:
  - `load<T>()` / `dump<T>()` - 数值读写
  - `load_array<T>()` / `dump_array<T>()` - 数组读写
  - `cheof()` - 安全字符读取（EOF 抛异常）
  - `reverse()` / `net_order()` / `disk_order()` - 字节序转换

### NbtInternal.hpp

**职责**: 内部实现头文件，包含模板实现细节。

**主要内容**:
- `varnum_max<T>()` - VarNum 最大字节数计算
- `zint_max<T>()` - Zint 最大字节数计算
- `load_varnum<T>()` / `dump_varnum<T>()` - VarNum 编解码模板
- `load_zint<T>()` / `dump_zint<T>()` - Zint 编解码模板

**注意**: 此文件不应被外部直接包含，仅供 `Nbt.cpp` 内部使用。

### Nbt.cpp

**职责**: 实现文件，提供所有函数的具体实现。

**主要内容**:
- Context 存储管理（使用 `ios_base::pword` 机制）
- VarInt/VarLong 实现
- 文本格式数值解析（Mojangson）
- 字符串读写（二进制和文本格式）
- 所有标签类型的 `read()` / `write()` / `copy()` 方法
- 列表标签的读取桥接函数
- `deduce_tag()` - 文本格式标签类型推断
- 复合标签的读写实现
- `std::to_string()` 重载

### LICENSE

**职责**: 原始库许可证文件。

MIT License，原作者 Ktlo (2020)。

## 文件关系

```
┌─────────────────────────────────────────────────────────────┐
│                      用户代码                                │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ #include "util/nbt/Nbt.hpp"
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                        Nbt.hpp                               │
│  - TagId 枚举                                                │
│  - Context 结构                                              │
│  - 预定义上下文 (Contexts::java, etc.)                       │
│  - 所有标签类型声明                                           │
│  - 类型别名 (CompoundTag, ListTag, etc.)                     │
│  - 工具函数声明/模板                                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ #include (仅 Nbt.cpp)
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    NbtInternal.hpp                           │
│  - VarNum/Zint 模板实现                                      │
│  - 内部编码函数                                               │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ #include
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                        Nbt.cpp                               │
│  - 所有函数的具体实现                                         │
│  - 文本格式解析                                               │
│  - 标签类型的 read/write/copy                                │
└─────────────────────────────────────────────────────────────┘
```

## 模块整体职责

本模块提供 Minecraft NBT (Named Binary Tag) 格式的完整序列化支持，包括：

1. **多格式支持**
   - Java Edition 大端序二进制格式（level.dat, player.dat）
   - Bedrock Edition 网络格式（小端序 + VarInt/Zigzag）
   - Bedrock Edition 磁盘格式（小端序二进制）
   - Mojangson 文本格式（命令行和调试输出）

2. **完整的类型系统**
   - 12 种 NBT 标签类型的完整实现
   - 类型安全的读写操作
   - 自动类型推断

3. **流式 API**
   - 使用 `std::iostream` 进行读写
   - 通过上下文切换格式

## 输入和输出

### 输入
- 二进制 NBT 数据（通过 `std::istream`）
- 文本格式 NBT 数据（Mojangson 格式）
- 程序构造的标签对象

### 输出
- 二进制 NBT 数据（通过 `std::ostream`）
- 文本格式 NBT 数据（Mojangson 格式）
- 字符串表示（通过 `std::to_string()`）

## 依赖项

### 外部依赖
- C++20 标准库
- `<iostream>` - 流操作
- `<memory>` - 智能指针
- `<vector>` / `<map>` - 容器
- `<type_traits>` - 类型特征

### 项目内部依赖
- `common/core/Types.hpp` - 基础类型定义（`i8`, `i16`, `i32`, `i64`, `u8`, `u32`, `u64`, `String`）
- `common/util/assert/AssertAll.hpp` - 断言库（仅 Nbt.cpp 使用）

### 被依赖
- `common/world/gen/feature/template/Template.hpp` - 结构模板存储
- `common/world/gen/feature/template/TemplateLoader.hpp` - 结构模板加载

## 使用方法

### 读取 Java Edition NBT 文件

```cpp
#include "util/nbt/Nbt.hpp"
#include <fstream>

using namespace mc::nbt;

// 读取 Java Edition NBT 文件
std::ifstream input("level.dat", std::ios::binary);
input >> contexts::java;  // 设置上下文
auto root = tags::compound_tag::read(input);

// 访问数据
auto& levelName = root->get<tags::string_tag>("LevelName");
auto& gameType = root->get<tags::int_tag>("GameType");
```

### 创建和写入 NBT 数据

```cpp
#include "util/nbt/Nbt.hpp"
#include <fstream>

using namespace mc::nbt;

// 创建 NBT 数据
tags::compound_tag player;
player.put("name", std::string("Steve"));
player.put("level", 100);
player.put("health", 20.0f);

// 添加列表
auto& inventory = player.tag<tags::list_tag>("Inventory");
// ... 添加物品

// 写入文件
std::ofstream output("player.dat", std::ios::binary);
output << contexts::java << player;
```

### Mojangson 文本格式

```cpp
#include "util/nbt/Nbt.hpp"
#include <iostream>

using namespace mc::nbt;

tags::compound_tag tag;
tag.put("name", std::string("Test"));
tag.put("value", 42);
tag.put("items", std::vector<int32_t>{1, 2, 3});

// 输出为 Mojangson 格式
std::cout << contexts::mojangson << tag;
// 输出: {name:"Test",value:42,items:[I;1,2,3]}
```

### 类型映射

| NBT 类型 | C++ 类型 | 标签类 |
|---------|---------|-------|
| Byte | int8_t | `tags::byte_tag` |
| Short | int16_t | `tags::short_tag` |
| Int | int32_t | `tags::int_tag` |
| Long | int64_t | `tags::long_tag` |
| Float | float | `tags::float_tag` |
| Double | double | `tags::double_tag` |
| String | std::string | `tags::string_tag` |
| ByteArray | std::vector<int8_t> | `tags::bytearray_tag` |
| IntArray | std::vector<int32_t> | `tags::intarray_tag` |
| LongArray | std::vector<int64_t> | `tags::longarray_tag` |
| List | - | `tags::list_tag` |
| Compound | std::map | `tags::compound_tag` |

### 项目类型别名

```cpp
namespace mc::nbt {
    using Tag = tags::tag;
    using CompoundTag = tags::compound_tag;
    using ListTag = tags::list_tag;
    using StringTag = tags::string_tag;
    using IntTag = tags::int_tag;
    using LongTag = tags::long_tag;
    using DoubleTag = tags::double_tag;
    using FloatTag = tags::float_tag;
    // ...
}
```

## 容易踩的坑

### 1. 忘记设置上下文

**问题**: 读取或写入时未设置正确的上下文，导致数据解析错误。

```cpp
// 错误：未设置上下文，默认使用 Java 格式
std::ifstream input("bedrock_level.dat", std::ios::binary);
auto root = tags::compound_tag::read(input);  // 可能失败

// 正确：先设置正确的上下文
std::ifstream input("bedrock_level.dat", std::ios::binary);
input >> contexts::bedrock_disk;  // Bedrock 磁盘格式
auto root = tags::compound_tag::read(input);
```

### 2. 列表类型不匹配

**问题**: NBT 列表只能包含相同类型的元素，写入时类型不一致会抛出异常。

```cpp
tags::tag_list_tag list;
list.value.push_back(std::make_unique<tags::int_tag>(1));
list.value.push_back(std::make_unique<tags::string_tag>("error"));  // 运行时错误！
```

### 3. 动态类型访问错误

**问题**: 使用 `get<T>()` 时类型不匹配会抛出 `std::bad_cast`。

```cpp
auto& value = compound.get<tags::int_tag>("someKey");  // 如果实际是 string_tag 会抛异常
```

**建议**: 在不确定类型时，先检查标签 ID：

```cpp
auto it = compound.value.find("someKey");
if (it != compound.value.end()) {
    if (it->second->id() == TagId::Int) {
        auto& value = dynamic_cast<tags::int_tag&>(*it->second).value;
        // 使用 value
    }
}
```

### 4. 空列表的类型推断

**问题**: 空列表在二进制格式中需要指定元素类型。

```cpp
// 空列表写入时会使用 TagId::End
tags::tag_list_tag emptyList;
// 写入二进制时元素类型为 End
```

### 5. 根复合标签的特殊处理

**问题**: 根复合标签在写入时不需要结束标记（`is_root = true`）。

```cpp
tags::compound_tag root(true);  // is_root = true
// 写入时不会添加结束标记
```

### 6. Mojangson 格式的字符串转义

**问题**: Mojangson 格式的字符串中的特殊字符需要转义。

```cpp
// 字符串包含引号时自动转义
tags::string_tag str("He said \"Hello\"");
std::cout << contexts::mojangson << str;
// 输出: "He said \"Hello\""
```

### 7. gzip 压缩

**问题**: Java Edition 的 NBT 文件通常使用 gzip 压缩，需要手动解压。

```cpp
// 需要使用 zlib 或其他库先解压
#include <zlib.h>
// ... 解压后使用 stringstream 包装数据
std::stringstream stream(decompressedData);
stream >> contexts::java;
auto root = tags::compound_tag::read(stream);
```

## 涉及的测试用例

**注意**: 目前项目中尚未发现专门的 NBT 单元测试文件。建议添加以下测试：

### 建议的测试用例

1. **基础类型读写测试**
   - 各数值类型的二进制格式读写
   - 字符串读写（含 UTF-8）
   - 数组读写

2. **上下文切换测试**
   - Java 格式读写
   - Bedrock 网络格式读写
   - Bedrock 磁盘格式读写
   - Mojangson 格式读写

3. **复合结构测试**
   - 嵌套复合标签
   - 嵌套列表
   - 混合嵌套

4. **边界条件测试**
   - 空复合标签
   - 空列表
   - 大数值边界
   - 长字符串

5. **错误处理测试**
   - 无效数据
   - 类型不匹配
   - 过早 EOF
   - 损坏的 NBT 结构

6. **与 TemplateLoader 集成测试**
   - 加载实际的结构模板 NBT 文件
   - 解析方块实体数据
   - 解析 Jigsaw 连接点信息

## 许可证

原始库采用 MIT 许可证，原作者为 Ktlo (2020)。详见 `LICENSE` 文件。

## 参考资料

- [Minecraft Wiki - NBT Format](https://minecraft.wiki/w/NBT_format)
- [Minecraft Wiki - Structure Block File Format](https://minecraft.wiki/w/Structure_Block_File_Format)
- MC 1.16.5 源码中的 NBT 实现
