# ESP32 DHT11 Web Monitor

An ESP32 reads temperature and humidity from a DHT11 sensor and serves them as
a live web page over wifi. As well as the current readings, it keeps 24 hours
of history and draws it as a chart you can drag across to read the exact
temperature and humidity at any point in time.

Open the board's IP address on any device on the same network — no app, no
cloud service, nothing installed.

Second project in my ESP32 series, after the LED blink.

![Overnight readings](screenshot.png)

## What it does

- Live temperature and humidity, updated every 30 seconds
- 24 hours of history, sampled every 10 minutes
- Auto-scaling chart — a 1.5°C swing fills the view instead of flatlining
- Separate labelled axes: temperature (orange, left), humidity (blue, right)
- Real clock times along the bottom, via NTP
- Drag or hover across the chart for a crosshair readout at any sample

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

Photos of the actual build:

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

Fill in your own network details at the top of the sketch:

```cpp
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

These are placeholders in this repo on purpose — real credentials should never
be committed.

## Running it

1. Upload the sketch (arrow button)
2. Open the Serial Monitor and set the baud rate to **115200**
3. Wait for the connection dots, then note the IP address it prints
   (something like `192.168.1.47`)
4. Type that address into a browser on the same wifi network

## Tuning the history window

Two constants control how far back the chart goes:

```cpp
#define HISTORY_SIZE  144          // number of samples kept
#define SAMPLE_MS     600000UL     // milliseconds between samples
```

144 samples × 10 minutes = 24 hours. For a 2-hour window instead, use `120`
and `60000UL`. To watch the chart fill up quickly while testing, drop
`SAMPLE_MS` to `10000` for a few minutes, then set it back.

## How it works

The board serves two things: the page itself at `/`, and the readings as JSON
at `/data`. The page fetches `/data` every 30 seconds and redraws the chart in
the browser, rather than reloading the whole page — that's what lets the
crosshair stay put while you drag it.

History lives in a circular buffer in RAM. Time comes from an NTP server and
is stored as UTC; the browser converts it to local time when drawing the
labels.

## Limitations

- **History is lost on power cycle.** It lives in RAM, so unplugging the board
  or pressing reset clears the chart and it starts filling again from empty.
- The DHT11 is a cheap sensor — roughly ±2°C and ±5% RH. Fine for watching
  trends in a room, not for anything that needs accuracy.
- It won't give a new reading more often than every 2 seconds.

## Troubleshooting

**"Failed to read from DHT sensor"** — check the `out` pin is really on GPIO 4,
that the module has power, and that every female connector is pushed fully onto
its pin. Loose connectors look fine and don't work.

**Endless dots, never connects** — the ESP32 only joins **2.4GHz** networks. It
cannot see 5GHz ones at all.

**Garbled characters in the Serial Monitor** — baud rate isn't set to 115200.

**Chart says "waiting for the first samples"** — normal on a fresh boot; it
needs two samples before a line can be drawn.

**Times on the x-axis show as `-30m` instead of a clock time** — NTP hasn't
synced yet. It falls back to relative times until it does.

## Notes

- This build runs the sensor on **5V**; it wouldn't read reliably on 3V3.
- The page is stored in flash with `PROGMEM` rather than built up as a String,
  which keeps RAM free for the history buffer.
