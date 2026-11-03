import matplotlib.pyplot as plt

# экспериментальные данные
sizes = [100, 1000, 10000]
rtt_ms = [5.5257, 5.1340, 7.1546]
throughput_kbps = [17.67, 190.22, 1364.95]

# настройка стилей графиков
plt.style.use('seaborn-v0_8-whitegrid' if 'seaborn-v0_8-whitegrid' in plt.style.available else 'default')

# график 1: время отклика (rtt)
plt.figure(figsize=(7, 4.5))
plt.plot(sizes, rtt_ms, marker='o', color='#1f77b4', linewidth=2, markersize=6, label='RTT (мс)')
plt.title('Зависимость времени обработки запроса (RTT) от размера текста', fontsize=11, fontweight='bold')
plt.xlabel('Размер сообщения, байт', fontsize=10)
plt.ylabel('Время отклика, мс', fontsize=10)
plt.xscale('log')
plt.grid(True, linestyle='--', alpha=0.6)
for x, y in zip(sizes, rtt_ms):
    plt.annotate(f"{y:.2f} мс", (x, y), textcoords="offset points", xytext=(0, 8), ha='center', fontsize=9)
plt.tight_layout()
plt.savefig('graph_rtt.png', dpi=300)
plt.close()

# график 2: пропускная способность
plt.figure(figsize=(7, 4.5))
plt.plot(sizes, throughput_kbps, marker='s', color='#2ca02c', linewidth=2, markersize=6, label='Пропускная способность')
plt.title('Зависимость пропускной способности от размера текста', fontsize=11, fontweight='bold')
plt.xlabel('Размер сообщения, байт', fontsize=10)
plt.ylabel('Пропускная способность, КБ/с', fontsize=10)
plt.xscale('log')
plt.yscale('log')
plt.grid(True, linestyle='--', alpha=0.6)
for x, y in zip(sizes, throughput_kbps):
    plt.annotate(f"{y:.1f} КБ/с", (x, y), textcoords="offset points", xytext=(0, 8), ha='center', fontsize=9)
plt.tight_layout()
plt.savefig('graph_throughput.png', dpi=300)
plt.close()

print("Графики успешно сохранены: graph_rtt.png и graph_throughput.png")