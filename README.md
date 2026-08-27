ESP32‑S3 驱动 OLED 屏幕，基于 LVGL 图形库实现图形界面演示项目。

## 项目简介
本项目基于 ESP‑IDF 开发框架，使用 LVGL 图形库在 OLED 显示屏上运行 GUI 界面。
可用于学习 LVGL 基础控件、菜单、仪表盘、动画等图形功能开发，适合嵌入式 UI 入门。

## 硬件平台
- 主控芯片：**ESP32‑S3**
- 显示屏：OLED (SSD1306 / SH1106) I2C
- 开发环境：ESP‑IDF v5.4+
- 图形库：LVGL

## 开发环境
- VS‑Code + ESP‑IDF 插件
- Windows / Linux
- 串口：COM13(示例端口，请根据自己设备修改)

## 编译 & 烧录

1. 打开项目文件夹至 VS‑Code
2. 加载 ESP‑IDF 环境
3. 设置目标芯片
```bash
idf.py set-target esp32s3
```
4. 编译项目
```bash
idf.py build
```
5. 烧录固件
```bash
idf.py flash
```
6. 打开串口监视器
```bash
idf.py monitor
```

> 💡 烧录失败提示端口被占用：请先关闭 `idf_monitor` 串口终端再重新烧录。

## 项目目录结构
```
9_Oled_LVGL/
├── main/                # 主程序代码
│   └── app_main.c      # 入口函数
├── components/         # 自定义组件、LVGL驱动
├── build/              # 编译输出文件(可删除重新编译)
├── CMakeLists.txt      # CMake编译配置
└── sdkconfig           # ESP‑IDF配置文件
```

## 功能清单
- ✅ LVGL 初始化与屏幕驱动
- ✅ OLED 屏幕基础显示
- ☐ 文本、按钮、标签控件演示
- ☐ 菜单界面
- ☐ 仪表盘动画（后续扩展）

## 注意事项
1. I2C 引脚号根据你的硬件接线修改代码
2. ssd1306 / sh1106 屏幕驱动代码不可混用
3. 上传 GitHub 建议忽略 build、.vscode、venv 缓存文件
    可添加 `.gitignore` 文件：
```
build/
.vscode/
*.log
```