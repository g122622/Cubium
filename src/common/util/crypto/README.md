# Crypto Module

加密和哈希工具库，提供密码学相关的实用功能。

## 目录结构

```
crypto/
├── Sha256.hpp       # SHA-256 哈希算法头文件
├── Sha256.cpp       # SHA-256 哈希算法实现
├── Md5.hpp          # MD5 哈希算法头文件
├── Md5.cpp          # MD5 哈希算法实现
└── README.md        # 本文档
```

## 模块详解

### Sha256 - SHA-256 哈希算法

符合 FIPS 180-4 标准的 SHA-256 哈希计算器，主要用于 Minecraft 协议中的种子哈希计算。

#### 主要功能

| 方法 | 说明 |
|------|------|
| `hash(std::span<const u8>)` | 计算字节数组的 SHA-256 哈希 |
| `hash(std::string_view)` | 计算字符串的 SHA-256 哈希 |
| `hashUint64(u64)` | 计算 64 位整数的 SHA-256 哈希 |
| `hashWorldSeed(u64)` | 计算世界种子的 hashedSeed |
| `toHexString(const Digest&)` | 将哈希结果转换为十六进制字符串 |
| `bytesToU64LE(std::span<const u8, 8>)` | 小端序字节转 u64 |
| `bytesToU64BE(std::span<const u8, 8>)` | 大端序字节转 u64 |

### Md5 - MD5 哈希算法

符合 RFC 1321 标准的 MD5 哈希计算器，主要用于 Minecraft 离线模式 UUID 生成。

**注意：MD5 不应用于安全敏感的场景，仅用于 UUID 生成等兼容性需求。**

#### 主要功能

| 方法 | 说明 |
|------|------|
| `hash(std::span<const u8>)` | 计算字节数组的 MD5 哈希 |
| `hash(std::string_view)` | 计算字符串的 MD5 哈希 |
| `toHexString(const Digest&)` | 将哈希结果转换为十六进制字符串 |

#### Minecraft 离线 UUID 生成

```cpp
#include "common/util/crypto/Md5.hpp"
#include "common/util/UuidUtils.hpp"

// 生成 Minecraft 离线模式 UUID
std::string input = "OfflinePlayer:Steve";
Md5::Digest md5 = Md5::hash(input);
Uuid uuid = uuidFromMd5(md5);  // 设置版本和变体
```

#### 使用示例

```cpp
#include "common/util/crypto/Sha256.hpp"

using namespace mc::util::crypto;

// 计算世界种子的 hashedSeed（MC 1.16.5 协议）
u64 worldSeed = 12345678901234ULL;
u64 hashedSeed = Sha256::hashWorldSeed(worldSeed);

// 计算字符串的 SHA-256 哈希
Sha256::Digest hash = Sha256::hash("Hello, World!");
std::string hexString = Sha256::toHexString(hash);
// 输出: "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f"

// 计算字节数组的哈希
std::vector<u8> data = {0x01, 0x02, 0x03, 0x04};
Sha256::Digest dataHash = Sha256::hash(std::span<const u8>(data.data(), data.size()));
```

#### Minecraft 协议中的使用

在 Minecraft 1.16.5+ 协议中，`hashedSeed` 是世界种子的 SHA-256 哈希前 8 字节：

```cpp
// 服务端：发送维度切换包时计算 hashedSeed
void ServerDimensionManager::sendDimensionChangePacket(PlayerId playerId, DimensionId newDim, const Vector3d& pos) {
    network::RespawnPacket packet;
    packet.setHashedSeed(Sha256::hashWorldSeed(m_seed));
    // ...
}

// 客户端：接收 hashedSeed 用于验证世界
void onRespawn(u64 hashedSeed) {
    if (hashedSeed != Sha256::hashWorldSeed(expectedSeed)) {
        // 世界种子不匹配
    }
}
```

#### 算法细节

1. **hashWorldSeed** 实现遵循 Guava 的 `Hashing.sha256().hashLong(seed).asLong()`:
   - 将 `u64` 种子以大端序转换为 8 字节
   - 计算 SHA-256 哈希得到 32 字节
   - 取前 8 字节以小端序解释为 `u64` 返回

2. **SHA-256 核心算法**:
   - 消息填充：添加 1 bit、0 bits 和 64 位长度
   - 消息调度：64 个 32 位字的扩展
   - 压缩函数：64 轮处理

## 依赖项

无外部依赖，纯 C++17 实现。

## 测试用例

| 测试文件 | 覆盖内容 |
|---------|---------|
| `tests/common/util/crypto/Sha256Test.cpp` | 标准测试向量、边界情况、种子哈希验证 |

## 参考资料

- [FIPS 180-4](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf) - SHA-256 标准规范
- [Minecraft Wiki - Protocol](https://wiki.vg/Protocol) - Minecraft 协议文档
- [Guava Hashing](https://github.com/google/guava/blob/master/guava/src/com/google/common/hash/Hashing.java) - 参考实现
