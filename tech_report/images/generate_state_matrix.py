import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# 使用新罗马字体
plt.rcParams['font.family'] = 'serif'
plt.rcParams['font.serif'] = ['Times New Roman', 'Times', 'DejaVu Serif']
plt.rcParams['mathtext.fontset'] = 'stix'
plt.rcParams['axes.unicode_minus'] = False

# 创建图形
fig, ax = plt.subplots(figsize=(12, 6))

# 定义状态矩阵
# facing: [NORTH, SOUTH, EAST, WEST] = [0, 1, 2, 3]
# lit: [false, true] = [0, 1]

facing_labels = ['NORTH', 'SOUTH', 'EAST', 'WEST']
lit_labels = ['false', 'true']

# 状态索引矩阵
# stateIndex = facing_index * 1 + lit_index * 4
state_matrix = np.array([
    [0, 1, 2, 3],   # lit=false
    [4, 5, 6, 7]    # lit=true
])

# 定义颜色
colors = [
    ['#E8F5E9', '#C8E6C9', '#A5D6A7', '#81C784'],  # lit=false: 绿色系
    ['#FFEBEE', '#FFCDD2', '#EF9A9A', '#E57373']   # lit=true: 红色系
]

# 绘制单元格
for i in range(2):
    for j in range(4):
        rect = mpatches.Rectangle(
            (j, 1-i), 1, 1,
            facecolor=colors[i][j],
            edgecolor='#333333',
            linewidth=2
        )
        ax.add_patch(rect)

        # 状态索引（字体调大）
        ax.text(j + 0.5, 1-i + 0.65, f'Index {state_matrix[i][j]}',
                ha='center', va='center', fontsize=20, fontweight='bold',
                color='#1a1a1a')

        # 状态描述（字体调大）
        ax.text(j + 0.5, 1-i + 0.35, f'facing={facing_labels[j]}',
                ha='center', va='center', fontsize=15, color='#555555')
        ax.text(j + 0.5, 1-i + 0.15, f'lit={lit_labels[i]}',
                ha='center', va='center', fontsize=15, color='#555555')

# 设置坐标轴
ax.set_xlim(-0.5, 4.5)
ax.set_ylim(-0.5, 2.5)

# 行标签（字体调大）
ax.text(-0.3, 1.5, 'lit=false', ha='right', va='center', fontsize=17, fontweight='bold', color='#2E7D32')
ax.text(-0.3, 0.5, 'lit=true', ha='right', va='center', fontsize=17, fontweight='bold', color='#C62828')

# 列标签（字体调大）
for j, label in enumerate(facing_labels):
    ax.text(j + 0.5, 2.15, f'facing={label}', ha='center', va='bottom', fontsize=16, fontweight='bold')

# 绘制 stride 说明（字体调大）
# facing 的 stride = 1
ax.annotate('', xy=(1, -0.3), xytext=(0, -0.3),
            arrowprops=dict(arrowstyle='<->', color='#1976D2', lw=2))
ax.text(0.5, -0.45, 'facing stride = 1', ha='center', va='top', fontsize=15, color='#1976D2')

# lit 的 stride = 4
ax.annotate('', xy=(4.3, 0.5), xytext=(4.3, 1.5),
            arrowprops=dict(arrowstyle='<->', color='#F57C00', lw=2))
ax.text(4.45, 1.0, 'lit\nstride = 4', ha='left', va='center', fontsize=15, color='#F57C00')

# 示例箭头：从索引0切换到索引2 (facing NORTH->EAST, lit保持false)
ax.annotate('', xy=(2.5, 1.5), xytext=(0.5, 1.5),
            arrowprops=dict(arrowstyle='->', color='#7B1FA2', lw=2.5,
                           connectionstyle='arc3,rad=0.3'))
ax.text(1.5, 1.85, 'with(FACING, EAST)', ha='center', va='bottom',
        fontsize=15, color='#7B1FA2', fontweight='bold')

# 隐藏坐标轴
ax.axis('off')

# 添加标题（字体调大）
ax.set_title('Block State Space Mapped to 1D Array (Furnace Example)\n'
             'facing: 4 values | lit: 2 values -> 8 states total',
             fontsize=18, fontweight='bold', pad=20)

# 图例（字体调大）
legend_elements = [
    mpatches.Patch(facecolor='#A5D6A7', edgecolor='#333', label='lit=false (unlit)'),
    mpatches.Patch(facecolor='#EF9A9A', edgecolor='#333', label='lit=true (lit)')
]
ax.legend(handles=legend_elements, loc='upper right', fontsize=14)

plt.tight_layout()
plt.savefig('/Users/a0000/dev/minecraft-reborn/tech_report/images/state_matrix.png',
            dpi=150, bbox_inches='tight', facecolor='white', edgecolor='none')
plt.close()

print("State matrix diagram saved to tech_report/images/state_matrix.png")
