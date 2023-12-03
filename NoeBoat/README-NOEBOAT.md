# NoeBoat 2023

Drive plenty of LEDs via real-time operating system and very simple button driver.
Add music and be happy


# Overall Led connections:

- Pin 2 is used for melody tone
- Pin 3,6,9,10,11 Fading Major Led
- Pin 13 Operation Led: just use for ACK board is working

Controls:
- Pin A0 is linked to a potentiometer
- 

Remember: PWM conflicts for pin 3,11 when using tone() function (it uses the related pwm interrupt)


