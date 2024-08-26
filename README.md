# About
This is an adaptation of Bluetooth classes from the [USB Host Library Rev. 2.0](https://github.com/felis/USB_Host_Shield_2.0) for running on ESP32 chips with embedded Bluetooth controller.

# How it works

Original classes from the [USB Host Library Rev. 2.0](https://github.com/felis/USB_Host_Shield_2.0) use HCI protocol to interact with USB Bluetooth dongle. This library replaces USB transport with VHCI (Virtual HCI) functions available in ESP32's ESP-IDF.

# Supported devices

Generally, all code for the BT devices from the [USB Host Library Rev. 2.0](https://github.com/felis/USB_Host_Shield_2.0) can be migrated, however this is yet to be done. BTW any help in doing this is appreciated, and please also [email](mailto:pink0D.github@gmail.com) me if you successfully used this library with any of the devices, which  I've not yet tested myself.

| Device         | Code migrated    | Tested with real device |
| :---           |    :----:        |         :----:          |
| BTHID (generic keyboard and mouse)          | :white_check_mark: | :x: |
| PS4BT          | :white_check_mark: | :white_check_mark: |
| PS5BT          | :white_check_mark: | :x: |
| XBOXONESBT     | :white_check_mark: | :x: |
| SPP (Serial Port Profile) | :x: | :x: |


# Current limitations

- This library can be used only with ESP32 SoCs/modules/boards having **Bluetooth Classic (BR/EDR)** support
- Library classes are not thread-safe. Since ESP32 runs a kind of multithreaded environment, using mutex is highly recommended to avoid data races (see example below)
- Currently, the library cannot be used with ESP32 Arduino Core, since it is prebuilt with Bludroid stack enabled by default, which makes impossible to use VHCI functions. You can try using Espressif's [ESP32 Arduino Lib Builder](https://github.com/espressif/esp32-arduino-lib-builder) to make a custom build of ESP32 Arduino Core with required options in sdkconfig.

# Using the library

1. Install ESP-IDF (the library was developed and tested with ESP-IDF v5.3)
2. Create a new project
3. Add dependency
- by using IDF Component Manager `idf.py add-dependency -pink0d/btd_vhci` (the command should be run from ESP-IDF Terminal if you are using VSCode plugin)
- alternatively, you can just copy repo contents to `components\btd_vhci` inside project root directory and manually add dependency  `REQUIRES btd_vhci nvs_flash` to the main component's `CMakeLists.txt`
4. Make changes to sdkconfig with ESP-IDF's Menuconfig:
- Bluetooth: Enabled
- Host: Disabled
- Controller: Enabled
- Bluetooth controller mode: BR/EDR Only
- BR/EDR Sync: HCI
- HCI mode: VHCI

This should result in the following contents inside sdkconfig
```
#
# Bluetooth
#
CONFIG_BT_ENABLED=y
CONFIG_BT_CONTROLLER_ONLY=y
CONFIG_BT_CONTROLLER_ENABLED=y
#
# Controller Options
#
CONFIG_BTDM_CTRL_MODE_BR_EDR_ONLY=y
CONFIG_BTDM_CTRL_BR_EDR_MAX_ACL_CONN=2
CONFIG_BTDM_CTRL_BR_EDR_MAX_SYNC_CONN=0
CONFIG_BTDM_CTRL_BR_EDR_SCO_DATA_PATH_HCI=y
CONFIG_BTDM_CTRL_BR_EDR_SCO_DATA_PATH_EFF=0
CONFIG_BTDM_CTRL_PCM_ROLE_EFF=0
CONFIG_BTDM_CTRL_PCM_POLAR_EFF=0
CONFIG_BTDM_CTRL_LEGACY_AUTH_VENDOR_EVT=y
CONFIG_BTDM_CTRL_LEGACY_AUTH_VENDOR_EVT_EFF=y
CONFIG_BTDM_CTRL_BLE_MAX_CONN_EFF=0
CONFIG_BTDM_CTRL_BR_EDR_MAX_ACL_CONN_EFF=2
CONFIG_BTDM_CTRL_BR_EDR_MAX_SYNC_CONN_EFF=0
CONFIG_BTDM_CTRL_PINNED_TO_CORE_0=y
CONFIG_BTDM_CTRL_PINNED_TO_CORE=0
CONFIG_BTDM_CTRL_HCI_MODE_VHCI=y
```
5. Rename `main.c` to `main.cpp` since you will have to use C++ classes from the library. Don't forget to update `CMakeLists.txt` and add `extern "C"` to `app_main`
6. Declare global instance for desired device class, for example `PS4BT PS4;` 
Note that no reference to **BTD** is required since it is now accessed through a static singleton.
7. Add initialization calls in main function 
- `nvs_flash_init()` which is needed by ESP32 to initilize flash
- `btd_vhci_init()`  library initialization, which also creates default background bluetooth update task 
- `xTaskCreatePinnedToCore(...)` to run your program code
- `btd_vhci_autoconnect(...)` to run auto-pairing task. When started, it looks for a saved MAC in ESP32's flash. If no saved MAC is found, the BTD is put in *'Pairing'* mode. When a new device is paired, it's MAC is saved to flash. Next time when the ESP32 is started, the BTD is put in *'Waiting for connections'* mode. If no connection is made in 30 seconds, the BTD goes to pairing mode again.
8. Implement task for reading controller's state
- it is important to call `btd_vhci_mutex_lock();` and `btd_vhci_mutex_unlock();` when accessing BT device instance to prevent data races with default background bluetooth update task 
- alternatively, you can call `btd_vhci_init(false);` and manually implement bluetooth update task with a call to `btd_vhci_update();` inside it (see btd_vhci.cpp for details)

Now the `main.cpp` should look like this:
```CPP
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "PS4BT.h"
#include "btd_vhci.h"

PS4BT PS4;

bool printAngle, printTouch;
uint8_t oldL2Value, oldR2Value;

static const char *LOG_TAG = "main";

// print controller status
void ps4_print() {
    if (PS4.connected()) {
        if (PS4.getAnalogHat(LeftHatX) > 137 || PS4.getAnalogHat(LeftHatX) < 117 || PS4.getAnalogHat(LeftHatY) > 137 || PS4.getAnalogHat(LeftHatY) < 117 || PS4.getAnalogHat(RightHatX) > 137 || PS4.getAnalogHat(RightHatX) < 117 || PS4.getAnalogHat(RightHatY) > 137 || PS4.getAnalogHat(RightHatY) < 117) {
        ESP_LOGI(LOG_TAG, "L_x = %d, L_y = %d, R_x = %d, R_y = %d",
                    PS4.getAnalogHat(LeftHatX),PS4.getAnalogHat(LeftHatY),
                    PS4.getAnalogHat(RightHatX),PS4.getAnalogHat(RightHatY));
        }

        if (PS4.getAnalogButton(L2) || PS4.getAnalogButton(R2)) { // These are the only analog buttons on the PS4 controller
        ESP_LOGI(LOG_TAG, "L2 = %d, R2 = %d",PS4.getAnalogButton(L2),PS4.getAnalogButton(R2));
        }
        if (PS4.getAnalogButton(L2) != oldL2Value || PS4.getAnalogButton(R2) != oldR2Value) // Only write value if it's different
        PS4.setRumbleOn(PS4.getAnalogButton(L2), PS4.getAnalogButton(R2));
        oldL2Value = PS4.getAnalogButton(L2);
        oldR2Value = PS4.getAnalogButton(R2);

        if (PS4.getButtonClick(PS))
        ESP_LOGI(LOG_TAG, "PS");
        if (PS4.getButtonClick(TRIANGLE)) {
        ESP_LOGI(LOG_TAG, "Triangle");
        PS4.setRumbleOn(RumbleLow);
        }
        if (PS4.getButtonClick(CIRCLE)) {
        ESP_LOGI(LOG_TAG, "Circle");
        PS4.setRumbleOn(RumbleHigh);
        }
        if (PS4.getButtonClick(CROSS)) {
        ESP_LOGI(LOG_TAG, "Cross");
        PS4.setLedFlash(10, 10); // Set it to blink rapidly
        }
        if (PS4.getButtonClick(SQUARE)) {
        ESP_LOGI(LOG_TAG, "Square");
        PS4.setLedFlash(0, 0); // Turn off blinking
        }

        if (PS4.getButtonClick(UP)) {
        ESP_LOGI(LOG_TAG, "UP");
        PS4.setLed(Red);
        } if (PS4.getButtonClick(RIGHT)) {
        ESP_LOGI(LOG_TAG, "RIGHT");
        PS4.setLed(Blue);
        } if (PS4.getButtonClick(DOWN)) {
        ESP_LOGI(LOG_TAG, "DOWN");
        PS4.setLed(Yellow);
        } if (PS4.getButtonClick(LEFT)) {
        ESP_LOGI(LOG_TAG, "LEFT");
        PS4.setLed(Green);
        }

        if (PS4.getButtonClick(L1))
        ESP_LOGI(LOG_TAG, "L1");
        if (PS4.getButtonClick(L3))
        ESP_LOGI(LOG_TAG, "L3");
        if (PS4.getButtonClick(R1))
        ESP_LOGI(LOG_TAG, "R1");
        if (PS4.getButtonClick(R3))
        ESP_LOGI(LOG_TAG, "R3");

        if (PS4.getButtonClick(SHARE))
        ESP_LOGI(LOG_TAG, "SHARE");
        if (PS4.getButtonClick(OPTIONS)) {
        ESP_LOGI(LOG_TAG, "OPTIONS");
        printAngle = !printAngle;
        }
        if (PS4.getButtonClick(TOUCHPAD)) {
        ESP_LOGI(LOG_TAG, "TOUCHPAD");
        printTouch = !printTouch;
        }

        if (printAngle) { // Print angle calculated using the accelerometer only
        ESP_LOGI(LOG_TAG,"Pitch: %lf Roll: %lf", PS4.getAngle(Pitch), PS4.getAngle(Roll));        
        }

        if (printTouch) { // Print the x, y coordinates of the touchpad
            if (PS4.isTouching(0) || PS4.isTouching(1)) // Print newline and carriage return if any of the fingers are touching the touchpad
                ESP_LOGI(LOG_TAG, "");
            for (uint8_t i = 0; i < 2; i++) { // The touchpad track two fingers
                if (PS4.isTouching(i)) { // Print the position of the finger if it is touching the touchpad
                ESP_LOGI(LOG_TAG, "X = %d, Y = %d",PS4.getX(i),PS4.getY(i));          
                }
            }
        }
    }
}

void ps4_loop_task(void *task_params) {
    while (1) { 
        btd_vhci_mutex_lock();      // lock mutex so controller's data is not updated meanwhile
        ps4_print();                // print PS4 status
        btd_vhci_mutex_unlock();    // unlock mutex
        vTaskDelay(1);
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret;

    // initialize flash
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // initilize the library
    ret = btd_vhci_init();
    if (ret != ESP_OK) {
        ESP_LOGE(LOG_TAG, "BTD init error!");
    }
    ESP_ERROR_CHECK( ret );

    // run example code
    xTaskCreatePinnedToCore(ps4_loop_task,"ps4_loop_task",10*1024,NULL,2,NULL,1);

    // run auto connect task
    btd_vhci_autoconnect(&PS4);

    while (1) {       
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // main task should not return
}
```

# Example projects

Example projects can be found here: [btd_vhci_examples_ESP-IDF](https://github.com/pink0D/btd_vhci_examples_ESP-IDF)

# More examples

Please look at the [examples](https://github.com/felis/USB_Host_Shield_2.0/tree/master/examples/Bluetooth) from the original library 

