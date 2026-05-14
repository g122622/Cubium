# 发光效果

本目录包含发光轮廓效果实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `GlowEffect.hpp` | 发光效果头文件 |
| `GlowEffect.cpp` | 发光效果实现 |

## 功能详解

### GlowEffect（发光效果）

用于渲染实体的发光轮廓，如：
- 发光药水效果（Glowing Effect）
- 团队发光颜色

**注意**：发光鱿鱼（Glow Squid）是 MC 1.17+ 添加的实体，MC 1.16.5 中不存在。

**发光效果来源检测**：

```cpp
bool GlowEffect::hasGlowEffect(Entity& entity) {
    // 1. 检查 Entity 的发光标志
    if (entity.isGlowing()) return true;

    // 2. 检查发光药水效果（仅 LivingEntity）
    if (auto* living = dynamic_cast<LivingEntity*>(&entity)) {
        if (living->hasEffect(::mc::entity::effect::EffectType::Glowing)) return true;
    }

    return false;
}
```

**发光效果来源**：
1. `Entity::setGlowing(true)` - 直接设置发光标志
2. 发光药水效果（`EffectType::Glowing`）
3. 团队发光规则（通过 `Entity::getTeam()` 获取队伍颜色）

**发光颜色获取**：

```cpp
math::Vector4f GlowEffect::getGlowColor(Entity& entity) {
    // 1. 检查实体是否在队伍中，使用团队颜色
    scoreboard::Team* team = entity.getTeam();
    if (team != nullptr) {
        text::TextFormatting teamColor = team->getColor();
        u32 argb = text::getFormattingColor(teamColor);
        if (argb != 0xFFFFFFFF && text::isColor(teamColor)) {
            return math::Vector4f(
                static_cast<f32>((argb >> 16) & 0xFF) / 255.0f, // R
                static_cast<f32>((argb >> 8) & 0xFF) / 255.0f,  // G
                static_cast<f32>(argb & 0xFF) / 255.0f,         // B
                1.0f                                              // A
            );
        }
    }

    // 2. 默认白色
    return math::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
}
```

**使用方法**：

```cpp
// 初始化
GlowEffect::initialize();

// 检查实体是否发光
if (GlowEffect::hasGlowEffect(entity)) {
    // 获取发光颜色
    Vector4f color = GlowEffect::getGlowColor(entity);

    // 渲染发光轮廓
    GlowEffect::renderGlow(entity, partialTicks, color);
}

// 渲染所有发光实体
GlowEffect::renderAllGlowing(partialTicks);

// 清理
GlowEffect::cleanup();
```

**发光颜色**：
- 默认：白色 (1, 1, 1, 1)
- 团队成员：团队颜色（通过 `Entity::getTeam()` 获取）

**团队颜色系统**：
- `Entity::getTeam()` - 基类默认返回 nullptr
- `ServerPlayer::getTeam()` - 重写，通过服务器记分板获取玩家所在队伍
- `Team::getColor()` - 获取队伍颜色（TextFormatting 枚举）
- `getFormattingColor()` - 将 TextFormatting 转换为 ARGB 颜色值

**渲染流程**：
1. 渲染实体到发光缓冲区
2. 应用模糊和膨胀效果
3. 将轮廓合成到主画面

**参考**：MC 1.16.5 发光轮廓渲染系统

## 命名空间

```cpp
namespace mc::client::renderer::entity::effect::glow {
    class GlowEffect;
}
```

## 依赖关系

```
GlowEffect.hpp
├── Types.hpp
└── Vector3.hpp

GlowEffect.cpp
├── GlowEffect.hpp
├── Entity.hpp
├── Team.hpp
└── TextStyle.hpp
```

## 测试用例

相关测试位于 `tests/server/scoreboard/GetTeamTest.cpp`：

- `EntityBaseClassGetTeamReturnsNullptr` - Entity 基类 getTeam() 返回 nullptr
- `ServerPlayerGetTeamWithoutServerReturnsNullptr` - ServerPlayer 无服务器时返回 nullptr
- `ScoreboardGetPlayersTeamReturnsTeam` - 记分板正确返回玩家队伍
- `TeamGetColor` - 队伍颜色获取和设置
- `TeamColorToVector4f` - 颜色转换测试
- `MultiplePlayersMultipleTeams` - 多玩家多队伍测试
