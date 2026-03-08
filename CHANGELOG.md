# Changelog

All notable changes to SafeCpp will be documented in this file.

## [1.0.0] - 2026-03-08

### Added
- **Core Detectors**
  - Use-after-free detection
  - Memory leak detection
  - Null pointer dereference detection
  
- **Configuration System**
  - JSON-based configuration files
  - Per-checker enable/disable settings
  - Severity level configuration (ERROR, WARNING, INFO)
  
- **Multiple Output Formats**
  - Text output (default)
  - JSON output for programmatic parsing
  - HTML output with visual styling
  - SARIF 2.1.0 output for CI/CD integration
  
- **Command-Line Interface**
  - `--format` option to select output format
  - `--output` option to specify output file
  - `--config` option to load configuration file
  - `--verbose` flag for detailed output
  
- **Build System**
  - Makefile with build, clean, test, and help targets
  - Automatic header dependency tracking
  
- **CI/CD Integration**
  - GitHub Actions workflow
  - Automated testing on push/PR
  - SARIF upload for GitHub Code Scanning
  
- **Examples**
  - use_after_free.cpp - Use-after-free violations
  - double_free.cpp - Double-free violations
  - memory_leak.cpp - Memory leak examples
  - null_pointer.cpp - Null dereference cases
  - safe_code.cpp - Safe code patterns

### Technical Details
- Built on LLVM 20 and Clang
- Uses Clang AST for static analysis
- Modular architecture with separate detectors
- Extensible report generation system
