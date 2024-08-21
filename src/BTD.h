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

#ifndef _btd_h_
#define _btd_h_

#include "extras.h"

#define HCI_COMMAND_DATA_PACKET 0x01
#define HCI_ACL_DATA_PACKET     0x02
#define HCI_EVENT_PACKET        0x04

//PID and VID of the Sony PS3 devices
#define PS3_VID                 0x054C  // Sony Corporation
#define PS3_PID                 0x0268  // PS3 Controller DualShock 3
#define PS3NAVIGATION_PID       0x042F  // Navigation controller
#define PS3MOVE_PID             0x03D5  // Motion controller

// These dongles do not present themselves correctly, so we have to check for them manually
#define IOGEAR_GBU521_VID       0x0A5C
#define IOGEAR_GBU521_PID       0x21E8
#define BELKIN_F8T065BF_VID     0x050D
#define BELKIN_F8T065BF_PID     0x065A

/* Bluetooth dongle data taken from descriptors */
#define BULK_MAXPKTSIZE         2048 // Max size for ACL data

// Used in control endpoint header for HCI Commands
#define bmREQ_HCI_OUT USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE

/* Bluetooth HCI states for hci_task() */
#define HCI_INIT_STATE                  0
#define HCI_RESET_STATE                 1
#define HCI_CLASS_STATE                 2
#define HCI_BDADDR_STATE                3
#define HCI_LOCAL_VERSION_STATE         4
#define HCI_WRITE_NAME_STATE            5
#define HCI_CHECK_DEVICE_SERVICE        6

#define HCI_INQUIRY_STATE               7 // These three states are only used if it should pair and connect to a device
#define HCI_CONNECT_DEVICE_STATE        8
#define HCI_CONNECTED_DEVICE_STATE      9

#define HCI_SCANNING_STATE              10
#define HCI_CONNECT_IN_STATE            11
#define HCI_REMOTE_NAME_STATE           12
#define HCI_CONNECTED_STATE             13
#define HCI_DISABLE_SCAN_STATE          14
#define HCI_DONE_STATE                  15
#define HCI_DISCONNECT_STATE            16
#define HCI_LOCAL_EXTENDED_FEATURES_STATE       17
#define HCI_WRITE_SIMPLE_PAIRING_STATE          18
#define HCI_SET_EVENT_MASK_STATE                19

/* HCI event flags*/
#define HCI_FLAG_CMD_COMPLETE           (1UL << 0)
#define HCI_FLAG_CONNECT_COMPLETE       (1UL << 1)
#define HCI_FLAG_DISCONNECT_COMPLETE    (1UL << 2)
#define HCI_FLAG_REMOTE_NAME_COMPLETE   (1UL << 3)
#define HCI_FLAG_INCOMING_REQUEST       (1UL << 4)
#define HCI_FLAG_READ_BDADDR            (1UL << 5)
#define HCI_FLAG_READ_VERSION           (1UL << 6)
#define HCI_FLAG_DEVICE_FOUND           (1UL << 7)
#define HCI_FLAG_CONNECT_EVENT          (1UL << 8)
#define HCI_FLAG_LOCAL_EXTENDED_FEATURES    (1UL << 9)

/* Macros for HCI event flag tests */
#define hci_check_flag(flag) (hci_event_flag & (flag))
#define hci_set_flag(flag) (hci_event_flag |= (flag))
#define hci_clear_flag(flag) (hci_event_flag &= ~(flag))

/* HCI Events managed */
#define EV_INQUIRY_COMPLETE                             0x01
#define EV_INQUIRY_RESULT                               0x02
#define EV_CONNECT_COMPLETE                             0x03
#define EV_INCOMING_CONNECT                             0x04
#define EV_DISCONNECT_COMPLETE                          0x05
#define EV_AUTHENTICATION_COMPLETE                      0x06
#define EV_REMOTE_NAME_COMPLETE                         0x07
#define EV_ENCRYPTION_CHANGE                            0x08
#define EV_CHANGE_CONNECTION_LINK                       0x09
#define EV_READ_REMOTE_VERSION_INFORMATION_COMPLETE     0x0C
#define EV_QOS_SETUP_COMPLETE                           0x0D
#define EV_COMMAND_COMPLETE                             0x0E
#define EV_COMMAND_STATUS                               0x0F
#define EV_ROLE_CHANGED                                 0x12
#define EV_NUM_COMPLETE_PKT                             0x13
#define EV_PIN_CODE_REQUEST                             0x16
#define EV_LINK_KEY_REQUEST                             0x17
#define EV_LINK_KEY_NOTIFICATION                        0x18
#define EV_DATA_BUFFER_OVERFLOW                         0x1A
#define EV_MAX_SLOTS_CHANGE                             0x1B
#define EV_LOOPBACK_COMMAND                             0x19
#define EV_PAGE_SCAN_REP_MODE                           0x20
#define EV_READ_REMOTE_EXTENDED_FEATURES_COMPLETE       0x23
#define EV_EXTENDED_INQUIRY_RESULT                      0x2F
#define EV_IO_CAPABILITY_REQUEST                        0x31
#define EV_IO_CAPABILITY_RESPONSE                       0x32
#define EV_USER_CONFIRMATION_REQUEST                    0x33
#define EV_SIMPLE_PAIRING_COMPLETE                      0x36

/* Bluetooth states for the different Bluetooth drivers */
#define L2CAP_WAIT                      0
#define L2CAP_DONE                      1

/* Used for HID Control channel */
#define L2CAP_CONTROL_CONNECT_REQUEST   2
#define L2CAP_CONTROL_CONFIG_REQUEST    3
#define L2CAP_CONTROL_SUCCESS           4
#define L2CAP_CONTROL_DISCONNECT        5

/* Used for HID Interrupt channel */
#define L2CAP_INTERRUPT_SETUP           6
#define L2CAP_INTERRUPT_CONNECT_REQUEST 7
#define L2CAP_INTERRUPT_CONFIG_REQUEST  8
#define L2CAP_INTERRUPT_DISCONNECT      9

/* Used for SDP channel */
#define L2CAP_SDP_WAIT                  10
#define L2CAP_SDP_SUCCESS               11

/* Used for RFCOMM channel */
#define L2CAP_RFCOMM_WAIT               12
#define L2CAP_RFCOMM_SUCCESS            13

#define L2CAP_DISCONNECT_RESPONSE       14 // Used for both SDP and RFCOMM channel

/* Bluetooth states used by some drivers */
#define TURN_ON_LED                     17
#define PS3_ENABLE_SIXAXIS              18
#define WII_CHECK_MOTION_PLUS_STATE     19
#define WII_CHECK_EXTENSION_STATE       20
#define WII_INIT_MOTION_PLUS_STATE      21

/* L2CAP event flags for HID Control channel */
#define L2CAP_FLAG_CONNECTION_CONTROL_REQUEST           (1UL << 0)
#define L2CAP_FLAG_CONFIG_CONTROL_SUCCESS               (1UL << 1)
#define L2CAP_FLAG_CONTROL_CONNECTED                    (1UL << 2)
#define L2CAP_FLAG_DISCONNECT_CONTROL_RESPONSE          (1UL << 3)

/* L2CAP event flags for HID Interrupt channel */
#define L2CAP_FLAG_CONNECTION_INTERRUPT_REQUEST         (1UL << 4)
#define L2CAP_FLAG_CONFIG_INTERRUPT_SUCCESS             (1UL << 5)
#define L2CAP_FLAG_INTERRUPT_CONNECTED                  (1UL << 6)
#define L2CAP_FLAG_DISCONNECT_INTERRUPT_RESPONSE        (1UL << 7)

/* L2CAP event flags for SDP channel */
#define L2CAP_FLAG_CONNECTION_SDP_REQUEST               (1UL << 8)
#define L2CAP_FLAG_CONFIG_SDP_SUCCESS                   (1UL << 9)
#define L2CAP_FLAG_DISCONNECT_SDP_REQUEST               (1UL << 10)

/* L2CAP event flags for RFCOMM channel */
#define L2CAP_FLAG_CONNECTION_RFCOMM_REQUEST            (1UL << 11)
#define L2CAP_FLAG_CONFIG_RFCOMM_SUCCESS                (1UL << 12)
#define L2CAP_FLAG_DISCONNECT_RFCOMM_REQUEST            (1UL << 13)

#define L2CAP_FLAG_DISCONNECT_RESPONSE                  (1UL << 14)

/* Macros for L2CAP event flag tests */
#define l2cap_check_flag(flag) (l2cap_event_flag & (flag))
#define l2cap_set_flag(flag) (l2cap_event_flag |= (flag))
#define l2cap_clear_flag(flag) (l2cap_event_flag &= ~(flag))

/* L2CAP signaling commands */
#define L2CAP_CMD_COMMAND_REJECT        0x01
#define L2CAP_CMD_CONNECTION_REQUEST    0x02
#define L2CAP_CMD_CONNECTION_RESPONSE   0x03
#define L2CAP_CMD_CONFIG_REQUEST        0x04
#define L2CAP_CMD_CONFIG_RESPONSE       0x05
#define L2CAP_CMD_DISCONNECT_REQUEST    0x06
#define L2CAP_CMD_DISCONNECT_RESPONSE   0x07
#define L2CAP_CMD_INFORMATION_REQUEST   0x0A
#define L2CAP_CMD_INFORMATION_RESPONSE  0x0B

// Used For Connection Response - Remember to Include High Byte
#define PENDING     0x01
#define SUCCESSFUL  0x00

/* Bluetooth L2CAP PSM - see http://www.bluetooth.org/Technical/AssignedNumbers/logical_link.htm */
#define SDP_PSM         0x01 // Service Discovery Protocol PSM Value
#define RFCOMM_PSM      0x03 // RFCOMM PSM Value
#define HID_CTRL_PSM    0x11 // HID_Control PSM Value
#define HID_INTR_PSM    0x13 // HID_Interrupt PSM Value

/* Used for SDP */
#define SDP_SERVICE_SEARCH_REQUEST                  0x02
#define SDP_SERVICE_SEARCH_RESPONSE                 0x03
#define SDP_SERVICE_ATTRIBUTE_REQUEST               0x04
#define SDP_SERVICE_ATTRIBUTE_RESPONSE              0x05
#define SDP_SERVICE_SEARCH_ATTRIBUTE_REQUEST        0x06 // See the RFCOMM specs
#define SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE       0x07 // See the RFCOMM specs
#define PNP_INFORMATION_UUID    0x1200
#define SERIALPORT_UUID         0x1101 // See http://www.bluetooth.org/Technical/AssignedNumbers/service_discovery.htm
#define L2CAP_UUID              0x0100

// Used to determine if it is a Bluetooth dongle
#define WI_SUBCLASS_RF      0x01 // RF Controller
#define WI_PROTOCOL_BT      0x01 // Bluetooth Programming Interface

#define BTD_MAX_ENDPOINTS   4
#define BTD_NUM_SERVICES    4 // Max number of Bluetooth services - if you need more than 4 simply increase this number

#define PAIR    1

class BluetoothService;

/**
 * The Bluetooth Dongle class will take care of all the USB communication
 * and then pass the data to the BluetoothService classes.
 */
class BTD  {
public:
        static BTD* instance() {
                static BTD inst;
                return &inst;
        }

        /**
         * Initialize the Bluetooth dongle.
         * @param  parent   Hub number.
         * @param  port     Port number on the hub.
         * @param  lowspeed Speed of the device.
         * @return          0 on success.
         */
        uint8_t Init();
        /**
         * Release the USB device.
         * @return 0 on success.
         */
        uint8_t Release();


        /**
         * Used to check if the dongle has been initialized.
         * @return True if it's ready.
         */
        virtual bool isReady() {
                return bPollEnable;
        };

        /** Disconnects both the L2CAP Channel and the HCI Connection for all Bluetooth services. */
        void disconnect();

        /**
         * Register Bluetooth dongle members/services.
         * @param  pService Pointer to BluetoothService class instance.
         * @return          The service ID on success or -1 on fail.
         */
        int8_t registerBluetoothService(BluetoothService *pService) {
                for(uint8_t i = 0; i < BTD_NUM_SERVICES; i++) {
                        if(!btService[i]) {
                                btService[i] = pService;
                                return i; // Return ID
                        }
                }
                return -1; // Error registering BluetoothService
        };

        /** @name HCI Commands */
        /**
         * Used to send a HCI Command.
         * @param data   Data to send.
         * @param nbytes Number of bytes to send.
         */
        void HCI_Command(uint8_t* data, uint16_t nbytes);
        /** Reset the Bluetooth dongle. */
        void hci_reset();
        /** Read the Bluetooth address of the dongle. */
        void hci_read_bdaddr();
        /** Read the HCI Version of the Bluetooth dongle. */
        void hci_read_local_version_information();
        /** Used to check if the dongle supports simple paring */
        void hci_read_local_extended_features(uint8_t page_number);
        /**
         * Set the local name of the Bluetooth dongle.
         * @param name Desired name.
         */
        void hci_write_local_name(const char* name);
        /** Used to enable simply paring if the dongle supports it */
        void hci_write_simple_pairing_mode(bool enable);
        /** Used to enable events related to simple paring */
        void hci_set_event_mask();
        /** Enable visibility to other Bluetooth devices. */
        void hci_write_scan_enable();
        /** Disable visibility to other Bluetooth devices. */
        void hci_write_scan_disable();
        /** Read the remote devices name. */
        void hci_remote_name();
        /** Accept the connection with the Bluetooth device. */
        void hci_accept_connection();
        /**
         * Disconnect the HCI connection.
         * @param handle The HCI Handle for the connection.
         */
        void hci_disconnect(uint16_t handle);
        /**
         * Respond with the pin for the connection.
         * The pin is automatically set for the Wii library,
         * but can be customized for the SPP library.
         */
        void hci_pin_code_request_reply();
        /** Respons when no pin was set. */
        void hci_pin_code_negative_request_reply();
        /**
         * Command is used to reply to a Link Key Request event from the BR/EDR Controller
         * if the Host does not have a stored Link Key for the connection.
         */
        void hci_link_key_request_negative_reply();
        /** Used to during simple paring to confirm that the we want to connect */
        void hci_user_confirmation_request_reply();
        /** Used to try to authenticate with the remote device. */
        void hci_authentication_request();
        /** Start a HCI inquiry. */
        void hci_inquiry();
        /** Cancel a HCI inquiry. */
        void hci_inquiry_cancel();
        /** Connect to last device communicated with. */
        void hci_connect();
        /** Used during simple paring to reply to a IO capability request */
        void hci_io_capability_request_reply();
        /**
         * Connect to device.
         * @param bdaddr Bluetooth address of the device.
         */
        void hci_connect(uint8_t *bdaddr);
        /** Used to a set the class of the device. */
        void hci_write_class_of_device();
        /**@}*/

        /** @name L2CAP Commands */
        /**
         * Used to send L2CAP Commands.
         * @param handle      HCI Handle.
         * @param data        Data to send.
         * @param nbytes      Number of bytes to send.
         * @param channelLow,channelHigh  Low and high byte of channel to send to.
         * If argument is omitted then the Standard L2CAP header: Channel ID (0x01) for ACL-U will be used.
         */
        void L2CAP_Command(uint16_t handle, uint8_t* data, uint8_t nbytes, uint8_t channelLow = 0x01, uint8_t channelHigh = 0x00);
        /**
         * L2CAP Connection Request.
         * @param handle HCI handle.
         * @param rxid   Identifier.
         * @param scid   Source Channel Identifier.
         * @param psm    Protocol/Service Multiplexer - see: https://www.bluetooth.org/Technical/AssignedNumbers/logical_link.htm.
         */
        void l2cap_connection_request(uint16_t handle, uint8_t rxid, uint8_t* scid, uint16_t psm);
        /**
         * L2CAP Connection Response.
         * @param handle HCI handle.
         * @param rxid   Identifier.
         * @param dcid   Destination Channel Identifier.
         * @param scid   Source Channel Identifier.
         * @param result Result - First send ::PENDING and then ::SUCCESSFUL.
         */
        void l2cap_connection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid, uint8_t result);
        /**
         * L2CAP Config Request.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param dcid   Destination Channel Identifier.
         */
        void l2cap_config_request(uint16_t handle, uint8_t rxid, uint8_t* dcid);
        /**
         * L2CAP Config Response.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param scid   Source Channel Identifier.
         */
        void l2cap_config_response(uint16_t handle, uint8_t rxid, uint8_t* scid);
        /**
         * L2CAP Disconnection Request.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param dcid   Device Channel Identifier.
         * @param scid   Source Channel Identifier.
         */
        void l2cap_disconnection_request(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid);
        /**
         * L2CAP Disconnection Response.
         * @param handle HCI Handle.
         * @param rxid   Identifier.
         * @param dcid   Device Channel Identifier.
         * @param scid   Source Channel Identifier.
         */
        void l2cap_disconnection_response(uint16_t handle, uint8_t rxid, uint8_t* dcid, uint8_t* scid);
        /**
         * L2CAP Information Response.
         * @param handle       HCI Handle.
         * @param rxid         Identifier.
         * @param infoTypeLow,infoTypeHigh  Infotype.
         */
        void l2cap_information_response(uint16_t handle, uint8_t rxid, uint8_t infoTypeLow, uint8_t infoTypeHigh);
        /**@}*/

        /** Use this to see if it is waiting for a incoming connection. */
        bool waitingForConnection;
        /** This is used by the service to know when to store the device information. */
        bool l2capConnectionClaimed;
        /** This is used by the SPP library to claim the current SDP incoming request. */
        bool sdpConnectionClaimed;
        /** This is used by the SPP library to claim the current RFCOMM incoming request. */
        bool rfcommConnectionClaimed;

        /** The name you wish to make the dongle show up as. It is set automatically by the SPP library. */
        const char* btdName;
        /** The pin you wish to make the dongle use for authentication. It is set automatically by the SPP and BTHID library. */
        const char* btdPin;

        /** The bluetooth dongles Bluetooth address. */
        uint8_t my_bdaddr[6];
        /** HCI handle for the last connection. */
        uint16_t hci_handle;
        /** Last incoming devices Bluetooth address. */
        uint8_t disc_bdaddr[6];
        /** Saved device Bluetooth address for auto connect */
        uint8_t saved_bdaddr[6];
        /** First 30 chars of last remote name. */
        char remote_name[30];
        /**
         * The supported HCI Version read from the Bluetooth dongle.
         * Used by the PS3BT library to check the HCI Version of the Bluetooth dongle,
         * it should be at least 3 to work properly with the library.
         */
        uint8_t hci_version;

        /** Call this function to pair with a Wiimote */
        void pairWithWiimote() {
                pairWithWii = true;
                hci_state = HCI_CHECK_DEVICE_SERVICE;
        };
        /** Used to only send the ACL data to the Wiimote. */
        bool connectToWii;
        /** True if a Wiimote is connecting. */
        bool incomingWii;
        /** True when it should pair with a Wiimote. */
        bool pairWithWii;
        /** True if it's the new Wiimote with the Motion Plus Inside or a Wii U Pro Controller. */
        bool motionPlusInside;
        /** True if it's a Wii U Pro Controller. */
        bool wiiUProController;

        /** Call this function to pair with a HID device */
        void pairWithHID() {
                waitingForConnection = false;
                pairWithHIDDevice = true;
                hci_state = HCI_CHECK_DEVICE_SERVICE;
        };
        /** Used to only send the ACL data to the HID device. */
        bool connectToHIDDevice;
        /** True if a HID device is connecting. */
        bool incomingHIDDevice;
        /** True when it should pair with a device like a mouse or keyboard. */
        bool pairWithHIDDevice;

        /** Used by the drivers to enable simple pairing */
        bool useSimplePairing;

        /* State machines */
        void HCI_event_task(uint8_t *packet, int size); // Poll the HCI event pipe
        void HCI_task(); // HCI state machine
        void ACL_event_task(uint8_t *packet, int size); // ACL input pipe

private:
        /**
         * Constructor for the BTD class.
         * @param  p   Pointer to USB class instance.
         */
        BTD();
        
        void Initialize(); // Set all variables, endpoint structs etc. to default values
        BluetoothService *btService[BTD_NUM_SERVICES];

        bool simple_pairing_supported;
        bool bPollEnable;

        bool pairWiiUsingSync; // True if pairing was done using the Wii SYNC button.
        bool checkRemoteName; // Used to check remote device's name before connecting.
        bool incomingPSController; // True if a PS4/PS5 controller is connecting
        uint8_t classOfDevice[3]; // Class of device of last device

        /* Variables used by high level HCI task */
        uint8_t hci_state; // Current state of Bluetooth HCI connection
        uint16_t hci_counter; // Counter used for Bluetooth HCI reset loops
        uint16_t hci_num_reset_loops; // This value indicate how many times it should read before trying to reset
        uint16_t hci_event_flag; // HCI flags of received Bluetooth events
        uint8_t inquiry_counter;

        uint8_t hcibuf[BULK_MAXPKTSIZE]; // General purpose buffer for HCI data
        uint8_t l2capinbuf[BULK_MAXPKTSIZE]; // General purpose buffer for L2CAP in data
        uint8_t l2capoutbuf[14]; // General purpose buffer for L2CAP out data
};

/** All Bluetooth services should inherit this class. */
class BluetoothService {
public:
        BluetoothService(BTD *p) : pBtd(p) {
                if(pBtd)
                        pBtd->registerBluetoothService(this); // Register it as a Bluetooth service
        };
        /**
         * Used to pass acldata to the Bluetooth service.
         * @param ACLData Pointer to the incoming acldata.
         */
        virtual void ACLData(uint8_t* ACLData) = 0;
        /** Used to run the different state machines in the Bluetooth service. */
        virtual void Run() = 0;
        /** Used to reset the Bluetooth service. */
        virtual void Reset() = 0;
        /** Used to disconnect both the L2CAP Channel and the HCI Connection for the Bluetooth service. */
        virtual void disconnect() = 0;

        /**
         * Used to call your own function when the device is successfully initialized.
         * @param funcOnInit Function to call.
         */
        void attachOnInit(void (*funcOnInit)(void)) {
                pFuncOnInit = funcOnInit; // TODO: This really belong in a class of it's own as it is repeated several times
        };

protected:
        /**
         * Called when a device is successfully initialized.
         * Use attachOnInit(void (*funcOnInit)(void)) to call your own function.
         * This is useful for instance if you want to set the LEDs in a specific way.
         */
        virtual void onInit() = 0;

        /** Used to check if the incoming L2CAP data matches the HCI Handle */
        bool checkHciHandle(uint8_t *buf, uint16_t handle) {
                return (buf[0] == (handle & 0xFF)) && (buf[1] == ((handle >> 8) | 0x20));
        }

        /** Pointer to function called in onInit(). */
        void (*pFuncOnInit)(void);

        /** Pointer to BTD instance. */
        BTD *pBtd;

        /** The HCI Handle for the connection. */
        uint16_t hci_handle;

        /** L2CAP flags of received Bluetooth events. */
        uint32_t l2cap_event_flag;

        /** Identifier for L2CAP commands. */
        uint8_t identifier;
};

#endif
