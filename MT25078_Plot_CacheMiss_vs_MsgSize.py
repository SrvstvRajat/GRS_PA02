import matplotlib.pyplot as plt

msg_sizes = [32, 128, 512, 2048]

A1 = {
    1: [11040, 10509, 19872, 28411],
    2: [5512, 10398, 18934, 26321],
    4: [54742, 9535, 8088, 11078],
    8: [5158, 11021, 17791, 23144],
}

A2 = {
    1: [9042, 8891, 15677, 21011],
    2: [8121, 9001, 14932, 20321],
    4: [17071, 6526, 7068, 59248],
    8: [16098, 7211, 8122, 57109],
}

A3 = {
    1: [5121, 6011, 7998, 12044],
    2: [5902, 6433, 8109, 13011],
    4: [18441, 198923, 7374, 42773],
    8: [17321, 184441, 9012, 40112],
}

for t in [1, 2, 4, 8]:
    plt.figure(figsize=(7,5))
    plt.plot(msg_sizes, A1[t], marker='o', label="A1 Two-copy")
    plt.plot(msg_sizes, A2[t], marker='o', label="A2 One-copy")
    plt.plot(msg_sizes, A3[t], marker='o', label="A3 Zero-copy")

    plt.xlabel("Message Size (bytes)")
    plt.ylabel("LLC Cache Misses")
    plt.title(f"LLC Cache Misses vs Message Size (Threads = {t})")
    plt.legend()
    plt.grid(True)

    plt.savefig(f"MT25078_CacheMiss_MsgSize_T{t}.png", dpi=300, bbox_inches="tight")
    plt.show()
