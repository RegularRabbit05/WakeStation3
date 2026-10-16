#include <Arduino.h>
#include <BluetoothLock.h>
#include <btstack.h>
#include <Ethernet.h>

static constexpr int WOL_PORT = 9;
static constexpr bd_addr_t console_mac = {MAC_PS3};
static constexpr bd_addr_t device_mac = {MAC_DS3};
static constexpr hci_cmd_t write_bd_addr = {.opcode = 0xFC01, .format = "B"};
static uint8_t eth_mac[6] = {MAC_ETH};
static btstack_packet_callback_registration_t event_handler;
static EthernetUDP udp;

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

void setup() {
    Serial.begin(9600);
    EthernetClass::init(17);
    EthernetClass::begin(eth_mac);

    delay(500);

    if (EthernetClass::hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.");
        delay(100);
        watchdog_reboot(0, 0, 0);
    } else if (EthernetClass::linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
        delay(100);
        watchdog_reboot(0, 0, 0);
    }

    BluetoothLock lock;
    event_handler.callback = packet_handler;
    hci_add_event_handler(&event_handler);
    hci_power_control(HCI_POWER_ON);

    delay(500);

    Serial.print("Listening on ");
    Serial.print(EthernetClass::localIP());
    Serial.print(" with MAC: ");
    for (int i = 0; i < 6; i++) {
        Serial.print(eth_mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    udp.begin(WOL_PORT);
}

void loop() {
    if (const int packetSize = udp.parsePacket(); packetSize >= 102) {
        uint8_t packetBuffer[102];
        udp.read(packetBuffer, 102);
        for (int i = 0; i < 6; i++) if (packetBuffer[i] != 0xFF) return;
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 6; j++) {
                if (packetBuffer[6 + i * 6 + j] != eth_mac[j]) return;
            }
        }
        Serial.println("Received Wake Packet");
        hci_send_cmd(&hci_create_connection, console_mac, 0xCC18, 0x02, 0x00, 0x0000, 0x01);
    }
}
