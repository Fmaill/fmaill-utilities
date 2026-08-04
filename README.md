# Fmaill Utilities

A modular Geode utility mod for Geometry Dash 2.2081.

## Included module: selective icon unlocker

The first module unlocks only the icon IDs configured by the user and writes the unlock markers to Geometry Dash's normal local save through `GameManager::unlockIcon` followed by `GameManager::save`.

It does **not** contact Geometry Dash servers, fake leaderboard records, create achievements, or record a false acquisition date. Geometry Dash's icon save data stores the unlocked state; the requirement text shown in the garage remains the game's normal static text.

### Usage

1. Install the compiled `.geode` file.
2. Open **Geode > Installed > Fmaill Utilities > Settings**.
3. Enter IDs for the desired categories. Commas and ranges are supported, for example: `1, 5, 20-25`.
4. Enable **Aplicar y guardar ahora**.
5. The mod applies the unlocks, saves the local game data, shows a result popup, and turns the action back off.

The changes remain in the local save after disabling or uninstalling the mod.

## Supported icon ranges for GD 2.2081

| Category | IDs |
|---|---:|
| Cube | 1-485 |
| Ship | 1-169 |
| Ball | 1-118 |
| UFO | 1-149 |
| Wave | 1-96 |
| Robot | 1-68 |
| Spider | 1-69 |
| Swing | 1-43 |

## GitHub Actions

The workflow in `.github/workflows/multi-platform.yml` follows Geode's official multi-platform action pattern. Every push and pull request builds Windows, macOS, iOS, Android 32-bit, and Android 64-bit versions, then combines them into one `.geode` artifact.

Open the repository's **Actions** tab, select the latest successful run, and download the **Fmaill Utilities** artifact.

## Local build

Install the Geode CLI and SDK, set `GEODE_SDK`, then run:

```bash
geode build
```

## Safety note

Back up your Geometry Dash account/save before testing any save-changing mod. Do not mix this utility with other mods that overwrite or restore an older save during the same session.
