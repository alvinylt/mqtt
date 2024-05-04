#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "MQTTClient.h"

// Helper functions
int publish(char *url, char *client_id);
int message_arrive(void *context, char *topicName, int topicLen, MQTTClient_message *message);

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
    char *client_id = malloc(strlen(id) + 5);
    sprintf(client_id, "pub-%s", id);

    // Create the MQTT client
    MQTTClient_create(&client, url, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 1024;
    conn_opts.cleansession = true;

    int status = MQTTClient_setCallbacks(client, NULL, NULL, message_arrive, NULL);

    // Establish a connection with the MQTT broker
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    // Subscribe to the three specified topics
    char *topics[] = {"request/qos", "request/delay", "request/instancecount"};
    int qos[] = {0, 0, 0};
    
    status = MQTTClient_subscribeMany(client, 3, topics, qos);
    if (status != MQTTCLIENT_SUCCESS) {
        fprintf(stdout, "Problem\n");
    }
    else {
        fprintf(stdout, "No problem\n");
    }


    getchar();

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}

int message_arrive(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    fprintf(stdout, "Message on topic %s: %.*s\n", topicName, message->payloadlen, (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return true;
}
