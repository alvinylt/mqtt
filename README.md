# MQTT Publisher and Analyser

**Alvin Tang (u7447218)**

This is an MQTT client implementation written in the C programming language.
There are two programs, a publisher and an analyser.

The [publisher](publisher.c) subscribes to the topics `request/qos`,
`request/delay` and `request/instancecount`. When valid values are received in
all three topics, it publishes values to the topic
`counter/<instance>/<qos>/<delay>` for 60 seconds.

The [analyser](analyser.c) publishes requests to the three topics under
`request` and observes the behaviour of the publishers.

## Initialisation

The programs are designed for the Linux operating system. GCC is required to
compile the program.

To compile the publisher and analyser, simply execute `make`.

### Eclipse Paho Library

The programs use [Eclipse Paho](https://eclipse.dev/paho/) as an external
library for functions managing MQTT-related operations.

The library can be installed on the system following the
instructions on the
[official website](https://eclipse.dev/paho/index.php?page=clients/c/index.php).

## Usage

The publisher can be executed using the command
`./publisher <Broker hostname> <Port> <Instance>`.
For instance, if the broker is hosted locally on port 1883, the command
`./publisher localhost 1883 3` creates three counter publishers with IDs
`pub-1`, `pub-2` and `pub-3`.

The bash script [activate_publisher.sh](activate_publisher.sh) contains a
command to activate all five counter publishers.

The analyser can be run with `./analyser <Broker hostname> <Port>`.

Last updated: 2024-05-06
