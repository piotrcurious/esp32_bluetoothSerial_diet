#include <esp_bt.h>
#include <esp_gap_bt_api.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_spp_api.h>

#define DEVICE_NAME "ESP32_BT"   // Bluetooth device name
#define RX_BUFFER_SIZE 1024      // Size of the receive buffer
#define TX_BUFFER_SIZE 1024      // Size of the transmit buffer
#define BRIDGE_BUFFER_SIZE 256   // Intermediate buffer size for bridging

// Minimal SerialBT class implementation with buffering and flow control
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
        return rx_buffer_length;
    }

    // Reads a single byte
    int read() {
        if (rx_buffer_length > 0) {
            int byte = rx_buffer[rx_buffer_read_index];
            rx_buffer_read_index = (rx_buffer_read_index + 1) % RX_BUFFER_SIZE;
            rx_buffer_length--;
            return byte;
        }
        return -1;
    }

    // Writes a single byte or string
    size_t write(uint8_t byte) {
        if (spp_handle && tx_buffer_length < TX_BUFFER_SIZE) {
            tx_buffer[tx_buffer_write_index] = byte;
            tx_buffer_write_index = (tx_buffer_write_index + 1) % TX_BUFFER_SIZE;
            tx_buffer_length++;
            process_tx_buffer();
            return 1;
        }
        return 0;
    }

    size_t write(const uint8_t *buffer, size_t size) {
        size_t bytes_written = 0;
        while (size--) {
            if (write(*buffer++)) {
                bytes_written++;
            } else {
                break; // Stop writing if buffer is full
            }
        }
        return bytes_written;
    }

    // Prints a string with a newline
    void println(const char* message) {
        write((const uint8_t*)message, strlen(message));
        write((const uint8_t*)"\r\n", 2);
    }

private:
    static bool isConnected;
    static uint32_t spp_handle;
    static uint8_t rx_buffer[RX_BUFFER_SIZE];
    static size_t rx_buffer_read_index;
    static size_t rx_buffer_write_index;
    static size_t rx_buffer_length;
    static uint8_t tx_buffer[TX_BUFFER_SIZE];
    static size_t tx_buffer_read_index;
    static size_t tx_buffer_write_index;
    static size_t tx_buffer_length;

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
                // Store incoming data into the receive buffer
                for (int i = 0; i < param->data_ind.len; i++) {
                    if (rx_buffer_length < RX_BUFFER_SIZE) {
                        rx_buffer[rx_buffer_write_index] = param->data_ind.data[i];
                        rx_buffer_write_index = (rx_buffer_write_index + 1) % RX_BUFFER_SIZE;
                        rx_buffer_length++;
                    }
                }
                break;

            case ESP_SPP_SRV_OPEN_EVT:
                spp_handle = param->srv_open.handle;
                isConnected = true;
                printf("Client connected\n");
                break;

            case ESP_SPP_CLOSE_EVT:
                isConnected = false;
                spp_handle = 0;
                printf("Client disconnected\n");
                break;

            case ESP_SPP_WRITE_EVT:
                // Remove written bytes from the transmit buffer
                tx_buffer_read_index = (tx_buffer_read_index + param->write.len) % TX_BUFFER_SIZE;
                tx_buffer_length -= param->write.len;
                break;

            default:
                break;
        }
    }

    // Processes the transmit buffer and sends data over Bluetooth
    static void process_tx_buffer() {
        if (spp_handle && tx_buffer_length > 0) {
            size_t to_send = min(tx_buffer_length, ESP_SPP_MAX_MTU);
            esp_spp_write(spp_handle, to_send, &tx_buffer[tx_buffer_read_index]);
        }
    }
};

// Static variable definitions
bool SerialBTClass::isConnected = false;
uint32_t SerialBTClass::spp_handle = 0;
uint8_t SerialBTClass::rx_buffer[RX_BUFFER_SIZE];
size_t SerialBTClass::rx_buffer_read_index = 0;
size_t SerialBTClass::rx_buffer_write_index = 0;
size_t SerialBTClass::rx_buffer_length = 0;
uint8_t SerialBTClass::tx_buffer[TX_BUFFER_SIZE];
size_t SerialBTClass::tx_buffer_read_index = 0;
size_t SerialBTClass::tx_buffer_write_index = 0;
size_t SerialBTClass::tx_buffer_length = 0;

// Create an instance of SerialBT
SerialBTClass SerialBT;

// Intermediate buffer for bridging data between hardware and Bluetooth serial
uint8_t bridge_buffer[BRIDGE_BUFFER_SIZE];
size_t bridge_buffer_length = 0;

// Hardware Serial to Bluetooth Serial bridge example with buffer handling
void setup() {
    Serial.begin(115200);             // Start hardware serial for debugging
    SerialBT.begin("ESP32_BT_Serial"); // Start Bluetooth serial

    Serial.println("Bluetooth Serial started");
}

void loop() {
    // Read data from hardware serial into the intermediate buffer
    while (Serial.available() && bridge_buffer_length < BRIDGE_BUFFER_SIZE) {
        bridge_buffer[bridge_buffer_length++] = Serial.read();
    }

    // Write data from the intermediate buffer to Bluetooth, respecting buffer limits
    size_t bytes_sent = SerialBT.write(bridge_buffer, bridge_buffer_length);
    if (bytes_sent > 0) {
        // Remove sent bytes from the bridge buffer
        memmove(bridge_buffer, bridge_buffer + bytes_sent, bridge_buffer_length - bytes_sent);
        bridge_buffer_length -= bytes_sent;
    }

    // Read data from Bluetooth and send it to hardware serial
    while (SerialBT.available()) {
        Serial.write(SerialBT.read());
    }
}
