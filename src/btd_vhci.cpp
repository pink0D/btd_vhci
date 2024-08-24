//
// Copyright (c) Dmitry Akulov. All rights reserved.
//
// Repository info:     https://github.com/pink0D/btd_vhci
// Contact information: pink0D.github@gmail.com
//
// Licensed under the GPLv2. See LICENSE file in the project root for details.
//

#include "btd_vhci.h"
#include "BTD.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"

#define STORAGE_NAMESPACE   "storage"
#define STORAGE_KEY         "BTDADDR"

static const char *LOG_TAG = "BTD VHCI";

static uint8_t sendbuf[BULK_MAXPKTSIZE];
static SemaphoreHandle_t send_mutex;

static SemaphoreHandle_t lock_mutex;

static RingbufHandle_t ring_hci;
static RingbufHandle_t ring_acl;

esp_err_t btd_vhci_send_packet(uint8_t packet_type, uint8_t *packet, int size) {

    xSemaphoreTake(send_mutex, portMAX_DELAY);
    
    // put packet type before payload
    sendbuf[0] = packet_type;

    for (int i=0; i<size; i++)
        memcpy(&sendbuf[1],packet,size);

    // send packet
    esp_vhci_host_send_packet(sendbuf, size+1);
    
    xSemaphoreGive(send_mutex);

    return ESP_OK;
}

static void controller_send_ready(void){}

static int host_rcv_pkt(uint8_t *data, uint16_t len)
{
    uint8_t packet_type = data[0];

    if (packet_type == HCI_EVENT_PACKET) {
        xRingbufferSend(ring_hci, &data[1],len-1, 0);
    }

    if (packet_type == HCI_ACL_DATA_PACKET) {
        xRingbufferSend(ring_acl, &data[1],len-1, 0); 
    }

    return 0;
}

static esp_vhci_host_callback_t vhci_host_cb = {
    controller_send_ready,
    host_rcv_pkt
};

void btd_vhci_mutex_lock() {
    xSemaphoreTake(lock_mutex, portMAX_DELAY);
}

void btd_vhci_mutex_unlock() {
    xSemaphoreGive(lock_mutex);
}

// read bluetooth data and run BTD state machine
void btd_vhci_update() {

    btd_vhci_mutex_lock();

    uint8_t *data_hci = NULL;
    size_t data_hci_size = 0;

    uint8_t *data_acl = NULL;
    size_t data_acl_size = 0;

    do { 

        data_hci = (uint8_t *)xRingbufferReceive(ring_hci, &data_hci_size, 0);
        if (data_hci != NULL) {
            BTD::instance()->HCI_event_task(data_hci, data_hci_size);
            vRingbufferReturnItem(ring_hci, (void *)data_hci);
        } else
            BTD::instance()->HCI_event_task(NULL, 0);

        BTD::instance()->HCI_task();
        

        data_acl = (uint8_t *)xRingbufferReceive(ring_acl, &data_acl_size, 0);
        if (data_acl != NULL) {
            BTD::instance()->ACL_event_task(data_acl, data_acl_size);
            vRingbufferReturnItem(ring_acl, (void *)data_acl);
        } else
            BTD::instance()->ACL_event_task(NULL, 0);

    } while (data_acl || data_hci);
    // loop without delay while more data is available from bluetooth device

    btd_vhci_mutex_unlock();
}

// default bluetooth task
void btd_vhci_update_task(void *task_params = 0) {

    while (1) {     
        btd_vhci_update();
        vTaskDelay(1);
    }
}

esp_err_t btd_vhci_init(bool createTask) {

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    esp_err_t ret;

    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "ESP Bluetooth controller initialize failed: %s", esp_err_to_name(ret));
        return ESP_FAIL ;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "ESP Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    if ((ret = esp_vhci_host_register_callback(&vhci_host_cb)) != ESP_OK) {
        ESP_LOGE(LOG_TAG, "ESP Bluetooth callback register fail: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ring_hci = xRingbufferCreate(2*1024, RINGBUF_TYPE_NOSPLIT);
    if (ring_hci == NULL) {
        ESP_LOGE(LOG_TAG, "ESP Failed to create ring buffer HCI");
        return ESP_FAIL;
    }

    ring_acl = xRingbufferCreate(2*1024, RINGBUF_TYPE_NOSPLIT);
    if (ring_acl == NULL) {
        ESP_LOGE(LOG_TAG, "ESP Failed to create ring buffer ACL");
        return ESP_FAIL;
    }

    send_mutex = xSemaphoreCreateMutex();
    if( send_mutex == NULL ) {
        ESP_LOGE(LOG_TAG, "ESP Failed to create send mutex");
        return ESP_FAIL;
    }

    lock_mutex = xSemaphoreCreateMutex();
    if( lock_mutex == NULL ) {
        ESP_LOGE(LOG_TAG, "ESP Failed to create lock mutex");
        return ESP_FAIL;
    }


    BTD::instance()->Init();

    if (createTask) {
        xTaskCreatePinnedToCore(btd_vhci_update_task,"btd_vhci_update_task",10*1024,NULL,2,NULL,0);
        ESP_LOGI(LOG_TAG, "BTD VHCI created default task");
    }

    ESP_LOGI(LOG_TAG, "BTD VHCI init successful");

    return ESP_OK;
}

void btd_vhci_autoconnect_task(void *task_params) {
 
    BTHID *bthid = (BTHID*) task_params;

    nvs_handle_t nvs_handle;    
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS error opening storage");
        return;
    }

    uint8_t saved_addr[6] = {0,0,0,0,0,0};
    size_t size = sizeof(saved_addr); 
    err = nvs_get_blob(nvs_handle, STORAGE_KEY, saved_addr, &size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(LOG_TAG, "NVS error reading storage");
        return;
    }

    nvs_close(nvs_handle);

    // try connect to saved device
    if (err == ESP_OK) {
        ESP_LOGI(LOG_TAG, "Read saved device address & connecting: %02X%02X%02X%02X%02X%02X",saved_addr[5],saved_addr[4],saved_addr[3],saved_addr[2],saved_addr[1],saved_addr[0]);

        bthid->connect(saved_addr);
        vTaskDelay(pdMS_TO_TICKS(30000));  
    }

    // if no connection was established, then go to pairing mode
    if (!bthid->connected) {
        ESP_LOGI(LOG_TAG, "Could not connect to saved device, going to pairing mode");

        bthid->pair();
        while (1) {
            if (bthid->connected) {
                bthid->getAddr(saved_addr);
                bthid->connect(saved_addr); // save address for BTD auto reconnect

                ESP_LOGI(LOG_TAG, "Saving device address: %02X%02X%02X%02X%02X%02X",saved_addr[5],saved_addr[4],saved_addr[3],saved_addr[2],saved_addr[1],saved_addr[0]);

                err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
                if (err != ESP_OK) {
                    ESP_LOGE(LOG_TAG, "NVS error opening storage");
                    return;
                }
                
                err = nvs_set_blob(nvs_handle, STORAGE_KEY, saved_addr, size);
                if (err != ESP_OK) { 
                    ESP_LOGE(LOG_TAG, "NVS error writing storage");
                    return;
                }

                err = nvs_commit(nvs_handle);
                if (err != ESP_OK) { 
                    ESP_LOGE(LOG_TAG, "NVS error commiting storage");
                    return;
                }

                nvs_close(nvs_handle);

                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t btd_vhci_autoconnect(BTHID *bthid) {

    xTaskCreatePinnedToCore(btd_vhci_autoconnect_task,"btd_vhci_autoconnect_task",10*1024,bthid,2,NULL,1);

    return ESP_OK;
}