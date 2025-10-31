/*
 * Copyright (C) 2025 Mohamed Elmoncef HAMDI
 * This file is part of netstat-monitor <https://github.com/moncef007/netstat-monitor>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define PROC_NET_DEV "/proc/net/dev"
#define MAX_LINE_LEN 1024
#define MAX_IFACE_LEN 64
#define DEFAULT_INTERVAL 2
#define HEADER_INTERVAL 20

typedef struct {
    char interface[MAX_IFACE_LEN];
    uint64_t rx_bytes;
    uint64_t rx_packets;
    uint64_t rx_errors;
    uint64_t rx_drops;
    uint64_t tx_bytes;
    uint64_t tx_packets;
    uint64_t tx_errors;
    uint64_t tx_drops;
    struct timespec timestamp;
    bool valid;
} net_stats_t;

static volatile sig_atomic_t keep_running = 1;

static void print_usage(const char *progname);
static void signal_handler(int signum);
static bool read_net_stats(const char *interface, net_stats_t *stats);
static bool parse_interface_line(const char *line, const char *interface, net_stats_t *stats);
static void format_bytes(uint64_t bytes, char *buffer, size_t bufsize);
static void format_rate(double rate, char *buffer, size_t bufsize);
static uint64_t safe_delta(uint64_t current, uint64_t previous);
static double calculate_rate(uint64_t delta, double elapsed_seconds);
static void print_header(void);
static void print_stats(const net_stats_t *current, const net_stats_t *previous, double elapsed);
static double timespec_diff(const struct timespec *start, const struct timespec *end);

/**
 * Print usage
 */
static void print_usage(const char *progname) {
    printf("Usage: %s <interface> [OPTIONS]\n", progname);
    printf("\nMonitor real-time network interface statistics from /proc/net/dev\n");
    printf("\nArguments:\n");
    printf("  <interface>              Network interface to monitor (e.g., eth0, ppp0, lo)\n");
    printf("\nOptions:\n");
    printf("  -i, --interval <seconds> Update interval in seconds (default: %d)\n", DEFAULT_INTERVAL);
    printf("  -n, --count <iterations> Number of iterations (default: unlimited)\n");
    printf("  -h, --help               Display this help message\n");
    printf("\nExamples:\n");
    printf("  %s eth0                  Monitor eth0 with default settings\n", progname);
    printf("  %s ppp0 -i 1 -n 60       Monitor ppp0 every 1 second for 60 iterations\n", progname);
    printf("\nSignals:\n");
    printf("  SIGINT (Ctrl+C), SIGTERM Gracefully exit and print summary\n");
    printf("\n");
}

/**
 * Signal handler for graceful shutdown
 */
static void signal_handler(int signum) {
    (void)signum;
    keep_running = 0;
}

/**
 * Calculate time difference in seconds between two timespec structures
 */
static double timespec_diff(const struct timespec *start, const struct timespec *end) {
    double diff = (end->tv_sec - start->tv_sec);
    diff += (end->tv_nsec - start->tv_nsec) / 1e9;
    return diff;
}

/**
 * Safely compute delta between two counter values, handling wraparound
 * Assumes 64-bit counters, but handles 32-bit wraparound gracefully
 */
static uint64_t safe_delta(uint64_t current, uint64_t previous) {
    if (current >= previous) {
        return current - previous;
    }
    /* Counter wrapped around - assume 32-bit wraparound */
    return (UINT64_C(0x100000000) - previous) + current;
}

/**
 * Calculate rate per second from delta and elapsed time
 */
static double calculate_rate(uint64_t delta, double elapsed_seconds) {
    if (elapsed_seconds <= 0.0) {
        return 0.0;
    }
    return (double)delta / elapsed_seconds;
}

/**
 * Format bytes with human-readable units (B, KB, MB, GB)
 */
static void format_bytes(uint64_t bytes, char *buffer, size_t bufsize) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double value = (double)bytes;
    
    while (value >= 1024.0 && unit_idx < 4) {
        value /= 1024.0;
        unit_idx++;
    }
    
    if (unit_idx == 0) {
        snprintf(buffer, bufsize, "%llu %s", (unsigned long long)bytes, units[unit_idx]);
    } else {
        snprintf(buffer, bufsize, "%.1f %s", value, units[unit_idx]);
    }
}

/**
 * Format rate (bytes/sec or packets/sec) with appropriate units
 */
static void format_rate(double rate, char *buffer, size_t bufsize) {
    const char *units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unit_idx = 0;
    double value = rate;
    
    while (value >= 1024.0 && unit_idx < 3) {
        value /= 1024.0;
        unit_idx++;
    }
    
    if (rate < 1.0) {
        snprintf(buffer, bufsize, "0 B/s");
    } else if (unit_idx == 0) {
        snprintf(buffer, bufsize, "%.0f %s", value, units[unit_idx]);
    } else {
        snprintf(buffer, bufsize, "%.1f %s", value, units[unit_idx]);
    }
}

/**
 * Parse a single line from /proc/net/dev for the specified interface
 * Expected format: "  eth0: 12345 678 ..."
 * Returns true if line matches interface and parsing succeeds
 */
static bool parse_interface_line(const char *line, const char *interface, net_stats_t *stats) {
    char iface_name[MAX_IFACE_LEN];
    char line_copy[MAX_LINE_LEN];

    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    char *colon = strchr(line_copy, ':');
    if (!colon) {
        return false;
    }

    *colon = '\0';
    const char *iface_start = line_copy;
    while (*iface_start == ' ' || *iface_start == '\t') {
        iface_start++;
    }

    strncpy(iface_name, iface_start, sizeof(iface_name) - 1);
    iface_name[sizeof(iface_name) - 1] = '\0';

    size_t len = strlen(iface_name);
    while (len > 0 && (iface_name[len - 1] == ' ' || iface_name[len - 1] == '\t')) {
        iface_name[--len] = '\0';
    }

    if (strcmp(iface_name, interface) != 0) {
        return false;
    }

    char *token_ptr = colon + 1;
    char *saveptr = NULL;
    unsigned long long values[16];
    int token_count = 0;

    char *token = strtok_r(token_ptr, " \t\n\r", &saveptr);
    while (token && token_count < 16) {
        char *endptr;
        errno = 0;
        values[token_count] = strtoull(token, &endptr, 10);

        if (errno != 0 || *endptr != '\0') {
            return false;
        }
        
        token_count++;
        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }

    if (token_count < 16) {
        return false;
    }

    strncpy(stats->interface, interface, sizeof(stats->interface) - 1);
    stats->interface[sizeof(stats->interface) - 1] = '\0';
    stats->rx_bytes = values[0];
    stats->rx_packets = values[1];
    stats->rx_errors = values[2];
    stats->rx_drops = values[3];
    stats->tx_bytes = values[8];
    stats->tx_packets = values[9];
    stats->tx_errors = values[10];
    stats->tx_drops = values[11];

    stats->valid = true;
    return true;
}

/**
 * Read network statistics for specified interface from /proc/net/dev
 * Returns true on success, false if interface not found or error
 */
static bool read_net_stats(const char *interface, net_stats_t *stats) {
    FILE *fp = fopen(PROC_NET_DEV, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open %s: %s\n", PROC_NET_DEV, strerror(errno));
        return false;
    }
    
    char line[MAX_LINE_LEN];
    bool found = false;
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        if (line_num <= 2) {
            continue;
        }

        if (parse_interface_line(line, interface, stats)) {
            found = true;

            if (clock_gettime(CLOCK_MONOTONIC, &stats->timestamp) != 0) {
                fprintf(stderr, "Warning: clock_gettime failed: %s\n", strerror(errno));
                stats->timestamp.tv_sec = 0;
                stats->timestamp.tv_nsec = 0;
            }
            
            break;
        }
    }
    
    fclose(fp);
    return found;
}

static void print_header(void) {
    printf("\n");
    printf("%-19s %-10s %15s %12s %10s %10s %8s %8s %15s %12s %10s %10s %8s %8s\n",
           "Timestamp", "Interface",
           "RxBytes", "ΔRx", "RxPkts", "ΔRx(p/s)", "RxErr", "RxDrop",
           "TxBytes", "ΔTx", "TxPkts", "ΔTx(p/s)", "TxErr", "TxDrop");
    printf("%-19s %-10s %15s %12s %10s %10s %8s %8s %15s %12s %10s %10s %8s %8s\n",
           "-------------------", "----------",
           "---------------", "------------", "----------", "----------", "--------", "--------",
           "---------------", "------------", "----------", "----------", "--------", "--------");
}


static void print_stats(const net_stats_t *current, const net_stats_t *previous, double elapsed) {
    char timestamp[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char rx_bytes_str[32], tx_bytes_str[32];
    format_bytes(current->rx_bytes, rx_bytes_str, sizeof(rx_bytes_str));
    format_bytes(current->tx_bytes, tx_bytes_str, sizeof(tx_bytes_str));

    char rx_rate_str[32] = "-", tx_rate_str[32] = "-";
    char rx_pkt_rate_str[32] = "-", tx_pkt_rate_str[32] = "-";
    
    if (previous && previous->valid) {
        uint64_t rx_delta = safe_delta(current->rx_bytes, previous->rx_bytes);
        uint64_t tx_delta = safe_delta(current->tx_bytes, previous->tx_bytes);
        uint64_t rx_pkt_delta = safe_delta(current->rx_packets, previous->rx_packets);
        uint64_t tx_pkt_delta = safe_delta(current->tx_packets, previous->tx_packets);
        
        double rx_rate = calculate_rate(rx_delta, elapsed);
        double tx_rate = calculate_rate(tx_delta, elapsed);
        double rx_pkt_rate = calculate_rate(rx_pkt_delta, elapsed);
        double tx_pkt_rate = calculate_rate(tx_pkt_delta, elapsed);
        
        format_rate(rx_rate, rx_rate_str, sizeof(rx_rate_str));
        format_rate(tx_rate, tx_rate_str, sizeof(tx_rate_str));
        snprintf(rx_pkt_rate_str, sizeof(rx_pkt_rate_str), "%.0f", rx_pkt_rate);
        snprintf(tx_pkt_rate_str, sizeof(tx_pkt_rate_str), "%.0f", tx_pkt_rate);
    }
    
    printf("%-19s %-10s %15s %12s %10llu %10s %8llu %8llu %15s %12s %10llu %10s %8llu %8llu\n",
           timestamp, current->interface,
           rx_bytes_str, rx_rate_str,
           (unsigned long long)current->rx_packets, rx_pkt_rate_str,
           (unsigned long long)current->rx_errors,
           (unsigned long long)current->rx_drops,
           tx_bytes_str, tx_rate_str,
           (unsigned long long)current->tx_packets, tx_pkt_rate_str,
           (unsigned long long)current->tx_errors,
           (unsigned long long)current->tx_drops);
}


int main(int argc, char *argv[]) {
    const char *interface = NULL;
    int interval = DEFAULT_INTERVAL;
    int max_iterations = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
    }
    
    if (argc < 2) {
        fprintf(stderr, "Error: No interface specified\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    interface = argv[1];
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interval") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", argv[i]);
                return EXIT_FAILURE;
            }
            interval = atoi(argv[++i]);
            if (interval <= 0) {
                fprintf(stderr, "Error: Invalid interval: %d\n", interval);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--count") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", argv[i]);
                return EXIT_FAILURE;
            }
            max_iterations = atoi(argv[++i]);
            if (max_iterations <= 0) {
                fprintf(stderr, "Error: Invalid count: %d\n", max_iterations);
                return EXIT_FAILURE;
            }
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        fprintf(stderr, "Warning: Cannot install SIGINT handler: %s\n", strerror(errno));
    }
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        fprintf(stderr, "Warning: Cannot install SIGTERM handler: %s\n", strerror(errno));
    }

    net_stats_t initial_stats = {0};
    if (!read_net_stats(interface, &initial_stats)) {
        fprintf(stderr, "Error: Interface '%s' not found in %s\n", interface, PROC_NET_DEV);
        fprintf(stderr, "Available interfaces:\n");

        FILE *fp = fopen(PROC_NET_DEV, "r");
        if (fp) {
            char line[MAX_LINE_LEN];
            int line_num = 0;
            while (fgets(line, sizeof(line), fp)) {
                line_num++;
                if (line_num <= 2) continue;

                char *colon = strchr(line, ':');
                if (colon) {
                    *colon = '\0';
                    const char *iface = line;
                    while (*iface == ' ' || *iface == '\t') iface++;

                    char iface_trimmed[MAX_IFACE_LEN];
                    strncpy(iface_trimmed, iface, sizeof(iface_trimmed) - 1);
                    iface_trimmed[sizeof(iface_trimmed) - 1] = '\0';

                    size_t len = strlen(iface_trimmed);
                    while (len > 0 && (iface_trimmed[len - 1] == ' ' || iface_trimmed[len - 1] == '\t')) {
                        iface_trimmed[--len] = '\0';
                    }
                    
                    fprintf(stderr, "  %s\n", iface_trimmed);
                }
            }
            fclose(fp);
        }
        return EXIT_FAILURE;
    }

    net_stats_t current_stats = {0}, previous_stats = {0};
    int iteration = 0;
    int lines_since_header = 0;

    printf("Monitoring interface: %s (interval: %d seconds", interface, interval);
    if (max_iterations > 0) {
        printf(", iterations: %d", max_iterations);
    }
    printf(")\n");
    printf("Press Ctrl+C to stop\n");

    print_header();

    while (keep_running && (max_iterations < 0 || iteration < max_iterations)) {
        if (!read_net_stats(interface, &current_stats)) {
            fprintf(stderr, "\nWarning: Failed to read stats for %s (interface may have disappeared)\n",
                    interface);
            sleep(interval);
            continue;
        }

        double elapsed = 0.0;
        if (previous_stats.valid) {
            elapsed = timespec_diff(&previous_stats.timestamp, &current_stats.timestamp);
        }

        print_stats(&current_stats, previous_stats.valid ? &previous_stats : NULL, elapsed);

        previous_stats = current_stats;

        iteration++;
        lines_since_header++;

        if (lines_since_header >= HEADER_INTERVAL) {
            print_header();
            lines_since_header = 0;
        }

        if (keep_running && (max_iterations < 0 || iteration < max_iterations)) {
            sleep(interval);
        }
    }

    printf("\n");
    if (!keep_running) {
        printf("Monitoring stopped by signal\n");
    }
    printf("Total iterations: %d\n", iteration);
    
    return EXIT_SUCCESS;
}
