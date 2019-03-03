#include "mbed.h"
#include "mbed_trace.h"
#include "mbed_events.h"
#include "lora_radio_helper.h"
#include "dev_eui_helper.h"
#include "LoRaWANInterface.h"

int app_duty_cycle = 10000;

// Device credentials, register device as OTAA in The Things Network and copy credentials here
static uint8_t DEV_EUI[] = { 0x00, 0x80, 0x00, 0x00, 0x04, 0x00, 0x39, 0x96};
static uint8_t APP_EUI[] = { 0x00, 0x25, 0x0C, 0x00, 0x00, 0x01, 0x00, 0x01 };
static uint8_t APP_KEY[] = { 0xB5, 0xE7, 0x1B, 0x6C, 0x21, 0x70, 0x90, 0x43, 0x12, 0x5F, 0xD1, 0xC5, 0x6E, 0xE0, 0xA4, 0x63 };

// The port we're sending and receiving on
#define MBED_CONF_LORA_APP_PORT     15

static void queue_next_send_message();

// EventQueue is required to dispatch events around
static EventQueue ev_queue;

// Constructing Mbed LoRaWANInterface and passing it down the radio object.
static LoRaWANInterface lorawan(radio);

static bool queued_message_waiting = false;


// Application specific callbacks
static lorawan_app_callbacks_t callbacks;

// LoRaWAN stack event handler
static void lora_event_handler(lorawan_event_t event);

// Send a message over LoRaWAN
static void send_message()
{
    #if 0
    if (queued_message_waiting) {
        int16_t retcode = lorawan.send(
                              queued_message._port,
                              queued_message._data,
                              queued_message._length,
                              queued_message._confirmed ? MSG_CONFIRMED_FLAG : MSG_UNCONFIRMED_FLAG);

        if (retcode < 0) {
            printf("send_message for queued_message on port %d failed (%d)\n", queued_message._port, retcode);
            queue_next_send_message();
        } else {
            free(queued_message._data);
            queued_message_waiting = false;
            printf("%d bytes scheduled for transmission on port %d\n", queued_message._length, queued_message._port);
        }

        return;
    }
    #endif

    uint8_t tx_buffer[2] = { 1, 2 };
    int packet_len = 2;
    printf("Sending %d bytes\n", packet_len);

    int16_t retcode = lorawan.send(MBED_CONF_LORA_APP_PORT, tx_buffer, packet_len, MSG_UNCONFIRMED_FLAG);

    // for some reason send() returns -1... I cannot find out why, the stack returns the right number. I feel that this is some weird Emscripten quirk
    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? printf("send - duty cycle violation\n")
        : printf("send() - Error code %d\n", retcode);
        return;
    }

    printf("%d bytes scheduled for transmission\n", retcode);
}

static void queue_next_send_message()
{
    int backoff;
    lorawan.get_backoff_metadata(backoff);

    if (backoff < app_duty_cycle) {
        backoff = app_duty_cycle;
    }

    ev_queue.call_in(backoff, &send_message);
}

int main()
{

    if (get_built_in_dev_eui(DEV_EUI, sizeof(DEV_EUI)) == 0) {
        printf("read built-in dev eui: %02x %02x %02x %02x %02x %02x %02x %02x\n",
               DEV_EUI[0], DEV_EUI[1], DEV_EUI[2], DEV_EUI[3], DEV_EUI[4], DEV_EUI[5], DEV_EUI[6], DEV_EUI[7]);
    }


    if (DEV_EUI[0] == 0x0 && DEV_EUI[1] == 0x0 && DEV_EUI[2] == 0x0 && DEV_EUI[3] == 0x0 && DEV_EUI[4] == 0x0 && DEV_EUI[5] == 0x0 && DEV_EUI[6] == 0x0 && DEV_EUI[7] == 0x0) {
        printf("Set your LoRaWAN credentials first!\n");
        return -1;
    }

    // Enable trace output for this demo, so we can see what the LoRaWAN stack does
    mbed_trace_init();

    if (lorawan.initialize(&ev_queue) != LORAWAN_STATUS_OK) {
        printf("LoRa initialization failed!\n");
        return -1;
    }

    // prepare application callbacks
    callbacks.events = mbed::callback(lora_event_handler);
    lorawan.add_app_callbacks(&callbacks);

    lorawan_connect_t connect_params;
    connect_params.connect_type = LORAWAN_CONNECTION_OTAA;
    connect_params.connection_u.otaa.dev_eui = DEV_EUI;
    connect_params.connection_u.otaa.app_eui = APP_EUI;
    connect_params.connection_u.otaa.app_key = APP_KEY;
    connect_params.connection_u.otaa.nb_trials = 3;

    lorawan_status_t retcode = lorawan.connect(connect_params);

    if (retcode == LORAWAN_STATUS_OK ||
            retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        printf("Connection error, code = %d\n", retcode);
        return -1;
    }

    printf("Connection - In Progress ...\r\n");

    // make your event queue dispatching events forever
    ev_queue.dispatch_forever();

    return 0;
}

// This is called from RX_DONE, so whenever a message came in
static void receive_message()
{
    uint8_t rx_buffer[255] = { 0 };
    uint8_t port;
    int flags;
    int16_t retcode = lorawan.receive(rx_buffer, sizeof(rx_buffer), port, flags);

    if (retcode < 0) {
        printf("receive() - Error code %d\n", retcode);
        return;
    }

    printf("Received %d bytes on port %u\n", retcode, port);

    printf("Data received on port %d (length %d): ", port, retcode);

    for (uint8_t i = 0; i < retcode; i++) {
        printf("%02x ", rx_buffer[i]);
    }
    printf("\n");

#if 0
    if (status != LW_UC_OK) {
        printf("Failed to handle command on port %d, status %d\n", port, status);
    }
#endif
}



// Event handler
static void lora_event_handler(lorawan_event_t event)
{
    switch (event) {
        case CONNECTED:
            printf("Connection - Successful\n");
            queue_next_send_message();
            break;
        case DISCONNECTED:
            ev_queue.break_dispatch();
            printf("Disconnected Successfully\n");
            break;
        case TX_DONE:
            printf("Message Sent to Network Server\n");
            queue_next_send_message();
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            printf("Transmission Error - EventCode = %d\n", event);
            queue_next_send_message();
            break;
        case RX_DONE:
            printf("Received message from Network Server\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("Error in reception - Code = %d\n", event);
            break;
        case JOIN_FAILURE:
            printf("OTAA Failed - Check Keys\n");
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}