homewatch - Wired home alarm replacement

This is an off-the-shelf combination of open-source hardware and
software that gives you a home alarm and monitoring system that
should be as secure as any of the home products out there, and
lets you pay around $6/month (US) for cellular SMS instead of
$30 or more per month.

This was originally created to replace a Brinks home security
system installed in 2008. Detailed notes on hardware, tools and
installation are in hardware.txt

The main components (with approximate prices in USD) are:
* Arduino Uno ($25)
* SEEEDStudio GPRS shield v2.0 ($35)
* SIM card (USMobile.com will give you SMS-only service for under
  $10 per month in the US)
* I2C IO expander board ($8)
* Raspberry Pi 3 ($30)
* Raspberry Pi touchscreen ($60)
* Level shifter board ($8)
* Powered USB speaker ($15)

Minimal soldering is needed (soldering header pins on the GPRS board
and level shifter board). A crimp kit for Arduino-style connectors is
highly recommended.

The two main hardware components are the core monitor, which is an
Arduino board which goes in the connection box; and the front panel,
which is a Raspberry Pi Model 3 with touchscreen that goes by the home's
main entrance (where presumably you have an alarm panel by ADT, Brinks,
etc)

All you need from your existing system is the wiring. Some systems have
a logic board in the connection box which handles battery backup. If the
logic board has AUX PWR terminals that supply 12V DC (often the supply
voltage is 12-18V AC) you can supply battery-backed power for the core
monitor. The sealed lead-acid batteries used often need to be replaced
every 5 years or so. These can be gotten at Fry's for about $25.

Two software components are needed:

* arduino_firmware - Arduino sketch for the core monitor. This runs
  on an Arduino UNO with the GPRS shield and I2C IO expander, and
  talks to the pimonitor software running on a Raspberry Pi. This will
  notify via text message without needing any other components.

* pimonitor - Qt/C++ GUI application to run on your front panel.
  This talks to the core monitor via serial connection.

