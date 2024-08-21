/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.

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

#include "BTD.h"
#include "btd_vhci.h"

static const char *LOG_TAG = "BTD";

BTD::BTD() :
connectToWii(false),
pairWithWii(false),
connectToHIDDevice(false),
pairWithHIDDevice(false),
useSimplePairing(false),
simple_pairing_supported(false),
bPollEnable(false) // Don't start polling before dongle is connected
{
        for(uint8_t i = 0; i < BTD_NUM_SERVICES; i++)
                btService[i] = NULL;

        Initialize(); // Set all variables, endpoint structs etc. to default values
}

uint8_t BTD::Init() {
        hci_num_reset_loops = 100; // only loop 100 times before trying to send the hci reset command
        hci_counter = 0;
        hci_state = HCI_INIT_STATE;
        waitingForConnection = false;
        bPollEnable = true;
        return 0; // Successful configuration
}

void BTD::Initialize() {
        uint8_t i;      
        
        for(i = 0; i < BTD_NUM_SERVICES; i++) {
                if(btService[i])
                        btService[i]->Reset(); // Reset all Bluetooth services
        }

        for (i = 0; i < 6; i++)
                saved_bdaddr[i] = 0;

        connectToWii = false;
        incomingWii = false;
        connectToHIDDevice = false;
        incomingHIDDevice = false;
        incomingPSController = false;
        bPollEnable = false; // Don't start polling before dongle is connected
        simple_pairing_supported = false;
}


/* Performs a cleanup after failed Init() attempt */
uint8_t BTD::Release() {
        Initialize(); // Set all variables, endpoint structs etc. to default values
        return 0;
}

/*uint8_t BTD::Poll() {
        if(!bPollEnable)
                return 0;
        if((int32_t)((uint32_t)millis() - qNextPollTime) >= 0L) { // Don't poll if shorter than polling interval
                qNextPollTime = (uint32_t)millis() + pollInterval; // Set new poll time
                HCI_event_task(); // Poll the HCI event pipe
                HCI_task(); // HCI state machine
                ACL_event_task(); // Poll the ACL input pipe too
        }
        return 0;
}*/

void BTD::disconnect() {
        for(uint8_t i = 0; i < BTD_NUM_SERVICES; i++)
                if(btService[i])
                        btService[i]->disconnect();
};

void BTD::HCI_event_task(uint8_t *packet, int size) {

        //if(!bPollEnable)
        //        return;

        if(size > 0) { 
                ESP_LOGD(LOG_TAG,"HCI Event 0x%02X size = %d",packet[0],size);
        
                memcpy(hcibuf,packet,size);

                switch(hcibuf[0]) { // Switch on event type
                        case EV_COMMAND_COMPLETE:
                                if(!hcibuf[5]) { // Check if command succeeded
                                        hci_set_flag(HCI_FLAG_CMD_COMPLETE); // Set command complete flag
                                        if((hcibuf[3] == 0x01) && (hcibuf[4] == 0x10)) { // Parameters from read local version information
                                                hci_version = hcibuf[6]; // Used to check if it supports 2.0+EDR - see http://www.bluetooth.org/Technical/AssignedNumbers/hci.htm

                                                if(!hci_check_flag(HCI_FLAG_READ_VERSION)) {
                                                        ESP_LOGD(LOG_TAG, "HCI version: 0x%02X",hci_version);
                                                }

                                                hci_set_flag(HCI_FLAG_READ_VERSION);
                                        } else if((hcibuf[3] == 0x04) && (hcibuf[4] == 0x10)) { // Parameters from read local extended features
                                                if(!hci_check_flag(HCI_FLAG_LOCAL_EXTENDED_FEATURES)) {

                                                        ESP_LOGD(LOG_TAG, "Page number: 0x%02X",hcibuf[6]);
                                                        ESP_LOGD(LOG_TAG, "Maximum page number: 0x%02X",hcibuf[7]);
                                                        ESP_LOGD(LOG_TAG, "Extended LMP features:");
                                                        for(uint8_t i = 0; i < 8; i++) {
                                                                ESP_LOGD(LOG_TAG, "0x%02X",hcibuf[8 + i]);                                                                
                                                        }

                                                        if(hcibuf[6] == 0) { // Page 0

                                                                ESP_LOGD(LOG_TAG, "Dongle ");

                                                                if(hcibuf[8 + 6] & (1U << 3)) {
                                                                        simple_pairing_supported = true;

                                                                        ESP_LOGD(LOG_TAG, "supports");

                                                                } else {
                                                                        simple_pairing_supported = false;

                                                                        ESP_LOGD(LOG_TAG, "does NOT support");

                                                                }

                                                                ESP_LOGD(LOG_TAG, " secure simple pairing (controller support)");

                                                        } else if(hcibuf[6] == 1) { // Page 1

                                                                ESP_LOGD(LOG_TAG, "Dongle ");
                                                                if(hcibuf[8 + 0] & (1U << 0))
                                                                        ESP_LOGD(LOG_TAG, "supports");
                                                                else
                                                                        ESP_LOGD(LOG_TAG, "does NOT support");
                                                                ESP_LOGD(LOG_TAG, " secure simple pairing (host support)");

                                                        }
                                                }

                                                hci_set_flag(HCI_FLAG_LOCAL_EXTENDED_FEATURES);
                                        } else if((hcibuf[3] == 0x09) && (hcibuf[4] == 0x10)) { // Parameters from read local bluetooth address
                                                for(uint8_t i = 0; i < 6; i++)
                                                        my_bdaddr[i] = hcibuf[6 + i];
                                                hci_set_flag(HCI_FLAG_READ_BDADDR);
                                        }
                                }
                                break;

                        case EV_COMMAND_STATUS:
                                if(hcibuf[2]) { // Show status on serial if not OK

                                        ESP_LOGD(LOG_TAG, "HCI Command Failed: 0x%02X",hcibuf[2]);
                                        ESP_LOGD(LOG_TAG, "Num HCI Command Packets: 0x%02X",hcibuf[3]);
                                        ESP_LOGD(LOG_TAG, "Command Opcode: 0x%02X%02X",hcibuf[4],hcibuf[5]);

                                }
                                break;

                        case EV_INQUIRY_COMPLETE:
                                if(inquiry_counter >= 5 && (pairWithWii || pairWithHIDDevice)) {
                                        inquiry_counter = 0;

                                        if(pairWithWii)
                                                ESP_LOGD(LOG_TAG, "Couldn't find Wiimote");
                                        else
                                                ESP_LOGD(LOG_TAG, "Couldn't find HID device");

                                        connectToWii = false;
                                        pairWithWii = false;
                                        connectToHIDDevice = false;
                                        pairWithHIDDevice = false;
                                        hci_state = HCI_SCANNING_STATE;
                                }
                                inquiry_counter++;
                                break;

                        case EV_INQUIRY_RESULT:
                        case EV_EXTENDED_INQUIRY_RESULT:
                                if(hcibuf[2]) { // Check that there is more than zero responses

                                        ESP_LOGD(LOG_TAG, "Number of responses: 0x%02X",hcibuf[2]); // This will always be 1 for an extended inquiry result

                                        for(uint8_t i = 0; i < hcibuf[2]; i++) {
                                                uint8_t classOfDevice_offset;
                                                if(hcibuf[0] == EV_INQUIRY_RESULT)
                                                        classOfDevice_offset = 9 * hcibuf[2]; // 6-byte bd_addr, 1 byte page_scan_repetition_mode, 2 byte reserved
                                                else
                                                        classOfDevice_offset = 8 * hcibuf[2]; // 6-byte bd_addr, 1 byte page_scan_repetition_mode, 1 byte reserved

                                                for(uint8_t j = 0; j < 3; j++)
                                                        classOfDevice[j] = hcibuf[3 + classOfDevice_offset + 3 * i + j];


                                                ESP_LOGD(LOG_TAG, "Class of device: 0x%02X%02X%02X",classOfDevice[2],classOfDevice[1],classOfDevice[0]);


                                                if(pairWithWii && (classOfDevice[2] == 0x00) && ((classOfDevice[1] & 0x0F) == 0x05) && (classOfDevice[0] & 0x0C)) { // See http://wiibrew.org/wiki/Wiimote#SDP_information
                                                        checkRemoteName = true; // Check remote name to distinguish between the different controllers

                                                        for(uint8_t j = 0; j < 6; j++)
                                                                disc_bdaddr[j] = hcibuf[3 + 6 * i + j];

                                                        hci_set_flag(HCI_FLAG_DEVICE_FOUND);
                                                        break;
                                                } else if(pairWithHIDDevice && ((classOfDevice[1] & 0x0F) == 0x05) && (classOfDevice[0] & 0xC8)) { // Check if it is a mouse, keyboard or a gamepad - see: http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html

                                                        checkRemoteName = true; // Used to print name in the serial monitor if serial debugging is enabled

                                                        if(classOfDevice[0] & 0x80)
                                                                ESP_LOGI(LOG_TAG, "Mouse found");
                                                        if(classOfDevice[0] & 0x40)
                                                                ESP_LOGI(LOG_TAG, "Keyboard found");
                                                        if(classOfDevice[0] & 0x08)
                                                                ESP_LOGI(LOG_TAG, "Gamepad found");

                                                        for(uint8_t j = 0; j < 6; j++)
                                                                disc_bdaddr[j] = hcibuf[3 + 6 * i + j];

                                                        hci_set_flag(HCI_FLAG_DEVICE_FOUND);
                                                        break;
                                                }
                                        }
                                }
                                break;

                        case EV_CONNECT_COMPLETE:
                                hci_set_flag(HCI_FLAG_CONNECT_EVENT);
                                if(!hcibuf[2]) { // Check if connected OK

                                        ESP_LOGI(LOG_TAG, "Connection established");

                                        hci_handle = hcibuf[3] | ((hcibuf[4] & 0x0F) << 8); // Store the handle for the ACL connection
                                        hci_set_flag(HCI_FLAG_CONNECT_COMPLETE); // Set connection complete flag
                                } else {
                                        hci_state = HCI_CHECK_DEVICE_SERVICE;

                                        ESP_LOGD(LOG_TAG, "Connection Failed: 0x%02X",hcibuf[2]);

                                }
                                break;

                        case EV_DISCONNECT_COMPLETE:
                                if(!hcibuf[2]) { // Check if disconnected OK
                                        hci_set_flag(HCI_FLAG_DISCONNECT_COMPLETE); // Set disconnect command complete flag
                                        hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE); // Clear connection complete flag
                                }
                                break;

                        case EV_REMOTE_NAME_COMPLETE:
                                if(!hcibuf[2]) { // Check if reading is OK
                                        for(uint8_t i = 0; i < min(sizeof (remote_name), sizeof (hcibuf) - 9); i++) {
                                                remote_name[i] = hcibuf[9 + i];
                                                if(remote_name[i] == '\0') // End of string
                                                        break;
                                        }
                                        // TODO: Always set '\0' in remote name!
                                        hci_set_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);

                                        ESP_LOGD(LOG_TAG, "Set flag HCI_FLAG_REMOTE_NAME_COMPLETE");

                                        //hci_set_flag(HCI_FLAG_CMD_COMPLETE); // set flag so the state machine knows remote name command was completed
                                } else {

                                        ESP_LOGD(LOG_TAG, "Set flag HCI_FLAG_REMOTE_NAME_COMPLETE");                                        

                                }
                                break;

                        case EV_INCOMING_CONNECT:
                                for(uint8_t i = 0; i < 6; i++)
                                        disc_bdaddr[i] = hcibuf[i + 2];

                                for(uint8_t i = 0; i < 3; i++)
                                        classOfDevice[i] = hcibuf[i + 8];

                                if(((classOfDevice[1] & 0x0F) == 0x05) && (classOfDevice[0] & 0xC8)) { // Check if it is a mouse, keyboard or a gamepad

                                        if(classOfDevice[0] & 0x80)
                                                ESP_LOGI(LOG_TAG, "Mouse is connecting");
                                        if(classOfDevice[0] & 0x40)
                                                ESP_LOGI(LOG_TAG, "Keyboard is connecting");
                                        if(classOfDevice[0] & 0x08)
                                                ESP_LOGI(LOG_TAG, "Gamepad is connecting");

                                        incomingHIDDevice = true;
                                }


                                ESP_LOGD(LOG_TAG, "Class of device: 0x%02X%02X%02X",classOfDevice[2],classOfDevice[1],classOfDevice[0]);

                                hci_set_flag(HCI_FLAG_INCOMING_REQUEST);
                                break;

                        case EV_PIN_CODE_REQUEST:
                                if(pairWithWii) {

                                        ESP_LOGI(LOG_TAG, "Pairing with Wiimote");

                                        hci_pin_code_request_reply();
                                } else if(btdPin != NULL) {

                                        ESP_LOGD(LOG_TAG, "Bluetooth pin is set too: %s", btdPin);

                                        hci_pin_code_request_reply();
                                } else {

                                        ESP_LOGD(LOG_TAG, "No pin was set");

                                        hci_pin_code_negative_request_reply();
                                }
                                break;

                        case EV_LINK_KEY_REQUEST:

                                ESP_LOGD(LOG_TAG, "Received Key Request");

                                hci_link_key_request_negative_reply();
                                break;

                        case EV_AUTHENTICATION_COMPLETE:
                                if(!hcibuf[2]) { // Check if pairing was successful
                                        if(pairWithWii && !connectToWii) {

                                                ESP_LOGD(LOG_TAG, "Pairing successful with Wiimote");

                                                connectToWii = true; // Used to indicate to the Wii service, that it should connect to this device
                                        } else if(pairWithHIDDevice && !connectToHIDDevice) {

                                                ESP_LOGD(LOG_TAG, "Pairing successful with HID device");

                                                connectToHIDDevice = true; // Used to indicate to the BTHID service, that it should connect to this device
                                        } else {

                                                ESP_LOGD(LOG_TAG, "Pairing was successful");

                                        }
                                } else {

                                        ESP_LOGD(LOG_TAG, "Pairing Failed: 0x%02X",hcibuf[2]);

                                        hci_disconnect(hci_handle);
                                        hci_state = HCI_DISCONNECT_STATE;
                                }
                                break;

                        case EV_IO_CAPABILITY_REQUEST:

                                ESP_LOGD(LOG_TAG, "Received IO Capability Request");

                                hci_io_capability_request_reply();
                                break;

                        case EV_IO_CAPABILITY_RESPONSE:

                                ESP_LOGD(LOG_TAG, "Received IO Capability Response: ");
                                ESP_LOGD(LOG_TAG, "IO capability: 0x%02X",hcibuf[8]);
                                ESP_LOGD(LOG_TAG, "OOB data present: 0x%02X",hcibuf[9]);
                                ESP_LOGD(LOG_TAG, "Authentication request: 0x%02X",hcibuf[10]);

                                break;

                        case EV_USER_CONFIRMATION_REQUEST:

                                ESP_LOGD(LOG_TAG, "User confirmation Request");

                                ESP_LOGD(LOG_TAG, ": Numeric value: ");
                                for(uint8_t i = 0; i < 4; i++) {
                                        ESP_LOGD(LOG_TAG, "0x%02X",hcibuf[8 + i]);
                                }

                                // Simply confirm the connection, as the host has no "NoInputNoOutput" capabilities
                                hci_user_confirmation_request_reply();
                                break;

                        case EV_SIMPLE_PAIRING_COMPLETE:

                                if(!hcibuf[2]) { // Check if connected OK
                                        ESP_LOGI(LOG_TAG, "Simple Pairing succeeded");
                                } else {
                                        ESP_LOGD(LOG_TAG, "Simple Pairing failed: 0x%02X",hcibuf[2]);
                                }

                                break;

                                /* We will just ignore the following events */
                        case EV_MAX_SLOTS_CHANGE:
                        case EV_NUM_COMPLETE_PKT:
                                break;
                        case EV_ROLE_CHANGED:
                        case EV_PAGE_SCAN_REP_MODE:
                        case EV_LOOPBACK_COMMAND:
                        case EV_DATA_BUFFER_OVERFLOW:
                        case EV_CHANGE_CONNECTION_LINK:
                        case EV_QOS_SETUP_COMPLETE:
                        case EV_LINK_KEY_NOTIFICATION:
                        case EV_ENCRYPTION_CHANGE:
                        case EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE:

                                if(hcibuf[0] != 0x00) {
                                        ESP_LOGD(LOG_TAG, "Ignore HCI Event: 0x%02X",hcibuf[0]);
                                }

                                break;

                        default:
                                if(hcibuf[0] != 0x00) {
                                        ESP_LOGD(LOG_TAG, "Unmanaged HCI Event: 0x%02X",hcibuf[0]);
                                        ESP_LOGD(LOG_TAG, ", data: ");
                                        for(uint16_t i = 0; i < hcibuf[1]; i++) {
                                                ESP_LOGD(LOG_TAG, "0x%02X",hcibuf[2 + i]);
                                        }
                                }
                                break;

                } // Switch
        }
}

/* Poll Bluetooth and print result */
void BTD::HCI_task() {
        switch(hci_state) {
                case HCI_INIT_STATE:
                        hci_counter++;
                        if(hci_counter > hci_num_reset_loops) { // wait until we have looped x times to clear any old events
                                hci_reset();
                                hci_state = HCI_RESET_STATE;
                                hci_counter = 0;
                        }
                        break;

                case HCI_RESET_STATE:
                        hci_counter++;
                        if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {
                                hci_counter = 0;

                                ESP_LOGD(LOG_TAG, "HCI Reset complete");

                                hci_state = HCI_CLASS_STATE;
                                hci_write_class_of_device();
                        } else if(hci_counter > hci_num_reset_loops) {
                                hci_num_reset_loops *= 10;
                                if(hci_num_reset_loops > 2000)
                                        hci_num_reset_loops = 2000;

                                ESP_LOGD(LOG_TAG, "No response to HCI Reset");

                                hci_state = HCI_INIT_STATE;
                                hci_counter = 0;
                        }
                        break;

                case HCI_CLASS_STATE:
                        if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {

                                ESP_LOGD(LOG_TAG, "Write class of device");

                                hci_state = HCI_BDADDR_STATE;
                                hci_read_bdaddr();
                        }
                        break;

                case HCI_BDADDR_STATE:
                        if(hci_check_flag(HCI_FLAG_READ_BDADDR)) {

                                ESP_LOGI(LOG_TAG, "Local Bluetooth Address: %02X%02X%02X%02X%02X%02X",my_bdaddr[5],my_bdaddr[4],my_bdaddr[3],my_bdaddr[2],my_bdaddr[1],my_bdaddr[0]);

                                hci_read_local_version_information();
                                hci_state = HCI_LOCAL_VERSION_STATE;
                        }
                        break;

                case HCI_LOCAL_VERSION_STATE: // The local version is used by the PS3BT class
                        if(hci_check_flag(HCI_FLAG_READ_VERSION)) {
                                if(btdName != NULL) {
                                        hci_write_local_name(btdName);
                                        hci_state = HCI_WRITE_NAME_STATE;
                                } else if(useSimplePairing) {
                                        hci_read_local_extended_features(0); // "Requests the normal LMP features as returned by Read_Local_Supported_Features"
                                        //hci_read_local_extended_features(1); // Read page 1
                                        hci_state = HCI_LOCAL_EXTENDED_FEATURES_STATE;
                                } else
                                        hci_state = HCI_CHECK_DEVICE_SERVICE;
                        }
                        break;

                case HCI_WRITE_NAME_STATE:
                        if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {

                                ESP_LOGD(LOG_TAG, "The name was set to: %s",btdName);

                                if(useSimplePairing) {
                                        hci_read_local_extended_features(0); // "Requests the normal LMP features as returned by Read_Local_Supported_Features"
                                        //hci_read_local_extended_features(1); // Read page 1
                                        hci_state = HCI_LOCAL_EXTENDED_FEATURES_STATE;
                                } else
                                        hci_state = HCI_CHECK_DEVICE_SERVICE;
                        }
                        break;

                case HCI_LOCAL_EXTENDED_FEATURES_STATE:
                        if(hci_check_flag(HCI_FLAG_LOCAL_EXTENDED_FEATURES)) {
                                if(simple_pairing_supported) {
                                        hci_write_simple_pairing_mode(true);
                                        hci_state = HCI_WRITE_SIMPLE_PAIRING_STATE;
                                } else
                                        hci_state = HCI_CHECK_DEVICE_SERVICE;
                        }
                        break;

                case HCI_WRITE_SIMPLE_PAIRING_STATE:
                        if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {

                                ESP_LOGI(LOG_TAG, "Simple pairing was enabled");

                                hci_set_event_mask();
                                hci_state = HCI_SET_EVENT_MASK_STATE;
                        }
                        break;

                case HCI_SET_EVENT_MASK_STATE:
                        if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {

                                ESP_LOGD(LOG_TAG, "Set event mask completed");

                                hci_state = HCI_CHECK_DEVICE_SERVICE;
                        }
                        break;

                case HCI_CHECK_DEVICE_SERVICE:
                        if(pairWithHIDDevice || pairWithWii) { // Check if it should try to connect to a Wiimote

                                if(pairWithWii)
                                        ESP_LOGI(LOG_TAG, "Starting inquiryPress 1 & 2 on the WiimoteOr press the SYNC button if you are using a Wii U Pro Controller or a Wii Balance Board");
                                else
                                        ESP_LOGI(LOG_TAG, "Please enable discovery of your device");

                                hci_inquiry();
                                hci_state = HCI_INQUIRY_STATE;
                        } else
                                hci_state = HCI_SCANNING_STATE; // Don't try to connect to a Wiimote
                        break;

                case HCI_INQUIRY_STATE:
                        if(hci_check_flag(HCI_FLAG_DEVICE_FOUND)) {
                                hci_inquiry_cancel(); // Stop inquiry

                                if(pairWithWii)
                                        ESP_LOGI(LOG_TAG, "Wiimote found");
                                else
                                        ESP_LOGI(LOG_TAG, "HID device found");

                                ESP_LOGD(LOG_TAG, "Now just create the instance like so:");
                                if(pairWithWii)
                                        ESP_LOGI(LOG_TAG, "WII Wii(&Btd);");
                                else
                                        ESP_LOGI(LOG_TAG, "BTHID bthid(&Btd);");

                                ESP_LOGD(LOG_TAG, "And then press any button on the ");
                                if(pairWithWii)
                                        ESP_LOGI(LOG_TAG, "Wiimote");
                                else
                                        ESP_LOGI(LOG_TAG, "device");

                                if(checkRemoteName) {
                                        hci_remote_name(); // We need to know the name to distinguish between the Wiimote, the new Wiimote with Motion Plus inside, a Wii U Pro Controller and a Wii Balance Board
                                        hci_state = HCI_REMOTE_NAME_STATE;
                                } else
                                        hci_state = HCI_CONNECT_DEVICE_STATE;
                        }
                        break;

                case HCI_CONNECT_DEVICE_STATE:
                        if(hci_check_flag(HCI_FLAG_CMD_COMPLETE)) {

                                if(pairWithWii)
                                        ESP_LOGI(LOG_TAG, "Connecting to Wiimote");
                                else
                                        ESP_LOGI(LOG_TAG, "Connecting to HID device");

                                checkRemoteName = false;
                                hci_connect();
                                hci_state = HCI_CONNECTED_DEVICE_STATE;
                        }
                        break;

                case HCI_CONNECTED_DEVICE_STATE:
                        if(hci_check_flag(HCI_FLAG_CONNECT_EVENT)) {
                                if(hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {

                                        if(pairWithWii)
                                                ESP_LOGI(LOG_TAG, "Connected to Wiimote");
                                        else
                                                ESP_LOGI(LOG_TAG, "Connected to HID device");

                                        hci_authentication_request(); // This will start the pairing with the device
                                        hci_state = HCI_SCANNING_STATE;
                                } else {

                                        ESP_LOGD(LOG_TAG, "Trying to connect one more time...");

                                        hci_connect(); // Try to connect one more time
                                }
                        }
                        break;

                case HCI_SCANNING_STATE:
                        if(!connectToWii && !pairWithWii && !connectToHIDDevice && !pairWithHIDDevice) {

                                ESP_LOGI(LOG_TAG, "Wait For Incoming Connection Request");

                                hci_write_scan_enable();
                                waitingForConnection = true;
                                hci_state = HCI_CONNECT_IN_STATE;
                        }
                        break;

                case HCI_CONNECT_IN_STATE:
                        if(hci_check_flag(HCI_FLAG_INCOMING_REQUEST)) {
                                waitingForConnection = false;

                                ESP_LOGI(LOG_TAG, "Incoming Connection Request");


                                if(classOfDevice[2] == 0 && classOfDevice[1] == 0x25 && classOfDevice[0] == 0x08) {

                                        ESP_LOGI(LOG_TAG, "PS4/PS5 controller is connecting");

                                        incomingPSController = true;
                                }

                                if (       disc_bdaddr[5]==saved_bdaddr[5] 
                                        && disc_bdaddr[4]==saved_bdaddr[4] 
                                        && disc_bdaddr[3]==saved_bdaddr[3]
                                        && disc_bdaddr[2]==saved_bdaddr[2]
                                        && disc_bdaddr[1]==saved_bdaddr[1] 
                                        && disc_bdaddr[0]==saved_bdaddr[0])
                                        {

                                                ESP_LOGI(LOG_TAG, "Saved device detected");

                                                hci_accept_connection();
                                                hci_state = HCI_CONNECTED_STATE;
                                        } else {
                                                hci_remote_name();
                                                hci_state = HCI_REMOTE_NAME_STATE;
                                        }
                        } else if(hci_check_flag(HCI_FLAG_DISCONNECT_COMPLETE))
                                hci_state = HCI_DISCONNECT_STATE;
                        break;

                case HCI_REMOTE_NAME_STATE:
                        if(hci_check_flag(HCI_FLAG_REMOTE_NAME_COMPLETE)) {

                                ESP_LOGI(LOG_TAG, "Remote Name: %s", remote_name);

                                if(strncmp((const char*)remote_name, "Nintendo", 8) == 0) {
                                        incomingWii = true;
                                        motionPlusInside = false;
                                        wiiUProController = false;
                                        pairWiiUsingSync = false;

                                        ESP_LOGD(LOG_TAG, "Wiimote is connecting");

                                        if(strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-TR", 22) == 0) {

                                                ESP_LOGD(LOG_TAG, " with Motion Plus Inside");

                                                motionPlusInside = true;
                                        } else if(strncmp((const char*)remote_name, "Nintendo RVL-CNT-01-UC", 22) == 0) {

                                                ESP_LOGD(LOG_TAG, " - Wii U Pro Controller");

                                                wiiUProController = motionPlusInside = pairWiiUsingSync = true;
                                        } else if(strncmp((const char*)remote_name, "Nintendo RVL-WBC-01", 19) == 0) {

                                                ESP_LOGD(LOG_TAG, " - Wii Balance Board");

                                                pairWiiUsingSync = true;
                                        }
                                }
                                if(classOfDevice[2] == 0 && classOfDevice[1] == 0x25 && classOfDevice[0] == 0x08 && strncmp((const char*)remote_name, "Wireless Controller", 19) == 0) {

                                        ESP_LOGI(LOG_TAG, "PS4/PS5 controller is connecting");

                                        incomingPSController = true;
                                }
                                if((pairWithWii || pairWithHIDDevice) && checkRemoteName)
                                        hci_state = HCI_CONNECT_DEVICE_STATE;
                                else {
                                        hci_accept_connection();
                                        hci_state = HCI_CONNECTED_STATE;
                                }
                        }
                        break;

                case HCI_CONNECTED_STATE:
                        if(hci_check_flag(HCI_FLAG_CONNECT_COMPLETE)) {

                                ESP_LOGI(LOG_TAG, "Connected to Device: %02X%02X%02X%02X%02X%02X",disc_bdaddr[5],disc_bdaddr[4],disc_bdaddr[3],disc_bdaddr[2],disc_bdaddr[1],disc_bdaddr[0]);

                                if(incomingPSController)
                                        connectToHIDDevice = true; // We should always connect to the PS4/PS5 controller

                                // Clear these flags for a new connection
                                l2capConnectionClaimed = false;
                                sdpConnectionClaimed = false;
                                rfcommConnectionClaimed = false;

                                hci_event_flag = 0;
                                hci_state = HCI_DONE_STATE;
                        }
                        break;

                case HCI_DONE_STATE:
                        hci_counter++;
                        if(hci_counter > 1000) { // Wait until we have looped 1000 times to make sure that the L2CAP connection has been started
                                hci_counter = 0;
                                hci_state = HCI_SCANNING_STATE;
                        }
                        break;

                case HCI_DISCONNECT_STATE:
                        if(hci_check_flag(HCI_FLAG_DISCONNECT_COMPLETE)) {

                                ESP_LOGI(LOG_TAG, "HCI Disconnected from Device");

                                hci_event_flag = 0; // Clear all flags

                                // Reset all buffers
                                memset(hcibuf, 0, BULK_MAXPKTSIZE);
                                memset(l2capinbuf, 0, BULK_MAXPKTSIZE);

                                connectToWii = incomingWii = pairWithWii = false;
                                connectToHIDDevice = incomingHIDDevice = pairWithHIDDevice = checkRemoteName = false;
                                incomingPSController = false;

                                hci_state = HCI_SCANNING_STATE;
                        }
                        break;
                default:
                        break;
        }
}

void BTD::ACL_event_task(uint8_t *packet, int size) {
        
        //if(!bPollEnable)
        //        return;

        if(size > 0 ) { // Check for errors
                memcpy(l2capinbuf,packet,size);
                //if(length > 0) { // Check if any data was read
                        for(uint8_t i = 0; i < BTD_NUM_SERVICES; i++) {
                                if(btService[i])
                                        btService[i]->ACLData(l2capinbuf);
                        }
                //}
        }
        for(uint8_t i = 0; i < BTD_NUM_SERVICES; i++)
                if(btService[i])
                        btService[i]->Run();
}

/************************************************************/
/*                    HCI Commands                        */

/************************************************************/
void BTD::HCI_Command(uint8_t* data, uint16_t nbytes) {
        hci_clear_flag(HCI_FLAG_CMD_COMPLETE);
        btd_vhci_send_packet(HCI_COMMAND_DATA_PACKET, data, nbytes);
}

void BTD::hci_reset() {
        hci_event_flag = 0; // Clear all the flags
        hcibuf[0] = 0x03; // HCI OCF = 3
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_write_scan_enable() {
        hci_clear_flag(HCI_FLAG_INCOMING_REQUEST);
        hcibuf[0] = 0x1A; // HCI OCF = 1A
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x01; // parameter length = 1
        if(btdName != NULL)
                hcibuf[3] = 0x03; // Inquiry Scan enabled. Page Scan enabled.
        else
                hcibuf[3] = 0x02; // Inquiry Scan disabled. Page Scan enabled.

        HCI_Command(hcibuf, 4);
}

void BTD::hci_write_scan_disable() {
        hcibuf[0] = 0x1A; // HCI OCF = 1A
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x01; // parameter length = 1
        hcibuf[3] = 0x00; // Inquiry Scan disabled. Page Scan disabled.

        HCI_Command(hcibuf, 4);
}

void BTD::hci_read_bdaddr() {
        hci_clear_flag(HCI_FLAG_READ_BDADDR);
        hcibuf[0] = 0x09; // HCI OCF = 9
        hcibuf[1] = 0x04 << 2; // HCI OGF = 4
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_read_local_version_information() {
        hci_clear_flag(HCI_FLAG_READ_VERSION);
        hcibuf[0] = 0x01; // HCI OCF = 1
        hcibuf[1] = 0x04 << 2; // HCI OGF = 4
        hcibuf[2] = 0x00;

        HCI_Command(hcibuf, 3);
}

void BTD::hci_read_local_extended_features(uint8_t page_number) {
        hci_clear_flag(HCI_FLAG_LOCAL_EXTENDED_FEATURES);
        hcibuf[0] = 0x04; // HCI OCF = 4
        hcibuf[1] = 0x04 << 2; // HCI OGF = 4
        hcibuf[2] = 0x01; // parameter length = 1
        hcibuf[3] = page_number;

        HCI_Command(hcibuf, 4);
}

void BTD::hci_accept_connection() {
        hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE);
        hcibuf[0] = 0x09; // HCI OCF = 9
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x07; // parameter length 7
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x00; // Switch role to master

        HCI_Command(hcibuf, 10);
}

void BTD::hci_remote_name() {
        hci_clear_flag(HCI_FLAG_REMOTE_NAME_COMPLETE);
        hcibuf[0] = 0x19; // HCI OCF = 19
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x0A; // parameter length = 10
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x01; // Page Scan Repetition Mode
        hcibuf[10] = 0x00; // Reserved
        hcibuf[11] = 0x00; // Clock offset - low byte
        hcibuf[12] = 0x00; // Clock offset - high byte

        HCI_Command(hcibuf, 13);
}

void BTD::hci_write_local_name(const char* name) {
        hcibuf[0] = 0x13; // HCI OCF = 13
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = strlen(name) + 1; // parameter length = the length of the string + end byte
        uint8_t i;
        for(i = 0; i < strlen(name); i++)
                hcibuf[i + 3] = name[i];
        hcibuf[i + 3] = 0x00; // End of string

        HCI_Command(hcibuf, 4 + strlen(name));
}

void BTD::hci_set_event_mask() {
        hcibuf[0] = 0x01; // HCI OCF = 01
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x08;
        // The first 6 bytes are the default of 1FFF FFFF FFFF
        // However we need to set bits 48-55 for simple pairing to work
        hcibuf[3] = 0xFF;
        hcibuf[4] = 0xFF;
        hcibuf[5] = 0xFF;
        hcibuf[6] = 0xFF;
        hcibuf[7] = 0xFF;
        hcibuf[8] = 0x1F;
        hcibuf[9] = 0xFF; // Enable bits 48-55 used for simple pairing
        hcibuf[10] = 0x00;

        HCI_Command(hcibuf, 11);
}

void BTD::hci_write_simple_pairing_mode(bool enable) {
        hcibuf[0] = 0x56; // HCI OCF = 56
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 1; // parameter length = 1
        hcibuf[3] = enable ? 1 : 0;

        HCI_Command(hcibuf, 4);
}

void BTD::hci_inquiry() {
        hci_clear_flag(HCI_FLAG_DEVICE_FOUND);
        hcibuf[0] = 0x01;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x05; // Parameter Total Length = 5
        hcibuf[3] = 0x33; // LAP: Genera/Unlimited Inquiry Access Code (GIAC = 0x9E8B33) - see https://www.bluetooth.org/Technical/AssignedNumbers/baseband.htm
        hcibuf[4] = 0x8B;
        hcibuf[5] = 0x9E;
        hcibuf[6] = 0x30; // Inquiry time = 61.44 sec (maximum)
        hcibuf[7] = 0x0A; // 10 number of responses

        HCI_Command(hcibuf, 8);
}

void BTD::hci_inquiry_cancel() {
        hcibuf[0] = 0x02;
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x00; // Parameter Total Length = 0

        HCI_Command(hcibuf, 3);
}

void BTD::hci_connect() {
        hci_connect(disc_bdaddr); // Use last discovered device
}

void BTD::hci_connect(uint8_t *bdaddr) {
        hci_clear_flag(HCI_FLAG_CONNECT_COMPLETE | HCI_FLAG_CONNECT_EVENT);
        hcibuf[0] = 0x05; // HCI OCF = 5
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x0D; // parameter Total Length = 13
        hcibuf[3] = bdaddr[0]; // 6 octet bdaddr (LSB)
        hcibuf[4] = bdaddr[1];
        hcibuf[5] = bdaddr[2];
        hcibuf[6] = bdaddr[3];
        hcibuf[7] = bdaddr[4];
        hcibuf[8] = bdaddr[5];
        hcibuf[9] = 0x18; // DM1 or DH1 may be used
        hcibuf[10] = 0xCC; // DM3, DH3, DM5, DH5 may be used
        hcibuf[11] = 0x01; // Page repetition mode R1
        hcibuf[12] = 0x00; // Reserved
        hcibuf[13] = 0x00; // Clock offset
        hcibuf[14] = 0x00; // Invalid clock offset
        hcibuf[15] = 0x00; // Do not allow role switch

        HCI_Command(hcibuf, 16);
}

void BTD::hci_pin_code_request_reply() {
        hcibuf[0] = 0x0D; // HCI OCF = 0D
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x17; // parameter length 23
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        if(pairWithWii) {
                hcibuf[9] = 6; // Pin length is the length of the Bluetooth address
                if(pairWiiUsingSync) {

                        ESP_LOGD(LOG_TAG, "Pairing with Wii controller via SYNC");

                        for(uint8_t i = 0; i < 6; i++)
                                hcibuf[10 + i] = my_bdaddr[i]; // The pin is the Bluetooth dongles Bluetooth address backwards
                } else {
                        for(uint8_t i = 0; i < 6; i++)
                                hcibuf[10 + i] = disc_bdaddr[i]; // The pin is the Wiimote's Bluetooth address backwards
                }
                for(uint8_t i = 16; i < 26; i++)
                        hcibuf[i] = 0x00; // The rest should be 0
        } else {
                hcibuf[9] = strlen(btdPin); // Length of pin
                uint8_t i;
                for(i = 0; i < strlen(btdPin); i++) // The maximum size of the pin is 16
                        hcibuf[i + 10] = btdPin[i];
                for(; i < 16; i++)
                        hcibuf[i + 10] = 0x00; // The rest should be 0
        }

        HCI_Command(hcibuf, 26);
}

void BTD::hci_pin_code_negative_request_reply() {
        hcibuf[0] = 0x0E; // HCI OCF = 0E
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x06; // parameter length 6
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];

        HCI_Command(hcibuf, 9);
}

void BTD::hci_link_key_request_negative_reply() {
        hcibuf[0] = 0x0C; // HCI OCF = 0C
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x06; // parameter length 6
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];

        HCI_Command(hcibuf, 9);
}

void BTD::hci_io_capability_request_reply() {
        hcibuf[0] = 0x2B; // HCI OCF = 2B
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x09;
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];
        hcibuf[9] = 0x03; // NoInputNoOutput
        hcibuf[10] = 0x00; // OOB authentication data not present
        hcibuf[11] = 0x00; // MITM Protection Not Required – No Bonding. Numeric comparison with automatic accept allowed

        HCI_Command(hcibuf, 12);
}

void BTD::hci_user_confirmation_request_reply() {
        hcibuf[0] = 0x2C; // HCI OCF = 2C
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x06; // parameter length 6
        hcibuf[3] = disc_bdaddr[0]; // 6 octet bdaddr
        hcibuf[4] = disc_bdaddr[1];
        hcibuf[5] = disc_bdaddr[2];
        hcibuf[6] = disc_bdaddr[3];
        hcibuf[7] = disc_bdaddr[4];
        hcibuf[8] = disc_bdaddr[5];

        HCI_Command(hcibuf, 9);
}

void BTD::hci_authentication_request() {
        hcibuf[0] = 0x11; // HCI OCF = 11
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x02; // parameter length = 2
        hcibuf[3] = (uint8_t)(hci_handle & 0xFF); //connection handle - low byte
        hcibuf[4] = (uint8_t)((hci_handle >> 8) & 0x0F); //connection handle - high byte

        HCI_Command(hcibuf, 5);
}

void BTD::hci_disconnect(uint16_t handle) { // This is called by the different services
        hci_clear_flag(HCI_FLAG_DISCONNECT_COMPLETE);
        hcibuf[0] = 0x06; // HCI OCF = 6
        hcibuf[1] = 0x01 << 2; // HCI OGF = 1
        hcibuf[2] = 0x03; // parameter length = 3
        hcibuf[3] = (uint8_t)(handle & 0xFF); //connection handle - low byte
        hcibuf[4] = (uint8_t)((handle >> 8) & 0x0F); //connection handle - high byte
        hcibuf[5] = 0x13; // reason

        HCI_Command(hcibuf, 6);
}

void BTD::hci_write_class_of_device() { // See http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
        hcibuf[0] = 0x24; // HCI OCF = 24
        hcibuf[1] = 0x03 << 2; // HCI OGF = 3
        hcibuf[2] = 0x03; // parameter length = 3
        hcibuf[3] = 0x04; // Robot
        hcibuf[4] = 0x08; // Toy
        hcibuf[5] = 0x00;

        HCI_Command(hcibuf, 6);
}
/*******************************************************************
 *                                                                 *
 *                        HCI ACL Data Packet                      *
 *                                                                 *
 *   buf[0]          buf[1]          buf[2]          buf[3]
 *   0       4       8    11 12      16              24            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |      HCI Handle       |PB |BC |       Data Total Length       |   HCI ACL Data Packet
 *  .-+-+-+-+-+-+-+-|-+-+-+-|-+-|-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *
 *   buf[4]          buf[5]          buf[6]          buf[7]
 *   0               8               16                            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |            Length             |          Channel ID           |   Basic L2CAP header
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *
 *   buf[8]          buf[9]          buf[10]         buf[11]
 *   0               8               16                            31 MSB
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.
 *  |     Code      |  Identifier   |            Length             |   Control frame (C-frame)
 *  .-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-.   (signaling packet format)
 */
/************************************************************/
/*                    L2CAP Commands                        */

/************************************************************/
void BTD::L2CAP_Command(uint16_t handle, uint8_t* data, uint8_t nbytes, uint8_t channelLow, uint8_t channelHigh) {
        uint8_t buf[8 + nbytes];
        buf[0] = (uint8_t)(handle & 0xff); // HCI handle with PB,BC flag
        buf[1] = (uint8_t)(((handle >> 8) & 0x0f) | 0x20);
        buf[2] = (uint8_t)((4 + nbytes) & 0xff); // HCI ACL total data length
        buf[3] = (uint8_t)((4 + nbytes) >> 8);
        buf[4] = (uint8_t)(nbytes & 0xff); // L2CAP header: Length
        buf[5] = (uint8_t)(nbytes >> 8);
        buf[6] = channelLow;
        buf[7] = channelHigh;

        for(uint16_t i = 0; i < nbytes; i++) // L2CAP C-frame
                buf[8 + i] = data[i];


        btd_vhci_send_packet(HCI_ACL_DATA_PACKET, buf, 8+nbytes);
}

void BTD::l2cap_connection_request(uint16_t handle, uint8_t rxid, uint8_t* scid, uint16_t psm) {
        l2capoutbuf[0] = L2CAP_CMD_CONNECTION_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = (uint8_t)(psm & 0xff); // PSM
        l2capoutbuf[5] = (uint8_t)(psm >> 8);
        l2capoutbuf[6] = scid[0]; // Source CID
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_connection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid, uint8_t result) {
        l2capoutbuf[0] = L2CAP_CMD_CONNECTION_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0]; // Destination CID
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0]; // Source CID
        l2capoutbuf[7] = scid[1];
        l2capoutbuf[8] = result; // Result: Pending or Success
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x00; // No further information
        l2capoutbuf[11] = 0x00;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

void BTD::l2cap_config_request(uint16_t handle, uint8_t rxid, uint8_t* dcid) {
        l2capoutbuf[0] = L2CAP_CMD_CONFIG_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0]; // Destination CID
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = 0x00; // Flags
        l2capoutbuf[7] = 0x00;
        l2capoutbuf[8] = 0x01; // Config Opt: type = MTU (Maximum Transmission Unit) - Hint
        l2capoutbuf[9] = 0x02; // Config Opt: length
        l2capoutbuf[10] = 0xFF; // MTU
        l2capoutbuf[11] = 0xFF;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

void BTD::l2cap_config_response(uint16_t handle, uint8_t rxid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_CONFIG_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x0A; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = scid[0]; // Source CID
        l2capoutbuf[5] = scid[1];
        l2capoutbuf[6] = 0x00; // Flag
        l2capoutbuf[7] = 0x00;
        l2capoutbuf[8] = 0x00; // Result
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x01; // Config
        l2capoutbuf[11] = 0x02;
        l2capoutbuf[12] = 0xA0;
        l2capoutbuf[13] = 0x02;

        L2CAP_Command(handle, l2capoutbuf, 14);
}

void BTD::l2cap_disconnection_request(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_REQUEST; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0];
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0];
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_disconnection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid) {
        l2capoutbuf[0] = L2CAP_CMD_DISCONNECT_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x04; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = dcid[0];
        l2capoutbuf[5] = dcid[1];
        l2capoutbuf[6] = scid[0];
        l2capoutbuf[7] = scid[1];

        L2CAP_Command(handle, l2capoutbuf, 8);
}

void BTD::l2cap_information_response(uint16_t handle, uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh) {
        l2capoutbuf[0] = L2CAP_CMD_INFORMATION_RESPONSE; // Code
        l2capoutbuf[1] = rxid; // Identifier
        l2capoutbuf[2] = 0x08; // Length
        l2capoutbuf[3] = 0x00;
        l2capoutbuf[4] = infoTypeLow;
        l2capoutbuf[5] = infoTypeHigh;
        l2capoutbuf[6] = 0x00; // Result = success
        l2capoutbuf[7] = 0x00; // Result = success
        l2capoutbuf[8] = 0x00;
        l2capoutbuf[9] = 0x00;
        l2capoutbuf[10] = 0x00;
        l2capoutbuf[11] = 0x00;

        L2CAP_Command(handle, l2capoutbuf, 12);
}

