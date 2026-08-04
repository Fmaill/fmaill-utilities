# Validation

The v0.2.1 package was checked with:

- C++23 unit tests for ID parsing, serialization and 12-item pagination.
- C++23 syntax compilation of the complete visual selector against API-compatible Geode stubs.
- AddressSanitizer and UndefinedBehaviorSanitizer for selection utilities.
- CMake configure/build against a local Geode-compatible test harness.
- JSON parsing for `mod.json`.
- YAML parsing for the GitHub Actions workflow.
- ZIP integrity and clean-package checks.

The immediately previous v0.2.0 source completed the repository's real five-platform Geode GitHub Actions workflow successfully. v0.2.1 keeps the same build configuration and core save/unlock flow, while changing the selector layout. The definitive real-platform validation for v0.2.1 occurs after pushing this package to GitHub Actions.
