#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/Popup.hpp>

#include "SelectionUtils.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <string_view>

using namespace geode::prelude;

namespace {
constexpr int kCategoryCount = 8;
constexpr int kIconsPerPage = 12;
constexpr int kGridColumns = 4;

struct IconCategory {
    std::string_view displayName;
    std::string_view saveKey;
    IconType type;
    int fallbackMaxID;
    float cardScale;
    float largeScale;
};

constexpr std::array<IconCategory, kCategoryCount> kCategories {{
    { "Cubos",   "visual-cube-selection",   IconType::Cube,   485, 0.82f, 1.45f },
    { "Naves",   "visual-ship-selection",   IconType::Ship,   169, 0.66f, 1.15f },
    { "Bolas",   "visual-ball-selection",   IconType::Ball,   118, 0.82f, 1.45f },
    { "UFO",     "visual-ufo-selection",    IconType::Ufo,    149, 0.66f, 1.15f },
    { "Wave",    "visual-wave-selection",   IconType::Wave,    96, 0.72f, 1.25f },
    { "Robots",  "visual-robot-selection",  IconType::Robot,   68, 0.56f, 0.95f },
    { "Spiders", "visual-spider-selection", IconType::Spider,  69, 0.56f, 0.95f },
    { "Swing",   "visual-swing-selection",  IconType::Swing,   43, 0.64f, 1.10f },
}};

int iconCountFor(IconCategory const& category) {
    auto gameManager = GameManager::sharedState();
    if (!gameManager) {
        return category.fallbackMaxID;
    }

    int count = gameManager->countForType(category.type);
    return count > 0 ? count : category.fallbackMaxID;
}

ccColor3B primaryPlayerColor() {
    auto gameManager = GameManager::sharedState();
    return gameManager
        ? gameManager->colorForIdx(gameManager->getPlayerColor())
        : ccColor3B { 255, 255, 255 };
}

ccColor3B secondaryPlayerColor() {
    auto gameManager = GameManager::sharedState();
    return gameManager
        ? gameManager->colorForIdx(gameManager->getPlayerColor2())
        : ccColor3B { 0, 255, 255 };
}

SimplePlayer* createIconPreview(int id, IconCategory const& category, float scale) {
    auto preview = SimplePlayer::create(1);
    if (!preview) {
        return nullptr;
    }

    preview->updatePlayerFrame(id, category.type);
    preview->setColors(primaryPlayerColor(), secondaryPlayerColor());
    preview->updateColors();
    preview->setScale(scale);
    return preview;
}

template <class Selector>
CCMenuItemSpriteExtra* createArrowButton(
    CCObject* target,
    Selector selector,
    bool pointsRight,
    float scale
) {
    auto sprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    if (!sprite) {
        return nullptr;
    }

    sprite->setScale(scale);
    sprite->setFlipX(pointsRight);
    return CCMenuItemSpriteExtra::create(sprite, target, selector);
}

class VisualIconSelectorPopup final : public Popup {
protected:
    std::array<std::set<int>, kCategoryCount> m_selected;
    int m_categoryIndex = 0;
    int m_page = 0;
    int m_previewID = 1;

    CCMenu* m_gridMenu = nullptr;
    CCMenu* m_navigationMenu = nullptr;
    CCMenu* m_actionMenu = nullptr;
    CCLayer* m_previewPanel = nullptr;

    CCLabelBMFont* m_categoryLabel = nullptr;
    CCLabelBMFont* m_categoryMetaLabel = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    CCLabelBMFont* m_selectedLabel = nullptr;

    bool setup() {
        this->setTitle("Mis iconos", "goldFont.fnt", 0.78f, 17.f);

        auto subtitle = CCLabelBMFont::create(
            "Toca un icono para agregarlo a tu seleccion",
            "chatFont.fnt"
        );
        subtitle->setScale(0.53f);
        subtitle->setColor({ 230, 222, 210 });
        subtitle->setPosition({ 245.f, 252.f });
        m_mainLayer->addChild(subtitle, 2);

        auto categoryBar = CCLayerColor::create({ 20, 12, 9, 90 }, 252.f, 38.f);
        categoryBar->setPosition({ 119.f, 205.f });
        m_mainLayer->addChild(categoryBar, 1);

        m_categoryLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_categoryLabel->setScale(0.62f);
        m_categoryLabel->setPosition({ 245.f, 228.f });
        m_mainLayer->addChild(m_categoryLabel, 3);

        m_categoryMetaLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_categoryMetaLabel->setScale(0.48f);
        m_categoryMetaLabel->setColor({ 205, 195, 180 });
        m_categoryMetaLabel->setPosition({ 245.f, 213.f });
        m_mainLayer->addChild(m_categoryMetaLabel, 3);

        m_gridMenu = CCMenu::create();
        m_gridMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(m_gridMenu, 5);

        m_navigationMenu = CCMenu::create();
        m_navigationMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(m_navigationMenu, 6);

        m_actionMenu = CCMenu::create();
        m_actionMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(m_actionMenu, 6);

        m_previewPanel = CCLayer::create();
        m_mainLayer->addChild(m_previewPanel, 4);

        m_pageLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_pageLabel->setScale(0.47f);
        m_pageLabel->setPosition({ 176.f, 25.f });
        m_mainLayer->addChild(m_pageLabel, 4);

        m_selectedLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_selectedLabel->setScale(0.40f);
        m_selectedLabel->setColor({ 230, 230, 230 });
        m_selectedLabel->setPosition({ 401.f, 76.f });
        m_mainLayer->addChild(m_selectedLabel, 5);

        loadSelections();
        buildStaticButtons();
        refreshAll();
        return true;
    }

    void loadSelections() {
        auto mod = Mod::get();
        for (int i = 0; i < kCategoryCount; ++i) {
            auto const& category = kCategories[i];
            auto saved = mod->getSavedValue<std::string>(std::string(category.saveKey), "");
            m_selected[i] = fmaill::selection::parseIDs(saved, iconCountFor(category));
        }
    }

    void saveSelections() const {
        auto mod = Mod::get();
        for (int i = 0; i < kCategoryCount; ++i) {
            mod->setSavedValue<std::string>(
                std::string(kCategories[i].saveKey),
                fmaill::selection::serializeIDs(m_selected[i])
            );
        }
    }

    std::size_t totalSelected() const {
        std::size_t total = 0;
        for (auto const& ids : m_selected) {
            total += ids.size();
        }
        return total;
    }

    bool isUnlocked(int id, IconCategory const& category) const {
        auto gameManager = GameManager::sharedState();
        return gameManager && gameManager->isIconUnlocked(id, category.type);
    }

    void buildStaticButtons() {
        if (auto previousCategory = createArrowButton(
            this,
            menu_selector(VisualIconSelectorPopup::onPreviousCategory),
            false,
            0.55f
        )) {
            previousCategory->setPosition({ 96.f, 224.f });
            m_navigationMenu->addChild(previousCategory);
        }

        if (auto nextCategory = createArrowButton(
            this,
            menu_selector(VisualIconSelectorPopup::onNextCategory),
            true,
            0.55f
        )) {
            nextCategory->setPosition({ 394.f, 224.f });
            m_navigationMenu->addChild(nextCategory);
        }

        if (auto previousPage = createArrowButton(
            this,
            menu_selector(VisualIconSelectorPopup::onPreviousPage),
            false,
            0.42f
        )) {
            previousPage->setPosition({ 115.f, 25.f });
            m_navigationMenu->addChild(previousPage);
        }

        if (auto nextPage = createArrowButton(
            this,
            menu_selector(VisualIconSelectorPopup::onNextPage),
            true,
            0.42f
        )) {
            nextPage->setPosition({ 237.f, 25.f });
            m_navigationMenu->addChild(nextPage);
        }

        auto clearSprite = ButtonSprite::create(
            "Limpiar", 84, true, "bigFont.fnt", "GJ_button_04.png", 28.f, 0.46f
        );
        auto clearButton = CCMenuItemSpriteExtra::create(
            clearSprite,
            this,
            menu_selector(VisualIconSelectorPopup::onClearCategory)
        );
        clearButton->setPosition({ 317.f, 26.f });
        m_actionMenu->addChild(clearButton);

        auto applySprite = ButtonSprite::create(
            "Desbloquear", 116, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.48f
        );
        auto applyButton = CCMenuItemSpriteExtra::create(
            applySprite,
            this,
            menu_selector(VisualIconSelectorPopup::onApply)
        );
        applyButton->setPosition({ 422.f, 26.f });
        m_actionMenu->addChild(applyButton);
    }

    void refreshAll() {
        refreshGrid();
        refreshHeader();
        refreshPreview();
        refreshFooter();
    }

    void refreshHeader() {
        auto const& category = kCategories[m_categoryIndex];
        int count = iconCountFor(category);

        m_categoryLabel->setString(std::string(category.displayName).c_str());
        m_categoryMetaLabel->setString(
            fmt::format("{} disponibles", count).c_str()
        );
    }

    void refreshGrid() {
        m_gridMenu->removeAllChildrenWithCleanup(true);

        auto const& category = kCategories[m_categoryIndex];
        int iconCount = iconCountFor(category);
        int pages = fmaill::selection::pageCount(iconCount, kIconsPerPage);
        m_page = std::clamp(m_page, 0, pages - 1);

        auto [firstID, lastID] = fmaill::selection::pageRange(
            m_page,
            iconCount,
            kIconsPerPage
        );

        if (m_previewID < firstID || m_previewID > lastID) {
            m_previewID = firstID;
        }

        for (int id = firstID; id <= lastID; ++id) {
            int slot = id - firstID;
            int column = slot % kGridColumns;
            int row = slot / kGridColumns;

            bool unlocked = isUnlocked(id, category);
            bool selected = m_selected[m_categoryIndex].contains(id);

            ccColor4B borderColor = selected
                ? ccColor4B { 88, 210, 115, 235 }
                : unlocked
                    ? ccColor4B { 80, 145, 215, 210 }
                    : ccColor4B { 105, 82, 62, 205 };

            auto card = CCLayerColor::create(borderColor, 66.f, 50.f);
            auto inner = CCLayerColor::create({ 28, 22, 20, 235 }, 62.f, 46.f);
            inner->setPosition({ 2.f, 2.f });
            card->addChild(inner, 1);

            if (auto preview = createIconPreview(id, category, category.cardScale)) {
                preview->setPosition({ 33.f, 29.f });
                card->addChild(preview, 2);
            }

            auto idLabel = CCLabelBMFont::create(
                fmt::format("#{}", id).c_str(),
                "chatFont.fnt"
            );
            idLabel->setScale(0.43f);
            idLabel->setColor({ 225, 220, 212 });
            idLabel->setPosition({ 33.f, 8.f });
            card->addChild(idLabel, 3);

            if (selected) {
                auto check = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
                if (check) {
                    check->setScale(0.34f);
                    check->setPosition({ 57.f, 42.f });
                    card->addChild(check, 4);
                }
            }

            auto button = CCMenuItemSpriteExtra::create(
                card,
                this,
                menu_selector(VisualIconSelectorPopup::onIcon)
            );
            button->setTag(id);
            button->setPosition({
                54.f + column * 73.f,
                174.f - row * 57.f
            });
            m_gridMenu->addChild(button);
        }
    }

    void refreshPreview() {
        m_previewPanel->removeAllChildrenWithCleanup(true);

        auto const& category = kCategories[m_categoryIndex];
        int iconCount = iconCountFor(category);
        m_previewID = std::clamp(m_previewID, 1, iconCount);

        bool unlocked = isUnlocked(m_previewID, category);
        bool selected = m_selected[m_categoryIndex].contains(m_previewID);

        auto panel = CCLayerColor::create({ 18, 13, 12, 120 }, 126.f, 137.f);
        panel->setPosition({ 335.f, 80.f });
        m_previewPanel->addChild(panel, 1);

        auto panelInner = CCLayerColor::create({ 52, 37, 28, 210 }, 120.f, 131.f);
        panelInner->setPosition({ 338.f, 83.f });
        m_previewPanel->addChild(panelInner, 2);

        auto previewTitle = CCLabelBMFont::create("Vista previa", "goldFont.fnt");
        previewTitle->setScale(0.44f);
        previewTitle->setPosition({ 398.f, 196.f });
        m_previewPanel->addChild(previewTitle, 3);

        if (auto preview = createIconPreview(m_previewID, category, category.largeScale)) {
            preview->setPosition({ 398.f, 153.f });
            m_previewPanel->addChild(preview, 3);
        }

        auto idLabel = CCLabelBMFont::create(
            fmt::format("#{}", m_previewID).c_str(),
            "bigFont.fnt"
        );
        idLabel->setScale(0.46f);
        idLabel->setPosition({ 398.f, 112.f });
        m_previewPanel->addChild(idLabel, 3);

        auto statusLabel = CCLabelBMFont::create(
            selected ? "Seleccionado" : unlocked ? "Ya obtenido" : "Disponible",
            "chatFont.fnt"
        );
        statusLabel->setScale(0.50f);
        statusLabel->setColor(
            selected
                ? ccColor3B { 130, 255, 150 }
                : unlocked
                    ? ccColor3B { 145, 200, 255 }
                    : ccColor3B { 235, 225, 210 }
        );
        statusLabel->setPosition({ 398.f, 92.f });
        m_previewPanel->addChild(statusLabel, 3);
    }

    void refreshFooter() {
        int pages = fmaill::selection::pageCount(
            iconCountFor(kCategories[m_categoryIndex]),
            kIconsPerPage
        );

        m_pageLabel->setString(
            fmt::format("{} / {}", m_page + 1, pages).c_str()
        );

        m_selectedLabel->setString(
            fmt::format("{} seleccionados", totalSelected()).c_str()
        );
    }

    void changeCategory(int direction) {
        m_categoryIndex = (m_categoryIndex + direction + kCategoryCount) % kCategoryCount;
        m_page = 0;
        m_previewID = 1;
        refreshAll();
    }

    void onPreviousCategory(CCObject*) {
        changeCategory(-1);
    }

    void onNextCategory(CCObject*) {
        changeCategory(1);
    }

    void onIcon(CCObject* sender) {
        int id = sender->getTag();
        m_previewID = id;

        auto const& category = kCategories[m_categoryIndex];
        if (!isUnlocked(id, category)) {
            auto& selected = m_selected[m_categoryIndex];
            if (selected.contains(id)) {
                selected.erase(id);
            }
            else {
                selected.insert(id);
            }
            saveSelections();
        }

        refreshGrid();
        refreshPreview();
        refreshFooter();
    }

    void onPreviousPage(CCObject*) {
        if (m_page > 0) {
            --m_page;
            refreshAll();
        }
    }

    void onNextPage(CCObject*) {
        int pages = fmaill::selection::pageCount(
            iconCountFor(kCategories[m_categoryIndex]),
            kIconsPerPage
        );
        if (m_page + 1 < pages) {
            ++m_page;
            refreshAll();
        }
    }

    void onClearCategory(CCObject*) {
        m_selected[m_categoryIndex].clear();
        saveSelections();
        refreshAll();
    }

    void onApply(CCObject*) {
        std::size_t requested = totalSelected();
        if (requested == 0) {
            FLAlertLayer::create(
                "Fmaill Utilities",
                "No has seleccionado ningun icono.",
                "OK"
            )->show();
            return;
        }

        auto gameManager = GameManager::sharedState();
        if (!gameManager) {
            FLAlertLayer::create(
                "Fmaill Utilities",
                "No se pudo acceder al guardado de Geometry Dash.",
                "OK"
            )->show();
            return;
        }

        int unlockedNow = 0;
        for (int i = 0; i < kCategoryCount; ++i) {
            auto const& category = kCategories[i];
            for (int id : m_selected[i]) {
                if (!gameManager->isIconUnlocked(id, category.type)) {
                    gameManager->unlockIcon(id, category.type);
                    ++unlockedNow;
                }
            }
        }

        gameManager->save();
        for (auto& ids : m_selected) {
            ids.clear();
        }
        saveSelections();
        refreshAll();

        auto message = fmt::format(
            "Se desbloquearon y guardaron <cg>{}</c> iconos en tu progreso local.",
            unlockedNow
        );
        FLAlertLayer::create(
            "Fmaill Utilities",
            message.c_str(),
            "OK"
        )->show();
    }

    void onClose(CCObject* sender) override {
        saveSelections();
        Popup::onClose(sender);
    }

public:
    static VisualIconSelectorPopup* create() {
        auto popup = new VisualIconSelectorPopup();
        if (popup && popup->init(490.f, 280.f, "GJ_square01.png") && popup->setup()) {
            popup->autorelease();
            return popup;
        }

        delete popup;
        return nullptr;
    }
};

SimplePlayer* createMenuButtonPreview() {
    auto gameManager = GameManager::sharedState();
    int cubeID = gameManager ? gameManager->getPlayerFrame() : 1;

    auto preview = SimplePlayer::create(cubeID);
    if (!preview) {
        return nullptr;
    }

    preview->setColors(primaryPlayerColor(), secondaryPlayerColor());
    preview->updateColors();
    return preview;
}
} // namespace

class $modify(FmaillUtilitiesMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        if (!Mod::get()->getSettingValue<bool>("show-main-menu-button")) {
            return true;
        }

        auto preview = createMenuButtonPreview();
        if (!preview) {
            return true;
        }

        auto buttonSprite = ButtonSprite::create(
            preview,
            54,
            true,
            48.f,
            "GJ_button_04.png",
            0.76f
        );
        if (!buttonSprite) {
            return true;
        }

        auto button = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(FmaillUtilitiesMenuLayer::onOpenIconSelector)
        );

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        menu->addChild(button);

        auto windowSize = CCDirector::sharedDirector()->getWinSize();
        button->setPosition({ 38.f, 38.f });
        menu->setContentSize(windowSize);
        this->addChild(menu, 20);
        return true;
    }

    void onOpenIconSelector(CCObject*) {
        if (auto popup = VisualIconSelectorPopup::create()) {
            popup->show();
        }
    }
};
