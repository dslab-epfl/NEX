import matplotlib.pyplot as plt
from matplotlib.transforms import offset_copy
import numpy as np
import seaborn as sns
from matplotlib.ticker import FuncFormatter
from matplotlib.font_manager import FontProperties
import matplotlib.font_manager as fm
from compiled_data.jpeg_data import performance_data as nex_jpeg_data_dict
from compiled_data.vta_data import performance_data as nex_vta_data_dict
from compiled_data.protoacc_data import performance_data as nex_protoacc_data_dict
from gem5_compiled_data.jpeg_data import performance_data as gem5_jpeg_data_dict
from gem5_compiled_data.vta_data import performance_data as gem5_vta_data_dict
from gem5_compiled_data.protoacc_data import performance_data as gem5_protoacc_data_dict

xtick_colors = ['#8B004B', '#006400', '#4B0082']

benchmarks = ['JPEG',
              'JPEG-2devices', 
              'JPEG-4devices', 
              'JPEG-8devices', 
              'VTA-4devices', 'VTA-8devices',
              'VTA-matmul', 'VTA-yolo-v3-tiny', 'VTA-resnet18', 'VTA-resnet34', 'VTA-resnet50', 
              'Protoacc-bench0', 'Protoacc-bench1', 'Protoacc-bench2', 'Protoacc-bench3', 'Protoacc-bench4', 'Protoacc-bench5']

real_time = np.array([
    [gem5_jpeg_data_dict['JPEG-Gem5-single-rtl']['real_time'], gem5_jpeg_data_dict['JPEG-Gem5-single-dsim']['real_time'], nex_jpeg_data_dict['JPEG-NEX-single-rtl']['real_time'], nex_jpeg_data_dict['JPEG-NEX-single-dsim']['real_time']],

    [gem5_jpeg_data_dict['JPEG-Gem5-2devices-multi-rtl']['real_time'], gem5_jpeg_data_dict['JPEG-Gem5-2devices-multi-dsim']['real_time'], nex_jpeg_data_dict['JPEG-NEX-2devices-multi-rtl']['real_time'], nex_jpeg_data_dict['JPEG-NEX-2devices-multi-dsim']['real_time']],

    [gem5_jpeg_data_dict['JPEG-Gem5-4devices-multi-rtl']['real_time'], gem5_jpeg_data_dict['JPEG-Gem5-4devices-multi-dsim']['real_time'], nex_jpeg_data_dict['JPEG-NEX-4devices-multi-rtl']['real_time'], nex_jpeg_data_dict['JPEG-NEX-4devices-multi-dsim']['real_time']],

    [gem5_jpeg_data_dict['JPEG-Gem5-8devices-multi-rtl']['real_time'], gem5_jpeg_data_dict['JPEG-Gem5-8devices-multi-dsim']['real_time'], nex_jpeg_data_dict['JPEG-NEX-8devices-multi-rtl']['real_time'], nex_jpeg_data_dict['JPEG-NEX-8devices-multi-dsim']['real_time']],

    [gem5_vta_data_dict['VTA-Gem5-4devices-resnet18_v1-rtl']['real_time'], gem5_vta_data_dict['VTA-Gem5-4devices-resnet18_v1-dsim']['real_time'], nex_vta_data_dict['VTA-NEX-4devices-resnet18_v1-rtl']['real_time'], nex_vta_data_dict['VTA-NEX-4devices-resnet18_v1-dsim']['real_time']],

    [gem5_vta_data_dict['VTA-Gem5-8devices-resnet18_v1-rtl']['real_time'], gem5_vta_data_dict['VTA-Gem5-8devices-resnet18_v1-dsim']['real_time'], nex_vta_data_dict['VTA-NEX-8devices-resnet18_v1-rtl']['real_time'], nex_vta_data_dict['VTA-NEX-8devices-resnet18_v1-dsim']['real_time']],

    [gem5_vta_data_dict['VTA-Gem5-matmul-rtl']['real_time'], gem5_vta_data_dict['VTA-Gem5-matmul-dsim']['real_time'], nex_vta_data_dict['VTA-NEX-matmul-rtl']['real_time'], nex_vta_data_dict['VTA-NEX-matmul-dsim']['real_time']],

    [gem5_vta_data_dict['VTA-Gem5-yolov3-rtl']['real_time'], gem5_vta_data_dict['VTA-Gem5-yolov3-dsim']['real_time'], nex_vta_data_dict['VTA-NEX-yolov3-rtl']['real_time'], nex_vta_data_dict['VTA-NEX-yolov3-dsim']['real_time']],

    [gem5_vta_data_dict['VTA-Gem5-resnet18_v1-rtl']['real_time'], gem5_vta_data_dict['VTA-Gem5-resnet18_v1-dsim']['real_time'], nex_vta_data_dict['VTA-NEX-resnet18_v1-rtl']['real_time'], nex_vta_data_dict['VTA-NEX-resnet18_v1-dsim']['real_time']],

    [gem5_vta_data_dict['VTA-Gem5-resnet34_v1-rtl']['real_time'], gem5_vta_data_dict['VTA-Gem5-resnet34_v1-dsim']['real_time'], nex_vta_data_dict['VTA-NEX-resnet34_v1-rtl']['real_time'], nex_vta_data_dict['VTA-NEX-resnet34_v1-dsim']['real_time']],

    [gem5_vta_data_dict['VTA-Gem5-resnet50_v1-rtl']['real_time'], gem5_vta_data_dict['VTA-Gem5-resnet50_v1-dsim']['real_time'], nex_vta_data_dict['VTA-NEX-resnet50_v1-rtl']['real_time'], nex_vta_data_dict['VTA-NEX-resnet50_v1-dsim']['real_time']],

    [gem5_protoacc_data_dict['Protoacc-Gem5-bench0-rtl']['real_time'], gem5_protoacc_data_dict['Protoacc-Gem5-bench0-dsim']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench0-rtl']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench0-dsim']['real_time']],

    [gem5_protoacc_data_dict['Protoacc-Gem5-bench1-rtl']['real_time'], gem5_protoacc_data_dict['Protoacc-Gem5-bench1-dsim']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench1-rtl']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench1-dsim']['real_time']],

    [gem5_protoacc_data_dict['Protoacc-Gem5-bench2-rtl']['real_time'], gem5_protoacc_data_dict['Protoacc-Gem5-bench2-dsim']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench2-rtl']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench2-dsim']['real_time']],

    [gem5_protoacc_data_dict['Protoacc-Gem5-bench3-rtl']['real_time'], gem5_protoacc_data_dict['Protoacc-Gem5-bench3-dsim']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench3-rtl']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench3-dsim']['real_time']],

    [gem5_protoacc_data_dict['Protoacc-Gem5-bench4-rtl']['real_time'], gem5_protoacc_data_dict['Protoacc-Gem5-bench4-dsim']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench4-rtl']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench4-dsim']['real_time']],

    [gem5_protoacc_data_dict['Protoacc-Gem5-bench5-rtl']['real_time'], gem5_protoacc_data_dict['Protoacc-Gem5-bench5-dsim']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench5-rtl']['real_time'], nex_protoacc_data_dict['Protoacc-NEX-bench5-dsim']['real_time']]
])

sim_data_baseline = np.array([
    gem5_jpeg_data_dict['JPEG-Gem5-single-rtl']['latency'],
    gem5_jpeg_data_dict['JPEG-Gem5-2devices-multi-rtl']['latency'],
    gem5_jpeg_data_dict['JPEG-Gem5-4devices-multi-rtl']['latency'],
    gem5_jpeg_data_dict['JPEG-Gem5-8devices-multi-rtl']['latency'],
    gem5_vta_data_dict['VTA-Gem5-4devices-resnet18_v1-rtl']['latency'],
    gem5_vta_data_dict['VTA-Gem5-8devices-resnet18_v1-rtl']['latency'],
    gem5_vta_data_dict['VTA-Gem5-matmul-rtl']['latency'],
    gem5_vta_data_dict['VTA-Gem5-yolov3-rtl']['latency'],
    gem5_vta_data_dict['VTA-Gem5-resnet18_v1-rtl']['latency'],
    gem5_vta_data_dict['VTA-Gem5-resnet34_v1-rtl']['latency'],
    gem5_vta_data_dict['VTA-Gem5-resnet50_v1-rtl']['latency'],
    gem5_protoacc_data_dict['Protoacc-Gem5-bench0-rtl']['latency'],
    gem5_protoacc_data_dict['Protoacc-Gem5-bench1-rtl']['latency'],
    gem5_protoacc_data_dict['Protoacc-Gem5-bench2-rtl']['latency'],
    gem5_protoacc_data_dict['Protoacc-Gem5-bench3-rtl']['latency'],
    gem5_protoacc_data_dict['Protoacc-Gem5-bench4-rtl']['latency'],
    gem5_protoacc_data_dict['Protoacc-Gem5-bench5-rtl']['latency']
])

sim_error_data_ratial = np.array([
    [1.0, gem5_jpeg_data_dict['JPEG-Gem5-single-dsim']['latency']/sim_data_baseline[0], nex_jpeg_data_dict['JPEG-NEX-single-rtl']['latency']/sim_data_baseline[0], nex_jpeg_data_dict['JPEG-NEX-single-dsim']['latency']/sim_data_baseline[0]],
    [1.0, gem5_jpeg_data_dict['JPEG-Gem5-2devices-multi-dsim']['latency']/sim_data_baseline[1], nex_jpeg_data_dict['JPEG-NEX-2devices-multi-rtl']['latency']/sim_data_baseline[1], nex_jpeg_data_dict['JPEG-NEX-2devices-multi-dsim']['latency']/sim_data_baseline[1]],
    [1.0, gem5_jpeg_data_dict['JPEG-Gem5-4devices-multi-dsim']['latency']/sim_data_baseline[2], nex_jpeg_data_dict['JPEG-NEX-4devices-multi-rtl']['latency']/sim_data_baseline[2], nex_jpeg_data_dict['JPEG-NEX-4devices-multi-dsim']['latency']/sim_data_baseline[2]],
    [1.0, gem5_jpeg_data_dict['JPEG-Gem5-8devices-multi-dsim']['latency']/sim_data_baseline[3], nex_jpeg_data_dict['JPEG-NEX-8devices-multi-rtl']['latency']/sim_data_baseline[3], nex_jpeg_data_dict['JPEG-NEX-8devices-multi-dsim']['latency']/sim_data_baseline[3]],
    [1.0, gem5_vta_data_dict['VTA-Gem5-4devices-resnet18_v1-dsim']['latency']/sim_data_baseline[4], nex_vta_data_dict['VTA-NEX-4devices-resnet18_v1-rtl']['latency']/sim_data_baseline[4], nex_vta_data_dict['VTA-NEX-4devices-resnet18_v1-dsim']['latency']/sim_data_baseline[4]],
    [1.0, gem5_vta_data_dict['VTA-Gem5-8devices-resnet18_v1-dsim']['latency']/sim_data_baseline[5], nex_vta_data_dict['VTA-NEX-8devices-resnet18_v1-rtl']['latency']/sim_data_baseline[5], nex_vta_data_dict['VTA-NEX-8devices-resnet18_v1-dsim']['latency']/sim_data_baseline[5]],
    [1.0, gem5_vta_data_dict['VTA-Gem5-matmul-dsim']['latency']/sim_data_baseline[6], nex_vta_data_dict['VTA-NEX-matmul-rtl']['latency']/sim_data_baseline[6], nex_vta_data_dict['VTA-NEX-matmul-dsim']['latency']/sim_data_baseline[6]],
    [1.0, gem5_vta_data_dict['VTA-Gem5-yolov3-dsim']['latency']/sim_data_baseline[7], nex_vta_data_dict['VTA-NEX-yolov3-rtl']['latency']/sim_data_baseline[7], nex_vta_data_dict['VTA-NEX-yolov3-dsim']['latency']/sim_data_baseline[7]],
    [1.0, gem5_vta_data_dict['VTA-Gem5-resnet18_v1-dsim']['latency']/sim_data_baseline[8], nex_vta_data_dict['VTA-NEX-resnet18_v1-rtl']['latency']/sim_data_baseline[8], nex_vta_data_dict['VTA-NEX-resnet18_v1-dsim']['latency']/sim_data_baseline[8]],
    [1.0, gem5_vta_data_dict['VTA-Gem5-resnet34_v1-dsim']['latency']/sim_data_baseline[9], nex_vta_data_dict['VTA-NEX-resnet34_v1-rtl']['latency']/sim_data_baseline[9], nex_vta_data_dict['VTA-NEX-resnet34_v1-dsim']['latency']/sim_data_baseline[9]],
    [1.0, gem5_vta_data_dict['VTA-Gem5-resnet50_v1-dsim']['latency']/sim_data_baseline[10], nex_vta_data_dict['VTA-NEX-resnet50_v1-rtl']['latency']/sim_data_baseline[10], nex_vta_data_dict['VTA-NEX-resnet50_v1-dsim']['latency']/sim_data_baseline[10]],
    [1.0, gem5_protoacc_data_dict['Protoacc-Gem5-bench0-dsim']['latency']/sim_data_baseline[11], nex_protoacc_data_dict['Protoacc-NEX-bench0-rtl']['latency']/sim_data_baseline[11], nex_protoacc_data_dict['Protoacc-NEX-bench0-dsim']['latency']/sim_data_baseline[11]],
    [1.0, gem5_protoacc_data_dict['Protoacc-Gem5-bench1-dsim']['latency']       /sim_data_baseline[12], nex_protoacc_data_dict['Protoacc-NEX-bench1-rtl']['latency']/sim_data_baseline[12], nex_protoacc_data_dict['Protoacc-NEX-bench1-dsim']['latency']/sim_data_baseline[12]],
    [1.0, gem5_protoacc_data_dict['Protoacc-Gem5-bench2-dsim']['latency']/sim_data_baseline[13], nex_protoacc_data_dict['Protoacc-NEX-bench2-rtl']['latency']/sim_data_baseline[13], nex_protoacc_data_dict['Protoacc-NEX-bench2-dsim']['latency']/sim_data_baseline[13]],
    [1.0, gem5_protoacc_data_dict['Protoacc-Gem5-bench3-dsim']['latency']/sim_data_baseline[14], nex_protoacc_data_dict['Protoacc-NEX-bench3-rtl']['latency']/sim_data_baseline[14], nex_protoacc_data_dict['Protoacc-NEX-bench3-dsim']['latency']/sim_data_baseline[14]],
    [1.0, gem5_protoacc_data_dict['Protoacc-Gem5-bench4-dsim']['latency']/sim_data_baseline[15], nex_protoacc_data_dict['Protoacc-NEX-bench4-rtl']['latency']/sim_data_baseline[15], nex_protoacc_data_dict['Protoacc-NEX-bench4-dsim']['latency']/sim_data_baseline[15]],
    [1.0, gem5_protoacc_data_dict['Protoacc-Gem5-bench5-dsim']['latency']/sim_data_baseline[16], nex_protoacc_data_dict['Protoacc-NEX-bench5-rtl']['latency']/sim_data_baseline[16], nex_protoacc_data_dict['Protoacc-NEX-bench5-dsim']['latency']/sim_data_baseline[16]]
])

sim_error_data = sim_error_data_ratial.copy()
for i in range(sim_error_data.shape[0]):
    for j in range(sim_error_data.shape[1]):
        sim_error_data[i, j] = abs(sim_error_data[i, j] - 1) * 100

print(sim_error_data[:, 1])
print(np.mean(sim_error_data[:, 1]))

speedup_data = real_time[:, 0][:, np.newaxis] / real_time

# Bar width and positions
figsize_x_ratio = 1.3
fig_height_size = 5
bar_padding_height = -4
bar_width = 0.22
bar_dis = 0.22
index = np.arange(len(benchmarks))
lw = 2.5  # linewidth of border of bars
xticklabels_rotation = 15
hatchpattern = ''
title_size = 17
ticklabel_size = 24
ticklabel_size_y = 40
label_size = ticklabel_size_y
tick_size = 13
# barvalue_size = 22 
barvalue_size = 28
legend_label_size = 40

# Custom legend labels
legend_labels = ['gem5+RTL', 'gem5+DSim', 'NEX+RTL', 'NEX+DSim']

# Define custom colors
# colors = sns.color_palette("crest", 16)  # Generates 4 pastel colors
border_color = 'white'  # Set the color of the bar borders
# colors = ['#003f5c', '#7a5195', '#ef5675', '#ffa600']
colors = ['#9dc6e2', '#3dd2ca', '#87ce63', '#ffa600']
colors = ['#3e3a71', '#a7407f', '#f45a5a', '#ffa600']
colors = ['#3e3a71', '#f45a5a', '#2ca02c', '#ffa600']

bar_rotate = 90
arial_narrow = FontProperties(family='Nimbus Sans Narrow')
# Create and save separate plots

alpha_value = 1
# Plot Simulation Time
fig1, ax1 = plt.subplots(figsize=(len(benchmarks)*figsize_x_ratio, fig_height_size))
for i in range(4):
    if i == 0: #gem5 + rtl
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        # ax1.bar_label(bars, padding=bar_padding_height + 4, fontsize=barvalue_size, labels=[f'{int(v)}' for v in bars.datavalues], rotation=25)
        # ax1.bar_label(bars, padding=bar_padding_height, fontsize=barvalue_size, labels=[f'{int(v)}' for v in bars.datavalues])
    elif i == 1:
        import math
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        print(speedup_data[:, i])
        ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{sp:.1f}x' for sp in speedup_data[:, i] ], rotation=bar_rotate, fontproperties=arial_narrow)
    elif i == 2:
        import math
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{sp:.1f}x' for sp in speedup_data[:, i]], rotation=bar_rotate, fontproperties=arial_narrow)
    else:
        import math
        bars = ax1.bar(index + i * bar_dis, real_time[:, i], bar_width, alpha=alpha_value, color=colors[i], edgecolor=border_color, linewidth=lw, label=legend_labels[i])
        ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{int(sp+0.1)}x' for sp in speedup_data[:, i]], rotation=bar_rotate, fontproperties=arial_narrow)
        # ax1.bar_label(bars, padding=bar_padding_height+4, fontsize=barvalue_size, labels=[f'{sp:.1f}x' for sp in speedup_data[:, i]], rotation=90)
        print(speedup_data[:, i])
ax1.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=100, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=1000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=10000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=100000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')

# f"{int(speedup_data[:, i])}x

# ax1.set_title('Simulation Cost', fontsize=title_size)
# ax1.set_xlabel('Applications', fontsize=label_size)
ax1.set_ylabel('Time(s)', fontsize=label_size)
ax1.set_xticks(index + 1.5 * bar_dis)
labels = ax1.set_xticklabels(benchmarks, rotation=xticklabels_rotation, fontweight='bold')
# labels = ax1.set_xticklabels(benchmarks)

labels[0].set_color(xtick_colors[0])
labels[1].set_color(xtick_colors[0])
labels[2].set_color(xtick_colors[0])
labels[3].set_color(xtick_colors[0])
for label in labels[4:11]:
    label.set_color(xtick_colors[1])
for label in labels[11:]:
    label.set_color(xtick_colors[2])

ax1.tick_params(axis='x', labelsize=ticklabel_size)
ax1.tick_params(axis='y', labelsize=ticklabel_size_y)
ax1.set_yscale('log')
ax1.set_xlim(-0.25, 17)
ax1.set_ylim(0, 400000)

for label in ax1.get_xticklabels():
    label.set_ha('right')  # Align tick labels to the right

# ax1.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax1.axhline(y=100, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax1.axhline(y=1000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax.axhline(y=100, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# ax.axhline(y=200, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')

# Ensure the y-axis includes the tick at y=20
yticks = ax1.get_yticks()  # Get existing y-ticks
yticks = [1,10,100,1000,10000, 100000] # Add y=20 if not already present
ax1.set_yticks(yticks)  # Update y-ticks
# fig1.legend(title="", labels=legend_labels, loc='upper right', fontsize=legend_label_size, handlelength=2.5, labelspacing=0.2, borderpad=0.05, bbox_to_anchor=(0.98, 0.9), ncol=2, title_fontsize=14, handletextpad=1)
fig1.legend(labels=legend_labels, loc='upper center', fontsize=legend_label_size, ncol=len(legend_labels), handlelength=1.5, labelspacing=1.5, borderpad=0.15, bbox_to_anchor=(0.52, 1.2))


plt.tight_layout()
# plt.show()
fig1.savefig('simtime_allbars.pdf', bbox_inches='tight')
plt.close(fig1)

bar_width = 0.2
bar_dis = 0.27

fig3, ax3 = plt.subplots(figsize=(len(benchmarks)*figsize_x_ratio, fig_height_size))
for i in range(1,4):
    bars = ax3.bar(index + i * bar_dis, sim_error_data[:, i], bar_width, color=colors[i], edgecolor=border_color, hatch=hatchpattern, linewidth=lw, label=legend_labels[i])
    ax3.bar_label(bars, padding=bar_padding_height+2, fontsize=barvalue_size, labels=[f'{v:.1f}%' for v in bars.datavalues], rotation=90)

ax3.axhline(y=5, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax3.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax3.axhline(y=15, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# Define a formatter function to append '%'
def percent_formatter(x, pos):
    return f"{x:.0f}%"

ax3.set_ylabel('Relative Error (%)', fontsize=label_size)
ax3.set_xticks(index + 2 * bar_dis)
labels = ax3.set_xticklabels(benchmarks, rotation=xticklabels_rotation, fontweight='bold')

labels[0].set_color(xtick_colors[0])
labels[1].set_color(xtick_colors[0])
labels[2].set_color(xtick_colors[0])
labels[3].set_color(xtick_colors[0])
for label in labels[4:11]:
    label.set_color(xtick_colors[1])
for label in labels[11:]:
    label.set_color(xtick_colors[2])

for label in ax3.get_xticklabels():
    label.set_ha('right')  # Align tick labels to the right

ax3.tick_params(axis='x', labelsize=ticklabel_size)
ax3.tick_params(axis='y', labelsize=ticklabel_size_y)
# ax3.set_yscale('log')
ax3.set_ylim(0,25)
ax3.set_xlim(-0.23, 17.2)
yticks = ax3.get_yticks()  # Get existing y-ticks
yticks = [5, 10, 15] # Add y=20 if not already present
ax3.set_yticks(yticks) 
# fig3.legend(title="", labels=legend_labels, loc='upper left', fontsize=legend_label_size, handlelength=1.5, labelspacing=0.2, borderpad=0.2, bbox_to_anchor=(0.06, 0.88), ncol=1, title_fontsize=14, handletextpad=1)
fig3.legend(labels=legend_labels[1:], loc='upper center', fontsize=legend_label_size, ncol=len(legend_labels), handlelength=1.5, labelspacing=1.5, borderpad=0.15, bbox_to_anchor=(0.54, 1.2))

plt.tight_layout()
# plt.show()
fig3.savefig('simerror_allbars.pdf', bbox_inches='tight')  # Save as PNG
plt.close(fig3)
