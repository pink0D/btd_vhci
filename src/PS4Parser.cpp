/* Copyright (C) 2014 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#include "PS4Parser.h"

enum DPADEnum {
        DPAD_UP = 0x0,
        DPAD_UP_RIGHT = 0x1,
        DPAD_RIGHT = 0x2,
        DPAD_RIGHT_DOWN = 0x3,
        DPAD_DOWN = 0x4,
        DPAD_DOWN_LEFT = 0x5,
        DPAD_LEFT = 0x6,
        DPAD_LEFT_UP = 0x7,
        DPAD_OFF = 0x8,
};

static const char *LOG_TAG = "PS4Parser";

int8_t PS4Parser::getButtonIndexPS4(ButtonEnum b) {
    const int8_t index = ButtonIndex(b);
    if ((uint8_t) index >= (sizeof(PS4_BUTTONS) / sizeof(PS4_BUTTONS[0]))) return -1;
    return index;
}

bool PS4Parser::checkDpad(ButtonEnum b) {
        switch (b) {
                case UP:
                        return ps4Data.btn.dpad == DPAD_LEFT_UP || ps4Data.btn.dpad == DPAD_UP || ps4Data.btn.dpad == DPAD_UP_RIGHT;
                case RIGHT:
                        return ps4Data.btn.dpad == DPAD_UP_RIGHT || ps4Data.btn.dpad == DPAD_RIGHT || ps4Data.btn.dpad == DPAD_RIGHT_DOWN;
                case DOWN:
                        return ps4Data.btn.dpad == DPAD_RIGHT_DOWN || ps4Data.btn.dpad == DPAD_DOWN || ps4Data.btn.dpad == DPAD_DOWN_LEFT;
                case LEFT:
                        return ps4Data.btn.dpad == DPAD_DOWN_LEFT || ps4Data.btn.dpad == DPAD_LEFT || ps4Data.btn.dpad == DPAD_LEFT_UP;
                default:
                        return false;
        }
}

bool PS4Parser::getButtonPress(ButtonEnum b) {
        const int8_t index = getButtonIndexPS4(b); if (index < 0) return 0;
        if (index <= LEFT) // Dpad
                return checkDpad(b);
        else
                return ps4Data.btn.val & (1UL << PS4_BUTTONS[index]);
}

bool PS4Parser::getButtonClick(ButtonEnum b) {
        const int8_t index = getButtonIndexPS4(b); if (index < 0) return 0;
        uint32_t mask = 1UL << PS4_BUTTONS[index];
        bool click = buttonClickState.val & mask;
        buttonClickState.val &= ~mask; // Clear "click" event
        return click;
}

uint8_t PS4Parser::getAnalogButton(ButtonEnum b) {
        const int8_t index = getButtonIndexPS4(b); if (index < 0) return 0;
        if (index == ButtonIndex(L2)) // These are the only analog buttons on the controller
                return ps4Data.trigger[0];
        else if (index == ButtonIndex(R2))
                return ps4Data.trigger[1];
        return 0;
}

uint8_t PS4Parser::getAnalogHat(AnalogHatEnum a) {
        return ps4Data.hatValue[(uint8_t)a];
}

void PS4Parser::Parse(uint8_t len, uint8_t *buf) {
        if (len > 1 && buf)  {

                if (esp_log_level_get(LOG_TAG) >= ESP_LOG_DEBUG) {
                        char printbuf[512];
                        for (uint8_t i = 0; i < len; i++) {
                                sprintf(&printbuf[i*2],"%02X",buf[i]);
                        }
                        buf[len*2] = '\0';
                        ESP_LOGD(LOG_TAG,"Report from device %s",printbuf);
                }

                if (buf[0] == 0x01) // Check report ID
                        memcpy(&ps4Data, buf + 1, min((uint8_t)(len - 1), sizeof(ps4Data)));
                else if (buf[0] == 0x11) { // This report is send via Bluetooth, it has an offset of 2 compared to the USB data
                        if (len < 4) {
                                ESP_LOGD(LOG_TAG,"PS4 Report is too short: %d", len);
                                return;
                        }
                        memcpy(&ps4Data, buf + 3, min((uint8_t)(len - 3), sizeof(ps4Data)));
                } else {
                        ESP_LOGD(LOG_TAG,"PS4 Unknown report id: %02X", buf[0]);
                        return;
                }

                if (ps4Data.btn.val != oldButtonState.val) { // Check if anything has changed
                        //use bitwise OR since button click checks run not after each report
                        buttonClickState.val |= ps4Data.btn.val & ~oldButtonState.val; // Update click state variable
                        oldButtonState.val = ps4Data.btn.val;

                        // The DPAD buttons does not set the different bits, but set a value corresponding to the buttons pressed, we will simply set the bits ourself
                        uint8_t newDpad = 0;
                        if (checkDpad(UP))
                                newDpad |= 1 << UP;
                        if (checkDpad(RIGHT))
                                newDpad |= 1 << RIGHT;
                        if (checkDpad(DOWN))
                                newDpad |= 1 << DOWN;
                        if (checkDpad(LEFT))
                                newDpad |= 1 << LEFT;
                        if (newDpad != oldDpad) {
                                buttonClickState.dpad = newDpad & ~oldDpad; // Override values
                                oldDpad = newDpad;
                        }
                }
        }

        if (ps4Output.reportChanged)
                sendOutputReport(&ps4Output); // Send output report
}

void PS4Parser::Reset() {
        uint8_t i;
        for (i = 0; i < sizeof(ps4Data.hatValue); i++)
                ps4Data.hatValue[i] = 127; // Center value
        ps4Data.btn.val = 0;
        oldButtonState.val = 0;
        for (i = 0; i < sizeof(ps4Data.trigger); i++)
                ps4Data.trigger[i] = 0;
        for (i = 0; i < sizeof(ps4Data.xy)/sizeof(ps4Data.xy[0]); i++) {
                for (uint8_t j = 0; j < sizeof(ps4Data.xy[0].finger)/sizeof(ps4Data.xy[0].finger[0]); j++)
                        ps4Data.xy[i].finger[j].touching = 1; // The bit is cleared if the finger is touching the touchpad
        }

        ps4Data.btn.dpad = DPAD_OFF;
        oldButtonState.dpad = DPAD_OFF;
        buttonClickState.dpad = 0;
        oldDpad = 0;

        ps4Output.bigRumble = ps4Output.smallRumble = 0;
        ps4Output.r = ps4Output.g = ps4Output.b = 0;
        ps4Output.flashOn = ps4Output.flashOff = 0;
        ps4Output.reportChanged = false;
};

