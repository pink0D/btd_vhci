/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */
#if !defined(__HIDBOOT_H__)
#define __HIDBOOT_H__

#include "extras.h"

#define UHS_HID_BOOT_KEY_ZERO           0x27
#define UHS_HID_BOOT_KEY_ENTER          0x28
#define UHS_HID_BOOT_KEY_SPACE          0x2c
#define UHS_HID_BOOT_KEY_CAPS_LOCK      0x39
#define UHS_HID_BOOT_KEY_SCROLL_LOCK    0x47
#define UHS_HID_BOOT_KEY_NUM_LOCK       0x53
#define UHS_HID_BOOT_KEY_ZERO2          0x62
#define UHS_HID_BOOT_KEY_PERIOD         0x63

// Don't worry, GCC will optimize the result to a final value.
#define bitsEndpoints(p) ((((p) & USB_HID_PROTOCOL_KEYBOARD)? 2 : 0) | (((p) & USB_HID_PROTOCOL_MOUSE)? 1 : 0))
#define totalEndpoints(p) ((bitsEndpoints(p) == 3) ? 3 : 2)
#define epMUL(p) ((((p) & USB_HID_PROTOCOL_KEYBOARD)? 1 : 0) + (((p) & USB_HID_PROTOCOL_MOUSE)? 1 : 0))

// Already defined in hid.h
// #define HID_MAX_HID_CLASS_DESCRIPTORS 5

/* Protocol Selection */
#define USB_HID_BOOT_PROTOCOL                   0x00
#define HID_RPT_PROTOCOL                        0x01

struct MOUSEINFO {

        struct {
                uint8_t bmLeftButton : 1;
                uint8_t bmRightButton : 1;
                uint8_t bmMiddleButton : 1;
                uint8_t bmDummy : 5;
        };
        int8_t dX;
        int8_t dY;
};

class HIDReportParser {
public:
        virtual void Parse(bool is_rpt_id, uint8_t len, uint8_t *buf) = 0;
};

class MouseReportParser : public HIDReportParser {

        union {
                MOUSEINFO mouseInfo;
                uint8_t bInfo[sizeof (MOUSEINFO)];
        } prevState;

public:
        void Parse(bool is_rpt_id, uint8_t len, uint8_t *buf);

protected:

        virtual void OnMouseMove(MOUSEINFO *mi __attribute__((unused))) {
        };

        virtual void OnLeftButtonUp(MOUSEINFO *mi __attribute__((unused))) {
        };

        virtual void OnLeftButtonDown(MOUSEINFO *mi __attribute__((unused))) {
        };

        virtual void OnRightButtonUp(MOUSEINFO *mi __attribute__((unused))) {
        };

        virtual void OnRightButtonDown(MOUSEINFO *mi __attribute__((unused))) {
        };

        virtual void OnMiddleButtonUp(MOUSEINFO *mi __attribute__((unused))) {
        };

        virtual void OnMiddleButtonDown(MOUSEINFO *mi __attribute__((unused))) {
        };
};

struct MODIFIERKEYS {
        uint8_t bmLeftCtrl : 1;
        uint8_t bmLeftShift : 1;
        uint8_t bmLeftAlt : 1;
        uint8_t bmLeftGUI : 1;
        uint8_t bmRightCtrl : 1;
        uint8_t bmRightShift : 1;
        uint8_t bmRightAlt : 1;
        uint8_t bmRightGUI : 1;
};

struct KBDINFO {

        struct {
                uint8_t bmLeftCtrl : 1;
                uint8_t bmLeftShift : 1;
                uint8_t bmLeftAlt : 1;
                uint8_t bmLeftGUI : 1;
                uint8_t bmRightCtrl : 1;
                uint8_t bmRightShift : 1;
                uint8_t bmRightAlt : 1;
                uint8_t bmRightGUI : 1;
        };
        uint8_t bReserved;
        uint8_t Keys[6];
};

struct KBDLEDS {
        uint8_t bmNumLock : 1;
        uint8_t bmCapsLock : 1;
        uint8_t bmScrollLock : 1;
        uint8_t bmCompose : 1;
        uint8_t bmKana : 1;
        uint8_t bmReserved : 3;
};

class KeyboardReportParser : public HIDReportParser {
        static const uint8_t numKeys[10];
        static const uint8_t symKeysUp[12];
        static const uint8_t symKeysLo[12];
        static const uint8_t padKeys[5];

protected:

        union {
                KBDINFO kbdInfo;
                uint8_t bInfo[sizeof (KBDINFO)];
        } prevState;

        union {
                KBDLEDS kbdLeds;
                uint8_t bLeds;
        } kbdLockingKeys;

        uint8_t OemToAscii(uint8_t mod, uint8_t key);

public:

        KeyboardReportParser() {
                kbdLockingKeys.bLeds = 0;
        };

        void Parse(bool is_rpt_id, uint8_t len, uint8_t *buf);

protected:

        virtual uint8_t HandleLockingKeys(uint8_t key) {
                //uint8_t old_keys = kbdLockingKeys.bLeds;

                switch(key) {
                        case UHS_HID_BOOT_KEY_NUM_LOCK:
                                kbdLockingKeys.kbdLeds.bmNumLock = ~kbdLockingKeys.kbdLeds.bmNumLock;
                                break;
                        case UHS_HID_BOOT_KEY_CAPS_LOCK:
                                kbdLockingKeys.kbdLeds.bmCapsLock = ~kbdLockingKeys.kbdLeds.bmCapsLock;
                                break;
                        case UHS_HID_BOOT_KEY_SCROLL_LOCK:
                                kbdLockingKeys.kbdLeds.bmScrollLock = ~kbdLockingKeys.kbdLeds.bmScrollLock;
                                break;
                }

                //if(old_keys != kbdLockingKeys.bLeds) {
                //        uint8_t lockLeds = kbdLockingKeys.bLeds;
                //        return (hid->SetReport(0, 0/*hid->GetIface()*/, 2, 0, 1, &lockLeds));
                //}

                return 0;
        };

        virtual void OnControlKeysChanged(uint8_t before __attribute__((unused)), uint8_t after __attribute__((unused))) {
        };

        virtual void OnKeyDown(uint8_t mod __attribute__((unused)), uint8_t key __attribute__((unused))) {
        };

        virtual void OnKeyUp(uint8_t mod __attribute__((unused)), uint8_t key __attribute__((unused))) {
        };

        virtual const uint8_t *getNumKeys() {
                return numKeys;
        };

        virtual const uint8_t *getSymKeysUp() {
                return symKeysUp;
        };

        virtual const uint8_t *getSymKeysLo() {
                return symKeysLo;
        };

        virtual const uint8_t *getPadKeys() {
                return padKeys;
        };
};

#endif 
