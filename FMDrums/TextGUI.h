#pragma once
#include "config.h"

#ifdef ENABLE_GUI

#include <Arduino.h> 
#include <U8g2lib.h>
#include <functional>
#include <vector>
#include <memory>
#include "button.h"
#include "encoder.h" 
#include "MenuItem.h" 


// MenuContext declaration
struct MenuContext {
    std::vector<MenuItem> items;
    String title;
    
    int parentIndex = -1;
    int scrollPosition = 0;
    int cursorPosition = 0;
    const MenuItem* sourceItem = nullptr; 
    int midiNote = -1;
}; 

class TextGUI {
public:
    TextGUI();
    void begin();
    void splash();
    void startMenu();
    void process();
    void draw();
    void fullUpdate();
    void message(const String& str);
    inline void pause(uint32_t count) { pause_counter = count; };
    void updateParentItemTitle(const String& newTitle);

    // Navigation methods
    void enterSubmenu(std::vector<MenuItem>&& items, const String& title = "",  const MenuItem* sourceItem = nullptr, int midiNote = -1);
    void goBack();
    void refreshCurrentMenu();

    inline int getCurrentNote() const  ;

    // Access to hardware
    U8G2& getDisplay() { return display; } 

    // Encoder/button handling , they are public so that they can be handled externally
    uint8_t encA;
    uint8_t encB;
    uint8_t btnState0;
    uint8_t btnState1;
    uint8_t btnState2;

private:
    volatile int32_t pause_counter = 0; // count down to 0 then process again
    bool inited = false; 
    MuxEncoder encoder;
    MuxButton button0;
    MuxButton button1;
    MuxButton button2;
    
    U8_OBJECT display;
    bool editingValue = false;
    // Menu state
    std::vector<MenuContext> menuStack;
    // int cursorPos = 0;
    bool needsRedraw = true;
    int cur_xt = 0;
    int cur_yt = 0;

    // Initial menu setup
    std::vector<MenuItem> createRootMenu( );

    // Event handlers
    void onEncoderTurn(int direction);
    void onButton0Event(MuxButton::btnEvents evt);
    void onButton1Event(MuxButton::btnEvents evt);
    void onButton2Event(MuxButton::btnEvents evt);
    
    // Rendering
    void renderDisplay();
    void renderMenu();
    void renderStatusBar();
    int partialDisplayUpdate();
    
    // Value adjustment helpers
    void adjustValue(int direction, MenuItem& item);

    inline void safeDrawUTF8(int x, int y, const char* str) {
        if (!str || !*str) str = "(null)";
        display.drawUTF8(x, y, str);
    }

};

#endif