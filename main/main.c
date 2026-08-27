#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "oled.h"

static const char *TAG = "main";

void app_main(void)
{
    if (!oled_init()) {
        ESP_LOGE(TAG, "OLED init failed");
        return;
    }

    oled_show_string(OLED_LINE_0, "你好，小猩！");
    oled_show_string(OLED_LINE_1, "Hello World 陕西理工大学 电子专业");
    oled_show_icon(OLED_LINE_1, OLED_ICON_AI);
    oled_show_string(OLED_LINE_2, "第三行空白测试");

    int counter = 0;
    oled_show_counter(OLED_LINE_3,counter);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        counter++;
        oled_show_counter(OLED_LINE_3,counter);
        ESP_LOGI(TAG, "counter=%d", counter);
    }
}