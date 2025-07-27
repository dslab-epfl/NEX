import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
from matplotlib.ticker import FuncFormatter

xtick_colors = ['#4d498e', '#ba478e', '#fe5e5e']
xtick_colors = ['#8B004B', '#006400', '#4B0082']
#3e3a71, #a7407f, #f45a5a,#ffa600


# Define the benchmarks and the data for simulation error and speedup over baseline
# benchmarks = ['jpeg-decode', 'vta-matmul', 'vta-yolo-v3-tiny', 'vta-resnet18', 'vta-resnet34', 'vta-resnet50', 'protoacc-bench0', 'protoacc-bench1', 'protoacc-bench2', 'protoacc-bench3', 'protoacc-bench4', 'protoacc-bench5']
benchmarks = ['JPEG',
              'JPEG-2devices', 
              'JPEG-4devices', 
              'JPEG-8devices', 
            #   'VTA-resnet18-4devices', 'VTA-resnet18-8devices',
              'VTA-4devices', 'VTA-8devices',
              'VTA-matmul', 'VTA-yolo-v3-tiny', 'VTA-resnet18', 'VTA-resnet34', 'VTA-resnet50', 
              'Protoacc-bench0', 'Protoacc-bench1', 'Protoacc-bench2', 'Protoacc-bench3', 'Protoacc-bench4', 'Protoacc-bench5']

# Simulation error data for each configuration (gem5+RTL, gem5+LPN, native+RTL, native+LPN)
real_time = np.array([
    [10914,147], #JPEG
    [10887,167], #JPEG
    [10869,133], #JPEG
    [11363,122], #JPEG


    # [2477,89.4], #JPEG
    # [1644, 146.0], #JPEG
    # [1402, 136], #JPEG
    # [902, 125], #JPEG
    [24869,40.5], #resent18 4devices
    [33399,38], #resent18 8devices

    [1510,23],  # matmul_opt
    [2983,5.8],  # detection
    [1127,2.6],  # resnet18
    [1256,4.5],   # resnet34
    [6868,25], # resnet50
    [324.5,48], # HP-Bench0
    [155.5,1.8], # HP-Bench1
    [1005,152], # HP-Bench2
    [215,25],  # HP-Bench3
    [248.5,30], # HP-Bench4
    [988,160]  # HP-Bench5
])

sim_error_data = np.array([
    [0.00, 11.51],  #JPEG
    [0.00, 8.34],
    [0.00, 9.70],
    [0.00, 8.54],

    # [0.0,0.5], #JPEG
    # [0.00, 1.58], #JPEG
    # [0.00, 1.77], #JPEG
    # [0.00, 3.02], #JPEG
    [0.00, 4.38],  #resnet18 4devices
    [0.00, 2.00],  #resnet18 8devices
    [0.00,7.25],       # matmul_opt
    [0.00,2.55],        # detection
    [0.00,1.70],        # resnet18
    [0.00, 3.48],      # resnet34
    [0.00,3.30],       #resnet50
    [0.00,6.95],  # HP-Bench0
    [0.00, 1.87],     # HP-Bench1
    [0.00, 13.78],     # HP-Bench2
    [0.00, 13.47],       # HP-Bench3
    [0.00,6.76],         # HP-Bench4
    [0.00, 11.41],  # HP-Bench5
])

print(sim_error_data[:, 1])
print(np.mean(sim_error_data[:, 1]))

print(sim_error_data[:, 1][4:11])
print("mean vta")
print(np.mean(sim_error_data[:, 1][4:11]))
print("max vta")
print(np.max(sim_error_data[:, 1][4:11]))
print("min vta")
print(np.min(sim_error_data[:, 1][4:11]))

# Speedup data
speedup_data = real_time[:, 0][:, np.newaxis] / real_time

print("max speedup ", np.max(speedup_data))
print(speedup_data)
print("min speedup ", np.min(speedup_data))
# Bar width and positions
figsize_x_ratio = 0.5
fig_height_size = 2.5
bar_padding_height = -2
bar_width = 0.25
bar_dis = 0.35
index = np.arange(len(benchmarks))
lw = 0.3  # linewidth of border of bars
xticklabels_rotation = 24
hatchpattern = ''
title_size = 15
label_size = 12
y_label_size = 14
ticklabel_size = label_size
barvalue_size = 17
legend_label_size = 17

# # Common settings
# figsize_x_ratio = 1.2
# fig_height_size = 3
# # bar_width = 0.25
# index = np.arange(len(benchmarks))
# lw = 0.3  # Linewidth of border of bars
# xticklabels_rotation = 0
# legend_labels = ['1 thread', '2 threads', '4 threads', '8 threads', '16 threads']
# # legend_labels = ['1 thread', '4 threads', '16 threads']
# # colors = sns.color_palette("Paired", len(legend_labels))  # Generates pastel colors
# colors = sns.color_palette("crest", len(legend_labels))  # Generates pastel colors
# border_color = 'white'  # Bar border color

# settings = ['Quantum=1us', 'Quantum=500ns', 'Quantum=2us', 'Quantum=4us']
# bar_width = 0.18  # Adjust bar width for five thread levels
# threshold = 40 # threshold for bar labels

left_offset = -bar_dis / 1.5
right_offset = bar_dis / 1.5

# Custom legend labels
legend_labels = ['gem5+RTL', 'NEX+DSim']

# Define custom colors
# colors = sns.color_palette("crest", 2)  # Generates 4 pastel colors
# sns.cubehelix_palette(start=.5, rot=-.75, as_cmap=True)
border_color = 'white'  # Set the color of the bar borders
colors = ['#9dc6e2', '#ffa600']
colors = ['#3e3a71', '#ffa600']
#3e3a71, #a7407f, #f45a5a,#ffa600


# Create and save separate plots

# Plot Simulation Time
fig1, ax1 = plt.subplots(figsize=(len(benchmarks)*figsize_x_ratio, fig_height_size))
for i in range(2):
    if i == 0:  # gem5+RTL (left bar)
        bars = ax1.bar(index + left_offset, real_time[:, i], bar_width,
                       color=colors[i], edgecolor=border_color, linewidth=lw,
                       label=legend_labels[i])
        # ax1.bar_label(bars, padding=bar_padding_height+2, fontsize=barvalue_size,
        #               labels=[f'{int(v)}' for v in bars.datavalues])
        
        # ax1.vlines(index + left_offset, real_time[:, i] * 0.7, real_time[:, i] * 1.05, colors='black', linewidth=0.8,label='_nolegend_')
    else:  # NEX+SplitSim (right bar)
        bars = ax1.bar(index + right_offset, real_time[:, i], bar_width,
                       color=colors[i], edgecolor=border_color, linewidth=lw, hatch='',
                       label=legend_labels[i])
        ax1.bar_label(bars, padding=bar_padding_height+2, fontsize=barvalue_size, labels=[f'{int(sp+0.1)}x' for sp in speedup_data[:, 1]], rotation=90)

        # ax1.vlines(index + right_offset, real_time[:, i] * 0.7, real_time[:, i] * 1.05, colors='black', linewidth=0.8,label='_nolegend_')
    # ax1.vlines(index + i * bar_width, real_time[:, i] * 0.7, real_time[:, i] * 1.15, colors='black', linewidth=0.8)


# ax1.set_title('Simulation Time', fontsize=title_size)
# ax1.set_xlabel('Applications', fontsize=label_size)
ax1.tick_params(axis='x', labelsize=ticklabel_size, pad=+5)
ax1.tick_params(axis='y', labelsize=label_size)
ax1.set_yscale('log')
ax1.set_ylim(0, 100000)
ax1.set_xlim(-0.5, 16.5)
ax1.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=100, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=1000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=10000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax1.axhline(y=100000, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')


ax1.set_ylabel('Time(s)', fontsize=y_label_size)
ax1.set_xticks(index + 0 * bar_width)
labels = ax1.set_xticklabels(benchmarks, rotation=xticklabels_rotation, fontweight='bold')

labels[0].set_color(xtick_colors[0])
labels[1].set_color(xtick_colors[0])
labels[2].set_color(xtick_colors[0])
labels[3].set_color(xtick_colors[0])
for label in labels[4:11]:
    label.set_color(xtick_colors[1])
for label in labels[11:]:
    label.set_color(xtick_colors[2])

for label in ax1.get_xticklabels():
    label.set_ha('right')  # Align tick labels to the right


# for label in ax1.get_xticklabels():
#     label.set_position((label.get_position()[0] - 5, label.get_position()[1]))


# Ensure the y-axis includes the tick at y=20
yticks = ax1.get_yticks()  # Get existing y-ticks
yticks = [1,10,100,1000,10000,100000] # Add y=20 if not already present
ax1.set_yticks(yticks)  # Update y-ticks
# fig1.legend(title="", labels=legend_labels, loc='upper right', fontsize=legend_label_size, handlelength=1.2, labelspacing=0.0, borderpad=0.02, bbox_to_anchor=(0.98, 0.95), ncol=1, title_fontsize=11, handletextpad=0)
fig1.legend(labels=legend_labels, loc='upper center', fontsize=legend_label_size, ncol=len(legend_labels), handlelength=1.5, labelspacing=1.5, borderpad=0.15, bbox_to_anchor=(0.54, 1.1))


plt.tight_layout()
# plt.show()
fig1.savefig('simtime_2bars.pdf', bbox_inches='tight')  # Save as PNG
plt.close(fig1)

# Plot Simulation Error Ratio Over Baseline
fig3, ax3 = plt.subplots(figsize=(len(benchmarks)*figsize_x_ratio, fig_height_size))
i=1
bars = ax3.bar(index + 1.5 * bar_width, sim_error_data[:, i], bar_width, color=colors[i], edgecolor=border_color, hatch=hatchpattern, linewidth=lw, label=legend_labels[i])
ax3.bar_label(bars, padding=bar_padding_height, fontsize=barvalue_size, labels=[f'{v:.1f}' for v in bars.datavalues])

# Define a formatter function to append '%'
def percent_formatter(x, pos):
    return f"{x:.0f}%"

# Set the custom formatter for the y-axis
ax3.yaxis.set_major_formatter(FuncFormatter(percent_formatter))

ax3.tick_params(axis='x', labelsize=ticklabel_size)
ax3.tick_params(axis='y', labelsize=label_size)
# ax3.set_yscale('log')
ax3.set_ylim(0,18)

ax3.set_ylabel('Relative Error (%)', fontsize=label_size)
ax3.set_xticks(index + 1.5 * bar_width)
labels = ax3.set_xticklabels(benchmarks, rotation=xticklabels_rotation, fontweight='bold')

labels[0].set_color(xtick_colors[0])
labels[1].set_color(xtick_colors[0])
labels[2].set_color(xtick_colors[0])
labels[3].set_color(xtick_colors[0])
labels[4].set_color(xtick_colors[1])
labels[5].set_color(xtick_colors[1])
labels[6].set_color(xtick_colors[1])
labels[7].set_color(xtick_colors[1])
labels[8].set_color(xtick_colors[1])
for label in labels[9:]:
    label.set_color(xtick_colors[2])
for label in ax3.get_xticklabels():
    label.set_ha('right')  # Align tick labels to the right


# fig3.legend(title="", labels=legend_labels, loc='upper left', fontsize=legend_label_size, handlelength=1.5, labelspacing=0.2, borderpad=0.2, bbox_to_anchor=(0.06, 0.88), ncol=1, title_fontsize=14, handletextpad=1)

ax3.axhline(y=5, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax3.axhline(y=10, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
ax3.axhline(y=15, color='gray', linestyle='--', linewidth=0.7, alpha=0.5, label='_nolegend_')
# fig3.legend(labels=legend_labels, loc='upper center', fontsize=label_size, ncol=len(legend_labels), handlelength=1.5, labelspacing=1.5, borderpad=0.15, bbox_to_anchor=(0.54, 1.1))

plt.tight_layout()
# plt.show()
fig3.savefig('simerror_2bars.pdf', bbox_inches='tight')  # Save as PNG
plt.close(fig3)

