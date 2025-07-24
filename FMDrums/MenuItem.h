#pragma once
#include "config.h"

#ifdef ENABLE_GUI

#include <Arduino.h> 
#include <U8g2lib.h>


// MenuItemType enum
enum class MenuItemType {
    INVALID,
    SUBMENU,
    ACTION,
    TOGGLE,
    VALUE,
    CUSTOM
};

class TextGUI;  // forward declaration


class MenuItem {
public:
    using CustomDrawFn = std::function<void(TextGUI&, U8G2&, int, int)>; 
    using CustomActionFn = std::function<bool(TextGUI&, int)>;
    using MenuAction = std::function<void(TextGUI&)>;
    using ValueGetter = std::function<int()>;
    using ValueSetter = std::function<void(int)>;
    using MenuGenerator = std::function<std::vector<MenuItem>()>;

    MenuItem();
    MenuItem(const MenuItem&);
    MenuItem& operator=(const MenuItem&);
    MenuItem(MenuItem&&) noexcept;
    MenuItem& operator=(MenuItem&&) noexcept;
    ~MenuItem();

    static MenuItem Action(const String&, MenuAction );
    static MenuItem Value(const String&, ValueGetter, ValueSetter, int, int, int);
    static MenuItem Toggle(const String&, ValueGetter, ValueSetter);
    static MenuItem Submenu(const String&, MenuGenerator);
    static MenuItem Submenu(std::function<String()> titleFn, std::function<std::vector<MenuItem>()> submenuFn);

    static MenuItem Custom(const String& title,
                       std::function<void(TextGUI&, U8G2&, int, int)> drawFn,
                       std::function<bool(TextGUI&, int)> action);
    static MenuItem Option(const String& title, ValueGetter getter, ValueSetter setter, const std::vector<String>& labels);

    String title;
    std::function<String()> dynamicTitle;
    MenuItemType type;

    union {
        struct {
            ValueGetter getter;
            ValueSetter setter;
            int min, max, step;
            String* labels = nullptr; // For Option
            int labelCount = 0;

        } value;

        struct {
            MenuGenerator generator;
        } submenu;

        struct {
            MenuAction action;
        } command;

        struct {
            std::function<void(TextGUI&, U8G2&, int, int)> customDraw;
            std::function<bool(TextGUI&, int)> customAction;
        } custom;

        struct {
            std::function<String()> dynamicTitle;
        } titleInfo;
    };

    void destroyCurrent();
    void moveFrom(MenuItem&&);
};

#endif