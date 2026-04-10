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

QueueHandle_t xQueueBotao;
QueueHandle_t xQueueLedVermelho;
QueueHandle_t xQueueLedAmarelo;

void callback_botao(uint gpio, uint32_t eventos) {
    if (eventos == 0x4) {
        xQueueSendFromISR(xQueueBotao, &gpio, 0);
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
    int sinal = 1;

    while (true) {
        if (xQueueReceive(xQueueBotao, &gpio, pdMS_TO_TICKS(5000))) {
            vTaskDelay(pdMS_TO_TICKS(200));

            if (gpio == BTN_PIN_R) {
                xQueueSend(xQueueLedVermelho, &sinal, 0);
            } else if (gpio == BTN_PIN_Y) {
                xQueueSend(xQueueLedAmarelo, &sinal, 0);
            }
        }
    }
}

void task_led_vermelho(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    bool piscando = false;
    bool led_state = false;
    int dado;

    while (true) {
        if (piscando) {
            if (xQueueReceive(xQueueLedVermelho, &dado, pdMS_TO_TICKS(100))) {
                piscando = false;
                led_state = false;
                gpio_put(LED_PIN_R, 0);
            } else {
                led_state = !led_state;
                gpio_put(LED_PIN_R, led_state);
            }
        } else {
            if (xQueueReceive(xQueueLedVermelho, &dado, pdMS_TO_TICKS(5000))) {
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
    int dado;

    while (true) {
        if (piscando) {
            if (xQueueReceive(xQueueLedAmarelo, &dado, pdMS_TO_TICKS(100))) {
                piscando = false;
                led_state = false;
                gpio_put(LED_PIN_Y, 0);
            } else {
                led_state = !led_state;
                gpio_put(LED_PIN_Y, led_state);
            }
        } else {
            if (xQueueReceive(xQueueLedAmarelo, &dado, pdMS_TO_TICKS(5000))) {
                piscando = true;
            }
        }
    }
}

int main() {
    stdio_init_all();

    xQueueBotao       = xQueueCreate(32, sizeof(uint));
    xQueueLedVermelho = xQueueCreate(1, sizeof(int));
    xQueueLedAmarelo  = xQueueCreate(1, sizeof(int));

    xTaskCreate(task_botao,        "Tarefa Botao",        256, NULL, 1, NULL);
    xTaskCreate(task_led_vermelho, "Tarefa LED Vermelho", 256, NULL, 1, NULL);
    xTaskCreate(task_led_amarelo,  "Tarefa LED Amarelo",  256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}
