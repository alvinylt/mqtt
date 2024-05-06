#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "MQTTClient.h"

// String buffer size
#define BUFFER_SIZE 128

// Perform each analysis for 60 seconds
#define TIME_LIMIT 60

// Helper functions
static MQTTClient *mqtt_connect(char *url);
static void analyse(MQTTClient *client, short int broker_to_analyser_qos,
                    short int qos, short int delay, short int instance_count);
static void mqtt_disconnect(MQTTClient *client);
static void listen_counter(MQTTClient *client);
static double find_median(double arr[], int n);
static int compare(const void *a, const void *b);

/**
 * The mian function executes the analyser.
 * To execute the program, the command should include the hostname and the port
 * number of the MQTT broker as arguments.
 */
int main(int argc, char *argv[]) {
    // Parse the command input
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Broker hostname> <Port>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Create the full URL to the broker using its hostname and port
    char *url = malloc(strlen(argv[1]) + strlen(argv[2]) + 9);
    sprintf(url, "mqtt://%s:%s", argv[1], argv[2]);

    // Initialise an MQTTClient handle and connect to the broker
    MQTTClient *client = mqtt_connect(url);

    // Perform the analysis
    int qos_levels[] = {0, 1, 2};
    int delay_length[] = {0, 1, 2, 4};
    int instance_counts[] = {1, 2, 3, 4, 5};
    for (int h = 0; h < 3; h++)
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 4; j++)
                for (int k = 0; k < 5; k++) {
                    fprintf(stdout, "Analysis:\n"
                        "    Broker-analyser QoS: level %d\n"
                        "    Publisher-broker QoS: level %d\n"
                        "    Delay time: %d ms\n"
                        "    Number of active publishers: %d\n",
                        qos_levels[h], qos_levels[i],
                        delay_length[j], instance_counts[k]);
                    analyse(client, qos_levels[h], qos_levels[i],
                            delay_length[j], instance_counts[k]);
                }

    // Disconnect from the broker before termination
    mqtt_disconnect(client);

    return EXIT_SUCCESS;
}

/**
 * Connect to the MQTT broker and set up the MQTTClient handle.
 * 
 * @param url URL to the MQTT broker
 * @return the MQTTClient handle object
 */
static MQTTClient *mqtt_connect(char *url) {
    // Create the MQTTClient handle
    MQTTClient *client = (MQTTClient *)malloc(sizeof(MQTTClient));

    // Initialise the MQTT client configurations
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    char *client_id = "analyser";

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

    return client;
}

/**
 * Analyse with a particular set of QoS, delay and instance count values.
 * 
 * @param client pointer to the MQTTClient handle object
 * @param broker_to_analyser_qos QoS level for the analyser's subscription
 * @param qos QoS level to be tested
 * @param delay delay length to be tested
 * @param instance_count instance count to be tested
 */
static void analyse(MQTTClient *client, short int broker_to_analyser_qos,
                    short int qos, short int delay, short int instance_count) {
    // Create a string containing the topic name
    char topic[BUFFER_SIZE];
    snprintf(topic, BUFFER_SIZE - 1, "counter/%d/%d/%d", 1, qos, delay);
    topic[BUFFER_SIZE - 1] = '\0';

    // Subscribe to the topic that shall be analysed
    int status = MQTTClient_subscribe(*client, topic, broker_to_analyser_qos);
    if (status != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Error: unable to subscribe to %s\n", topic);
        mqtt_disconnect(client);
        exit(EXIT_FAILURE);
    }

    // Formulate the request messages to be published
    char qos_msg[BUFFER_SIZE];
    char delay_msg[BUFFER_SIZE];
    char instance_count_msg[BUFFER_SIZE];
    snprintf(qos_msg, BUFFER_SIZE - 1, "%d", qos);
    qos_msg[BUFFER_SIZE - 1] = '\0';
    snprintf(delay_msg, BUFFER_SIZE - 1, "%d", delay);
    delay_msg[BUFFER_SIZE - 1] = '\0';
    snprintf(instance_count_msg, BUFFER_SIZE - 1, "%d", instance_count);
    instance_count_msg[BUFFER_SIZE - 1] = '\0';

    MQTTClient_message qos_message = MQTTClient_message_initializer;
    qos_message.payload = qos_msg;
    qos_message.payloadlen = strlen(qos_msg);
    qos_message.qos = broker_to_analyser_qos;
    qos_message.retained = false;

    MQTTClient_message delay_message = MQTTClient_message_initializer;
    delay_message.payload = delay_msg;
    delay_message.payloadlen = strlen(delay_msg);
    delay_message.qos = broker_to_analyser_qos;
    delay_message.retained = false;

    MQTTClient_message instance_count_message = MQTTClient_message_initializer;
    instance_count_message.payload = instance_count_msg;
    instance_count_message.payloadlen = strlen(instance_count_msg);
    instance_count_message.qos = broker_to_analyser_qos;
    instance_count_message.retained = false;

    // Publish the request messages
    MQTTClient_publishMessage(*client, "request/qos", &qos_message, NULL);
    MQTTClient_publishMessage(*client, "request/delay", &delay_message, NULL);
    MQTTClient_publishMessage(*client, "request/instancecount", &instance_count_message, NULL);

    // Listen to the current topic for the counter's responses for analysis
    listen_counter(client);

    // Unsubscribe from the current topic
    MQTTClient_unsubscribe(*client, topic);
}

/**
 * Listen to the counter publisher pub-1's response and evaluate the statistics.
 * 
 * @param client pointer to the MQTTClient handle object
 */
static void listen_counter(MQTTClient *client) {
    // Specify the time length of the entire analysis
    time_t end_time = time(NULL) + TIME_LIMIT;

    // The last counter number received on the topic
    long int last_count = -1;
    // The total number of messages
    long int number_of_messages = 0;
    // The number of out-of-order messages
    int out_of_order_count = 0;

    // Delay time length
    double delays[1000000];

    // Listen to the topic for the specified period of time (60 seconds)
    while (time(NULL) < end_time) {
        MQTTClient_message *message = NULL;
        char *topic_name = NULL;
        int topic_len = -1;

        // Record the time required before receiving the next message
        double start_time = clock();
        if (MQTTClient_receive(*client, &topic_name, &topic_len, &message, 5)
                != MQTTCLIENT_SUCCESS) {
            continue;
        }

        // Analyse the incoming counter message
        if (topic_name != NULL) {
            int this_count = atoi((char *)message->payload);
            // The incoming message is out of order
            if (this_count < last_count + 1) {
                out_of_order_count++;
                number_of_messages++;
            }
            else if (this_count > last_count + 1) {
                last_count = this_count;
                number_of_messages++;
            }
            // The incoming message is in order
            else {
                double end_time = clock();
                delays[number_of_messages++] = (end_time - start_time) / CLOCKS_PER_SEC * 1000;
                last_count = this_count;
            }

            MQTTClient_freeMessage(&message);
            MQTTClient_free(topic_name);
        }
    }

    // The average rate of incoming messages
    double average_msg_rate = number_of_messages / TIME_LIMIT;
    // The percentage of out-of-order messages (amongst all incoming messages)
    double out_of_order_msg_rate = out_of_order_count / number_of_messages;
    // The average delay time length
    double avg_delay = find_median(delays, number_of_messages);

    // Print the statistics to terminal output
    fprintf(stdout,
            "Last counter value received: %ld\n"
            "Total number of messages: %ld\n"
            "Average incoming message rate: %d per second\n"
            "Percentage of out-of-order messages: %.2f%%\n"
            "Average delay time length: %.4f ms\n\n",
        last_count, number_of_messages, (int)average_msg_rate,
        out_of_order_msg_rate / 100, avg_delay);
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

static double find_median(double arr[], int n) {
    qsort(arr, n, sizeof(double), compare);
    return n % 2 != 0 ? arr[n / 2] : (arr[n / 2 - 1] + arr[n / 2]) / 2.0;
}

static int compare(const void *a, const void *b) {
    double difference = (*(double*)a - *(double*)b);
    if (difference < 0) return -1;
    else if (difference > 0) return 1;
    else return 0;
}
