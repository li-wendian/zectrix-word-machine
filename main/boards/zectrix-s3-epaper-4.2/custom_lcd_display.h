#ifndef __CUSTOM_LCD_DISPLAY_H__
#define __CUSTOM_LCD_DISPLAY_H__

#include <stdint.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <lvgl.h>

#include <functional>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "lcd_display.h"

/* Display color */
typedef enum {
    DRIVER_COLOR_WHITE  = 0xff,
    DRIVER_COLOR_BLACK  = 0x00,
    FONT_BACKGROUND = DRIVER_COLOR_WHITE,
}COLOR_IMAGE;

typedef struct {
    uint8_t cs;
    uint8_t dc;
    uint8_t rst;
    uint8_t busy;
    uint8_t mosi;
    uint8_t scl;
    uint8_t power;
    int spi_host;
    int buffer_len;
}custom_lcd_spi_t;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rect;

class CustomLcdDisplay : public LcdDisplay {
public:
    CustomLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                  int width, int height, int offset_x, int offset_y,
                  bool mirror_x, bool mirror_y, bool swap_xy,custom_lcd_spi_t _lcd_spi_data);
    ~CustomLcdDisplay();

    void WriteRaw1bpp(int x, int y, int w, int h, const uint8_t* data, size_t len) override;
    void DrawTexts(const std::vector<TextItem>& texts, bool clear) override;

    void EPD_Init();
    void EPD_Clear();
    void EPD_Display();

    void EPD_DisplayPartBaseImage();
    void EPD_Init_Partial();
    void EPD_DisplayPart();
    void EPD_DrawColorPixel(uint16_t x, uint16_t y,uint8_t color);

    // Immediate refresh without forcing a full e-paper update.
    void RequestUrgentRefresh() override;
    // Force a full e-paper refresh on the next immediate update.
    void RequestUrgentFullRefresh() override;

    // Refresh state for sleep gating
    bool IsRefreshPending();

    // Notify when refresh transitions from busy to idle.
    void SetOnRefreshIdle(std::function<void()> cb);
    void SetNextKickMs(uint32_t kick_ms);
    
private:
    const custom_lcd_spi_t lcd_spi_data;
    const int Width;
    const int Height;
    spi_device_handle_t spi = nullptr;
    bool spi_bus_inited = false;
    uint8_t *buffer      = nullptr;   // 1bpp framebuffer
    uint8_t *prev_buffer = nullptr;   // optional
    uint8_t *tx_buf      = nullptr;   // snapshot buffer for async send (size = buffer_len)

    // LVGL
    static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p);

    // SPI/GPIO
    void spi_gpio_init();
    void spi_port_init();
    void spi_port_rx_init();
    void read_busy();

    void set_cs_1(){gpio_set_level((gpio_num_t)lcd_spi_data.cs,1);}
    void set_cs_0(){gpio_set_level((gpio_num_t)lcd_spi_data.cs,0);}
    void set_dc_1(){gpio_set_level((gpio_num_t)lcd_spi_data.dc,1);}
    void set_dc_0(){gpio_set_level((gpio_num_t)lcd_spi_data.dc,0);}
    void set_rst_1(){gpio_set_level((gpio_num_t)lcd_spi_data.rst,1);}
    void set_rst_0(){gpio_set_level((gpio_num_t)lcd_spi_data.rst,0);}
    void set_scl_1(){gpio_set_level((gpio_num_t)lcd_spi_data.scl,1);}
    void set_scl_0(){gpio_set_level((gpio_num_t)lcd_spi_data.scl,0);}

    void SPI_SendByte(uint8_t data);
    uint8_t SPI_RecvByte();
    uint8_t EPD_RecvData();
    void EPD_PowerOn();
    void EPD_PowerOff();
    void EPD_SendData(uint8_t data);
    void EPD_SendCommand(uint8_t command);
    void writeBytes(uint8_t *buf,int len);
    void writeBytes(const uint8_t *buf, int len);

    // SSD1683 helpers
    void EPD_SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);
    void EPD_SetCursor(uint16_t Xstart, uint16_t Ystart);
    void EPD_TurnOnDisplay();
    void EPD_TurnOnDisplayPart();
    void EPD_SetFullWindowAndCounter(); // ***关键：恢复全屏窗口+计数器***

    // Helper functions for partial display
    void bitInterleave(unsigned char bytes1, unsigned char bytes2);
    void WRITE_WHITE_TO_HLINE();
    void WRITE_HLINE_TO_VLINE();
    void WRITE_VLINE_TO_HLINE();

    uint8_t bw_threshold    = 200;

    // -------------------------
    // Async refresh mechanism
    // -------------------------
    void start_refresh_task();
    void stop_refresh_task();
    static void refresh_task_entry(void *arg);
    void refresh_task_loop();

    SemaphoreHandle_t dirty_mutex = nullptr;
    TaskHandle_t      refresh_task = nullptr;

    Rect dirty = {0,0,0,0};
    bool pending = false;

    bool urgent_refresh = false;
    bool force_full_refresh_ = false;
    TickType_t last_sample_tick = 0;
    int sample_interval_ms = 300; // 节流：采样间隔（可调 200~800）

    bool prev_buffer_synced = false;  // 标志：prev_buffer 是否已与屏幕同步
    bool refresh_in_progress = false;
    bool refresh_busy_seen_ = false;
    uint32_t next_kick_ms_ = 0;
    std::function<void()> on_refresh_idle_;

    void UpdateDisplayBusyLocked();
    bool CheckRefreshIdleLocked();

    // 文本渲染辅助
    void render_text_to_buffer(const char* text, int x, int y, const lv_font_t* font);
};

#endif // __CUSTOM_LCD_DISPLAY_H__
