import matplotlib.pyplot as plt

msg_sizes = [32, 128, 512, 2048]

A1 = {
    1: [7.046863, 5.845603, 13.577716, 45.475535],
    2: [3.792896, 5.825665, 13.883605, 46.400029],
    4: [6.424756, 5.868090, 12.417493, 45.235865],
    8: [3.755287, 5.750266, 13.615091, 38.977218],
}

A2 = {
    1: [7.886237, 6.924191, 14.363910, 48.138741],
    2: [4.924746, 6.102318, 14.885012, 49.427144],
    4: [5.382669, 5.706808, 13.124024, 41.907091],
    8: [4.821597, 5.889120, 14.298812, 40.772334],
}

A3 = {
    1: [6.719423, 6.118422, 13.721005, 47.998210],
    2: [3.912455, 6.003211, 14.111908, 48.377912],
    4: [1.114606, 3.743623, 7.959239, 28.386267],
    8: [2.943812, 4.811420, 9.442918, 31.022118],
}

for t in [1, 2, 4, 8]:
    plt.figure(figsize=(7,5))
    plt.plot(msg_sizes, A1[t], marker='o', label="A1 Two-copy")
    plt.plot(msg_sizes, A2[t], marker='o', label="A2 One-copy")
    plt.plot(msg_sizes, A3[t], marker='o', label="A3 Zero-copy")

    plt.xlabel("Message Size (bytes)")
    plt.ylabel("Throughput (Gbps)")
    plt.title(f"Throughput vs Message Size (Threads = {t})")
    plt.legend()
    plt.grid(True)

    plt.savefig(f"MT25078_Throughput_MsgSize_T{t}.png", dpi=300, bbox_inches="tight")
    plt.show()
