#include <Arduino.h>
#include <BluetoothLock.h>
#include <btstack.h>
#include <WiFi.h>

static constexpr int WOL_PORT = 9;
static constexpr bd_addr_t console_mac = {MAC_PS3};
static constexpr bd_addr_t device_mac = {MAC_DS3};
static constexpr hci_cmd_t write_bd_addr = {.opcode = 0xFC01, .format = "B"};
static btstack_packet_callback_registration_t event_handler;
static WiFiUDP udp;

static void packet_handler(const uint8_t packet_type, const uint16_t, uint8_t *packet, const uint16_t) {
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    const uint8_t event = hci_event_packet_get_type(packet);
    if (event == BTSTACK_EVENT_STATE && btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
        hci_send_cmd(&write_bd_addr, device_mac);
        return;
    }
    if (event != HCI_EVENT_COMMAND_COMPLETE || hci_event_command_complete_get_command_opcode(packet) != write_bd_addr.opcode) {
        return;
    }
    if (hci_event_command_complete_get_return_parameters(packet)[0] != ERROR_CODE_SUCCESS) {}
}

static void mac_bytes(uint8_t mac[6]) {
    const String macStr = WiFi.macAddress();
    int values[6];
    sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x", // NOLINT(*-err34-c)
           &values[0], &values[1], &values[2],
           &values[3], &values[4], &values[5]);
    for (int i = 0; i < 6; i++) mac[i] = static_cast<uint8_t>(values[i]);
}

void setup() {
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi...");
    int maxRetries = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        maxRetries++;
        if (maxRetries > 80) {
            Serial.println(" Failed!");
            delay(100);
            watchdog_reboot(0, 0, 0);
        }
    }
    Serial.print("\nOK! > ");
    Serial.println(WiFi.localIP());
    Serial.print("WoL MAC: ");
    Serial.println(WiFi.macAddress());

    delay(500);

    BluetoothLock lock;
    event_handler.callback = packet_handler;
    hci_add_event_handler(&event_handler);
    hci_power_control(HCI_POWER_ON);

    delay(500);
    udp.begin(WOL_PORT);
}

void loop() {
    if (const int packetSize = udp.parsePacket(); packetSize >= 102) {
        uint8_t packetBuffer[102];
        udp.read(packetBuffer, 102);
        for (int i = 0; i < 6; i++) if (packetBuffer[i] != 0xFF) return;
        uint8_t macAddress[6];
        mac_bytes(macAddress);
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 6; j++) {
                if (packetBuffer[6 + i * 6 + j] != macAddress[j]) return;
            }
        }
        Serial.println("Received Wake Packet");
        hci_send_cmd(&hci_create_connection, console_mac, 0xCC18, 0x02, 0x00, 0x0000, 0x01);
    }
}
