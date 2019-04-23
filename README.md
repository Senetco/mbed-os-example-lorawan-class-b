# Example LoRaWAN Class B application for Mbed-OS 
This is an example LoRaWAN Class B application based on Mbed-OS LoRaWAN protocol APIs. The Mbed-OS LoRaWAN stack Class B implementation is
compliant with LoRaWAN v1.0.3 specification. 
* [LoRaWAN Specification V1.0.3](https://lora-alliance.org/resource-hub/lorawantm-specification-v103)

## Getting started
This application can work with any Network Server if you have correct credentials for said Network Server. 
The LoRaWAN Class B implementation was developed and tested against the [Senet LoRaWAN Network](https://www.senetco.com)

## Target configuration
This application runs on any Mbed-enabled development board, but requires some configuration. See the [porting guide](docs/porting-guide.md) for more information.

## Download the application
1. Install [Mbed CLI](https://os.mbed.com/docs/v5.10/tools/installation-and-setup.html) and the [GNU ARM Embedded Toolchain 6](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm).
1. Import this repository via:

    ```
    $ mbed import https://github.com/Senetco/mbed-os-example-lorawan-class-b
    ```
    OR 

    ```
    git clone git@github.com:Senetco/mbed-os-example-lorawan-class-b.git
    $ cd mbed-os-example-lorawan-class-b
    $ mbed deploy
    ```
## LoRaWAN Configuration 
Open the file mbed_app.json in the root directory of your application. This file contains all the user specific configurations your application and the Mbed OS LoRaWAN stack need. Network credentials are typically provided by LoRa network provider

### OTAA 
Please add Device EUI, Application EUI and Application Key needed for Over-the-air-activation(OTAA). For example:

    "lora.device-eui": "{ YOUR_DEVICE_EUI }",
    "lora.application-eui": "{ YOUR_APPLICATION_EUI }",
    "lora.application-key": "{ YOUR_APPLICATION_KEY }"

For an end-device with a built-in Device EUI, you can add EUI read logic to source/helpers/dev_eui_helper.h. 

### ABP
For Activation-By-Personalization (ABP) connection method, modify the mbed_app.json to enable ABP. You can do it by simply turning off OTAA. For example:

"lora.over-the-air-activation": false

In addition to that, you need to provide Application Session Key, Network Session Key and Device Address. For example:

    "lora.appskey": "{ YOUR_APPLICATION_SESSION_KEY }",
    "lora.nwkskey": "{ YOUR_NETWORK_SESSION_KEY }",
    "lora.device-address": " YOUR_DEVICE_ADDRESS_IN_HEX  " 

### Selecting a PHY
Selection of a specific PHY layer happens at compile time. By default, the Mbed OS LoRaWAN stack uses EU 868 MHz PHY. An example of selecting a PHY can be:
        "phy": {
            "help": "LoRa PHY region. 0 = EU868 (default), 1 = AS923, 2 = AU915, 3 = CN470, 4 = CN779, 5 = EU433, 6 = IN865, 7 = KR920, 8 = US915, 9 = US915_HYBRID",
            "value": "0"
        },

### Class B 
* Ping Slot Periodicity 
    * Periodicity is an unsigned 3 bit integer encoding the ping slot period. 
    * The number of ping slots per beacon period is 2^(7-Periodicity)
    * Periodicity = 0 - opens a ping slot approximately every second during the beacon period 
    * Periodicity = 7 - opens a ping slot once every 128 seconds which is the maximum ping period supported by the LoRaWAN Class B specification  
   
    For example, to open a ping slot 8 times per beacon window interval, approximately once every 16 seconds:

        "lora.ping-slot-periodicity": 4 

* Class B Scheduling Debug Tracing 
    * Enable debug tracing of Beacon and Ping slot scheduling

    For example, to enable extra beacon and ping slot debug trace:

        "lora.class-b-extra-debug-trace-level": 2

## Build this application 
1. Build this application via:

    ```
    $ mbed compile -m auto -t YOUR_TOOLCHAIN --profile=./mbed-os/tools/profiles/debug.json
    ```

1. Drag `BUILD/YOUR_TARGET/GCC_ARM-TINY/mbed-os-example-lorawan-class-b.bin` onto your development board (mounts as flash storage device).
1. When flashing is complete, attach a serial monitor on baud rate 115,200.

For optimized builds you can build without the RTOS enabled, with newlib-nano, and a different printf library. Some background is in [this blog post](https://os.mbed.com/blog/entry/Reducing-memory-usage-with-a-custom-prin/). To do this:

1. Rename `.mbedignore_no_rtos` to `.mbedignore`.
1. Build with:

    ```
    $ mbed compile -m YOUR_TARGET -t YOUR_TOOLCHAIN --profile=./profiles/tiny.json
    ```

## Module support
Here is a nonexhaustive list of boards and modules tested with Mbed OS LoRaWAN stack Class B operation
* NucleoL476RG + Mbed radio shield (SX1276MB1LAS or SX1262MB2xAS) 
* NucleoL073RZ + Mbed radio shield (SX1276MB1LAS or SX1262MB2xAS) 
* L-TEK FF1705

## Known Issues
*  No RX1 receptions with optimized build and SX1262 radio shield.  
*  No Multicast Ping Slot support (Coming soon) 