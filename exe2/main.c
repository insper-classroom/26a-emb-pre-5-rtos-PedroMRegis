#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_PIN_R = 28;
const int BTN_PIN_G = 26;

const int LED_PIN_R = 4;
const int LED_PIN_G = 6;

SemaphoreHandle_t xSemaphore_r;
SemaphoreHandle_t xSemaphore_g;
SemaphoreHandle_t xSemaphore_isr_r;

void btn_callback(uint gpio, uint32_t event){
  if (event == 0x4 && gpio == BTN_PIN_R){
    xSemaphoreGiveFromISR(xSemaphore_isr_r, 0);
  }
  
  if(event == 0x4 && gpio == BTN_PIN_G){
    xSemaphoreGiveFromISR(xSemaphore_g, 0);
  }
}

void btn_1_task(void *p) {
  gpio_init(BTN_PIN_R);
  gpio_set_dir(BTN_PIN_R, GPIO_IN);
  gpio_pull_up(BTN_PIN_R);
  gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

  while (true) {
    if (xSemaphoreTake(xSemaphore_isr_r, portMAX_DELAY) == pdTRUE) {
      xSemaphoreGive(xSemaphore_r);
      vTaskDelay(pdMS_TO_TICKS(200));
      xSemaphoreTake(xSemaphore_isr_r, 0);
    }
  }
}

void led_1_task(void *p) {
  gpio_init(LED_PIN_R);
  gpio_set_dir(LED_PIN_R, GPIO_OUT);
  
  int delay = 250;

  while (true) {
    if (xSemaphoreTake(xSemaphore_r, portMAX_DELAY) == pdTRUE) {
      gpio_put(LED_PIN_R, 1);
      vTaskDelay(pdMS_TO_TICKS(delay));
      gpio_put(LED_PIN_R, 0);
      vTaskDelay(pdMS_TO_TICKS(delay));
    }
  }
}

void led_2_task(void *p){
  gpio_init(LED_PIN_G);
  gpio_set_dir(LED_PIN_G, GPIO_OUT);

  gpio_init(BTN_PIN_G);
  gpio_set_dir(BTN_PIN_G, GPIO_IN);
  gpio_pull_up(BTN_PIN_G);
  gpio_set_irq_enabled(BTN_PIN_G, GPIO_IRQ_EDGE_FALL, true);
  
  int delay = 250;

  while(true){
    if(xSemaphoreTake(xSemaphore_g, pdMS_TO_TICKS(500))){
      gpio_put(LED_PIN_G, 1);
      vTaskDelay(pdMS_TO_TICKS(delay));
      gpio_put(LED_PIN_G, 0);
      vTaskDelay(pdMS_TO_TICKS(delay));
    }
  }
}

int main() {
  stdio_init_all();
  printf("Start RTOS \n");

  xSemaphore_isr_r = xSemaphoreCreateBinary();
  xSemaphore_r = xSemaphoreCreateBinary();
  xSemaphore_g = xSemaphoreCreateBinary();

  xTaskCreate(btn_1_task, "BTN_Task 1", 256, NULL, 1, NULL);
  xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
  xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);

  vTaskStartScheduler();

  while (true)
    ;
}