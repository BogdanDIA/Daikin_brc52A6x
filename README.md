# Daikin_brc52A6x
Control of Daikin AC with ESPHome. The aim is to be able to control the AC from both Home Assistant and remote control.

1. The regular way using an IR receiver and an IR transmitter

The carrier frequency is 38kHz so a TSOP2238 can be used like in the schematics below. I am using the ESP01S together with an adapter that allows using a 5V power supply and also 5V tolerant input/output signals.

![alt text](images/adapter5V-3.3V.png)

The connection of ESP to the receiver and transmitter looks like this:

![alt text](images/txrx_sch.png)

When you build the ESPHome image you should have the transmitter and receiver defined like this:
remote_transmitter:
  pin:
    number: GPIO1
    mode:
      output: true
  carrier_duty_percent: 50%
