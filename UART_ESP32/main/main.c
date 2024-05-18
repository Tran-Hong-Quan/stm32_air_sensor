#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN 17
#define RXD_PIN 16
#define RTS_PIN -1
#define RCS_PIN -1

static void led_Task(void *args);
void initUart(void);
static void tx_task(void *arg);
static void rx_task(void *arg);

// Controll led blink using UART

void app_main(void)
{
    initUart();
    xTaskCreate(rx_task, "uart_rx_task", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 2048, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(led_Task, "led_Task", 2048, NULL, configMAX_PRIORITIES - 3, NULL);
}

#pragma region UART

void initUart(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, RTS_PIN, RCS_PIN);
}

int sendData(const char *logName, const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_2, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    char msg[8] = "Hello \n\r";
    for (int i = strlen(msg); i < 8; i++)
    {
        msg[i] = 0;
    }

    while (1)
    {
        sendData(TX_TASK_TAG, msg);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: %s", rxBytes, data);
            // fscanf(data,"%d",&ledStatus);
            // ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

#pragma endregion

#pragma region LED

static void led_Task(void *args)
{
    gpio_config_t led_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = 1 << GPIO_NUM_2};

    gpio_config(&led_gpio_config);

    TickType_t delay = pdMS_TO_TICKS(1000.0);

    uint32_t value = 0;
    while (1)
    {
        value = !value;
        gpio_set_level(GPIO_NUM_2, value);
        vTaskDelay(delay);
        // ESP_LOGI("LED", "%ld", value);
    }
}

#pragma endregion