#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MQTTClient.h"

// Timeout for receiving incoming messages: around 17 minutes
#define TIMEOUT 1048576

// Helper functions for receiving and publishing messages
static MQTTClient *mqtt_connect(char *url, int instance);
static void listen_request(MQTTClient *client);
static void publish_counter(MQTTClient *client);
static void mqtt_disconnect(MQTTClient *client);

// Global variables: topics for subscription
char *subs_topics[] = {"request/qos", "request/delay", "request/instancecount"};

// Global variables: values from the subscribed topics
int values[] = {-1, -1, -1};
const int *qos = &values[0];
const int *delay = &values[1];
const int *instance_count = &values[2];

/**
 * The main function to execute the publisher program.
 * To execute the program, the command should include the MQTT broker's hostname
 * and its port, as well as the instance number of this publisher.
 */
int main(int argc, char *argv[]) {
    // Parse the command input
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Broker hostname> <Port> <Instance>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Create the full URL to the broker using its hostname and port number
    char *url = malloc(strlen(argv[1]) + strlen(argv[2]) + 9);
    sprintf(url, "mqtt://%s:%s", argv[1], argv[2]);

    // Check that the instance number is within the range from 1 to 5
    int instance = atoi(argv[3]);
    if (instance < 1 || instance > 5) {
        fprintf(stderr, "Error: instance number must be in between 1 and 5\n");
        exit(EXIT_FAILURE);
    }

    // Initialise an MQTTClient handle and connect to the broker
    MQTTClient *client = mqtt_connect(url, instance);
    free(url);

    // Listen for incoming messages (QoS, delay and instance count)
    listen_request(client);

    // Publish messages to the broker
    publish_counter(client);

    // Disconnect from the broker
    mqtt_disconnect(client);

    return 0;
}

/**
 * Connect to the MQTT broker and set up the MQTTClient handle.
 * 
 * @param url URL to the MQTT broker
 * @param instance Instance number of this publisher
 * @return the MQTTClient handle object
 */
static MQTTClient *mqtt_connect(char *url, int instance) {
    // Create the MQTTClient handle
    MQTTClient *client = (MQTTClient *)malloc(sizeof(MQTTClient));

    // Initialise the MQTT client configurations
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    char *client_id = malloc(6);
    sprintf(client_id, "pub-%d", instance);

    // Create the MQTT client
    MQTTClient_create(client, url, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 1024;
    conn_opts.cleansession = true;

    // Establish a connection with the MQTT broker
    int status = MQTTClient_connect(*client, &conn_opts);
    if (status != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Failed to connect, return code %d\n", status);
        mqtt_disconnect(client);
        exit(EXIT_FAILURE);
    }

    return client;
}

/**
 * Subscribe to the three specified topics and listen to incoming messages
 * until all values have been received.
 * 
 * If all three values are retained on the broker's record, they should be
 * immediately received and written to the global variables. Otherwise, this
 * functions waits for any incoming messages on these topics until all these
 * values are received or timeout expires.
 * 
 * @param client pointer to the MQTTClient handle object
 */
static void listen_request(MQTTClient *client) {
    // Subscribe to the three specified topics
    int subs_qos[] = {0, 0, 0};
    int status = MQTTClient_subscribeMany(*client, 3, subs_topics, subs_qos);
    if (status != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Error subscribing to the three topics\n");
        return;
    }

    // Variables for managing incoming messages
    MQTTClient_message *message = NULL;
    char *topic = NULL;
    int topic_length = -1;

    // Read incoming messages at subscribed topics
    while (*qos == -1 || *delay == -1 || *instance_count == -1) {
        // Wait for an income message
        MQTTClient_receive(*client, &topic, &topic_length, &message, TIMEOUT);

        // The message pointer is NULL when timeout expires
        if (message == NULL) {
            fprintf(stderr, "Timeout\n");
        }

        // Determine the topic and write the value to the relevant variable
        for (int i = 0; i <= 2; i++) {
            if (strcmp(topic, subs_topics[i]) == 0) {
                values[i] = atoi((char *)message->payload);
            }
        }

        // Free the memory allocated for the incoming message
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topic);
    }
}

/**
 * Publish the incrementing counter messages to the MQTT broker.
 * 
 * @param client pointer to the MQTTClient handle object
 */
static void publish_counter(MQTTClient *client) {
    (void)client;
    fprintf(stdout, "QoS: %d\n", *qos);
    fprintf(stdout, "Delay: %d\n", *delay);
    fprintf(stdout, "Instance count: %d\n", *instance_count);
}

/**
 * Disconnect from the MQTT broker before terminating the program.
 * 
 * @param client pointer to the MQTTClient handle object
 */
static void mqtt_disconnect(MQTTClient *client) {
    MQTTClient_disconnect(*client, 10000);
    MQTTClient_destroy(client);
    free(client);
}
