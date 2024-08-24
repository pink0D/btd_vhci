//
// Copyright (c) Dmitry Akulov. All rights reserved.
//
// Repository info:     https://github.com/pink0D/btd_vhci
// Contact information: pink0D.github@gmail.com
//
// Licensed under the GPLv2. See LICENSE file in the project root for details.
//

#ifndef _btd_vhci_h_
#define _btd_vhci_h_

#include "freertos/FreeRTOS.h"
#include "BTHID.h"

esp_err_t btd_vhci_init(bool createTask = true);
esp_err_t btd_vhci_autoconnect(BTHID *bthid);
esp_err_t btd_vhci_send_packet(uint8_t packet_type, uint8_t *packet, int size);

void btd_vhci_mutex_lock();
void btd_vhci_mutex_unlock();
void btd_vhci_update();

#endif