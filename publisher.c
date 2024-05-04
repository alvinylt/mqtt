#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

#define URL "tcp://localhost:1883"
#define CLIENTID "client123"
#define TOPIC "topic"
#define QOS 0

void connectionLost(void *context, char *cause);
int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message);

int main() {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, URL, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connectionLost, messageArrived, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    pubmsg.payload = "Hello, MQTT!";
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
        "on topic %s for client with ClientID: %s\n",
        (int)(10000/1000), pubmsg.payload, TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, 10000);
    printf("Message with delivery token %d delivered\n", token);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return rc;
}

void connectionLost(void *context, char *cause) {
    printf("\nConnection lost\n");
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    printf("\nMessage arrived\n");
    return 1;
}
