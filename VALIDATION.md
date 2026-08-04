# Validation report

Checked before packaging:

- `mod.json` parses correctly and targets Geode `5.7.1` / Geometry Dash `2.2081`.
- GitHub Actions workflow parses correctly and includes Windows, macOS, iOS, Android32 and Android64.
- `CMakeLists.txt` configures successfully with a Geode-compatible CMake shim.
- `IconUnlocker.cpp` passes a C++23 syntax check.
- The ID parser was tested with individual IDs, duplicate IDs, reversed ranges, clipped ranges, invalid entries and empty input.
- SDK signatures were checked against Geode v5.7.1, including `listenForSettingChanges<bool>`, settings access, and official `IconType` values.

The authoritative full multi-platform build is the GitHub Actions run started after this package is pushed.
