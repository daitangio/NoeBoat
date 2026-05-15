# MagiLamp 2026

Drive plenty of LEDs via easy web interface, glued with full feature RTOS.
This project is fair complex.

The project is a lamp which change led power based on current time.
You can set a "night mode" and a "quarter change mode" activated around 0,15,30,45

## Setup

Create a file called arduin_secrets.h with SECRET_SSID and SECRET_OPTIONAL_PASS
to be able to connect to your preferred WI-FI

## Overall Led connections:

- Pin 2 is used for melody tone
- Pin 6,9,10 Fading Major Led
- Pin 13 Operation Led: just use for ACK board is working

Pin 3,11 cannot be used because used by tone() function


## Web Api

### Status:
Report system status:

curl http://192.168.1.16:80/sys

### Engage led dance with cycling leds:

curl http://192.168.1.141:80/cycle1

## Credits

Originally based on 210_R4_FreeRTOS_Cloud1.ino