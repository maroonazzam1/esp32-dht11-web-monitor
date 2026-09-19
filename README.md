# ESP32 DHT11 Web Monitor

An ESP32 reads temperature and humidity from a DHT11 sensor and serves them
as a live web page over wifi. Open the board's IP address on any device on the
same network and the readings update automatically every 5 seconds.

Second project in my ESP32 series, after the LED blink.

## Hardware

- ESP32-WROOM-32 dev board
- DHT11 temperature & humidity sensor (3-pin module)
- Female-to-female jumper wires
- Breadboard (used for the ground rail)
- USB-C cable

## Wiring

The 3-pin DHT11 module is labelled `+`, `out`, `-` rather than VCC/DATA/GND.

| DHT11 pin | Connects to |
|---|---|
| `+` | **5V** on the ESP32 |
| `out` | **GPIO 4** (marked `D4`) |
| `-` | **GND** (via the blue rail on the breadboard) |

Photos of the actual build are in this repo:

![Full setup](setup-wide.jpg)
![Sensor wiring close-up](sensor-closeup.jpg)
![ESP32 header](esp32-pins.jpg)

**Unplug the USB-C cable before changing any wiring.**

## Software setup

1. Arduino IDE → Library Manager (books icon in the left sidebar)
2. Install **DHT sensor library** by Adafruit
3. Accept the prompt to also install **Adafruit Unified Sensor**
4. Tools → Board → your ESP32 board, Tools → Port → the port that appears
   when the board is plugged in

## Before uploading

Open the sketch and fill in your own network details:

```cpp
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

These are placeholders in this repo on purpose — real credentials should
never be committed.

## Running it

1. Upload the sketch (arrow button)
2. Open the Serial Monitor and set the baud rate to **115200**
3. Wait for the connection dots, then note the IP address it prints
   (something like `192.168.1.47`)
4. Type that address into a browser on the same wifi network

## Troubleshooting

**"Failed to read from DHT sensor"** — check the `out` pin is really on GPIO 4,
that the module has power, and that every female connector is pushed fully
onto its pin. Loose connectors look fine and don't work.

**Endless dots, never connects** — the ESP32 only joins **2.4GHz** networks.
It cannot see 5GHz ones at all.

**Garbled characters in the Serial Monitor** — baud rate isn't set to 115200.

**Readings never change** — the DHT11 is slow; it won't give a new reading more
often than every 2 seconds. Breathe on the sensor to confirm it's live.

## Notes

- This build runs the sensor on **5V**; it wouldn't read reliably on 3V3.
- The web page refreshes itself with an HTML meta tag rather than JavaScript,
  which keeps the sketch small.
