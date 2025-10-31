# Contributing to netstat-monitor

Thank you for your interest in contributing to netstat-monitor! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow

## How to Contribute

### Reporting Bugs

Before creating a bug report, please check existing issues. When creating a bug report, include:

- **Description**: Clear description of the issue
- **Steps to reproduce**: Minimal steps to reproduce the behavior
- **Expected behavior**: What you expected to happen
- **Actual behavior**: What actually happened
- **Environment**: OS, kernel version, gcc version
- **Output**: Relevant program output or error messages

### Suggesting Enhancements

Enhancement suggestions are welcome! Please include:

- **Use case**: Why is this enhancement needed?
- **Proposed solution**: How would it work?
- **Alternatives considered**: Other approaches you've thought about
- **Breaking changes**: Will it break existing functionality?

### Pull Requests

1. **Fork the repository** and create a feature branch
2. **Follow coding standards** (see below)
3. **Write tests** for new functionality
4. **Update documentation** (README.md, comments)
5. **Test thoroughly** on multiple interfaces and scenarios
6. **Commit with clear messages** following Conventional Commits

#### Coding Standards

```c
// Follow these conventions:

// 1. Indentation: 4 spaces (no tabs)
if (condition) {
    do_something();
}

// 2. Braces: K&R style (opening brace on same line)
void function_name(int param) {
    // code here
}

// 3. Naming:
//    - Functions: snake_case
//    - Variables: snake_case  
//    - Constants: UPPER_CASE
//    - Structs: snake_case_t suffix

// 4. Comments: Explain WHY, not WHAT
// Calculate delta to handle counter wraparound
uint64_t delta = safe_delta(current, previous);

// 5. Line length: Max 100 characters (prefer 80)

// 6. Function length: Max 50 lines (prefer smaller)

// 7. Error handling: Always check return values
if (fopen(...) == NULL) {
    fprintf(stderr, "Error: %s\n", strerror(errno));
    return false;
}
```

#### Compilation Requirements

Your code must:
- Compile with `-Wall -Wextra -Wpedantic` with zero warnings
- Support C11 standard
- Be POSIX.1-2008 compliant
- Have no external dependencies beyond libc
- Work on both 32-bit and 64-bit systems

Test compilation:
```bash
make clean
make
make debug
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror src/netstat_monitor.c
```

#### Testing Requirements

Before submitting a PR, test:

1. **Basic functionality**
   ```bash
   make test
   ./netstat_monitor lo -i 1 -n 5
   ```

2. **Invalid inputs**
   ```bash
   ./netstat_monitor nonexistent
   ./netstat_monitor eth0 -i 0
   ./netstat_monitor eth0 -n -1
   ```

3. **Signal handling**
   ```bash
   ./netstat_monitor eth0  # Press Ctrl+C
   ```

4. **Multiple interfaces**
   ```bash
   ./netstat_monitor lo -n 3
   ./netstat_monitor eth0 -n 3
   ./netstat_monitor wlan0 -n 3
   ```

5. **Edge cases**
   - Interface with no traffic
   - Interface with high traffic (>1 Gbps if available)
   - Very short intervals (-i 1)
   - Very long intervals (-i 60)

### Commit Message Format

Follow Conventional Commits:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Code style (formatting, no logic change)
- `refactor`: Code restructuring (no behavior change)
- `perf`: Performance improvement
- `test`: Adding/updating tests
- `build`: Build system changes
- `ci`: CI/CD changes
- `chore`: Maintenance tasks

**Examples:**
```
feat(parser): add support for /sys/class/net statistics

fix(rate): correct calculation for wrapped 64-bit counters

docs(readme): add example for monitoring wireless interfaces

perf(parser): optimize strtok_r usage in hot path

refactor(format): extract unit scaling into separate function
```

## Development Workflow

### Setting Up Development Environment

```bash
# Clone the repository
git clone https://github.com/moncef007/netstat-monitor.git
cd netstat-monitor

# Create a feature branch
git checkout -b feature/my-enhancement

# Make changes and test
make clean && make
make test

# Commit changes
git add .
git commit -m "feat(scope): brief description"

# Push to your fork
git push origin feature/my-enhancement

# Create pull request on GitHub
```

### Code Review Process

1. Automated checks run (compilation, basic tests)
2. Maintainer reviews code for:
   - Code quality and style
   - Test coverage
   - Documentation completeness
   - Performance implications
3. Address feedback and update PR
4. Once approved, maintainer merges

## Areas Needing Contribution

Priority areas for contribution:

### High Priority
- [ ] Support for multiple interfaces simultaneously
- [ ] JSON output format option
- [ ] Moving average calculations
- [ ] Support for /sys/class/net as alternative source

### Medium Priority  
- [ ] Color-coded output (ncurses or ANSI)
- [ ] Alert thresholds for errors/drops
- [ ] Export to CSV/Prometheus format
- [ ] IPv4/IPv6 statistics if available

### Low Priority
- [ ] Curses-based TUI
- [ ] Historical data logging
- [ ] Web dashboard
- [ ] Configuration file support

### Testing & Documentation
- [ ] Unit tests for parsing functions
- [ ] Integration test suite
- [ ] Man page
- [ ] Example scripts

## Architecture Guidelines

### Adding New Features

When adding features, maintain:

1. **Single responsibility**: Each function does one thing
2. **No global state**: Avoid globals except volatile sig_atomic_t
3. **Error propagation**: Return bool/NULL on error, set errno
4. **Resource cleanup**: Use early returns, close files in error paths
5. **Const correctness**: Use const for read-only parameters

### Performance Considerations

- Avoid dynamic allocation in monitoring loop
- Minimize system calls per iteration
- Use stack buffers with reasonable sizes
- Profile before optimizing (measure first!)

### Security Considerations

- Validate all inputs (especially interface names)
- Use safe string functions (strncpy, not strcpy)
- Check buffer sizes before writing
- Handle integer overflow in calculations
- No format string vulnerabilities

## Questions?

- **Issues**: Use GitHub Issues for bug reports and questions
- **Discussions**: Use GitHub Discussions for general questions

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (GPLv2).

## Recognition

Contributors are listed in CHANGELOG.md and may be mentioned in release notes.

Thank you for contributing to netstat-monitor!