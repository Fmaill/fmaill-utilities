#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <set>
#include <string>
#include <string_view>

using namespace geode::prelude;

namespace {
struct IconCategory {
    std::string_view settingKey;
    std::string_view displayName;
    IconType type;
    int maxID;
};

// Named IconType values avoid relying on undocumented numeric casts.
// The limits below match Geometry Dash 2.2081's icon key ranges.
constexpr std::array<IconCategory, 8> kCategories {{
    { "cube-ids",   "Cubos",  IconType::Cube,   485 },
    { "ship-ids",   "Naves",  IconType::Ship,   169 },
    { "ball-ids",   "Bolas",  IconType::Ball,   118 },
    { "ufo-ids",    "UFO",    IconType::Ufo,    149 },
    { "wave-ids",   "Wave",   IconType::Wave,    96 },
    { "robot-ids",  "Robot",  IconType::Robot,   68 },
    { "spider-ids", "Spider", IconType::Spider,  69 },
    { "swing-ids",  "Swing",  IconType::Swing,   43 },
}};

std::string trim(std::string value) {
    auto isNotSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::optional<int> parseInteger(std::string const& value) {
    try {
        std::size_t consumed = 0;
        int result = std::stoi(value, &consumed);
        if (consumed != value.size()) {
            return std::nullopt;
        }
        return result;
    }
    catch (...) {
        return std::nullopt;
    }
}

std::set<int> parseIDs(std::string input, IconCategory const& category) {
    std::replace(input.begin(), input.end(), ';', ',');

    std::set<int> ids;
    std::size_t start = 0;

    while (start <= input.size()) {
        auto end = input.find(',', start);
        auto token = trim(input.substr(start, end == std::string::npos ? std::string::npos : end - start));

        if (!token.empty()) {
            auto dash = token.find('-');
            if (dash == std::string::npos) {
                auto parsed = parseInteger(token);
                if (parsed && *parsed >= 1 && *parsed <= category.maxID) {
                    ids.insert(*parsed);
                }
                else {
                    log::warn("Ignoring invalid {} icon ID: '{}'", category.displayName, token);
                }
            }
            else {
                auto first = parseInteger(trim(token.substr(0, dash)));
                auto last = parseInteger(trim(token.substr(dash + 1)));

                if (!first || !last) {
                    log::warn("Ignoring invalid {} icon range: '{}'", category.displayName, token);
                }
                else {
                    int rangeStart = std::min(*first, *last);
                    int rangeEnd = std::max(*first, *last);
                    rangeStart = std::max(rangeStart, 1);
                    rangeEnd = std::min(rangeEnd, category.maxID);

                    if (rangeStart > rangeEnd) {
                        log::warn("Ignoring out-of-range {} icon range: '{}'", category.displayName, token);
                    }
                    else {
                        for (int id = rangeStart; id <= rangeEnd; ++id) {
                            ids.insert(id);
                        }
                    }
                }
            }
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return ids;
}

void applyConfiguredUnlocks(bool showResult) {
    auto mod = Mod::get();
    if (!mod->getSettingValue<bool>("apply-icon-unlocks")) {
        return;
    }

    auto gameManager = GameManager::sharedState();
    if (!gameManager) {
        log::error("GameManager is not available; icon unlocks were not applied.");
        mod->setSettingValue<bool>("apply-icon-unlocks", false);
        return;
    }

    int processed = 0;

    for (auto const& category : kCategories) {
        auto configured = mod->getSettingValue<std::string>(category.settingKey);
        auto ids = parseIDs(configured, category);

        for (int id : ids) {
            gameManager->unlockIcon(id, category.type);
            ++processed;
        }
    }

    if (processed > 0) {
        // unlockIcon writes the unlock marker into GameManager's value keeper.
        // save() persists it into Geometry Dash's normal local save data.
        gameManager->save();
        log::info("Applied and saved {} configured icon unlock(s).", processed);
    }
    else {
        log::warn("No valid icon IDs were configured; nothing was changed.");
    }

    // This setting behaves like a one-shot action rather than a permanent mode.
    mod->setSettingValue<bool>("apply-icon-unlocks", false);

    if (showResult) {
        auto message = processed > 0
            ? fmt::format(
                "Se desbloquearon y guardaron <cg>{}</c> iconos configurados."
                "<br><br>Los cambios permanecen en el guardado local incluso si desactivas el mod.",
                processed
            )
            : std::string("No se encontraron IDs válidos para desbloquear.");

        FLAlertLayer::create("Fmaill Utilities", message.c_str(), "OK")->show();
    }
}
} // namespace

class $modify(FmaillUtilitiesMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        // Covers the case where the action was enabled before restarting the game.
        applyConfiguredUnlocks(true);
        return true;
    }
};

$execute {
    // Applies immediately when the action is enabled from Geode's settings UI.
    listenForSettingChanges<bool>("apply-icon-unlocks", [](bool enabled) {
        if (enabled) {
            applyConfiguredUnlocks(true);
        }
    });
}
