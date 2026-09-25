#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""解析 UMDH 堆快照：按调用栈聚合总字节数，输出 Top 分配点与按模块归类汇总。"""
import io, re, sys, collections

ENTRY = re.compile(r'^([0-9A-F]+) bytes \+ ([0-9A-F]+) at ([0-9A-F]+) by BackTrace([0-9A-F]+)\s*$')
HEAP = re.compile(r'^\*- - - - - - - - - - Heap ([0-9A-F]+)')

def parse(path):
    """返回 (entries, stacks)：
    entries: list of (heap, req_bytes, ovh_bytes, addr, bt_id)
    stacks:  bt_id -> list[int] 地址（自栈顶开始）
    """
    entries = []
    stacks = {}
    cur_heap = None
    cur_bt = None
    cur_stack = None
    with io.open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = HEAP.match(line)
            if m:
                cur_heap = m.group(1)
                cur_bt = None
                continue
            m = ENTRY.match(line)
            if m:
                if cur_bt is not None and cur_stack is not None:
                    stacks.setdefault(cur_bt, cur_stack)
                req = int(m.group(1), 16); ovh = int(m.group(2), 16)
                addr = m.group(3); bt = m.group(4)
                entries.append((cur_heap, req, ovh, addr, bt))
                cur_bt = bt
                cur_stack = []
                continue
            s = line.strip()
            if cur_stack is not None and re.fullmatch(r'[0-9A-F]{8,16}', s):
                cur_stack.append(int(s, 16))
        if cur_bt is not None and cur_stack is not None:
            stacks.setdefault(cur_bt, cur_stack)
    return entries, stacks

def main():
    path = sys.argv[1]
    entries, stacks = parse(path)
    print(f"entries: {len(entries)}   stacks: {len(stacks)}")
    total_req = sum(e[1] for e in entries)
    total_ovh = sum(e[2] for e in entries)
    print(f"total requested: {total_req/1048576:.2f} MB   overhead: {total_ovh/1048576:.2f} MB")
    print()

    # 按完整栈聚合
    agg = collections.defaultdict(lambda: [0, 0, 0])  # bt -> [count, req, ovh]
    for heap, req, ovh, addr, bt in entries:
        a = agg[bt]; a[0] += 1; a[1] += req; a[2] += ovh

    rows = sorted(agg.items(), key=lambda kv: -(kv[1][1] + kv[1][2]))
    print(f"=== Top 40 allocation sites by total bytes (req+ovh) ===")
    out = []
    for bt, (n, req, ovh) in rows[:40]:
        out.append((bt, n, req, ovh))
        print(f"  #{bt:>10}  n={n:>6}  {(req+ovh)/1048576:>9.3f} MB  (req {req/1048576:.3f})")

    # 输出栈地址，便于后续符号化
    with io.open(path + '.stacks.txt', 'w', encoding='ascii') as f:
        for bt, n, req, ovh in out:
            f.write(f"### {bt} n={n} bytes={req+ovh}\n")
            for a in stacks.get(bt, []):
                f.write(f"{a:016X}\n")
    print()
    print(f"stacks written to {path}.stacks.txt")

    # 按每层第一个非 ntdll 帧粗分类
    print()
    print("=== 按尺寸档位（requested）聚合 ===")
    buckets = collections.defaultdict(lambda: [0, 0])
    for heap, req, ovh, addr, bt in entries:
        b = (64 if req < 64 else 256 if req < 256 else 1024 if req < 1024 else
             4096 if req < 4096 else 65536 if req < 65536 else
             1048576 if req < 1048576 else 16 * 1048576 if req < 16*1048576 else 1 << 62)
        buckets[b][0] += 1; buckets[b][1] += req + ovh
    for b in sorted(buckets):
        n, t = buckets[b]
        label = f">=16MB" if b == (1<<62) else (f"1-16MB" if b == 16*1048576 else f"64KB-1MB" if b == 1048576 else f"4-64KB" if b == 65536 else f"1-4KB" if b == 4096 else f"256B-1KB" if b == 1024 else f"64-256B" if b == 256 else f"<64B")
        print(f"  {label:>10}  n={n:>7}  {t/1048576:>9.2f} MB")

if __name__ == '__main__':
    main()
