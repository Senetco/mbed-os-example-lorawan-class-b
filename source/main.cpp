#include "mbed.h"
#include "mbed_trace.h"
#include "mbed_events.h"
#include "lora_radio_helper.h"
#include "dev_eui_helper.h"
#include "LoRaWANInterface.h"
#include "platform/Callback.h"


#define APP_DUTY_CYCLE_CLASS_A 10000
#define APP_DUTY_CYCLE_CLASS_B 60000
#define PING_SLOT_PERIODICITY 4

// Use Dish 700MHz downlink channels
bool dish_downlink_mode = false; 

int app_duty_cycle = APP_DUTY_CYCLE_CLASS_A;

bool device_time_synched = false;
bool ping_slot_synched = false;
bool send_queued = false;

uint16_t beacon_locks = 0;
uint16_t beacon_misses = 0;
uint16_t rx_count = 0;

// Device credentials, register device as OTAA in The Things Network and copy credentials here
static uint8_t DEV_EUI[] = MBED_CONF_LORA_DEVICE_EUI; 
static uint8_t APP_EUI[] = MBED_CONF_LORA_APPLICATION_EUI; 
static uint8_t APP_KEY[] = MBED_CONF_LORA_APPLICATION_KEY; 

// The port we're sending and receiving on
#define MBED_CONF_LORA_APP_PORT     15

static void queue_next_send_message();
static void print_received_beacon();

// EventQueue is required to dispatch events around
static EventQueue ev_queue;

// Constructing Mbed LoRaWANInterface and passing it down the radio object.
static LoRaWANInterface lorawan(radio);

// Application specific callbacks
static lorawan_app_callbacks_t callbacks;

// LoRaWAN stack event handler
static void lora_event_handler(lorawan_event_t event);


// Send a message over LoRaWAN
static void send_message()
{
    send_queued = false;

    if(!device_time_synched)
    {
        lorawan_status_t retcode = lorawan.add_device_time_request(); 
        if(retcode != LORAWAN_STATUS_OK)
        {
            printf("Add device time request failed (%d)", retcode);
        }
    }

    if(!ping_slot_synched)
    {
        lorawan_status_t retcode = lorawan.set_ping_slot_info(PING_SLOT_PERIODICITY); 
        if(retcode != LORAWAN_STATUS_OK)
        {
            printf("Set ping slot info failed (%d)", retcode);
        }
    }

    uint8_t tx_buffer[6]; 
    tx_buffer[0] = (beacon_locks>>8);
    tx_buffer[1] = beacon_locks &0xff;
    tx_buffer[2] = (beacon_misses>>8);
    tx_buffer[3] = beacon_misses &0xff;
    tx_buffer[4] = (rx_count>>8);
    tx_buffer[5] = rx_count & 0xff;
    
    int packet_len = sizeof(tx_buffer);
    printf("Sending %d bytes\n", packet_len);

    int16_t retcode = lorawan.send(MBED_CONF_LORA_APP_PORT, tx_buffer, packet_len, MSG_UNCONFIRMED_FLAG);

    // for some reason send() returns -1... I cannot find out why, the stack returns the right number. I feel that this is some weird Emscripten quirk
    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? printf("send - duty cycle violation\n")
        : printf("send() - Error code %d\n", retcode);

        queue_next_send_message();
        return;
    }

    printf("%d bytes scheduled for transmission\n", retcode);
}

static void queue_next_send_message()
{
    if(send_queued)
        return;

    if(app_duty_cycle != 0){
        int backoff;
        lorawan.get_backoff_metadata(backoff);

        if (backoff < app_duty_cycle) {
            backoff = app_duty_cycle;
        }
        printf("Next send in %d seconds\r\n",backoff/1000);
        send_queued = true;
        ev_queue.call_in(backoff, &send_message);
    }
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
    connect_params.connection_u.otaa.nb_trials = 255;
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
    rx_count++;

}

// Event handler
static void lora_event_handler(lorawan_event_t event)
{
    lorawan_status_t status;

    switch (event) {
        case CONNECTED:
            printf("Connection - Successful\n");
            if(device_time_synched){
                lorawan.enable_beacon_acquisition();
            }
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
            printf("Received Message from Network Server\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("Error in reception - Code = %d\n", event);
            break;
        case JOIN_FAILURE:
            printf("OTAA Failed - Check Keys\n");
            break;
        case DEVICE_TIME_SYNCHED:
            printf("Device Time Received from Network Server\n");
            device_time_synched = true;
            app_duty_cycle = APP_DUTY_CYCLE_CLASS_B;
            status = lorawan.enable_beacon_acquisition();
            if(status != LORAWAN_STATUS_OK){
                printf("Beacon Acquisition Error - EventCode = %d\n", status);
            }
            break;
        case PING_SLOT_INFO_SYNCHED:
            printf("Ping Slot Setttings Synchronized with Network Server\n");
            ping_slot_synched = true;
            queue_next_send_message();
            break;
        case BEACON_NOT_FOUND:
            beacon_misses++;
            device_time_synched = false;
            app_duty_cycle = APP_DUTY_CYCLE_CLASS_A;
            queue_next_send_message();
            break;
        case BEACON_FOUND:
            beacon_locks++;
            print_received_beacon();
            status = lorawan.set_device_class(CLASS_B);
            if(status == LORAWAN_STATUS_OK){
                app_duty_cycle = APP_DUTY_CYCLE_CLASS_B;
            }
            else{
                printf("Switch Device Class A -> B Error - EventCode = %d\n", status);
                device_time_synched = false;
                app_duty_cycle = APP_DUTY_CYCLE_CLASS_A;
            }
            break;
        case BEACON_LOCK:
            print_received_beacon();
            printf("Total Beacon Lock Count=%u\r\n", beacon_locks);
            beacon_locks++;
            break;
        case BEACON_MISS:
            beacon_misses++;
            printf("Total Beacon Misses=%u\r\n", beacon_misses);
            break;
        case SWITCH_CLASS_B_TO_A:
            printf("Reverted Class B -> A\n");
            device_time_synched = false;
            send_message();
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}

void print_received_beacon()
{
    lorawan_beacon_t beacon;
    lorawan_status_t status;

    status = lorawan.get_received_beacon(beacon);
    if(status != LORAWAN_STATUS_OK){
        printf("Get Received Beacon Error - EventCode = %d\n",status);
    }

    printf("\nReceived Beacon Time=%lu, GwSpecific=", beacon.time);
    for(uint8_t i=0; i< sizeof(beacon.gw_specific); i++){
        printf("%02X",beacon.gw_specific[i]);
    }
    printf("\n");

}

