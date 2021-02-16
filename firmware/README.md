# Flash command

AVRISP:
```
All LEDs:
avrdude -p t13 -c avrisp -b 19200 -u -Uflash:w:all_leds.hex:a -Ulfuse:w:0x65:m -Uhfuse:w:0xFD:m
Single LED:
avrdude -p t13 -c avrisp -b 19200 -u -Uflash:w:single_led.hex:a -Ulfuse:w:0x65:m -Uhfuse:w:0xFD:m
```

USBAsp:
```
All LEDs:
avrdude -p t13 -c usbasp -u -Uflash:w:all_leds.hex:a -Ulfuse:w:0x65:m -Uhfuse:w:0xFD:m
Single LED:
avrdude -p t13 -c usbasp -u -Uflash:w:single_led.hex:a -Ulfuse:w:0x65:m -Uhfuse:w:0xFD:m
```