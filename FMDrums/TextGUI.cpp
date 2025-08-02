#include "config.h"

#ifdef ENABLE_GUI 
#include "TextGUI.h"
#include <U8g2lib.h>  
#include "MenuStructure.h"
#include <new> 
#include "esp_log.h"
#include <functional>  



static const char* TAG = "TextGUI";
 
TextGUI::TextGUI() :
      display(U8_ROTATE, U8X8_PIN_NONE, DISPLAY_SCL, DISPLAY_SDA)
    , encA(0), encB(0), btnState0(0), btnState1(0), btnState2(0)
    {}


void TextGUI::begin() {
    pinMode(ENC0_A_PIN, SIG_INPUT_MODE);
    pinMode(ENC0_B_PIN, SIG_INPUT_MODE);
    pinMode(BTN0_PIN, SIG_INPUT_MODE);
    pinMode(BTN1_PIN, SIG_INPUT_MODE);
    pinMode(BTN2_PIN, SIG_INPUT_MODE);
    pinMode(DISPLAY_SDA, OUTPUT);
    pinMode(DISPLAY_SCL, OUTPUT);
    
    display.begin(); 
    display.setContrast(255);
    display.setFont(u8g2_font_6x12_m_symbols);
    display.enableUTF8Print();
    display.setDrawColor(1);
    display.setFontMode(0);
    display.setFontPosTop();

    inited = true;
}

void TextGUI::startMenu() {
    if (!inited) return;

    encoder.bind(0, &encA, &encB, [this](int, int dir) {
        this->onEncoderTurn(dir);
    }, MuxEncoder::MODE_QUAD_STEP , (gpio_num_t)ENC0_A_PIN);

    button0.bind(0, &btnState0, [this](int, MuxButton::btnEvents evt) {
        this->onButton0Event(evt);
    });
    
    button1.bind(1, &btnState1, [this](int, MuxButton::btnEvents evt) {
        this->onButton1Event(evt);
    });

    button2.bind(2, &btnState2, [this](int, MuxButton::btnEvents evt) {
        this->onButton2Event(evt);
    });
    enterSubmenu(MenuStructure::createRootMenu(), "Main Menu"); 
}

void TextGUI::process() {
    if (pause_counter > 0) {
        pause_counter--;
        return;
    }

    // Read GUI input
    encA = digitalRead(ENC0_A_PIN);
    encB = digitalRead(ENC0_B_PIN);

    btnState0 = digitalRead(BTN0_PIN);
    btnState1 = digitalRead(BTN1_PIN);
    btnState2 = digitalRead(BTN2_PIN);
    
    encoder.process();
    button0.process();
    button1.process();
    button2.process();
}

void TextGUI::draw() {
    if (pause_counter > 0) {
        pause_counter--;
        return;
    }
    
    if (partialDisplayUpdate() == 0) {
        renderDisplay();
    }
}

void TextGUI::renderDisplay() {
    display.clearBuffer();
    renderMenu();
    renderStatusBar();
}

void TextGUI::fullUpdate() {
    if (!inited) return;
    display.clearBuffer();
    renderMenu();
    renderStatusBar();
    display.sendBuffer();
}


void TextGUI::renderMenu() {
    if (menuStack.empty()) return;

    auto& current = menuStack.back();
    const int cursor = current.cursorPosition;
    const int total = current.items.size();
    const uint8_t lineHeight = 11;
    const uint8_t maxVisible = 5;

    // Adjust scroll to keep cursor visible
    if (cursor < current.scrollPosition) {
        current.scrollPosition = cursor;
    } else if (cursor >= current.scrollPosition + maxVisible) {
        current.scrollPosition = cursor - maxVisible + 1;
    }

    // Clamp scroll
    if (current.scrollPosition > total - maxVisible) {
        current.scrollPosition = std::max(0, total - maxVisible);
    }

    int start = current.scrollPosition;
    int end = std::min(start + maxVisible, total);

    uint8_t y = 0;

    // Draw title if available
    if (!current.title.isEmpty()) {
        safeDrawUTF8(0, y, current.title.c_str());
        y += lineHeight;
    }

    for (int i = start; i < end; ++i) {
        const auto& item = current.items[i];
        const bool isCursor = (i == cursor);

        if (isCursor) {
            display.drawStr(0, y, ">");
        }

        // Title resolution
        String label;
        if (item.dynamicTitle) { 
            label = item.dynamicTitle();
        } else {
            label = item.title;
        }

        switch (item.type) {
            case MenuItemType::TOGGLE: {
                safeDrawUTF8(8, y, label.c_str());
                const char* stateStr = item.value.getter() ? "[X] " : "[ ] ";
                safeDrawUTF8(display.getDisplayWidth() - display.getUTF8Width(stateStr), y, stateStr);
                break;
            }

            case MenuItemType::VALUE: {
                if (!item.value.getter) {
                    break;
                }
                safeDrawUTF8(8, y, label.c_str());
                int value = 0;
                try {
                    value = item.value.getter(); 
                } catch (const std::bad_function_call& e) {
                    ESP_LOGE("GUI", "Bad function call in item getter: %s", e.what());
                    break;
                }
                String valStr;

                if (item.value.labels && item.value.labelCount > 0) {
                    if (value >= 0 && value < item.value.labelCount) {
                        const String& valLabel = item.value.labels[value];
                        valStr = isCursor && editingValue ? ">" + valLabel + "<" : " " + valLabel + " ";
                    } else {
                        ESP_LOGW(TAG, "Invalid label index: value=%d, labelCount=%d", value, item.value.labelCount);
                        valStr = "?";
                    }
                } else {
                    valStr = isCursor && editingValue ? ">" + String(value) + "<" : " " + String(value) + " ";
                }

                safeDrawUTF8(display.getDisplayWidth() - display.getUTF8Width(valStr.c_str()), y, valStr.c_str());
                break;
            }

            case MenuItemType::CUSTOM: {
                if (!label.isEmpty()) safeDrawUTF8(8, y, label.c_str());
                if (item.custom.customDraw) {
                    item.custom.customDraw(*this, display, 8, y + lineHeight); // offset for multiline
                }
                break;
            }

            default: { // SUBMENU, ACTION, OPTION etc.
                safeDrawUTF8(8, y, label.c_str());
                if (item.type == MenuItemType::SUBMENU) {
                    safeDrawUTF8(display.getDisplayWidth() - 8, y, ">");
                }
                break;
            }
        }

        y += lineHeight;
    }
}

void TextGUI::updateParentItemTitle(const String& newTitle) {
    if (menuStack.size() >= 2) {
        auto& parent = menuStack[menuStack.size() - 2];
        if (parent.cursorPosition >= 0 && parent.cursorPosition < parent.items.size()) {
            parent.items[parent.cursorPosition].title = newTitle;
        }
    }
}


void TextGUI::renderStatusBar() {
    if (!inited) return;
    char buf[49];
   // synth.getActivityString(buf);
    // safeDrawUTF8(14, display.getDisplayHeight() - 9, buf);
}

void TextGUI::enterSubmenu(std::vector<MenuItem>&& items, const String& title, const MenuItem* sourceItem, int selectedNote) {
    MenuContext newContext;
    newContext.items = std::move(items);
    newContext.title = title;
    newContext.sourceItem = sourceItem;
    newContext.midiNote = selectedNote;

    if (!menuStack.empty()) {
        newContext.parentIndex = menuStack.back().cursorPosition;
    } else {
        newContext.parentIndex = 0;  // or -1 if you want to mark as root
    }

    menuStack.push_back(std::move(newContext));
    needsRedraw = true;
    ESP_LOGI("GUI", "enterSubmenu: title='%s', midiNote=%d", title.c_str(), selectedNote);
}

void TextGUI::goBack() {
    if (menuStack.size() > 1) {
        menuStack.pop_back();

        // Defensive check: reset any references to previous layer
        editingValue = false;
    }

    // Only refresh if stack is not empty
    if (!menuStack.empty()) {
        refreshCurrentMenu();
    }
}

void TextGUI::message(const String& str) {
    display.clearBuffer();
    safeDrawUTF8(0, display.getDisplayHeight() / 2, str.c_str());
    display.sendBuffer();
    pause(1000); // 1000 task cycles ~ 0.5sec
}


void TextGUI::refreshCurrentMenu() {
    if (menuStack.empty()) return;

    auto& ctx = menuStack.back();
    if (ctx.sourceItem && ctx.sourceItem->type == MenuItemType::SUBMENU) {
        ctx.items = ctx.sourceItem->submenu.generator();
        ctx.title = ctx.sourceItem->dynamicTitle ? ctx.sourceItem->dynamicTitle() : ctx.sourceItem->title;
    }
    needsRedraw = true;
}


void TextGUI::onButton1Event(MuxButton::btnEvents evt) {
    int note =  getCurrentNote();
    ESP_LOGD(TAG, "Button1 event: %d, note: %d", evt, note);
    if (note >= 0) {
        if (evt == MuxButton::EVENT_PRESS) {
            synth.handleNoteOn(note, 100);  // velocity 100 or as desired
        } else if (evt == MuxButton::EVENT_RELEASE) {
            synth.handleNoteOff(note);
        }
    }
}

void TextGUI::onButton2Event(MuxButton::btnEvents evt) {
    if (evt == MuxButton::EVENT_PRESS) {
        goBack();
    }
}

void TextGUI::onButton0Event(MuxButton::btnEvents evt) {
    if (menuStack.empty()) return;

    auto& current = menuStack.back();
    int& cursor = current.cursorPosition;

    if (cursor < 0 || cursor >= current.items.size()) return;
    auto& item = current.items[cursor];

    if (evt == MuxButton::EVENT_CLICK) {
        switch (item.type) {
            case MenuItemType::SUBMENU:
                ESP_LOGI("TextGUI", "Triggered submenu: %s",
                         item.title.length() ? item.title.c_str() : "<dynamic>");
                enterSubmenu(item.submenu.generator(),
                    item.dynamicTitle ? item.dynamicTitle() : item.title,
                    &item);
                break;

            case MenuItemType::ACTION:
                if (item.command.action) item.command.action(*this);
                break;

            case MenuItemType::TOGGLE:
                item.value.setter(!item.value.getter());
                needsRedraw = true;
                break;

            case MenuItemType::VALUE:
                editingValue = !editingValue;
                needsRedraw = true;
                break;

            case MenuItemType::CUSTOM:
                if (item.custom.customAction) {
                    bool stay = item.custom.customAction(*this, 0);
                    if (!stay) {
                        goBack();  // Only call goBack() if customAction wants to exit
                    }
                }
                break;

            default:
                break;
        }
    } else if (evt == MuxButton::EVENT_LONGPRESS) {
        goBack();
    }
}

void TextGUI::onEncoderTurn(int direction) {
    if (menuStack.empty()) return;

    auto& current = menuStack.back();
    int& cursor = current.cursorPosition;

    if (current.items.empty()) return;
    auto& item = current.items[cursor];

    if (item.type == MenuItemType::VALUE && editingValue) {
        int mux = 1;
        if (encoder.isHiSpeed()) {
            mux = (item.value.max - item.value.min) / 100 / item.value.step;
            if (mux < 2) mux = 2;
        }
        adjustValue(direction * mux, item);
        return;
    }

    if (item.type == MenuItemType::CUSTOM && item.custom.customAction) {
        item.custom.customAction(*this, direction);
        return;
    }

    // Normal navigation
    cursor = constrain(cursor + direction, 0, static_cast<int>(current.items.size()) - 1);

    // Scroll window update
    if (cursor < current.scrollPosition)
        current.scrollPosition = cursor;
    else if (cursor >= current.scrollPosition + 6)
        current.scrollPosition = cursor - 5;

    needsRedraw = true;
}


void TextGUI::adjustValue(int direction, MenuItem& item) {
    if (item.type != MenuItemType::VALUE || !item.value.setter) return;
    
    int current = item.value.getter();
    int newValue = current + (direction * item.value.step);
    newValue = constrain(newValue, item.value.min, item.value.max);
    
    if (newValue != current) {

        item.value.setter(newValue);
        needsRedraw = true;
        ESP_LOGI(TAG, "Value adjusted to: %d :", newValue);
    }

    ESP_LOGI(TAG, "New value: %d current %d max %d min %d", newValue, current, item.value.max, item.value.min);
}

int TextGUI::partialDisplayUpdate() {
    
    if (pause_counter > 0) {
        pause_counter--;
        return 1;
    }

    static const int send_tiles = 4;
    static const int block_h = display.getBufferTileHeight();
    static const int block_w = display.getBufferTileWidth();
    display.updateDisplayArea(cur_xt, cur_yt, send_tiles, 1);
    cur_xt += send_tiles;
    if (cur_xt >= block_w) {
        cur_xt = 0;
        cur_yt++;
    }
    cur_yt %= block_h;
    return cur_xt + cur_yt;
}

inline int TextGUI::getCurrentNote() const { 
    for (auto it = menuStack.rbegin(); it != menuStack.rend(); ++it) {
        if (it->midiNote >= 0) return it->midiNote;
    }
    const auto& ctx = menuStack.back();

    if (ctx.cursorPosition < 0 || ctx.cursorPosition >= static_cast<int>(ctx.items.size())) return -1;

    // to have some note sounding (useful in patches list 0-127) 
    return ctx.cursorPosition;
}

#endif