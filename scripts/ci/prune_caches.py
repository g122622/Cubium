#!/usr/bin/env python3
"""清理 GitHub Actions 缓存：同一前缀只保留最新的若干份。

## 为什么需要

GitHub 对每个仓库的 Actions 缓存总量有 **10 GB** 硬上限，超出后按 LRU 自动驱逐。

本项目每轮 nightly 都会 `actions/cache/save` 一份 ~2 GB 的 ccache（key 形如
`ccache-linux-nightly-<run_id>`，每轮不同），且**从不删除旧的**。三轮下来就是 6 GB，
加上 fuzz 的缓存与 vcpkg 缓存，很快触顶。

触顶的后果不是「缓存变小」这么简单，而是**命中率归零**：被驱逐后新一轮只能回退到更旧的
缓存，而那份缓存的编译选项往往已经变了（例如加上 `-Xclang -fno-pch-timestamp` 之后），
ccache 把「编译选项变了」视为不同的命令，于是所有目标全部 miss —— 构建时间从 20 分钟
劣化到 55 分钟，且看不出任何直接原因。

## 做法

列出仓库全部缓存，筛出 key 以指定前缀开头的，按创建时间倒序，保留最新 N 份，删除其余。

注意**必须在 cache/save 之后运行**：刚保存的那份是最新的，会被保留下来。

## 用法

    python3 scripts/ci/prune_caches.py \
        --repo g122622/Cubium \
        --prefix ccache-linux-nightly- \
        --keep 1

依赖 `gh` CLI（GitHub 托管 runner 预装）与 `GH_TOKEN` 环境变量（需 `actions: write`）。

本脚本**永不以非零码退出**：它跑在构建成功之后，自身失败不应把一次成功的构建染红。
出错时打印告警并放行（缓存清理失败只会让下一轮慢一些，不影响本轮结论）。
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys

# 缓存列表接口：一次最多返回 100 条（GitHub 默认分页大小）。
_PER_PAGE = 100


def _gh_api_list(repo: str) -> list[dict]:
    """拉取仓库的全部 Actions 缓存条目。"""
    gh = shutil.which("gh")
    if gh is None:
        raise RuntimeError("未找到 gh CLI")

    entries: list[dict] = []
    page = 1
    while True:
        proc = subprocess.run(
            [
                gh, "api",
                f"repos/{repo}/actions/caches?per_page={_PER_PAGE}&page={page}",
            ],
            capture_output=True, text=True, timeout=120,
        )
        if proc.returncode != 0:
            raise RuntimeError(f"gh api 失败（page {page}）：{proc.stderr.strip()}")

        payload = json.loads(proc.stdout)
        batch = payload.get("actions_caches", [])
        entries.extend(batch)
        if len(batch) < _PER_PAGE:
            break
        page += 1

    return entries


def _gh_api_delete(repo: str, cache_id: int) -> bool:
    """删除指定缓存。成功返回 True。"""
    gh = shutil.which("gh")
    assert gh is not None
    proc = subprocess.run(
        [gh, "api", "-X", "DELETE", f"repos/{repo}/actions/caches/{cache_id}"],
        capture_output=True, text=True, timeout=120,
    )
    if proc.returncode != 0:
        print(f"[prune] 删除缓存 {cache_id} 失败：{proc.stderr.strip()}", file=sys.stderr)
        return False
    return True


def _fmt_mb(size_bytes: int) -> str:
    return f"{size_bytes / 1_000_000:.0f} MB"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="按前缀清理 Actions 缓存，只保留最新的若干份")
    parser.add_argument("--repo", required=True, help="owner/repo")
    parser.add_argument("--prefix", required=True, help="缓存 key 前缀，如 ccache-linux-nightly-")
    parser.add_argument("--keep", type=int, default=1, help="保留最新的几份（默认 1）")
    parser.add_argument("--dry-run", action="store_true", help="只打印计划，不实际删除")
    args = parser.parse_args(argv)

    try:
        entries = _gh_api_list(args.repo)
    except (RuntimeError, subprocess.SubprocessError, json.JSONDecodeError, OSError) as exc:
        print(f"[prune] 拉取缓存列表失败，跳过清理：{exc}", file=sys.stderr)
        return 0

    matched = [e for e in entries if str(e.get("key", "")).startswith(args.prefix)]
    # 创建时间倒序：最新的在前，保留前 --keep 份。
    matched.sort(key=lambda e: str(e.get("created_at", "")), reverse=True)

    keep = max(0, args.keep)
    survivors = matched[:keep]
    victims = matched[keep:]

    print(f"[prune] 前缀 `{args.prefix}` 共 {len(matched)} 份缓存；保留 {len(survivors)} 份，"
          f"计划删除 {len(victims)} 份")

    freed = 0
    for entry in survivors:
        print(f"[prune]   保留 {entry.get('key')}（{_fmt_mb(int(entry.get('size_in_bytes', 0)))}，"
              f"{entry.get('created_at')}）")

    if args.dry_run:
        for entry in victims:
            print(f"[prune]   [dry-run] 将删除 {entry.get('key')}"
                  f"（{_fmt_mb(int(entry.get('size_in_bytes', 0)))}，{entry.get('created_at')}）")
        return 0

    for entry in victims:
        cache_id = int(entry.get("id", 0))
        if cache_id <= 0:
            continue
        if _gh_api_delete(args.repo, cache_id):
            size = int(entry.get("size_in_bytes", 0))
            freed += size
            print(f"[prune]   已删除 {entry.get('key')}（{_fmt_mb(size)}）")

    print(f"[prune] 共释放约 {_fmt_mb(freed)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
