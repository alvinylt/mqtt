# MQTT Publisher and Analyser

**Alvin Tang (u7447218)**

This is an implementation of an MQTT publisher and an analyser written in the C
programming language.

## Initialisation

The programs are designed for Linux.
A typical Linux installation with GCC is required.

### Installing Eclipse Paho

The programs use [Eclipse Paho](https://eclipse.dev/paho/) as the library.

- `git clone https://github.com/eclipse/paho.mqtt.c.git`
- `cd paho.mqtt.c`
- `sudo make install`

### Compiling the Publisher and Analyser

1. Enter the directory.
2. Execute `make`

The publisher can be run using the command `./publisher`.

Last updated: 2024-05-04
