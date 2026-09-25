#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 cdb 的 ln 解析结果合并回 UMDH 的 Top 分配点，输出可读的分配点报告。"""
import io, re, sys

def load_syms(path):
    """===ADDR=== 后紧跟 ln 输出行（可能多行），取第一个含 `|` 的符号描述行。"""
    syms = {}
    cur = None
    with io.open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            m = re.match(r'^===([0-9A-F]{16})===$', line.strip())
            if m:
                cur = m.group(1)
                continue
            if cur is None:
                continue
            s = line.strip()
            # 形如: (00007ff7`b95b1000)   minecraft_server!mc::foo+0x12   |  (E:\...\Foo.cpp:123)
            mm = re.match(r'^\(([0-9a-f`]+)\)\s+(\S+)\s+\|\s+(.*)$', s)
            if mm:
                syms[cur] = (mm.group(2), mm.group(3).strip())
                cur = None
    return syms

def main():
    syms = load_syms(sys.argv[1])
    stacks = io.open(sys.argv[2], encoding='ascii').read()
    blocks = re.split(r'^### ', stacks, flags=re.M)[1:]
    print(f"resolved symbols: {len(syms)}   blocks: {len(blocks)}")
    print()
    for b in blocks:
        head, *rest = b.splitlines()
        parts = head.split()
        bt = parts[0]
        n = parts[1].split('=')[1]
        nbytes = int(parts[2].split('=')[1])
        print(f"### {bt}  n={n}  total={nbytes/1048576:.3f} MB")
        for l in rest:
            l = l.strip()
            if not re.fullmatch(r'[0-9A-F]{16}', l):
                continue
            s = syms.get(l)
            if s:
                print(f"    {s[0]:<95} {s[1][:90]}")
            else:
                print(f"    {l}  (unresolved)")
        print()

if __name__ == '__main__':
    main()
