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