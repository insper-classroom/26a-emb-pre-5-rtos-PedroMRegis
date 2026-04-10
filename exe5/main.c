/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemaphoreLedR;
SemaphoreHandle_t xSemaphoreLedY;

void callback_botao(uint gpio, uint32_t eventos) {
    if (eventos == 0x4) {
        xQueueSendFromISR(xQueueBtn, &gpio, 0);
    }
}

void task_botao(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &callback_botao);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    uint gpio;

    while (true) {
        if (xQueueReceive(xQueueBtn, &gpio, pdMS_TO_TICKS(5000))) {
            vTaskDelay(pdMS_TO_TICKS(200));

            if (gpio == BTN_PIN_R) {
                xSemaphoreGive(xSemaphoreLedR);
            } else if (gpio == BTN_PIN_Y) {
                xSemaphoreGive(xSemaphoreLedY);
            }
        }
    }
}

void task_led_vermelho(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    bool piscando = false;
    bool led_state = false;

    while (true) {
        if (piscando) {
            if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(100))) {
                piscando = false;
                led_state = false;
                gpio_put(LED_PIN_R, 0);
            } else {
                led_state = !led_state;
                gpio_put(LED_PIN_R, led_state);
            }
        } else {
            if (xSemaphoreTake(xSemaphoreLedR, pdMS_TO_TICKS(5000))) {
                piscando = true;
            }
        }
    }
}

void task_led_amarelo(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    bool piscando = false;
    bool led_state = false;

    while (true) {
        if (piscando) {
            if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(100))) {
                piscando = false;
                led_state = false;
                gpio_put(LED_PIN_Y, 0);
            } else {
                led_state = !led_state;
                gpio_put(LED_PIN_Y, led_state);
            }
        } else {
            if (xSemaphoreTake(xSemaphoreLedY, pdMS_TO_TICKS(5000))) {
                piscando = true;
            }
        }
    }
}

int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(32, sizeof(uint));
    xSemaphoreLedR = xSemaphoreCreateBinary();
    xSemaphoreLedY = xSemaphoreCreateBinary();

    xTaskCreate(task_botao, "Task Botao", 256, NULL, 1, NULL);
    xTaskCreate(task_led_vermelho, "Task LED Vermelho", 256, NULL, 1, NULL);
    xTaskCreate(task_led_amarelo, "Task LED Amarelo", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}
