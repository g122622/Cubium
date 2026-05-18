import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import Rectangle, FancyBboxPatch
import numpy as np

# 使用新罗马字体
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.serif'] = ['Times New Roman', 'Times', 'DejaVu Serif']
plt.rcParams['mathtext.fontset'] = 'stix'
plt.rcParams['axes.unicode_minus'] = False
plt.rcParams['font.size'] = 14

fig, axes = plt.subplots(1, 2, figsize=(16, 9))

# 颜色定义
colors = {
    'header': '#1565C0',
    'blocks': '#4CAF50',
    'biomes': '#FF9800',
    'skylight': '#FFF59D',
    'blocklight': '#FFCC80',
    'region': '#9E9E9E',
    'chunk': '#64B5F6',
    'section': '#81C784',
}

# ============ 左图：Anvil格式 vs Section粒度存储 ============
ax1 = axes[0]
ax1.set_xlim(0, 16)
ax1.set_ylim(0, 12)
ax1.axis('off')
ax1.set_title('Storage Granularity Comparison', fontsize=20, fontweight='bold', pad=15)

# Anvil格式 (左侧)
ax1.text(4, 11.5, 'Anvil Format (Java Edition)', ha='center', va='center', fontsize=16, fontweight='bold')

# 区域文件
ax1.add_patch(FancyBboxPatch((0.5, 9), 7, 2, boxstyle='round,pad=0.1',
                              facecolor='#E0E0E0', edgecolor='#616161', linewidth=2))
ax1.text(4, 10, 'Region File (.mca)', ha='center', va='center', fontsize=13, fontweight='bold')
ax1.text(4, 9.5, '32x32 chunks = 1024 chunks', ha='center', va='center', fontsize=11, color='#666666')

# 区块 (整个区块作为一个单元)
ax1.add_patch(FancyBboxPatch((1, 6), 6, 2.5, boxstyle='round,pad=0.1',
                              facecolor='#BBDEFB', edgecolor='#1976D2', linewidth=2))
ax1.text(4, 7.25, 'Chunk (16x16x256)', ha='center', va='center', fontsize=13, fontweight='bold')
ax1.text(4, 6.6, 'One unit - update 1 block', ha='center', va='center', fontsize=11, color='#666666')
ax1.text(4, 6.2, '= rewrite entire chunk!', ha='center', va='center', fontsize=11, color='#D32F2F', fontweight='bold')

# 问题列表
problems = [
    'Problem 1: Large granularity',
    'Problem 2: NBT + Zlib overhead',
    'Problem 3: Single thread access',
    'Problem 4: Fragmentation over time'
]
for i, problem in enumerate(problems):
    ax1.text(4, 5 - i * 0.5, problem, ha='center', va='center', fontsize=11, color='#D32F2F')

# Section粒度存储 (右侧)
ax1.text(12, 11.5, 'Section Storage (This Project)', ha='center', va='center', fontsize=16, fontweight='bold')

# RocksDB
ax1.add_patch(FancyBboxPatch((8.5, 9), 7, 2, boxstyle='round,pad=0.1',
                              facecolor='#E8F5E9', edgecolor='#4CAF50', linewidth=2))
ax1.text(12, 10, 'RocksDB (LSM Tree)', ha='center', va='center', fontsize=13, fontweight='bold')
ax1.text(12, 9.5, 'Key-Value Store + ZSTD', ha='center', va='center', fontsize=11, color='#666666')

# Section (细粒度)
ax1.add_patch(FancyBboxPatch((9, 5.5), 6, 3, boxstyle='round,pad=0.1',
                              facecolor='#C8E6C9', edgecolor='#388E3C', linewidth=2))
ax1.text(12, 8, 'Section (16x16x16)', ha='center', va='center', fontsize=13, fontweight='bold')
ax1.text(12, 7.3, '16 sections per chunk', ha='center', va='center', fontsize=11, color='#666666')
ax1.text(12, 6.8, 'Update 1 block = rewrite', ha='center', va='center', fontsize=11, color='#388E3C')
ax1.text(12, 6.3, 'only 1 section!', ha='center', va='center', fontsize=11, color='#388E3C', fontweight='bold')

# 优势列表
advantages = [
    'Advantage 1: Fine granularity',
    'Advantage 2: Binary format (no NBT)',
    'Advantage 3: Multi-thread safe',
    'Advantage 4: Built-in compression'
]
for i, adv in enumerate(advantages):
    ax1.text(12, 5 - i * 0.5, adv, ha='center', va='center', fontsize=11, color='#2E7D32')

# ============ 右图：Section数据格式 ============
ax2 = axes[1]
ax2.set_xlim(0, 16)
ax2.set_ylim(0, 12)
ax2.axis('off')
ax2.set_title('Section Data Format', fontsize=20, fontweight='bold', pad=15)

# SectionKey
ax2.add_patch(FancyBboxPatch((1, 10), 14, 1.5, boxstyle='round,pad=0.1',
                              facecolor='#E3F2FD', edgecolor='#1976D2', linewidth=2))
ax2.text(8, 11, 'SectionKey (13 bytes)', ha='center', va='center', fontsize=14, fontweight='bold')

# Key字段
key_fields = [
    (1, 2, 'Dim\n2B', '#90CAF9'),
    (3, 5, 'ChunkX\n4B', '#64B5F6'),
    (8, 5, 'ChunkZ\n4B', '#42A5F5'),
    (13, 1.5, 'SecY\n1B', '#2196F3'),
    (14.5, 0.5, 'Rsv\n0.5B', '#1976D2'),
]
for x, w, label, color in key_fields:
    rect = Rectangle((x, 10.1), w, 1.3, facecolor=color, edgecolor='black', linewidth=1)
    ax2.add_patch(rect)
    ax2.text(x + w/2, 10.75, label, ha='center', va='center', fontsize=9, fontweight='bold')

# SectionData
ax2.add_patch(FancyBboxPatch((1, 0.5), 14, 9, boxstyle='round,pad=0.1',
                              facecolor='#FFF8E1', edgecolor='#F57C00', linewidth=2))
ax2.text(8, 9, 'SectionData', ha='center', va='center', fontsize=14, fontweight='bold')

# Header
ax2.add_patch(Rectangle((1.5, 7.5), 13, 1.2, facecolor='#1565C0', edgecolor='black', linewidth=1.5))
ax2.text(8, 8.1, 'Header (12 bytes)', ha='center', va='center', fontsize=12, fontweight='bold', color='white')
ax2.text(8, 7.7, 'version | flags | nonEmptyCount | hash', ha='center', va='center', fontsize=10, color='#BBDEFB')

# BlockStates
ax2.add_patch(Rectangle((1.5, 5.5), 13, 1.5, facecolor='#4CAF50', edgecolor='black', linewidth=1.5))
ax2.text(8, 6.4, 'BlockStates (variable)', ha='center', va='center', fontsize=12, fontweight='bold', color='white')
ax2.text(8, 5.9, '4096 x u32 block IDs, ZSTD compressed', ha='center', va='center', fontsize=10, color='#C8E6C9')

# Biomes
ax2.add_patch(Rectangle((1.5, 3.8), 13, 1.2, facecolor='#FF9800', edgecolor='black', linewidth=1.5))
ax2.text(8, 4.5, 'Biomes (128 bytes)', ha='center', va='center', fontsize=12, fontweight='bold', color='white')
ax2.text(8, 4.0, '64 x u16 biome IDs (4x4x4 sampling)', ha='center', va='center', fontsize=10, color='#FFF3E0')

# SkyLight (optional)
ax2.add_patch(Rectangle((1.5, 2.1), 6, 1.2, facecolor='#FFF59D', edgecolor='black', linewidth=1.5))
ax2.text(4.5, 2.8, 'SkyLight (optional)', ha='center', va='center', fontsize=11, fontweight='bold')
ax2.text(4.5, 2.3, '2048 bytes', ha='center', va='center', fontsize=10, color='#666666')

# BlockLight (optional)
ax2.add_patch(Rectangle((8.5, 2.1), 6, 1.2, facecolor='#FFCC80', edgecolor='black', linewidth=1.5))
ax2.text(11.5, 2.8, 'BlockLight (optional)', ha='center', va='center', fontsize=11, fontweight='bold')
ax2.text(11.5, 2.3, '2048 bytes', ha='center', va='center', fontsize=10, color='#666666')

# 优化说明
ax2.text(8, 1.2, 'Optimization: Empty sections store only header (12 bytes)',
         ha='center', va='center', fontsize=11, color='#D32F2F', fontweight='bold')

plt.tight_layout()
plt.savefig('/Users/a0000/dev/minecraft-reborn/tech_report/images/chunk_storage.png',
            dpi=150, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()

print("Chunk storage diagram saved to images/chunk_storage.png")
