---
name: mc-update-doc
description: 修改用户指定的文档，使其内容符合`docs\PROJECT_CONVENTIONS.md`中的要求
---

## 任务简介

修改用户指定的文档，使其内容符合`docs\PROJECT_CONVENTIONS.md`中的要求

## 任务详细流程

先阅读：/docs/PROJECT_CONVENTIONS.md

重点关注其中“README.md 使用指南”部分的要求。

原则上，单个 README.md 不应该超过300~500行（巨型目录下的除外，因为巨型目录光是目录结构树就很大了）。对于超限的 README.md，按照相关要求进行缩减。

对于不允许出现的那些内容，无论文档行数是否超限，都必须要全部删除。

不要进行git提交、不要执行编译命令。

注意：你被要求审查的文档可能没有上述这些问题，这种情况下你直接放行即可，你不一定必须修改文档内容。
