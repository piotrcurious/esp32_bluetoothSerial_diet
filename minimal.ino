#include <esp_bt.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_spp_api.h>

// Define your Bluetooth device name
#define DEVICE_NAME "ESP32_BT"

// SPP Server parameters
static const esp_spp_mode_t spp_mode = ESP_SPP_MODE_CB; // Callback mode
static const esp_spp_sec_t security = ESP_SPP_SEC_NONE;
static const esp_spp_role_t role = ESP_SPP_ROLE_SLAVE;

// Bluetooth event handler
void bt_event_handler(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            esp_bt_dev_set_device_name(DEVICE_NAME);
            esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
            esp_spp_start_srv(security, role, 0, "SPP_SERVER");
            break;

        case ESP_SPP_START_EVT:
            printf("SPP server started\n");
            break;

        case ESP_SPP_DATA_IND_EVT:
            // Handle received data
            printf("Received data: %.*s\n", param->data_ind.len, param->data_ind.data);
            break;

        case ESP_SPP_WRITE_EVT:
            printf("Data written\n");
            break;

        case ESP_SPP_SRV_OPEN_EVT:
            printf("Client connected\n");
            break;

        case ESP_SPP_CLOSE_EVT:
            printf("Client disconnected\n");
            break;

        default:
            break;
    }
}

void setup() {
    // Initialize the Serial for debug purposes
    Serial.begin(115200);

    // Release memory used by Bluetooth classic
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    // Bluetooth controller initialization
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);

    // Bluedroid stack initialization and enabling
    esp_bluedroid_init();
    esp_bluedroid_enable();

    // SPP profile initialization and event handler registration
    esp_spp_register_callback(bt_event_handler);
    esp_spp_init(spp_mode);
}

void loop() {
    // You can implement read/write operations here
    // For example, to write data to the Bluetooth serial:
    const char* message = "Hello, Bluetooth!";
    esp_spp_write(0, strlen(message), (uint8_t*)message); // Write to client (if connected)
}
