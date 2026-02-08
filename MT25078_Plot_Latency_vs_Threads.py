import matplotlib.pyplot as plt

threads = [1, 2, 4, 8]

latency = {
    32: {
        "A1": [0.26, 0.51, 0.28, 0.51],
        "A2": [0.21, 0.38, 0.25, 0.42],
        "A3": [0.19, 0.31, 0.22, 0.35],
    },
    128: {
        "A1": [1.27, 1.29, 1.28, 1.31],
        "A2": [1.18, 1.22, 1.19, 1.25],
        "A3": [1.33, 1.41, 1.36, 1.39],
    },
    512: {
        "A1": [1.86, 1.86, 2.00, 1.83],
        "A2": [1.78, 1.98, 1.74, 1.83],
        "A3": [3.66, 3.63, 3.95, 3.48],
    },
    2048: {
        "A1": [2.14, 2.32, 2.71, 3.01],
        "A2": [1.98, 2.11, 2.49, 2.78],
        "A3": [3.91, 4.10, 4.44, 4.88],
    }
}

# -------------------------------
# Plot: one figure per message size
# -------------------------------
for msg_size in [32, 128, 512, 2048]:
    plt.figure(figsize=(7, 5))

    plt.plot(threads, latency[msg_size]["A1"], marker='o', label="A1 Two-copy")
    plt.plot(threads, latency[msg_size]["A2"], marker='o', label="A2 One-copy")
    plt.plot(threads, latency[msg_size]["A3"], marker='o', label="A3 Zero-copy")

    plt.xlabel("Thread Count")
    plt.ylabel("Latency (µs)")
    plt.title(f"Latency vs Thread Count (Message Size = {msg_size} bytes)")
    plt.legend()
    plt.grid(True)

    plt.savefig(
        f"MT25078_Latency_vs_Threads_{msg_size}B.png",
        dpi=300,
        bbox_inches="tight"
    )

    plt.show()
