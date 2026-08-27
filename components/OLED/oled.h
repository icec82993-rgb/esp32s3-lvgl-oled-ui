#include <stdbool.h>
#include <stdint.h>
#include "oled_icons.h"



/** 蓉华板 SSD1306 OLED，I2C */
#define OLED_I2C_SDA_GPIO   4
#define OLED_I2C_SCL_GPIO   5
#define OLED_I2C_ADDR       0x3C
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_MIRROR_X       true  //屏幕坐标定位 x轴
#define OLED_MIRROR_Y       true  //屏幕坐标定位 y轴

#define OLED_LINE_0         0
#define OLED_LINE_1         1
#define OLED_LINE_2         2
#define OLED_LINE_3         3
#define OLED_MAX_LINES      4

bool oled_init(void);

/** 内容行 0~2 */
void oled_show_string(int line, const char *text);

/** 在某行前显示图标 */
void oled_show_icon(int line, const char *icon);

/**计数: N */
void oled_show_counter(int line, int value);

void oled_clear(void);
