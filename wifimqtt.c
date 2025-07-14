#include "hardware/structs/rosc.h"
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"

#define MQTT_SERVER_HOST "broker.hivemq.com"
#define MQTT_SERVER_PORT 1883
#define MQTT_TLS 0
#define BUFFER_SIZE 64

#define LED_PIN_R 13
#define LED_PIN_G 11


#define PIN_STATUS 15

typedef struct MQTT_CLIENT_T_ {
    ip_addr_t remote_addr;
    mqtt_client_t *mqtt_client;
    u32_t counter;
} MQTT_CLIENT_T;

MQTT_CLIENT_T* mqtt_client_init(void) {
    MQTT_CLIENT_T *state = calloc(1, sizeof(MQTT_CLIENT_T));
    return state;
}

void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T*)callback_arg;
    if (ipaddr) {
        state->remote_addr = *ipaddr;
        printf("DNS resolved: %s\n", ip4addr_ntoa(ipaddr));
    } else {
        printf("DNS resolution failed.\n");
    }
}

void run_dns_lookup(MQTT_CLIENT_T *state) {
    printf("Resolving %s...\n", MQTT_SERVER_HOST);
    if (dns_gethostbyname(MQTT_SERVER_HOST, &(state->remote_addr), dns_found, state) == ERR_INPROGRESS) {
        while (state->remote_addr.addr == 0) {
            cyw43_arch_poll();
            sleep_ms(10);
        }
    }
}

void mqtt_pub_cb(void *arg, err_t err) {
    printf("Publish status: %d\n", err);
}

void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        gpio_put(LED_PIN_G, 1);
        printf("MQTT connected.\n");
    } else {
        gpio_put(LED_PIN_R, 1);
        printf("MQTT connection failed: %d\n", status);
    }
}

err_t mqtt_publish_status(MQTT_CLIENT_T *state, int pin_value) {
    char msg[BUFFER_SIZE];
    snprintf(msg, BUFFER_SIZE, "{\"status\":%d}", pin_value);
    return mqtt_publish(state->mqtt_client, "bitdoglab/status", msg, strlen(msg), 0, 0, mqtt_pub_cb, NULL);
}

err_t mqtt_connect_to_broker(MQTT_CLIENT_T *state) {
    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = "bitdoglab-monitor";
    return mqtt_client_connect(state->mqtt_client, &state->remote_addr, MQTT_SERVER_PORT, mqtt_connection_cb, state, &ci);
}

void mqtt_run_loop(MQTT_CLIENT_T *state) {
    state->mqtt_client = mqtt_client_new();
    if (!state->mqtt_client) {
        printf("Failed to create MQTT client\n");
        return;
    }

    if (mqtt_connect_to_broker(state) != ERR_OK) {
        printf("MQTT connect failed.\n");
        return;
    }

    while (true) {
        cyw43_arch_poll();

        if (mqtt_client_is_connected(state->mqtt_client)) {
            int pin_value = gpio_get(PIN_STATUS);
            mqtt_publish_status(state, pin_value);
            printf("Pin %d status: %d\n", PIN_STATUS, pin_value);
            sleep_ms(1000);
        } else {
            printf("MQTT disconnected. Reconnecting...\n");
            sleep_ms(2000);
            mqtt_connect_to_broker(state);
        }
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_init(LED_PIN_G); gpio_set_dir(LED_PIN_G, GPIO_OUT);
    gpio_put(LED_PIN_R, 0); gpio_put(LED_PIN_G, 0);

    gpio_init(PIN_STATUS);
    gpio_set_dir(PIN_STATUS, GPIO_IN);
    gpio_pull_up(PIN_STATUS); 

    if (cyw43_arch_init()) {
        printf("Failed to init WiFi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("WiFi connection failed\n");
        return 1;
    }

    printf("Connected to WiFi.\n");

    MQTT_CLIENT_T *state = mqtt_client_init();
    run_dns_lookup(state);
    mqtt_run_loop(state);

    cyw43_arch_deinit();
    return 0;
}
