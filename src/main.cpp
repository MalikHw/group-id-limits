#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/modify/EditLevelLayer.hpp>
#include <Geode/modify/SetGroupIDLayer.hpp>
#include <alphalaneous.level-storage-api/include/LevelStorageAPI.hpp>

using namespace geode::prelude;

class GroupRangePopup : public geode::Popup {
protected:
    TextInput* m_fromInput;
    TextInput* m_toInput;

    bool init() {
        if (!Popup::init(420.f, 280.f))
            return false;
        this->setTitle("Set Group ID Range");

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        m_fromInput = TextInput::create(180.f, "From...", "bigFont.fnt");
        m_fromInput->setCommonFilter(CommonFilter::Int);
        m_fromInput->setMaxCharCount(4);
        m_fromInput->setPosition(winSize / 2 - CCPoint{ 100.f, 0.f});
        m_mainLayer->addChild(m_fromInput);

        m_toInput = TextInput::create(180.f, "To...", "bigFont.fnt");
        m_toInput->setCommonFilter(CommonFilter::Int);
        m_toInput->setMaxCharCount(4);
        m_toInput->setPosition(winSize / 2 + CCPoint{ 100.f, 0.f});
        m_mainLayer->addChild(m_toInput);

        // Load existing values
        if (auto lel = LevelEditorLayer::get()) {
            auto from = alpha::level_storage::getSavedValue<int>(lel, "group-range-from");
            auto to = alpha::level_storage::getSavedValue<int>(lel, "group-range-to");
            if (from > 0) m_fromInput->setString(std::to_string(from));
            if (to > 0) m_toInput->setString(std::to_string(to));
        }

        auto buttonMenu = CCMenu::create();
        buttonMenu->setPosition(winSize / 2 - CCPoint{ 0.f, 80.f});
        buttonMenu->setLayout(RowLayout::create()->setGap(20.f));

        auto setBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Set", "bigFont.fnt", "GJ_button_01.png"),
            this,
            menu_selector(GroupRangePopup::onSet)
        );
        buttonMenu->addChild(setBtn);

        auto clearBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Clear", "bigFont.fnt", "GJ_button_06.png"),
            this,
            menu_selector(GroupRangePopup::onClear)
        );
        buttonMenu->addChild(clearBtn);

        auto cancelBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Cancel", "bigFont.fnt", "GJ_button_05.png"),
            this,
            menu_selector(GroupRangePopup::onCancel)
        );
        buttonMenu->addChild(cancelBtn);

        buttonMenu->updateLayout();
        m_mainLayer->addChild(buttonMenu);

        return true;
    }

    void onSet(CCObject*) {
        int from = std::stoi(m_fromInput->getString().empty() ? "0" : m_fromInput->getString());
        int to = std::stoi(m_toInput->getString().empty() ? "0" : m_toInput->getString());

        if (from <= 0 || to <= 0) {
            FLAlertLayer::create("Error", "Both values must be greater than 0!", "OK")->show();
            return;
        }

        if (from > to) {
            FLAlertLayer::create("Error", "From value must be less than To value!", "OK")->show();
            return;
        }

        if (from > 9999 || to > 9999) {
            FLAlertLayer::create("Error", "Values cannot exceed 9999!", "OK")->show();
            return;
        }

        if (auto lel = LevelEditorLayer::get()) {
            alpha::level_storage::setSavedValue(lel, "group-range-from", from);
            alpha::level_storage::setSavedValue(lel, "group-range-to", to);
        }

        this->onClose(nullptr);
    }

    void onClear(CCObject*) {
        m_fromInput->setString("");
        m_toInput->setString("");
        if (auto lel = LevelEditorLayer::get()) {
            alpha::level_storage::setSavedValue(lel, "group-range-from", 0);
            alpha::level_storage::setSavedValue(lel, "group-range-to", 0);
        }
    }

    void onCancel(CCObject*) {
        this->onClose(nullptr);
    }

public:
    static GroupRangePopup* create() {
        auto ret = new GroupRangePopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(MyEditLevelLayer, EditLevelLayer) {
    void onOpenRangePopup(CCObject*) {
        GroupRangePopup::create()->show();
    }

    bool init(GJGameLevel* level) {
        if (!EditLevelLayer::init(level)) return false;

        auto spr = CCSprite::create("button.png"_spr);
        if (!spr) {
            spr = CCSprite::createWithSpriteFrameName("GJ_editBtn_001.png");
        }
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MyEditLevelLayer::onOpenRangePopup));
        m_buttonMenu->addChild(btn);
        m_buttonMenu->updateLayout();

        return true;
    }
};

class $modify(MySetGroupIDLayer, SetGroupIDLayer) {
    struct Fields {
        int m_rangeFrom = 0;
        int m_rangeTo = 0;
    };

    bool init(GameObject* obj, CCArray* objs) {
        if (!SetGroupIDLayer::init(obj, objs)) return false;

        if (auto lel = LevelEditorLayer::get()) {
            m_fields->m_rangeFrom = alpha::level_storage::getSavedValue<int>(lel, "group-range-from");
            m_fields->m_rangeTo = alpha::level_storage::getSavedValue<int>(lel, "group-range-to");
        }

        return true;
    }

    void addGroupID(int id) {
        if (m_fields->m_rangeFrom > 0 && m_fields->m_rangeTo > 0) {
            if (id < m_fields->m_rangeFrom || id > m_fields->m_rangeTo) {
                auto alert = FLAlertLayer::create(
                    this,
                    "Warning",
                    fmt::format("Group ID {} is outside the set range ({} - {}). Do you want to proceed?", 
                        id, m_fields->m_rangeFrom, m_fields->m_rangeTo),
                    "Cancel", "Ignore"
                );
                alert->setTag(id);
                alert->show();
                return;
            }
        }
        SetGroupIDLayer::addGroupID(id);
    }

    void FLAlert_Clicked(FLAlertLayer* alert, bool btn2) {
        if (alert->getTag() > 0 && btn2) {
            SetGroupIDLayer::addGroupID(alert->getTag());
        }
    }

    void onNextFreeEditorLayer1(CCObject* sender) {
        if (m_fields->m_rangeFrom > 0 && m_fields->m_rangeTo > 0) {
            if (auto lel = LevelEditorLayer::get()) {
                int nextFree = findNextFreeInRange(lel, m_fields->m_rangeFrom, m_fields->m_rangeTo);
                if (nextFree != -1) {
                    m_groupIDValue = nextFree;
                    updateGroupIDLabel();
                    return;
                } else {
                    auto alert = FLAlertLayer::create(
                        this,
                        "Warning",
                        fmt::format("No free group IDs available in the range ({} - {}). Do you want to ignore the range?", 
                            m_fields->m_rangeFrom, m_fields->m_rangeTo),
                        "Cancel", "Ignore"
                    );
                    alert->setTag(-2);
                    alert->show();
                    return;
                }
            }
        }
        SetGroupIDLayer::onNextFreeEditorLayer1(sender);
    }

    void onNextFreeEditorLayer2(CCObject* sender) {
        if (m_fields->m_rangeFrom > 0 && m_fields->m_rangeTo > 0) {
            if (auto lel = LevelEditorLayer::get()) {
                int nextFree = findNextFreeInRange(lel, m_fields->m_rangeFrom, m_fields->m_rangeTo);
                if (nextFree != -1) {
                    m_groupIDValue = nextFree;
                    updateGroupIDLabel();
                    return;
                } else {
                    auto alert = FLAlertLayer::create(
                        this,
                        "Warning",
                        fmt::format("No free group IDs available in the range ({} - {}). Do you want to ignore the range?", 
                            m_fields->m_rangeFrom, m_fields->m_rangeTo),
                        "Cancel", "Ignore"
                    );
                    alert->setTag(-3);
                    alert->show();
                    return;
                }
            }
        }
        SetGroupIDLayer::onNextFreeEditorLayer2(sender);
    }

    int findNextFreeInRange(LevelEditorLayer* lel, int from, int to) {
        auto usedGroups = getUsedGroupIDs(lel);
        for (int i = from; i <= to; i++) {
            if (!usedGroups.count(i)) {
                return i;
            }
        }
        return -1;
    }

    std::unordered_set<int> getUsedGroupIDs(LevelEditorLayer* lel) {
        std::unordered_set<int> used;
        auto objects = lel->getAllObjects();
        for (unsigned int i = 0; i < objects->count(); ++i) {
            auto obj = static_cast<GameObject*>(objects->objectAtIndex(i));
            auto groups = obj->m_groups;
            for (unsigned int j = 0; j < groups->count(); ++j) {
                auto group = static_cast<CCInteger*>(groups->objectAtIndex(j));
                used.insert(group->getValue());
            }
        }
        return used;
    }
};
// rip