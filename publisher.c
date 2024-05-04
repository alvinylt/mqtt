#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MQTTClient.h"

// Global constants
#define TOPIC "topic"
#define QOS 0

// Helper functions
int publish(char *url, char *client_id);

/**
 * The main function to execute the publisher program.
 */
int main(int argc, char *argv[]) {
    // Parse the command input
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Broker hostname> <Port> <Client ID>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    char *url = malloc(strlen(argv[0]) + strlen(argv[1]) + 9);
    sprintf(url, "mqtt://%s:%s", argv[1], argv[2]);
    char *client_id = argv[3];

    // Connect to the broker to perform the required tasks
    int rc = publish(url, client_id);

    // Termiante the program and return rc
    free(url);
    return rc;
}

/**
 * Connect to the MQTT broker and subscribe to the set of "request" topics.
 * Once new values in these topics are observed, start publishing.
 * 
 * @param url URL to the MQTT broker
 * @param id Client ID of this publisher
 */
int publish(char *url, char *id) {
    // Initialise the MQTT client configurations
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    // Create the MQTT client
    MQTTClient_create(&client, url, id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    // Establish a connection with the MQTT broker
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    // Define the message to be published
    pubmsg.payload = "Hello, MQTT!";  // Actual message
    pubmsg.payloadlen = strlen(pubmsg.payload);  // Length of the message
    pubmsg.qos = QOS;  // Quality of service level
    pubmsg.retained = false;  // Whether the broker shall retain the message

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    rc = MQTTClient_waitForCompletion(client, token, 10000);
    printf("Message with delivery token %d delivered\n", token);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}
