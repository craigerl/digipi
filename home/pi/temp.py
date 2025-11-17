#!/usr/bin/python3
import time
import board
import adafruit_bme680
i2c = board.I2C()  # uses board.SCL and board.SDA
bme680 = adafruit_bme680.Adafruit_BME680_I2C(i2c, debug=False)
bme680.sea_level_pressure = 1013.25
temperature_offset = -10
for i in range(3):
   F = bme680.temperature
   #print(str(F))
   time.sleep(0.5)
F = 9.0/5.0 * bme680.temperature + temperature_offset + 32
print("DigiPi Tracker - %0.0fF" % round(F) )

