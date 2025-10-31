# Changelog

All notable changes to netstat_monitor will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Multiple interface monitoring support
- JSON output format
- Moving average calculations
- Color-coded output
- Alert thresholds

## [0.0.1] - 2025-10-31

### Added
- Initial release of netstat-monitor
- Real-time network interface statistics monitoring from /proc/net/dev

### Implementation Details
- C11/POSIX.1-2008 compliant implementation
- Stack-based memory management (no dynamic allocation)
- Per-iteration file open/close for /proc/net/dev consistency

### Tested On
- Linux kernel 5.x+ with /proc/net/dev support
- Physical interfaces (eth0)
- Virtual interfaces (lo, veth pairs)
- PPP interfaces (ppp0)
- Wireless interfaces (wlan0)

### Known Limitations
- Linux-specific (/proc dependency)
- Single interface monitoring only
- Assumes 32-bit counter wraparound behavior

---

[0.0.1]: https://github.com/moncef007/netstat-monitor/releases/tag/v0.0.1