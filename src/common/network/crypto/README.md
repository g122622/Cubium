# Network Crypto 模块

Java wire 格式的加密 + 压缩。真 Java 在线互通的 RSA 握手 + AES-CFB8 + zlib 压缩；离线模式（我方服务端默认）跳过加密走明文 wire。

## 目录结构

```
src/common/network/crypto/
├── Crypt.hpp/.cpp         # 共享密钥生成（16 字节 AES 密钥 + 随机字节，OpenSSL RAND_bytes）
├── AesCfb8.hpp/.cpp       # AES-128-CFB8 流加解密（OpenSSL EVP，IV=密钥本身，流式状态跨包保持）
├── RsaHandshake.hpp/.cpp  # RSA-1024/PKCS1 登录握手（OpenSSL EVP_PKEY，X509 公钥/PKCS8 私钥）
└── ZlibCodec.hpp/.cpp     # MC 网络压缩/解压（zlib + threshold，VarInt 数据长度前缀格式）
```

## 协议要点（Java 1.21.11，对照 net.minecraft.util.Crypt）

- **AES-CFB8**：`AES/CFB8/NoPadding`，密钥 16 字节（AES-128），IV = 密钥本身。流式（cipher.update 不 doFinal），输入 n 字节产出 n 字节无膨胀。一个 AesCfb8 实例对应一个方向，状态跨包保持。
- **RSA**：1024 位，公钥 X509 SubjectPublicKeyInfo DER，私钥 PKCS8。padding = PKCS1Padding（Java "RSA" 裸算法名在 SunJCE 下等价 RSA/ECB/PKCS1Padding）。1024 位密钥密文固定 128 字节。
- **压缩**：线格式 `VarInt(数据长度) + payload`。数据长度 0=未压缩；>0=解压后长度 + zlib 压缩流。threshold：< 则不压缩（写 0），>= 则压缩；-1 禁用。上限：解压后 8MB、压缩后 2MB。

## pipeline 相对位置

出站 compress→frame→encrypt；入站 decrypt→frame→decompress。本模块是 pipeline 层 handler（CompressionHandlers/CipherHandlers）的底层实现，不直接被 Connection 持有——Connection 经 handler 间接调用。

## 上下游外部依赖关系

- **上游依赖**：`common/core/Result`、OpenSSL（`libcrypto`，RAND/EVP）、zlib（`ZLIB::ZLIB`，deflate/inflate）。
- **下游**：`pipeline/CompressionHandlers`（用 ZlibCodec）、`pipeline/CipherHandlers`（用 AesCfb8）、`backend/java/handshake/JavaLoginHandshaker`（用 RsaHandshake + Crypt）。

## 容易踩的坑

1. **AesCfb8 实例不可每包重建**：CFB8 流式状态跨包保持，每包新建会重置链式反馈。Connection 加密层一个方向持一个实例，生命周期与连接加密层一致。
2. **IV = 密钥本身**：Java 用 `new IvParameterSpec(key.getEncoded())`，OpenSSL EVP_EncryptInit_ex 传同一字节作 key 和 iv。
3. **CFB8 不调 doFinal/final**：EVP_CipherUpdate 即可，设 padding=0；调 final 会引入 padding 错误。
4. **ZlibCodec 的 VarInt 是内联实现**：本模块不依赖 buffer/ByteBuf（避免反向依赖），VarInt 读写自实现，与 ByteBuf::writeVarUInt 同算法。
5. **RSA 加密明文上限**：1024 位 PKCS1 padding 开销 11 字节，明文须 < 128-11=117 字节。共享密钥 16 字节、verify token 4 字节均远小于上限。
6. **OpenSSL 头文件不泄漏**：AesCfb8.hpp 成员用 `void*` 持 EVP_CIPHER_CTX，header 不 include OpenSSL；只有 .cpp include。避免 OpenSSL 头污染上游。
7. **ErrorCode 无 Internal**：OpenSSL 失败用 `ErrorCode::Unknown`（项目 ErrorCode 枚举无 Internal）。
