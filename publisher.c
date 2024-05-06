#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include "MQTTClient.h"

// Timeout for receiving incoming messages
#define TIMEOUT 1048576

// Publish messages for 60 seconds
#define TIME_LIMIT 60

// String buffer sizes
#define BUFFER_SIZE 128

// Helper functions for receiving and publishing messages
static MQTTClient *mqtt_connect(char *url, const short int instance);
static void listen_request(MQTTClient *client);
static void publish_counter(MQTTClient *client, const short int instance);
static void mqtt_disconnect(MQTTClient *client);

// Global variables: topics for subscription
char *subs_topics[] = {"request/qos", "request/delay", "request/instancecount"};

// Global variables: values from the subscribed topics
short int values[] = {-1, -1, -1};
short int *qos = NULL;
short int *delay = NULL;
short int *instance_count = NULL;

/**
 * The main function to execute the publisher program.
 * To execute the program, the command should include the MQTT broker's hostname
 * and its port, as well as the instance number (1-5) of this publisher.
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
    const short int instance = atoi(argv[3]);
    if (instance < 1 || instance > 5) {
        fprintf(stderr, "Error: instance number must be in between 1 and 5\n");
        exit(EXIT_FAILURE);
    }

    // Initialise an MQTTClient handle and connect to the broker
    MQTTClient *client = mqtt_connect(url, instance);
    free(url);

    while (true) {
        // Refresh the QoS, delay and instance count records
        qos = delay = instance_count = NULL;

        // Listen for incoming messages (QoS, delay and instance count)
        listen_request(client);

        // Publish messages to the broker
        publish_counter(client, instance);
    }

    // Disconnect from the broker if the while loop iteration stops
    mqtt_disconnect(client);

    return EXIT_SUCCESS;
}

/**
 * Connect to the MQTT broker and set up the MQTTClient handle.
 * 
 * @param url URL to the MQTT broker
 * @param instance Instance number of this publisher
 * @return the MQTTClient handle object
 */
static MQTTClient *mqtt_connect(char *url, const short int instance) {
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
        fprintf(stderr, "Error: failed to connect, return code %d\n", status);
        mqtt_disconnect(client);
        exit(EXIT_FAILURE);
    }

    // Subscribe to the three specified topics
    int subs_qos[] = {0, 0, 0};
    status = MQTTClient_subscribeMany(*client, 3, subs_topics, subs_qos);
    if (status != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Error: cannot subscribe to the three topics\n");
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
    // Variables for managing incoming messages
    MQTTClient_message *message = NULL;
    char *topic = NULL;
    int topic_length = -1;

    // Read incoming messages at subscribed topics
    while (qos == NULL || delay == NULL || instance_count == NULL) {
        // Wait for an income message
        MQTTClient_receive(*client, &topic, &topic_length, &message, TIMEOUT);

        // The message pointer is NULL when timeout expires
        if (message == NULL) {
            fprintf(stderr, "Error: timeout expires\n");
            mqtt_disconnect(client);
            exit(EXIT_FAILURE);
        }

        // Determine the topic and write the value to the relevant variable
        if (strcmp(topic, subs_topics[0]) == 0) {
            values[0] = atoi((char *)message->payload);
            qos = &values[0];
        }
        else if (strcmp(topic, subs_topics[1]) == 0) {
            values[1] = atoi((char *)message->payload);
            delay = &values[1];
        }
        else if (strcmp(topic, subs_topics[2]) == 0) {
            values[2] = atoi((char *)message->payload);
            instance_count = &values[2];
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
static void publish_counter(MQTTClient *client, const short int instance) {
    // Check the validity of the values
    bool values_valid = true;
    if (*qos < 0 || *qos > 2) {
        fprintf(stderr, "Error: invalid QoS level %d\n", *qos);
        values_valid = false;
    }
    if (*delay != 0 && *delay != 1 && *delay != 2 && *delay != 4) {
        fprintf(stderr, "Error: invalid delay length %d\n", *delay);
        values_valid = false;
    }
    if (*instance_count < 1 || *instance_count > 5) {
        fprintf(stderr, "Error: invalid instance count %d\n", *instance_count);
        values_valid = false;
    }

    // Do not proceed if any values are invalid
    if (!values_valid) return;

    // Do not proceed if the instance count is smaller than this publisher's ID
    if (*instance_count < instance) {
        return;
    }

    // Topic to which message shall be published
    char topic[BUFFER_SIZE];
    snprintf(topic, BUFFER_SIZE, "counter/%d/%d/%d", instance, *qos, *delay);
    topic[BUFFER_SIZE - 1] = '\0';

    // Counter value to be published to the broker
    long int counter = 0;

    // For evaluating the 60-second time limit
    time_t start_time, current_time;
    double elapsed_time;
    time(&start_time);

    // Message to be published
    MQTTClient_message message = MQTTClient_message_initializer;
    message.qos = *qos;
    message.retained = false;
    char payload[BUFFER_SIZE];
    
    // Publish messages until the 60-second time limit elapses
    while (true) {
        // Define the payload of the message
        snprintf(payload, BUFFER_SIZE, "%ld", counter);
        payload[BUFFER_SIZE - 1] = '\0';
        message.payload = payload;
        message.payloadlen = strlen(payload);

        // Publish the message
        MQTTClient_publishMessage(*client, topic, &message, NULL);

        // Increment the counter
        counter++;

        // Stop publishing if the 60-second time limit is over
        time(&current_time);
        elapsed_time = difftime(current_time, start_time);
        if (elapsed_time >= TIME_LIMIT)
            break;

        // Delay the next message publishing for the specified time frame
        sleep(*delay/1000);
    }
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
