#include "config.h"
#ifdef ENABLE_GUI

#include <Arduino.h>
#include "MenuItem.h"
#include "TextGUI.h"


MenuItem::MenuItem() {
 //   type = MenuItemType::ACTION;
 //   new (&command.action) MenuAction(nullptr);
    type = MenuItemType::INVALID;
    title = ""; 
}

MenuItem::MenuItem(MenuItem&& other) noexcept {
    moveFrom(std::move(other));
}

// Copy constructor
MenuItem::MenuItem(const MenuItem& other) {
    title = other.title;
    type = other.type; 
    switch (type) {
        case MenuItemType::VALUE:
        case MenuItemType::TOGGLE:
            new (&value.getter) ValueGetter(other.value.getter);
            new (&value.setter) ValueSetter(other.value.setter);
            value.min = other.value.min;
            value.max = other.value.max;
            value.step = other.value.step;
            value.labels = other.value.labels;
            value.labelCount = other.value.labelCount;
            break;
        case MenuItemType::SUBMENU:
            new (&submenu.generator) MenuGenerator(other.submenu.generator);
            break;
        case MenuItemType::ACTION:
            new (&command.action) MenuAction(other.command.action);
            break;
        case MenuItemType::CUSTOM:
            new (&custom.customDraw) decltype(custom.customDraw)(other.custom.customDraw);
            new (&custom.customAction) decltype(custom.customAction)(other.custom.customAction);
            break;
    }
}



// Copy assignment operator
MenuItem& MenuItem::operator=(const MenuItem& other) {
    if (this != &other) {
        destroyCurrent();
        new (this) MenuItem(other);
    }
    return *this;
}
// Move -"-
MenuItem& MenuItem::operator=(MenuItem&& other) noexcept {
    if (this != &other) {
        destroyCurrent();
        moveFrom(std::move(other));
    }
    return *this;
}


MenuItem::~MenuItem() {
    destroyCurrent();
}

void MenuItem::destroyCurrent() {
    switch (type) {
        case MenuItemType::VALUE:
        case MenuItemType::TOGGLE:
            value.getter.~ValueGetter();
            value.setter.~ValueSetter();
            break;

        case MenuItemType::SUBMENU:
            submenu.generator.~MenuGenerator();
            break;

        case MenuItemType::ACTION:
            command.action.~MenuAction();
            break;

        case MenuItemType::CUSTOM:
            custom.customDraw.~CustomDrawFn();
            custom.customAction.~CustomActionFn();
            break;

        default:
            break;
    }
    type = MenuItemType::INVALID;
}

void MenuItem::moveFrom(MenuItem&& other) {
    title = std::move(other.title);
    if (other.dynamicTitle) {
        dynamicTitle = std::move(other.dynamicTitle);
    }
    type = other.type;

    switch (type) {
        case MenuItemType::VALUE:
        case MenuItemType::TOGGLE:
            new (&value.getter) ValueGetter(std::move(other.value.getter));
            new (&value.setter) ValueSetter(std::move(other.value.setter));
            value.min = other.value.min;
            value.max = other.value.max;
            value.step = other.value.step;
            value.labels = other.value.labels;
            value.labelCount = other.value.labelCount;
            break;
        case MenuItemType::SUBMENU:
            new (&submenu.generator) MenuGenerator(std::move(other.submenu.generator));
            break;
        case MenuItemType::ACTION:
            new (&command.action) MenuAction(std::move(other.command.action));
            break;
        case MenuItemType::CUSTOM:
            new (&custom.customDraw) decltype(custom.customDraw)(std::move(other.custom.customDraw));
            new (&custom.customAction) decltype(custom.customAction)(std::move(other.custom.customAction));

            break;
    }
    // Safe fallback for moved-from

    other.destroyCurrent();  // Safe reset
}


// Factory methods
MenuItem MenuItem::Submenu(const String& title, MenuGenerator generator) {
    MenuItem item{};
    item.title = title;
    item.type = MenuItemType::SUBMENU;
    new (&item.submenu.generator) MenuGenerator(std::move(generator));
    return item;
}

MenuItem MenuItem::Submenu(std::function<String()> dynamicTitleFn, MenuGenerator generator) {
    MenuItem item{};
    item.dynamicTitle = std::move(dynamicTitleFn);
    item.title = item.dynamicTitle();  // <-- critical fix
    item.type = MenuItemType::SUBMENU;
    new (&item.submenu.generator) MenuGenerator(std::move(generator));
    return item;
}

MenuItem MenuItem::Action(const String& title, MenuAction action ) {
    MenuItem item{};
    item.title = title; 
    item.type = MenuItemType::ACTION;
    new (&item.command.action) MenuAction(std::move(action));
    return item;
}

MenuItem MenuItem::Toggle(const String& title, ValueGetter getter, ValueSetter setter) {
    MenuItem item;
    item.title = title;
    item.type = MenuItemType::TOGGLE;

    new (&item.value.getter) ValueGetter(std::move(getter));
    new (&item.value.setter) ValueSetter(std::move(setter));

    item.value.min = 0;
    item.value.max = 1;
    item.value.step = 1;

    return item;
}

MenuItem MenuItem::Value(const String& title, ValueGetter getter, ValueSetter setter,
                         int min, int max, int step) {
    MenuItem item;
    item.title = title;
    item.type = MenuItemType::VALUE;

    // Proper placement new construction for std::function inside union
    new (&item.value.getter) ValueGetter(std::move(getter));
    new (&item.value.setter) ValueSetter(std::move(setter));

    item.value.min = min;
    item.value.max = max;
    item.value.step = step;
    item.value.labels = nullptr;
    item.value.labelCount = 0;
    
    return item;
}

MenuItem MenuItem::Custom(const String& title, 
                          std::function<void(TextGUI&, U8G2&, int, int)> drawFn,
                          std::function<bool(TextGUI&, int)> action) {
    MenuItem item{};
    item.title = title;
    item.type = MenuItemType::CUSTOM;
    new (&item.custom.customDraw) decltype(item.custom.customDraw)(std::move(drawFn));
    if (action) {
        new (&item.custom.customAction) decltype(item.custom.customAction)(std::move(action));
    }
    return item;
}


MenuItem MenuItem::Option(const String& title, ValueGetter getter, ValueSetter setter, const std::vector<String>& labels) {
    MenuItem item;
    item.title = title;
    item.type = MenuItemType::VALUE;

    new (&item.value.getter) ValueGetter(std::move(getter));
    new (&item.value.setter) ValueSetter(std::move(setter));
    item.value.min = 0;
    item.value.max = labels.size() - 1;
    item.value.step = 1;
    item.value.labels = const_cast<String*>(labels.data());  // Safe if labels outlives menu
    item.value.labelCount = labels.size();

    return item;
}

#endif
