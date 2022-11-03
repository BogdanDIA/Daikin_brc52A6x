# Daikin_brc52A6x
Control of Daikin AC with ESPHome. The aim is to be able to control the AC from both Home Assistant and remote control.

## The regular way using an IR receiver and an IR transmitter

The carrier frequency is 38kHz so a TSOP2238 can be used like in the schematics below. I am using the ESP01S together with an adapter that allows using a 5V power supply and also 5V tolerant input/output signals.

![alt text](images/adapter5V-3.3V.png)

The connection of ESP to the receiver and transmitter looks like this:

![alt text](images/txrx_sch.png)

When you build the ESPHome image you should have the transmitter defined like this:
```
remote_transmitter:
  pin:
    number: GPIO1
    mode:
      output: true
  carrier_duty_percent: 50%
```
and the receiver defined linke this:
```
remote_receiver:
  id: ir_receiver 
  pin:
    number: GPIO3
    inverted: True
    mode:
      input: true
      pullup: true
  tolerance: 25%
```

## The simplified way that do not use receiver and transmitter

This method uses the fact that the AC receiver's output is open drain and can be connected with both receiver and transmitter pins of the ESP.
![alt text](images/simple_sch.png)

This time the tranmitter should be defined like this where the output pin is open drain:

```
remote_transmitter:
  pin:
    number: GPIO1
    inverted: true
    mode:
      output: true
      open_drain: true
  carrier_duty_percent: 50%
```
