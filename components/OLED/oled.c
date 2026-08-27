#include "oled.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

// 声明外部字体：中文常规字体、图标字体
LV_FONT_DECLARE(font_puhui_14_1);
LV_FONT_DECLARE(font_awesome_14_1);

static const char *TAG = "oled";

// I2C总线、屏幕IO、屏幕句柄
static i2c_master_bus_handle_t s_i2c_bus;
static esp_lcd_panel_io_handle_t s_panel_io;
static esp_lcd_panel_handle_t s_panel;

// 每行对应的文字控件、图标控件数组（最大4行）
static lv_obj_t *s_line_labels[OLED_MAX_LINES];
static lv_obj_t *s_line_icons[OLED_MAX_LINES];

// 单行高度、有效显示行数
static int s_line_h;
static int s_visible_lines;

/**
 * @brief  获取LVGL锁，防止多任务刷屏花屏、崩溃
 * @param  ms: 超时时间ms，-1为永久等待
 * @retval 成功返回true，失败false
 */
static bool oled_lock(int ms)
{
    return lvgl_port_lock(ms);
}

/**
 * @brief  释放LVGL锁
 */
static void oled_unlock(void)
{
    lvgl_port_unlock();
}

/**
 * @brief  创建I2C主机总线
 * @param  sda: SDA引脚
 * @param  scl: SCL引脚
 * @retval ESP_OK成功
 */
static esp_err_t i2c_bus_create(int sda, int scl)
{
    // 如果已有总线，先销毁防止重复初始化
    if (s_i2c_bus) {
        i2c_del_master_bus(s_i2c_bus);
        s_i2c_bus = NULL;
    }

    // I2C总线配置
    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,        // 过滤毛刺干扰
        .flags.enable_internal_pullup = true, // 开启内部上拉
    };
    return i2c_new_master_bus(&cfg, &s_i2c_bus);
}

/**
 * @brief  SSD1306屏幕底层初始化
 * @param  addr: I2C设备地址
 * @param  height: 屏幕高度
 * @retval ESP_OK成功
 */
static esp_err_t panel_init(uint8_t addr, uint8_t height)
{
    // 先释放旧句柄，防止重复初始化报错
    if (s_panel) {
        esp_lcd_panel_del(s_panel);
        s_panel = NULL;
    }
    if (s_panel_io) {
        esp_lcd_panel_io_del(s_panel_io);
        s_panel_io = NULL;
    }

    // I2C设备IO配置
    esp_lcd_panel_io_i2c_config_t io = {
        .dev_addr = addr,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .scl_speed_hz = 100000,  // I2C低速100K，稳定不花屏
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c_v2(s_i2c_bus, &io, &s_panel_io), TAG, "io");

    // 屏幕设备基础配置
    esp_lcd_panel_dev_config_t pcfg = {
        .reset_gpio_num = -1, // 无复位引脚
        .bits_per_pixel = 1,  // OLED单色1bit像素
    };

    // SSD1306专用配置（指定屏幕高度）
    esp_lcd_panel_ssd1306_config_t ssd = { .height = height };
    pcfg.vendor_config = &ssd;

    // 执行屏幕初始化、复位、点亮屏幕
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ssd1306(s_panel_io, &pcfg, &s_panel), TAG, "ssd1306");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "on");

    return ESP_OK;
}

/**
 * @brief  检测并初始化屏幕（固定引脚、固定地址）
 * @retval true屏幕正常，false异常
 */
static bool panel_detect(void)
{
    // 创建I2C总线
    if (i2c_bus_create(OLED_I2C_SDA_GPIO, OLED_I2C_SCL_GPIO) != ESP_OK) {
        return false;
    }

    // 初始化SSD1306屏幕
    if (panel_init(OLED_I2C_ADDR, OLED_HEIGHT) == ESP_OK) {
        return true;
    }

    // 初始化失败，销毁总线释放资源
    i2c_del_master_bus(s_i2c_bus);
    s_i2c_bus = NULL;
    return false;
}

/**
 * @brief  LVGL图形库初始化、绑定屏幕设备
 * @retval ESP_OK成功
 */
static esp_err_t lvgl_init(void)
{
    // 初始化LVGL端口
    ESP_RETURN_ON_ERROR(lvgl_port_init(&((lvgl_port_cfg_t)ESP_LVGL_PORT_INIT_CONFIG())), TAG, "lvgl");

    // 屏幕显示参数配置
    lvgl_port_display_cfg_t d = {
        .io_handle = s_panel_io,
        .panel_handle = s_panel,
        .buffer_size = OLED_WIDTH * OLED_HEIGHT, // 全屏缓存大小
        .double_buffer = false,
        .hres = OLED_WIDTH,
        .vres = OLED_HEIGHT,
        .monochrome = true, // 单色OLED
        .rotation = { .mirror_x = OLED_MIRROR_X, .mirror_y = OLED_MIRROR_Y },
        .flags = { .buff_dma = 1 },
    };

    // 挂载显示设备到LVGL
    return lvgl_port_add_disp(&d) ? ESP_OK : ESP_FAIL;
}

/**
 * @brief  创建单行UI布局：【图标 + 文字】横向布局
 * @param  parent: 父容器(屏幕画布)
 * @param  y: 该行Y轴起始坐标
 * @param  ico: 返回该行图标控件句柄
 * @param  txt: 返回该行文字控件句柄
 */
static void make_line_row(lv_obj_t *parent, int y, lv_obj_t **ico, lv_obj_t **txt)
{
    // 1. 创建行容器（整行背景、布局载体）
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, OLED_WIDTH, s_line_h);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_style_pad_all(row, 2, 0);       // 左右留2px边距，文字不贴边
    lv_obj_set_style_border_width(row, 0, 0);  // 无边框
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0); // 透明背景
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);    // 子控件横向排列
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE); // 禁止滚动

    // 2. 创建图标控件（固定16px宽度，默认隐藏）
    *ico = lv_label_create(row);
    lv_obj_set_style_text_font(*ico, &font_awesome_14_1, 0);
    lv_obj_set_width(*ico, 16);
    lv_label_set_text(*ico, "");
    lv_obj_add_flag(*ico, LV_OBJ_FLAG_HIDDEN); // 默认隐藏，有图标再显示

    // 3. 创建文字控件（自动填充剩余宽度、长文字滚动）
    *txt = lv_label_create(row);
    lv_obj_set_style_text_font(*txt, &font_puhui_14_1, 0);
    lv_label_set_text(*txt, "");
    lv_obj_set_flex_grow(*txt, 1); // 自动占满剩余空间
    lv_label_set_long_mode(*txt, LV_LABEL_LONG_SCROLL_CIRCULAR); // 长文字循环滚动
}

/**
 * @brief  初始化全局UI界面：创建4行显示布局
 */
static void setup_ui(void)
{
    // 固定64屏：4行全部显示，均分屏幕高度
    s_visible_lines = OLED_MAX_LINES;
    s_line_h = OLED_HEIGHT / s_visible_lines;

    // 获取屏幕根画布
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_text_font(scr, &font_puhui_14_1, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // 循环创建4行UI
    const int top_margin = 2; // 顶部2px偏移，防止第一行裁切
    for (int i = 0; i < OLED_MAX_LINES; i++) {
        int y = top_margin + i * s_line_h;
        make_line_row(scr, y, &s_line_icons[i], &s_line_labels[i]);
        if (i >= s_visible_lines) {
            lv_obj_add_flag(lv_obj_get_parent(s_line_labels[i]), LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * @brief  OLED整体初始化入口
 * @retval true成功，false失败
 */
bool oled_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(100)); // 上电延时稳定屏幕

    // 硬件I2C+屏幕初始化
    if (!panel_detect()) {
        ESP_LOGE(TAG, "OLED not found (I2C NACK)");
        return false;
    }

    // LVGL图形库初始化
    if (lvgl_init() != ESP_OK) {
        return false;
    }

    // 创建UI界面
    if (!oled_lock(-1)) {
        return false;
    }
    setup_ui();
    oled_unlock();

    // 打印初始化信息
    ESP_LOGI(TAG, "OK %dx%d SDA=%d SCL=%d addr=0x%02X line_h=%d",
         OLED_WIDTH, OLED_HEIGHT, OLED_I2C_SDA_GPIO, OLED_I2C_SCL_GPIO, OLED_I2C_ADDR, s_line_h);
    return true;
}

/**
 * @brief  指定行显示文字
 * @param  line: 行号(0~3)
 * @param  text: 显示字符串
 */
void oled_show_string(int line, const char *text)
{
    // 行号合法性校验、非空校验、加锁保护
    if (line < 0 || line >= OLED_MAX_LINES || !text || !oled_lock(100)) {
        return;
    }
    lv_label_set_text(s_line_labels[line], text);
    oled_unlock();
}

/**
 * @brief  指定行显示图标
 * @note   有图标则显示图标+文字右移，无图标则隐藏图标、文字顶格
 * @param  line: 行号(0~3)
 * @param  icon: 图标宏字符串
 */
void oled_show_icon(int line, const char *icon)
{
    if (line < 0 || line >= OLED_MAX_LINES || !oled_lock(100))
    {
        return;
    }

    lv_obj_t *ico = s_line_icons[line];
    if (icon && icon[0])
    {
        // 有效图标：显示图标、刷新内容
        lv_label_set_text(ico, icon);
        lv_obj_clear_flag(ico, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        // 空图标：清空并隐藏，文字自动顶格
        lv_label_set_text(ico, "");
        lv_obj_add_flag(ico, LV_OBJ_FLAG_HIDDEN);
    }

    oled_unlock();
}

/**
 * @brief  指定行显示数字
 * @param  line: 行号(0~3)
 * @param  value: 整型数字
 */
void oled_show_counter(int line,int value)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    oled_show_string(line, buf);
}

/**
 * @brief  清空屏幕所有行文字和图标
 */
void oled_clear(void)
{
    if (!oled_lock(100)) {
        return;
    }

    // 清空所有可见行
    for (int i = 0; i < s_visible_lines; i++) {
        lv_label_set_text(s_line_labels[i], "");
        lv_label_set_text(s_line_icons[i], "");
    }

    oled_unlock();
}
