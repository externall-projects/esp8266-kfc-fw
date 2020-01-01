/**
* Author: sascha_lammers@gmx.de
*/

#pragma once

#include <Arduino_compat.h>
#include <vector>

#ifndef DEBUG_BOOTSTRAP_MENU
#define DEBUG_BOOTSTRAP_MENU                    0
#endif

#include "push_pack.h"

// 11 byte RAM usage per menu entry when using PROGMEM strings

class BootstrapMenu {
public:
    typedef uint8_t menu_item_id_t;

    typedef struct __attribute__packed__ {
        menu_item_id_t id;
        menu_item_id_t parentMenuId;
        uint8_t label_progmem : 1;
        uint8_t URI_progmem : 1;
    } MenuItemFlags_t;

    class Item {
    public:
        Item() : _label(nullptr), _URI(nullptr)
        {
            memset(&_flags, 0, sizeof(_flags));
            _flags.id = INVALID_ID;
            _flags.parentMenuId = INVALID_ID;
        }
        Item(BootstrapMenu &menu) : Item()
        {
            _flags.id = menu._getUnqiueId();
        }

        // needs to be called to free memory
        void _destroy() {
            clearLabel();
            clearURI();
        }

        void clearLabel() {
            if (_flags.label_progmem == 0 && _label) {
                free(_label);
            }
            _label = nullptr;
        }
        void clearURI() {
            if (_flags.URI_progmem == 0 && _URI) {
                free(_URI);
            }
            _URI = nullptr;
        }

        void setLabel(const char *label) {
            clearLabel();
            _label = strdup(label);
            _flags.label_progmem = 0;
        }

        inline void setLabel(const String &label) {
            setLabel(label.c_str());
        }

        void setLabel(const __FlashStringHelper *label) {
            clearLabel();
            _label = (char *)label;
            _flags.label_progmem = 1;
        }

        bool hasLabel() const {
            return _label != nullptr;
        }

        void setURI(const __FlashStringHelper *uri) {
            clearURI();
            _URI = (char *)uri;
            _flags.URI_progmem = 1;
        }

        void setURI(const char *uri) {
            clearURI();
            _URI = strdup(uri);
            _flags.URI_progmem = 0;
        }

        inline void setURI(const String &uri) {
            setURI(uri.c_str());
        }

        bool hashURI() const {
            return _URI != nullptr;
        }

        String getLabel() const {
            if (_label) {
                if (_flags.label_progmem) {
                    return String(FPSTR(_label));
                }
                else {
                    return String(_label);
                }
            }
            return String();
        }

        String getURI() const {
            if (_URI) {
                if (_flags.URI_progmem) {
                    return String(FPSTR(_URI));
                }
                else {
                    return String(_URI);
                }
            }
            return String();
        }

        void setParentMenuId(menu_item_id_t menuId) {
            _flags.parentMenuId = menuId;
        }

        menu_item_id_t getParentMenuId() const {
            return _flags.parentMenuId;
        }

        menu_item_id_t getId() const {
            return _flags.id;
        }

    private:
        char *_label;
        char *_URI;
        MenuItemFlags_t _flags;
    };

    typedef std::vector<Item> ItemsVector;
    typedef std::vector<Item>::iterator ItemsVectorIterator;

    static const menu_item_id_t INVALID_ID = ~0;

public:
    BootstrapMenu();
    ~BootstrapMenu();

    menu_item_id_t addMenu(const __FlashStringHelper* label, menu_item_id_t afterId = 0);
    menu_item_id_t addMenu(const String &label, menu_item_id_t afterId = 0);
    menu_item_id_t addSubMenu(const __FlashStringHelper *label, const __FlashStringHelper *uri, menu_item_id_t parentMenuId, menu_item_id_t afterId = 0);
    menu_item_id_t addSubMenu(const String& label, const __FlashStringHelper *uri, menu_item_id_t parentMenuId, menu_item_id_t afterId = 0);
    menu_item_id_t addSubMenu(const __FlashStringHelper *label, const String &uri, menu_item_id_t parentMenuId, menu_item_id_t afterId = 0);
    menu_item_id_t addSubMenu(const String &label, const String &uri, menu_item_id_t parentMenuId, menu_item_id_t afterId = 0);

    menu_item_id_t findMenuByLabel(const String& label, menu_item_id_t menuId = INVALID_ID) const;
    menu_item_id_t findMenuByURI(const String& uri, menu_item_id_t menuId = INVALID_ID) const;
    // get number of menu items
    menu_item_id_t getItemCount(menu_item_id_t menuId) const;

    // create menu item
    void html(Print& output, menu_item_id_t menuId, bool dropDown);
    // create main menu
    void html(Print& output);

    // create sub menu
    void htmlSubMenu(Print& output, menu_item_id_t menuId, uint8_t active);

    // get menu item by id
    ItemsVectorIterator getItem(menu_item_id_t menuId);

// public:
//     void createCache();
// private:
//     String _cacheFilename;

private:
    friend Item;

    menu_item_id_t _add(Item &item, menu_item_id_t afterId);
    menu_item_id_t _getUnqiueId();


private:
    ItemsVector _items;
    menu_item_id_t _unqiueId;
};

#include "pop_pack.h"
