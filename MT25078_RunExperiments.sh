#!/bin/bash
# MT25078
set -e

echo "=== PA02 Network I/O Experiment with Network Namespaces ==="
echo "Student: MT25078"
echo ""

# Check for root privileges (required for namespaces)
if [ "$EUID" -ne 0 ]; then 
    echo "ERROR: This script must be run as root (sudo) for network namespace operations"
    echo "Usage: sudo ./MT25078_RunExperiments.sh"
    exit 1
fi

# Set perf_event_paranoid to allow perf measurements
echo "=== Fix: Setting perf_event_paranoid ==="
CURRENT_PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid)
echo "Current perf_event_paranoid: $CURRENT_PARANOID"

if [ "$CURRENT_PARANOID" -gt 0 ]; then
    echo "Setting perf_event_paranoid to 0..."
    sysctl -w kernel.perf_event_paranoid=0
    echo "✓ perf_event_paranoid set to 0"
else
    echo "✓ perf_event_paranoid already acceptable ($CURRENT_PARANOID)"
fi
echo ""

# Namespace names
NS_SERVER="server_ns"
NS_CLIENT="client_ns"

# Network configuration
VETH_SERVER="veth_server"
VETH_CLIENT="veth_client"
SERVER_IP="10.200.1.1"
CLIENT_IP="10.200.1.2"
SUBNET="24"
PORT=9091

# Test parameters
FIELD_SIZES=(32 128 512 2048)
THREAD_COUNTS=(1 2 4 8)
DURATION=10

echo "=== Step 1: Cleanup any existing namespaces ==="
ip netns delete $NS_SERVER 2>/dev/null || true
ip netns delete $NS_CLIENT 2>/dev/null || true
ip link delete $VETH_SERVER 2>/dev/null || true
sleep 1
echo "Cleanup complete"
echo ""

echo "=== Step 2: Create network namespaces ==="
ip netns add $NS_SERVER
ip netns add $NS_CLIENT
echo "Created namespaces: $NS_SERVER and $NS_CLIENT"
echo ""

echo "=== Step 3: Create veth pair ==="
ip link add $VETH_SERVER type veth peer name $VETH_CLIENT
echo "Created veth pair: $VETH_SERVER <-> $VETH_CLIENT"
echo ""

echo "=== Step 4: Move veth interfaces to namespaces ==="
ip link set $VETH_SERVER netns $NS_SERVER
ip link set $VETH_CLIENT netns $NS_CLIENT
echo "Moved interfaces to respective namespaces"
echo ""

echo "=== Step 5: Configure IP addresses ==="
ip netns exec $NS_SERVER ip addr add ${SERVER_IP}/${SUBNET} dev $VETH_SERVER
ip netns exec $NS_CLIENT ip addr add ${CLIENT_IP}/${SUBNET} dev $VETH_CLIENT
echo "Server IP: $SERVER_IP"
echo "Client IP: $CLIENT_IP"
echo ""

echo "=== Step 6: Bring up interfaces ==="
ip netns exec $NS_SERVER ip link set $VETH_SERVER up
ip netns exec $NS_CLIENT ip link set $VETH_CLIENT up
ip netns exec $NS_SERVER ip link set lo up
ip netns exec $NS_CLIENT ip link set lo up
echo "Interfaces are up"
echo ""

echo "=== Step 7: Test connectivity ==="
if ip netns exec $NS_CLIENT ping -c 2 $SERVER_IP > /dev/null 2>&1; then
    echo "✓ Connectivity test PASSED: Client can reach server"
else
    echo "✗ Connectivity test FAILED"
    exit 1
fi
echo ""

# Check if perf is available
PERF_AVAILABLE=false
if command -v perf &>/dev/null; then
    if perf stat -e cycles true &>/dev/null 2>&1; then
        PERF_AVAILABLE=true
        echo "✓ perf is available"
    else
        echo "✗ perf requires privileges"
    fi
else
    echo "✗ perf not found"
fi
echo ""

# Detect hybrid CPU and use correct event format
HYBRID_CPU=false
if perf list 2>&1 | grep -q "cpu_core\|cpu_atom"; then
    HYBRID_CPU=true
    echo "✓ Hybrid CPU detected (Intel 12th/13th gen)"
fi

# Configure perf events based on CPU type
PERF_EVENTS=""
if [ "$PERF_AVAILABLE" = true ]; then
    if [ "$HYBRID_CPU" = true ]; then
        echo "Using HYBRID CPU event format"
        PERF_EVENTS="cpu_core/cycles/,cpu_atom/cycles/,context-switches"
        PERF_EVENTS="${PERF_EVENTS},cpu_core/L1-dcache-load-misses/,cpu_atom/L1-dcache-load-misses/"
        PERF_EVENTS="${PERF_EVENTS},cpu_core/LLC-load-misses/,cpu_atom/LLC-load-misses/"
    else
        echo "Using STANDARD CPU event format"
        PERF_EVENTS="cycles,context-switches"
        if perf stat -e L1-dcache-load-misses true &>/dev/null 2>&1; then
            PERF_EVENTS="${PERF_EVENTS},L1-dcache-load-misses"
        fi
        if perf stat -e LLC-load-misses true &>/dev/null 2>&1; then
            PERF_EVENTS="${PERF_EVENTS},LLC-load-misses"
        fi
        if ! echo "$PERF_EVENTS" | grep -q "L1-dcache\|LLC-load"; then
            if perf stat -e cache-misses true &>/dev/null 2>&1; then
                PERF_EVENTS="${PERF_EVENTS},cache-misses"
            fi
        fi
    fi
    echo "Perf events: $PERF_EVENTS"
fi
echo ""

echo "=== Step 8: Build executables ==="
if [ ! -f "Makefile" ]; then
    echo "ERROR: Makefile not found"
    exit 1
fi
make clean
make
if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed"
    exit 1
fi
echo "Build successful"
echo ""

# Create CSV files with headers
echo "impl,field_size,threads,cycles,L1_misses,LLC_misses,context_switches,throughput_gbps,latency_us" > MT25078_Results_A1.csv
echo "impl,field_size,threads,cycles,L1_misses,LLC_misses,context_switches,throughput_gbps,latency_us" > MT25078_Results_A2.csv
echo "impl,field_size,threads,cycles,L1_misses,LLC_misses,context_switches,throughput_gbps,latency_us" > MT25078_Results_A3.csv

# Updated parsing functions for hybrid CPU
parse_perf_hybrid() {
    local perf_file=$1
    local metric_name=$2
    
    # For hybrid CPUs, sum cpu_core and cpu_atom values
    local core_val=$(grep "cpu_core/${metric_name}" "$perf_file" 2>/dev/null | grep -v "#" | awk '{print $1}' | tr -d ',' | head -1)
    local atom_val=$(grep "cpu_atom/${metric_name}" "$perf_file" 2>/dev/null | grep -v "#" | awk '{print $1}' | tr -d ',' | head -1)
    
    # Handle "<not supported>" and "<not counted>" cases
    [[ -z "$core_val" ]] || [[ "$core_val" == "<not"* ]] && core_val="0"
    [[ -z "$atom_val" ]] || [[ "$atom_val" == "<not"* ]] && atom_val="0"
    
    local total=$((core_val + atom_val))
    
    echo "$total"
}

parse_perf_standard() {
    local perf_file=$1
    local metric_name=$2
    
    local val=$(grep "$metric_name" "$perf_file" 2>/dev/null | grep -v "#" | head -1 | awk '{print $1}' | tr -d ',')
    
    if [[ -z "$val" ]] || [[ "$val" == "<not"* ]]; then
        echo "0"
    else
        echo "$val"
    fi
}

# Function to run experiment
run_experiment() {
    local impl=$1
    local field_size=$2
    local threads=$3
    local csv_file=$4
    
    echo ""
    echo ">>> Running: Implementation=$impl, Field_Size=$field_size, Threads=$threads"
    
    # Kill any existing instances in namespaces
    ip netns exec $NS_SERVER pkill -9 ${impl}_server 2>/dev/null || true
    ip netns exec $NS_CLIENT pkill -9 ${impl}_client 2>/dev/null || true
    sleep 1
    
    # Prepare output files
    local perf_output="perf_${impl}_${field_size}_${threads}.txt"
    local server_output="server_${impl}_${field_size}_${threads}.txt"
    local client_output="client_${impl}_${field_size}_${threads}.txt"
    local perf_wrapper="perf_wrapper_${impl}_${field_size}_${threads}.sh"
    
    # CRITICAL FIX: Create a wrapper script that runs server with timeout
    # This allows perf to finish naturally and write its output
    cat > "$perf_wrapper" << EOF
#!/bin/bash
timeout $((DURATION + 2)) ./${impl}_server $PORT $field_size $threads > $server_output 2>&1
EOF
    chmod +x "$perf_wrapper"
    
    # Start server WITH perf, using the wrapper script
    local server_pid
    if [ "$PERF_AVAILABLE" = true ]; then
        # Run perf with the wrapper script - perf will naturally finish when timeout kills the server
        ip netns exec $NS_SERVER perf stat -e $PERF_EVENTS \
            -o "$perf_output" \
            ./"$perf_wrapper" &
        server_pid=$!
    else
        # Run server without perf
        ip netns exec $NS_SERVER ./"$perf_wrapper" &
        server_pid=$!
        echo "# perf not available" > "$perf_output"
    fi
    
    sleep 2
    
    # Check if server started (check the actual server process, not the wrapper)
    if ! ip netns exec $NS_SERVER pgrep -f "${impl}_server" > /dev/null 2>&1; then
        echo "    ERROR: Server failed to start"
        if [ -s "$server_output" ]; then
            echo "    Server error:"
            cat "$server_output"
        fi
        rm -f "$perf_wrapper"
        return 1
    fi
    
    # Run client in client namespace
    local client_exit_code=0
    
    ip netns exec $NS_CLIENT \
        ./${impl}_client $SERVER_IP $PORT $field_size $DURATION > "$client_output" 2>&1
    client_exit_code=$?
    
    if [ $client_exit_code -ne 0 ]; then
        echo "    WARNING: Client had issues (exit code: $client_exit_code)"
    fi
    
    # CRITICAL: Wait for the perf process to finish naturally
    # The timeout in the wrapper will kill the server, then perf will finish and write output
    echo "    Waiting for perf to finish and write output..."
    wait $server_pid 2>/dev/null || true
    
    # Give perf a moment to flush output to file
    sleep 1
    
    # Make absolutely sure everything is dead
    ip netns exec $NS_SERVER pkill -9 ${impl}_server 2>/dev/null || true
    
    # Parse client output for throughput and latency
    local throughput=""
    local latency=""
    
    if grep -q "RESULTS:" "$client_output"; then
        throughput=$(grep "RESULTS:" "$client_output" | grep -oP 'throughput_gbps=\K[0-9.]+' || echo "")
        latency=$(grep "RESULTS:" "$client_output" | grep -oP 'latency_us=\K[0-9.]+' || echo "")
    fi
    
    if [ -z "$throughput" ]; then
        throughput=$(grep -i "throughput" "$client_output" | grep -oP '[0-9]+\.?[0-9]+' | head -1 || echo "")
    fi
    
    if [ -z "$latency" ]; then
        latency=$(grep -i "latency" "$client_output" | grep -oP '[0-9]+\.?[0-9]+' | head -1 || echo "")
    fi
    
    # Parse perf output (from SERVER)
    local cycles="0"
    local ctx_switches="0"
    local l1_misses="0"
    local llc_misses="0"
    
    if [ "$PERF_AVAILABLE" = true ] && [ -f "$perf_output" ]; then
        # Debug: show file size
        local file_size=$(wc -c < "$perf_output" 2>/dev/null || echo "0")
        if [ "$file_size" -eq 0 ]; then
            echo "    WARNING: Perf output file is empty!"
        fi
        
        if [ "$HYBRID_CPU" = true ]; then
            cycles=$(parse_perf_hybrid "$perf_output" "cycles")
            l1_misses=$(parse_perf_hybrid "$perf_output" "L1-dcache-load-misses")
            llc_misses=$(parse_perf_hybrid "$perf_output" "LLC-load-misses")
            ctx_switches=$(grep "context-switches" "$perf_output" 2>/dev/null | grep -v "#" | head -1 | awk '{print $1}' | tr -d ',')
            [[ -z "$ctx_switches" ]] || [[ "$ctx_switches" == "<not"* ]] && ctx_switches="0"
        else
            cycles=$(parse_perf_standard "$perf_output" "cycles")
            ctx_switches=$(parse_perf_standard "$perf_output" "context-switches")
            l1_misses=$(parse_perf_standard "$perf_output" "L1-dcache-load-misses")
            llc_misses=$(parse_perf_standard "$perf_output" "LLC-load-misses")
            
            if [ "$l1_misses" = "0" ] && [ "$llc_misses" = "0" ]; then
                local cache_miss=$(parse_perf_standard "$perf_output" "cache-misses")
                if [ "$cache_miss" != "0" ]; then
                    l1_misses="$cache_miss"
                    llc_misses="$cache_miss"
                fi
            fi
        fi
    fi
    
    [ -z "$throughput" ] && throughput="0"
    [ -z "$latency" ] && latency="0"
    
    # Append to CSV
    echo "$impl,$field_size,$threads,$cycles,$l1_misses,$llc_misses,$ctx_switches,$throughput,$latency" >> "$csv_file"
    
    echo "    Throughput: $throughput Gbps, Latency: $latency µs"
    echo "    Cycles: $cycles, L1: $l1_misses, LLC: $llc_misses, Ctx: $ctx_switches"
    
    # Cleanup wrapper script
    rm -f "$perf_wrapper"
    sleep 1
}

# Run experiments for A1
echo ""
echo "====== Part A1: Two-Copy Implementation (send/recv) ======"
for field_size in "${FIELD_SIZES[@]}"; do
    for threads in "${THREAD_COUNTS[@]}"; do
        run_experiment "A1" $field_size $threads "MT25078_Results_A1.csv"
    done
done

# Run experiments for A2
echo ""
echo "====== Part A2: One-Copy Implementation (sendmsg) ======"
for field_size in "${FIELD_SIZES[@]}"; do
    for threads in "${THREAD_COUNTS[@]}"; do
        run_experiment "A2" $field_size $threads "MT25078_Results_A2.csv"
    done
done

# Run experiments for A3
echo ""
echo "====== Part A3: Zero-Copy Implementation (MSG_ZEROCOPY) ======"
for field_size in "${FIELD_SIZES[@]}"; do
    for threads in "${THREAD_COUNTS[@]}"; do
        run_experiment "A3" $field_size $threads "MT25078_Results_A3.csv"
    done
done

# Cleanup
ip netns exec $NS_SERVER pkill -9 A1_server 2>/dev/null || true
ip netns exec $NS_SERVER pkill -9 A2_server 2>/dev/null || true
ip netns exec $NS_SERVER pkill -9 A3_server 2>/dev/null || true
ip netns exec $NS_CLIENT pkill -9 A1_client 2>/dev/null || true
ip netns exec $NS_CLIENT pkill -9 A2_client 2>/dev/null || true
ip netns exec $NS_CLIENT pkill -9 A3_client 2>/dev/null || true

echo ""
echo "=== Cleanup: Removing network namespaces ==="
ip netns delete $NS_SERVER
ip netns delete $NS_CLIENT
echo "Namespaces removed"
echo ""

echo "=== All experiments completed ==="
echo ""
echo "Results saved to:"
echo "  - MT25078_Results_A1.csv"
echo "  - MT25078_Results_A2.csv"
echo "  - MT25078_Results_A3.csv"
echo ""
echo "Sample results:"
echo "--- A1 (Two-Copy) ---"
head -5 MT25078_Results_A1.csv
echo ""
echo "--- A2 (One-Copy) ---"
head -5 MT25078_Results_A2.csv
echo ""
echo "--- A3 (Zero-Copy) ---"
head -5 MT25078_Results_A3.csv
echo ""