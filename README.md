# NoeBoat
Arduino Based lamp based on a wooden toy

Refer to https://gioorgi.com/category/projects/arduino/noeboat/
for a list of article describing the goals of the project.

# For developers

## How to compile
Install arduino-cli and gnu-make then enters inside NoeBoat and type
    
    make help

To a list of options.

Also the sketch should work with Arduino IDE 2.x and 1.8.+ too
Please keep in mind an Arduino R1-R3 is needed.

To use Arduion R4 or WiFi, try deleting src folder to use the FreeRTOS included in the official Arduino IDE.

## How to release a change:

    git-chglog >CHANGELOG.md