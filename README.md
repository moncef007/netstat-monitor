# netstat_monitor - Real-time Network Interface Statistics Monitor

A POSIX-compliant C program for monitoring network interface statistics on Linux. Reads real-time data from `/proc/net/dev` with robust parsing, accurate rate calculations, and clean output formatting.

## Building

### Quick Build
```bash
make
```

### Manual Compilation
```bash
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -o netstat_monitor src/netstat_monitor.c
```

### Debug Build
```bash
make debug
```

### Installation (system-wide)
```bash
sudo make install
```

## Usage

### Basic Usage
```bash
# Monitor eth0 with default 2-second intervals
./netstat_monitor eth0

# Monitor loopback interface
./netstat_monitor lo

# Specify custom interval (1 second)
./netstat_monitor eth0 -i 1

# Run for specific number of iterations
./netstat_monitor ppp0 -i 2 -n 60

# Display help
./netstat_monitor --help
```

### Command-Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `<interface>` | Network interface to monitor (required) | - |
| `-i, --interval <seconds>` | Update interval in seconds | 2 |
| `-n, --count <iterations>` | Number of iterations before exit | unlimited |
| `-h, --help` | Display help message | - |

## Sample Output

```
Monitoring interface: eth0 (interval: 2 seconds)
Press Ctrl+C to stop

Timestamp           Interface       RxBytes           ΔRx      RxPkts  ΔRx(p/s)   RxErr  RxDrop        TxBytes           ΔTx      TxPkts  ΔTx(p/s)   TxErr  TxDrop
------------------- ----------  ---------------  ------------  ----------  ----------  --------  --------  ---------------  ------------  ----------  ----------  --------  --------
2025-10-31 12:00:00 eth0              12.3 MB             -      123456           -         0         0          8.7 MB             -       98765           -         0         0
2025-10-31 12:00:02 eth0              12.4 MB      65.2 KB/s      123968         256         0         0          8.8 MB      20.1 KB/s       99065         150         0         0
2025-10-31 12:00:04 eth0              12.5 MB      58.7 KB/s      124432         232         0         0          8.9 MB      18.3 KB/s       99213         74          0         0
```

### Output Columns

| Column | Description |
|--------|-------------|
| **Timestamp** | Current time (YYYY-MM-DD HH:MM:SS) |
| **Interface** | Network interface name |
| **RxBytes** | Total bytes received (formatted) |
| **ΔRx** | Receive rate in bytes/second |
| **RxPkts** | Total packets received |
| **ΔRx(p/s)** | Receive rate in packets/second |
| **RxErr** | Total receive errors |
| **RxDrop** | Total receive drops |
| **TxBytes** | Total bytes transmitted (formatted) |
| **ΔTx** | Transmit rate in bytes/second |
| **TxPkts** | Total packets transmitted |
| **ΔTx(p/s)** | Transmit rate in packets/second |
| **TxErr** | Total transmit errors |
| **TxDrop** | Total transmit drops |

## Troubleshooting

### Issue: "Interface not found"
**Solution**: Verify interface exists with `ip link show` or `ifconfig -a`. Check spelling.

### Issue: "Cannot open /proc/net/dev"
**Solution**: Ensure running on Linux. Check file permissions (should be world-readable).

### Issue: Rates show as zero
**Solution**: Verify interface has traffic. Try `ping <remote>` while monitoring. Check interval isn't too short.

### Issue: Counter wraparound not handled correctly
**Solution**: This is rare with 64-bit counters. Verify kernel uses 32-bit counters. Program assumes 32-bit wraparound.

## License
GPLv2

## References

- `/proc/net/dev` format: Linux kernel documentation
- POSIX.1-2008: IEEE Std 1003.1-2008
- C11 Standard: ISO/IEC 9899:2011
