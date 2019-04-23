# Porting guide

This application runs on every Mbed-enabled development board, but some configuration needs to be set.

## Adding a new section in mbed_app.json

The [mbed_app.json](../mbed_app.json) file contains all the configuration for the application, and this also contains per-target configuration. To start a new port you'll need to add a new section for your board under `target_overrides`. The key of the configuration is the name of your target. You can find this name through Mbed CLI:

```
$ mbed detect
[mbed] Detected DISCO_L475VG_IOT01A, port /dev/tty.usbmodem14403, mounted /Volumes/DIS_L4IOT, interface version 0221:
[mbed] Supported toolchains for DISCO_L475VG_IOT01A
| Target              | mbed OS 2 | mbed OS 5 |    uARM   |    IAR    |    ARM    |  GCC_ARM  |
|---------------------|-----------|-----------|-----------|-----------|-----------|-----------|
| DISCO_L475VG_IOT01A | Supported | Supported | Supported | Supported | Supported | Supported |
Supported targets: 1
Supported toolchains: 4
```

Here `DISCO_L475VG_IOT01A` is the target name.

Thus, add the following section:

```json
        "DISCO_L475VG_IOT01A": {
        }
```

All configuration options in this porting guide need to be added to this section.

## LoRa radio

You need to tell the application how the radio is connected to your development board over the SPI interface. For the [SX1272](https://os.mbed.com/components/SX1272MB2xAS/) and [SX1276](https://os.mbed.com/components/SX1276MB1xAS/) shields the configuration is:

```json
            "lora-radio":          "SX1276",
            "lora-spi-mosi":       "D11",
            "lora-spi-miso":       "D12",
            "lora-spi-sclk":       "D13",
            "lora-cs":             "D10",
            "lora-reset":          "A0",
            "lora-dio0":           "D2",
            "lora-dio1":           "D3",
            "lora-dio2":           "D4",
            "lora-dio3":           "D5",
            "lora-dio4":           "D8",
            "lora-dio5":           "D9",
            "lora-rf-switch-ctl1": "NC",
            "lora-rf-switch-ctl2": "NC",
            "lora-txctl":          "NC",
            "lora-rxctl":          "NC",
            "lora-ant-switch":     "A4",
            "lora-pwr-amp-ctl":    "NC",
            "lora-tcxo":           "NC",
```
Set `lora-radio` to `SX1272` if you use the SX1272 shield.

For the [SX126xMB2xAS](https://os.mbed.com/components/SX126xMB2xAS) shield the configuration is:
```json
            "lora-radio":           "SX126X",
            "lora-spi-mosi":        "D11",
            "lora-spi-miso":        "D12",
            "lora-spi-sclk":        "D13",
            "lora-cs":              "D7",
            "lora-ant-switch":      "D8",
            "lora-reset":           "A0",
            "lora-dio1":            "D5",
            "lora-busy":            "D3",
            "lora-tcxo":            "A3",
            "lora-device-sel":      "A2",
            "lora-freq-sel":        "A1"
```
