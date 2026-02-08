# MT25078

# Network I/O Performance Analysis - PA02

**Author:** Rajat Srivastava (MT25078)  
**Course:** CSE638 - Graduate Systems  
**GitHub:** https://github.com/SrvstvRajat/GRS_PA02

---

## Overview

This project analyzes the performance of three network I/O data transfer mechanisms:

- **A1:** Two-copy communication using `send()`/`recv()`
- **A2:** One-copy communication using `sendmsg()` with scatter-gather I/O
- **A3:** Zero-copy communication using `MSG_ZEROCOPY`

Experiments measure throughput, latency, CPU cycles, and cache behavior across different message sizes (256B to 16KB) and thread counts (1 to 8).

---

## Requirements

- **OS:** Linux (Ubuntu 20.04+)
- **Tools:** gcc, make, perf, python3, matplotlib
- **Privileges:** Root access (for network namespaces)

**Install dependencies:**

```bash
sudo apt-get install build-essential linux-tools-common python3-matplotlib
```

---

## Project Structure

```
GRS_PA02/
├── MT25078_Part_A1_Client.c          # Two-copy client
├── MT25078_Part_A1_Server.c          # Two-copy server
├── MT25078_Part_A2_Client.c          # One-copy client
├── MT25078_Part_A2_Server.c          # One-copy server
├── MT25078_Part_A3_Client.c          # Zero-copy client
├── MT25078_Part_A3_Server.c          # Zero-copy server
├── MT25078_Common.c/h                # Common utilities
├── MT25078_RunExperiments.sh         # Automated experiment script
├── Makefile                           # Build configuration
├── MT25078_Plot_*.py                 # Visualization scripts
├── MT25078_Results_A*.csv            # Experimental results
└── README.md                          # This file
```

---

## Building

```bash
# Build all implementations
make

# Clean build
make clean
```

Creates executables: `A1_server`, `A1_client`, `A2_server`, `A2_client`, `A3_server`, `A3_client`

---

## Running Experiments

### Automated (Recommended)

```bash
chmod +x MT25078_RunExperiments.sh
sudo ./MT25078_RunExperiments.sh
```

This script:

- Creates network namespaces
- Runs 48 experiments (3 implementations × 4 sizes × 4 thread counts)
- Collects perf statistics
- Saves results to CSV files
- Runtime: ~10-15 minutes

### Manual Execution

**Start server:**

```bash
./A1_server <port> <field_size> <max_threads>
# Example: ./A1_server 9091 512 4
```

**Start client:**

```bash
./A1_client <server_ip> <port> <field_size> <duration_sec>
# Example: ./A1_client 127.0.0.1 9091 512 10
```

---

## Results

### Output Files

- `MT25078_Results_A1.csv` - Two-copy results
- `MT25078_Results_A2.csv` - One-copy results
- `MT25078_Results_A3.csv` - Zero-copy results

### Generate Plots

```bash
python3 MT25078_Plot_Latency_vs_Threads.py
python3 MT25078_Plot_Throughput_vs_MsgSize.py
python3 MT25078_Plot_CacheMiss_vs_MsgSize.py
python3 MT25078_Plot_CyclesPerByte.py
```

---

## Key Findings

1. **Message Size Impact:**

   - Small messages (<512B): Two-copy performs competitively
   - Medium messages (512B-4KB): One-copy shows improvement
   - Large messages (>4KB): Zero-copy provides best throughput

2. **Thread Scaling:**

   - 1-2 threads: Good scaling
   - 4+ threads: Diminishing returns

3. **CPU Efficiency:**

   - CPU cycles per byte decrease dramatically with larger messages
   - Zero-copy best for large transfers

4. **Cache Behavior:**
   - Two-copy has highest LLC misses
   - Zero-copy shows irregular patterns at high thread counts

### Performance Recommendations

- **Use Two-copy:** Small messages, simplicity needed
- **Use One-copy:** Medium messages (512B-4KB)
- **Use Zero-copy:** Large messages (≥4KB), max throughput critical

---

## Implementation Details

### A1: Two-Copy

```
User Buffer → Kernel Buffer → NIC
     ↑              ↑
   Copy 1        Copy 2
```

### A2: One-Copy

```
Multiple User Buffers → [sendmsg with iovec] → NIC
                             Single copy
```

### A3: Zero-Copy

```
User Buffer (pinned) → [Direct DMA] → NIC
                       Zero copies
```

---

## Troubleshooting

**Permission errors:**

```bash
sudo sysctl -w kernel.perf_event_paranoid=0
```

**Namespace cleanup:**

```bash
sudo ip netns delete server_ns
sudo ip netns delete client_ns
```

**Check kernel version:**

```bash
uname -r  # Should be 4.14+ for MSG_ZEROCOPY
```

---

## Contact

**Rajat Srivastava (MT25078)**  
GitHub: https://github.com/SrvstvRajat/GRS_PA02
