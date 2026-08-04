#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/Popup.hpp>

#include "SelectionUtils.hpp"

#include <array>
#include <set>
#include <string>
#include <string_view>

using namespace geode::prelude;

namespace {
constexpr int kCategoryCount = 8;
constexpr int kIconsPerPage = 18;
constexpr int kGridColumns = 6;

struct IconCategory {
    std::string_view shortName;
    std::string_view displayName;
    std::string_view saveKey;
    IconType type;
    int fallbackMaxID;
    float previewScale;
};

constexpr std::array<IconCategory, kCategoryCount> kCategories {{
    { "CUBO",   "Cubos",   "visual-cube-selection",   IconType::Cube,   485, 0.76f },
    { "NAVE",   "Naves",   "visual-ship-selection",   IconType::Ship,   169, 0.62f },
    { "BOLA",   "Bolas",   "visual-ball-selection",   IconType::Ball,   118, 0.76f },
    { "UFO",    "UFO",     "visual-ufo-selection",    IconType::Ufo,    149, 0.62f },
    { "WAVE",   "Wave",    "visual-wave-selection",   IconType::Wave,    96, 0.68f },
    { "ROBOT",  "Robots",  "visual-robot-selection",  IconType::Robot,   68, 0.52f },
    { "SPIDER", "Spiders", "visual-spider-selection", IconType::Spider,  69, 0.52f },
    { "SWING",  "Swings",  "visual-swing-selection",  IconType::Swing,   43, 0.60f },
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

SimplePlayer* createIconPreview(int id, IconCategory const& category) {
    auto preview = SimplePlayer::create(1);
    if (!preview) {
        return nullptr;
    }

    preview->updatePlayerFrame(id, category.type);
    preview->setColors(primaryPlayerColor(), secondaryPlayerColor());
    preview->updateColors();
    preview->setScale(category.previewScale);
    return preview;
}

class VisualIconSelectorPopup final : public Popup {
protected:
    std::array<std::set<int>, kCategoryCount> m_selected;
    int m_categoryIndex = 0;
    int m_page = 0;

    CCMenu* m_categoryMenu = nullptr;
    CCMenu* m_gridMenu = nullptr;
    CCMenu* m_pageMenu = nullptr;
    CCMenu* m_actionMenu = nullptr;
    CCLabelBMFont* m_categoryLabel = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    CCLabelBMFont* m_selectionLabel = nullptr;

    bool setup() {
        this->setTitle("Selector visual de iconos", "goldFont.fnt", 0.72f, 19.f);

        auto subtitle = CCLabelBMFont::create(
            "Toca un icono para seleccionarlo. Azul = ya obtenido.",
            "chatFont.fnt"
        );
        subtitle->setScale(0.55f);
        subtitle->setColor({ 235, 235, 235 });
        subtitle->setPosition({ 240.f, 259.f });
        m_mainLayer->addChild(subtitle, 2);

        m_categoryLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_categoryLabel->setScale(0.50f);
        m_categoryLabel->setPosition({ 240.f, 211.f });
        m_mainLayer->addChild(m_categoryLabel, 2);

        m_pageLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_pageLabel->setScale(0.48f);
        m_pageLabel->setPosition({ 240.f, 58.f });
        m_mainLayer->addChild(m_pageLabel, 2);

        m_selectionLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_selectionLabel->setScale(0.43f);
        m_selectionLabel->setAnchorPoint({ 0.f, 0.5f });
        m_selectionLabel->setPosition({ 20.f, 58.f });
        m_mainLayer->addChild(m_selectionLabel, 2);

        m_categoryMenu = CCMenu::create();
        m_categoryMenu->setPosition({ 0.f, 0.f });
        m_categoryMenu->setContentSize({ 460.f, 28.f });
        m_mainLayer->addChild(m_categoryMenu, 5);

        m_gridMenu = CCMenu::create();
        m_gridMenu->setPosition({ 0.f, 0.f });
        m_gridMenu->setContentSize({ 440.f, 140.f });
        m_mainLayer->addChild(m_gridMenu, 5);

        m_pageMenu = CCMenu::create();
        m_pageMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(m_pageMenu, 5);

        m_actionMenu = CCMenu::create();
        m_actionMenu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(m_actionMenu, 5);

        loadSelections();
        buildPageButtons();
        buildActionButtons();
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

    void refreshAll() {
        refreshCategoryButtons();
        refreshGrid();
        refreshLabels();
    }

    void refreshCategoryButtons() {
        m_categoryMenu->removeAllChildrenWithCleanup(true);

        for (int i = 0; i < kCategoryCount; ++i) {
            auto const& category = kCategories[i];
            auto texture = i == m_categoryIndex ? "GJ_button_02.png" : "GJ_button_04.png";
            auto sprite = ButtonSprite::create(
                std::string(category.shortName).c_str(),
                49,
                true,
                "bigFont.fnt",
                texture,
                22.f,
                0.42f
            );
            auto button = CCMenuItemSpriteExtra::create(
                sprite,
                this,
                menu_selector(VisualIconSelectorPopup::onCategory)
            );
            button->setTag(i);
            button->setPosition({ 30.f + i * 56.f, 231.f });
            m_categoryMenu->addChild(button);
        }
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

        for (int id = firstID; id <= lastID; ++id) {
            int slot = id - firstID;
            int column = slot % kGridColumns;
            int row = slot / kGridColumns;

            bool unlocked = isUnlocked(id, category);
            bool selected = m_selected[m_categoryIndex].contains(id);

            ccColor4B cardColor = selected
                ? ccColor4B { 65, 185, 95, 210 }
                : unlocked
                    ? ccColor4B { 55, 125, 210, 190 }
                    : ccColor4B { 85, 55, 35, 175 };

            auto card = CCLayerColor::create(cardColor, 60.f, 42.f);
            card->setContentSize({ 60.f, 42.f });

            auto inner = CCLayerColor::create({ 25, 25, 32, 205 }, 56.f, 38.f);
            inner->setPosition({ 2.f, 2.f });
            card->addChild(inner, 1);

            if (auto preview = createIconPreview(id, category)) {
                preview->setPosition({ 30.f, 25.f });
                card->addChild(preview, 2);
            }

            auto idLabel = CCLabelBMFont::create(
                fmt::format("#{}", id).c_str(),
                "chatFont.fnt"
            );
            idLabel->setScale(0.47f);
            idLabel->setPosition({ 30.f, 7.f });
            card->addChild(idLabel, 3);

            if (selected) {
                auto selectedLabel = CCLabelBMFont::create("SEL", "bigFont.fnt");
                selectedLabel->setScale(0.28f);
                selectedLabel->setColor({ 210, 255, 210 });
                selectedLabel->setPosition({ 49.f, 34.f });
                card->addChild(selectedLabel, 4);
            }
            else if (unlocked) {
                auto ownedLabel = CCLabelBMFont::create("OK", "bigFont.fnt");
                ownedLabel->setScale(0.28f);
                ownedLabel->setColor({ 210, 235, 255 });
                ownedLabel->setPosition({ 50.f, 34.f });
                card->addChild(ownedLabel, 4);
            }

            auto button = CCMenuItemSpriteExtra::create(
                card,
                this,
                menu_selector(VisualIconSelectorPopup::onIcon)
            );
            button->setTag(id);
            button->setPosition({
                55.f + column * 74.f,
                180.f - row * 44.f
            });
            m_gridMenu->addChild(button);
        }
    }

    void refreshLabels() {
        auto const& category = kCategories[m_categoryIndex];
        int count = iconCountFor(category);
        int pages = fmaill::selection::pageCount(count, kIconsPerPage);

        m_categoryLabel->setString(
            fmt::format("{} - {} iconos", category.displayName, count).c_str()
        );
        m_pageLabel->setString(
            fmt::format("Pagina {} / {}", m_page + 1, pages).c_str()
        );
        m_selectionLabel->setString(
            fmt::format("Seleccionados: {}", totalSelected()).c_str()
        );
    }

    void buildPageButtons() {
        auto prevSprite = ButtonSprite::create(
            "<", 34, true, "bigFont.fnt", "GJ_button_04.png", 24.f, 0.65f
        );
        auto prevButton = CCMenuItemSpriteExtra::create(
            prevSprite,
            this,
            menu_selector(VisualIconSelectorPopup::onPreviousPage)
        );
        prevButton->setPosition({ 190.f, 58.f });
        m_pageMenu->addChild(prevButton);

        auto nextSprite = ButtonSprite::create(
            ">", 34, true, "bigFont.fnt", "GJ_button_04.png", 24.f, 0.65f
        );
        auto nextButton = CCMenuItemSpriteExtra::create(
            nextSprite,
            this,
            menu_selector(VisualIconSelectorPopup::onNextPage)
        );
        nextButton->setPosition({ 290.f, 58.f });
        m_pageMenu->addChild(nextButton);
    }

    void buildActionButtons() {
        auto clearSprite = ButtonSprite::create(
            "LIMPIAR", 88, true, "bigFont.fnt", "GJ_button_04.png", 28.f, 0.48f
        );
        auto clearButton = CCMenuItemSpriteExtra::create(
            clearSprite,
            this,
            menu_selector(VisualIconSelectorPopup::onClearCategory)
        );
        clearButton->setPosition({ 70.f, 25.f });
        m_actionMenu->addChild(clearButton);

        auto pageSprite = ButtonSprite::create(
            "TODA LA PAG.", 112, true, "bigFont.fnt", "GJ_button_04.png", 28.f, 0.40f
        );
        auto pageButton = CCMenuItemSpriteExtra::create(
            pageSprite,
            this,
            menu_selector(VisualIconSelectorPopup::onSelectPage)
        );
        pageButton->setPosition({ 190.f, 25.f });
        m_actionMenu->addChild(pageButton);

        auto applySprite = ButtonSprite::create(
            "APLICAR Y GUARDAR", 166, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.43f
        );
        auto applyButton = CCMenuItemSpriteExtra::create(
            applySprite,
            this,
            menu_selector(VisualIconSelectorPopup::onApply)
        );
        applyButton->setPosition({ 365.f, 25.f });
        m_actionMenu->addChild(applyButton);
    }

    void onCategory(CCObject* sender) {
        m_categoryIndex = std::clamp(sender->getTag(), 0, kCategoryCount - 1);
        m_page = 0;
        refreshAll();
    }

    void onIcon(CCObject* sender) {
        int id = sender->getTag();
        auto& selected = m_selected[m_categoryIndex];

        auto const& category = kCategories[m_categoryIndex];
        if (isUnlocked(id, category)) {
            return;
        }

        if (selected.contains(id)) {
            selected.erase(id);
        }
        else {
            selected.insert(id);
        }

        saveSelections();
        refreshGrid();
        refreshLabels();
    }

    void onPreviousPage(CCObject*) {
        if (m_page > 0) {
            --m_page;
            refreshGrid();
            refreshLabels();
        }
    }

    void onNextPage(CCObject*) {
        int pages = fmaill::selection::pageCount(
            iconCountFor(kCategories[m_categoryIndex]),
            kIconsPerPage
        );
        if (m_page + 1 < pages) {
            ++m_page;
            refreshGrid();
            refreshLabels();
        }
    }

    void onClearCategory(CCObject*) {
        m_selected[m_categoryIndex].clear();
        saveSelections();
        refreshGrid();
        refreshLabels();
    }

    void onSelectPage(CCObject*) {
        auto const& category = kCategories[m_categoryIndex];
        int iconCount = iconCountFor(category);
        auto [firstID, lastID] = fmaill::selection::pageRange(
            m_page,
            iconCount,
            kIconsPerPage
        );

        for (int id = firstID; id <= lastID; ++id) {
            if (!isUnlocked(id, category)) {
                m_selected[m_categoryIndex].insert(id);
            }
        }

        saveSelections();
        refreshGrid();
        refreshLabels();
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
        if (popup && popup->init(480.f, 300.f, "GJ_square01.png") && popup->setup()) {
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
            62,
            true,
            54.f,
            "GJ_button_04.png",
            0.72f
        );
        if (!buttonSprite) {
            return true;
        }

        auto label = CCLabelBMFont::create("ICONOS", "bigFont.fnt");
        label->setScale(0.30f);
        label->setPosition({ 31.f, 6.f });
        buttonSprite->addChild(label, 4);

        auto button = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(FmaillUtilitiesMenuLayer::onOpenIconSelector)
        );

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        menu->addChild(button);

        auto windowSize = CCDirector::sharedDirector()->getWinSize();
        button->setPosition({ 43.f, 43.f });
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
