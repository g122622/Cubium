---
name: mc-improve-code-style
description: 提升用户指定文件的代码风格，使其更优雅、复用性更高，并且符合项目规范和最佳实践。
---

## 任务简介

提升用户指定文件的代码风格，使其更优雅、复用性更高，并且符合项目规范和最佳实践。用户指定的文件范围中可能有文件不存在，忽略不存在的文件即可。

## 任务详细流程

先阅读：

/docs/CODE_CONVENTIONS.md
/docs/PROJECT_CONVENTIONS.md

### 重点关注：
1. 代码规范的 #5.5 禁止出现“对齐MC”之类的注释
2. 类的private成员函数全部加上下划线前缀；私有成员变量使用m_前缀
3. 项目中有不少文件使用了硬编码的世界高度边界（Y=0和Y=255 256 127 128 320 等硬编码值），必须改为使用world::命名空间下的常量。特别是以后高度限制可能从0-255放宽到-64-320，必须避免硬编码。
4. 硬编码的区块长宽（可能硬编码为16或者>>4 <<4等），也要替换了
5. 硬编码的圆周率等数学常量
6. 清理过于频繁的防御性编码（参见项目规范）
7. 代码规范的 ## 8. include路径规范
8. 使用int long float double等基本类型时，必须使用项目规范中规定的i8 u8 i16 u16 i32 u32 i64 u64 f32 f64等类型别名，禁止直接使用基本类型
9. 其他违反代码和项目规范的地方
10. 删掉所有spdlog::debug和spdlog::trace的日志输出（它们会影响性能），保留spdlog::info及以上级别的日志输出。
11. 暂时的简化实现、不完整实现、因为未实现等开发进度原因而导致暂时未使用的代码、函数和变量等必须保留TODO注释（注释中要有明文`TODO`，便于全文搜索），如果对应逻辑缺少TODO注释则需要补上TODO注释，以便未来的开发者知道哪里需要完善实现。
12. 若文件中的注释太少，适当补充一些。
13. 缺少mit版权头的文件需要补上。（版权头可以参考项目中其他文件的版权头）
14. 移动构造函数、移动赋值函数、运算符等尽量加上noexcept修饰符，以提升性能。其他能加noexcept修饰符的函数也尽量加上noexcept修饰符，以提升性能。

需注意，你的更改尽量不要改动原有逻辑

最后，必须使用clang-format对你修改的文件进行格式化：

```
clang-format -i src\common\xxx\Foo.cpp
clang-format -i src\common\xxx\Foo.hpp
```

不要提交代码、不要编译。

注意：你被要求审查的文件可能没有上述这些问题，这种情况下你直接放行即可，你不一定必须修改代码。

你只允许修改指定的文件，如果你的修改超出了指定文件的范围（比如修了指定文件的代码，但是这个修改同时影响了其他文件（待修改代码对应的测试文件不算）），此时你必须放弃修改、直接放行，并在相应代码处添加TODO注释，说明这个地方可能需要提升，但由于修改会影响其他文件，所以暂时不修改。

参考：版权头：

```cpp
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

```
