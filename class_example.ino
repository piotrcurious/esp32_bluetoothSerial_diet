#include <esp_bt.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_spp_api.h>

#define DEVICE_NAME "ESP32_BT"  // Bluetooth device name

// Minimal SerialBT class implementation
class SerialBTClass {
public:
    SerialBTClass() : isConnected(false), spp_handle(0) {}

    // Initializes Bluetooth and sets up SPP
    bool begin(const char* name = DEVICE_NAME) {
        esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        if (esp_bt_controller_init(&bt_cfg) != ESP_OK) {
            return false;
        }
        if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
            return false;
        }
        if (esp_bluedroid_init() != ESP_OK || esp_bluedroid_enable() != ESP_OK) {
            return false;
        }

        // Register callback and initialize SPP
        esp_spp_register_callback(bt_event_handler);
        esp_spp_init(ESP_SPP_MODE_CB);
        esp_bt_dev_set_device_name(name);
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");

        return true;
    }

    // Returns true if connected to a client
    bool connected() {
        return isConnected;
    }

    // Returns the number of bytes available to read
    int available() {
        return rx_buffer.length();
    }

    // Reads a single byte
    int read() {
        if (rx_buffer.length() > 0) {
            int byte = rx_buffer[0];
            rx_buffer.remove(0, 1);
            return byte;
        }
        return -1;
    }

    // Writes a single byte or string
    size_t write(uint8_t byte) {
        if (spp_handle) {
            esp_spp_write(spp_handle, 1, &byte);
            return 1;
        }
        return 0;
    }

    size_t write(const uint8_t *buffer, size_t size) {
        if (spp_handle) {
            esp_spp_write(spp_handle, size, buffer);
            return size;
        }
        return 0;
    }

    // Prints a string with a newline
    void println(const char* message) {
        write((const uint8_t*)message, strlen(message));
        write((const uint8_t*)"\r\n", 2);
    }

private:
    static bool isConnected;
    static uint32_t spp_handle;
    static String rx_buffer;

    // Bluetooth event handler function
    static void bt_event_handler(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
        switch (event) {
            case ESP_SPP_INIT_EVT:
                printf("SPP initialized\n");
                break;

            case ESP_SPP_START_EVT:
                printf("SPP server started\n");
                break;

            case ESP_SPP_DATA_IND_EVT:
                // Append incoming data to buffer
                rx_buffer.concat((char*)param->data_ind.data, param->data_ind.len);
                break;

            case ESP_SPP_SRV_OPEN_EVT:
                // Client connected
                spp_handle = param->srv_open.handle;
                isConnected = true;
                printf("Client connected\n");
                break;

            case ESP_SPP_CLOSE_EVT:
                // Client disconnected
                isConnected = false;
                spp_handle = 0;
                printf("Client disconnected\n");
                break;

            default:
                break;
        }
    }
};

// Static variable definitions
bool SerialBTClass::isConnected = false;
uint32_t SerialBTClass::spp_handle = 0;
String SerialBTClass::rx_buffer = "";

// Create an instance of SerialBT
SerialBTClass SerialBT;

// Hardware Serial to Bluetooth Serial bridge example
void setup() {
    Serial.begin(115200);             // Start hardware serial for debugging
    SerialBT.begin("ESP32_BT_Serial"); // Start Bluetooth serial

    Serial.println("Bluetooth Serial started");
}

void loop() {
    // If data is available on hardware serial, send it to Bluetooth
    if (Serial.available()) {
        char incoming = Serial.read();
        SerialBT.write(incoming);
    }

    // If data is available on Bluetooth, send it to hardware serial
    if (SerialBT.available()) {
        char incoming = SerialBT.read();
        Serial.write(incoming);
    }
}
