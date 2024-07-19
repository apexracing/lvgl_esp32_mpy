#include "display.h"

#include "py/runtime.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "lvgl_esp32_display";

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           32
#define LCD_PARAM_BITS         8
static const st77916_lcd_init_cmd_t lcd_init_cmds[] = {
        {0xF0, (uint8_t[]){0x28}, 1, 0},
        {0xF2, (uint8_t[]){0x28}, 1, 0},
        {0x7C, (uint8_t[]){0xD1}, 1, 0},
        {0x83, (uint8_t[]){0xE0}, 1, 0},
        {0x84, (uint8_t[]){0x61}, 1, 0},
        {0xF2, (uint8_t[]){0x82}, 1, 0},
        {0xF0, (uint8_t[]){0x00}, 1, 0},
        {0xF0, (uint8_t[]){0x01}, 1, 0},
        {0xF1, (uint8_t[]){0x01}, 1, 0},
        {0xB0, (uint8_t[]){0x49}, 1, 0},
        {0xB1, (uint8_t[]){0x4A}, 1, 0},
        {0xB2, (uint8_t[]){0x1F}, 1, 0},
        {0xB4, (uint8_t[]){0x46}, 1, 0},
        {0xB5, (uint8_t[]){0x34}, 1, 0},
        {0xB6, (uint8_t[]){0xD5}, 1, 0},
        {0xB7, (uint8_t[]){0x30}, 1, 0},
        {0xB8, (uint8_t[]){0x04}, 1, 0},
        {0xBA, (uint8_t[]){0x00}, 1, 0},
        {0xBB, (uint8_t[]){0x08}, 1, 0},
        {0xBC, (uint8_t[]){0x08}, 1, 0},
        {0xBD, (uint8_t[]){0x00}, 1, 0},
        {0xC0, (uint8_t[]){0x80}, 1, 0},
        {0xC1, (uint8_t[]){0x10}, 1, 0},
        {0xC2, (uint8_t[]){0x37}, 1, 0},
        {0xC3, (uint8_t[]){0x80}, 1, 0},
        {0xC4, (uint8_t[]){0x10}, 1, 0},
        {0xC5, (uint8_t[]){0x37}, 1, 0},
        {0xC6, (uint8_t[]){0xA9}, 1, 0},
        {0xC7, (uint8_t[]){0x41}, 1, 0},
        {0xC8, (uint8_t[]){0x01}, 1, 0},
        {0xC9, (uint8_t[]){0xA9}, 1, 0},
        {0xCA, (uint8_t[]){0x41}, 1, 0},
        {0xCB, (uint8_t[]){0x01}, 1, 0},
        {0xD0, (uint8_t[]){0x91}, 1, 0},
        {0xD1, (uint8_t[]){0x68}, 1, 0},
        {0xD2, (uint8_t[]){0x68}, 1, 0},
        {0xF5, (uint8_t[]){0x00, 0xA5}, 2, 0},
        {0xF1, (uint8_t[]){0x10}, 1, 0},
        {0xF0, (uint8_t[]){0x00}, 1, 0},
        {0xF0, (uint8_t[]){0x02}, 1, 0},
        {0xE0, (uint8_t[]){0x70, 0x09, 0x12, 0x0c, 0x0b, 0x27, 0x38, 0x54, 0x4e, 0x19, 0x15, 0x15, 0x2c, 0x2f}, 14, 0},
        {0xE1, (uint8_t[]){0x70, 0x08, 0x11, 0x0c, 0x0b, 0x27, 0x38, 0x43, 0x4c, 0x18, 0x14, 0x14, 0x2b, 0x24}, 14, 0},
        {0xF0, (uint8_t[]){0x10}, 1, 0},
        {0xF3, (uint8_t[]){0x10}, 1, 0},
        {0xE0, (uint8_t[]){0x08}, 1, 0},
        {0xE1, (uint8_t[]){0x00}, 1, 0},
        {0xE2, (uint8_t[]){0x0B}, 1, 0},
        {0xE3, (uint8_t[]){0x00}, 1, 0},
        {0xE4, (uint8_t[]){0xE0}, 1, 0},
        {0xE5, (uint8_t[]){0x06}, 1, 0},
        {0xE6, (uint8_t[]){0x21}, 1, 0},
        {0xE7, (uint8_t[]){0x00}, 1, 0},
        {0xE8, (uint8_t[]){0x05}, 1, 0},
        {0xE9, (uint8_t[]){0x82}, 1, 0},
        {0xEA, (uint8_t[]){0xDF}, 1, 0},
        {0xEB, (uint8_t[]){0x89}, 1, 0},
        {0xEC, (uint8_t[]){0x20}, 1, 0},
        {0xED, (uint8_t[]){0x14}, 1, 0},
        {0xEE, (uint8_t[]){0xFF}, 1, 0},
        {0xEF, (uint8_t[]){0x00}, 1, 0},
        {0xF8, (uint8_t[]){0xFF}, 1, 0},
        {0xF9, (uint8_t[]){0x00}, 1, 0},
        {0xFA, (uint8_t[]){0x00}, 1, 0},
        {0xFB, (uint8_t[]){0x30}, 1, 0},
        {0xFC, (uint8_t[]){0x00}, 1, 0},
        {0xFD, (uint8_t[]){0x00}, 1, 0},
        {0xFE, (uint8_t[]){0x00}, 1, 0},
        {0xFF, (uint8_t[]){0x00}, 1, 0},
        {0x60, (uint8_t[]){0x42}, 1, 0},
        {0x61, (uint8_t[]){0xE0}, 1, 0},
        {0x62, (uint8_t[]){0x40}, 1, 0},
        {0x63, (uint8_t[]){0x40}, 1, 0},
        {0x64, (uint8_t[]){0x02}, 1, 0},
        {0x65, (uint8_t[]){0x00}, 1, 0},
        {0x66, (uint8_t[]){0x40}, 1, 0},
        {0x67, (uint8_t[]){0x03}, 1, 0},
        {0x68, (uint8_t[]){0x00}, 1, 0},
        {0x69, (uint8_t[]){0x00}, 1, 0},
        {0x6A, (uint8_t[]){0x00}, 1, 0},
        {0x6B, (uint8_t[]){0x00}, 1, 0},
        {0x70, (uint8_t[]){0x42}, 1, 0},
        {0x71, (uint8_t[]){0xE0}, 1, 0},
        {0x72, (uint8_t[]){0x40}, 1, 0},
        {0x73, (uint8_t[]){0x40}, 1, 0},
        {0x74, (uint8_t[]){0x02}, 1, 0},
        {0x75, (uint8_t[]){0x00}, 1, 0},
        {0x76, (uint8_t[]){0x40}, 1, 0},
        {0x77, (uint8_t[]){0x03}, 1, 0},
        {0x78, (uint8_t[]){0x00}, 1, 0},
        {0x79, (uint8_t[]){0x00}, 1, 0},
        {0x7A, (uint8_t[]){0x00}, 1, 0},
        {0x7B, (uint8_t[]){0x00}, 1, 0},
        {0x80, (uint8_t[]){0x38}, 1, 0},
        {0x81, (uint8_t[]){0x00}, 1, 0},
        {0x82, (uint8_t[]){0x04}, 1, 0},
        {0x83, (uint8_t[]){0x02}, 1, 0},
        {0x84, (uint8_t[]){0xDC}, 1, 0},
        {0x85, (uint8_t[]){0x00}, 1, 0},
        {0x86, (uint8_t[]){0x00}, 1, 0},
        {0x87, (uint8_t[]){0x00}, 1, 0},
        {0x88, (uint8_t[]){0x38}, 1, 0},
        {0x89, (uint8_t[]){0x00}, 1, 0},
        {0x8A, (uint8_t[]){0x06}, 1, 0},
        {0x8B, (uint8_t[]){0x02}, 1, 0},
        {0x8C, (uint8_t[]){0xDE}, 1, 0},
        {0x8D, (uint8_t[]){0x00}, 1, 0},
        {0x8E, (uint8_t[]){0x00}, 1, 0},
        {0x8F, (uint8_t[]){0x00}, 1, 0},
        {0x90, (uint8_t[]){0x38}, 1, 0},
        {0x91, (uint8_t[]){0x00}, 1, 0},
        {0x92, (uint8_t[]){0x08}, 1, 0},
        {0x93, (uint8_t[]){0x02}, 1, 0},
        {0x94, (uint8_t[]){0xE0}, 1, 0},
        {0x95, (uint8_t[]){0x00}, 1, 0},
        {0x96, (uint8_t[]){0x00}, 1, 0},
        {0x97, (uint8_t[]){0x00}, 1, 0},
        {0x98, (uint8_t[]){0x38}, 1, 0},
        {0x99, (uint8_t[]){0x00}, 1, 0},
        {0x9A, (uint8_t[]){0x0A}, 1, 0},
        {0x9B, (uint8_t[]){0x02}, 1, 0},
        {0x9C, (uint8_t[]){0xE2}, 1, 0},
        {0x9D, (uint8_t[]){0x00}, 1, 0},
        {0x9E, (uint8_t[]){0x00}, 1, 0},
        {0x9F, (uint8_t[]){0x00}, 1, 0},
        {0xA0, (uint8_t[]){0x38}, 1, 0},
        {0xA1, (uint8_t[]){0x00}, 1, 0},
        {0xA2, (uint8_t[]){0x03}, 1, 0},
        {0xA3, (uint8_t[]){0x02}, 1, 0},
        {0xA4, (uint8_t[]){0xDB}, 1, 0},
        {0xA5, (uint8_t[]){0x00}, 1, 1},
        {0xA6, (uint8_t[]){0x00}, 1, 0},
        {0xA7, (uint8_t[]){0x00}, 1, 0},
        {0xA8, (uint8_t[]){0x38}, 1, 0},
        {0xA9, (uint8_t[]){0x00}, 1, 0},
        {0xAA, (uint8_t[]){0x05}, 1, 0},
        {0xAB, (uint8_t[]){0x02}, 1, 0},
        {0xAC, (uint8_t[]){0xDD}, 1, 0},
        {0xAD, (uint8_t[]){0x00}, 1, 0},
        {0xAE, (uint8_t[]){0x00}, 1, 0},
        {0xAF, (uint8_t[]){0x00}, 1, 0},
        {0xB0, (uint8_t[]){0x38}, 1, 0},
        {0xB1, (uint8_t[]){0x00}, 1, 0},
        {0xB2, (uint8_t[]){0x07}, 1, 0},
        {0xB3, (uint8_t[]){0x02}, 1, 0},
        {0xB4, (uint8_t[]){0xDF}, 1, 0},
        {0xB5, (uint8_t[]){0x00}, 1, 0},
        {0xB6, (uint8_t[]){0x00}, 1, 0},
        {0xB7, (uint8_t[]){0x00}, 1, 0},
        {0xB8, (uint8_t[]){0x38}, 1, 0},
        {0xB9, (uint8_t[]){0x00}, 1, 0},
        {0xBA, (uint8_t[]){0x09}, 1, 0},
        {0xBB, (uint8_t[]){0x02}, 1, 0},
        {0xBC, (uint8_t[]){0xE1}, 1, 0},
        {0xBD, (uint8_t[]){0x00}, 1, 0},
        {0xBE, (uint8_t[]){0x00}, 1, 0},
        {0xBF, (uint8_t[]){0x00}, 1, 0},
        {0xC0, (uint8_t[]){0x22}, 1, 0},
        {0xC1, (uint8_t[]){0xAA}, 1, 0},
        {0xC2, (uint8_t[]){0x65}, 1, 0},
        {0xC3, (uint8_t[]){0x74}, 1, 0},
        {0xC4, (uint8_t[]){0x47}, 1, 0},
        {0xC5, (uint8_t[]){0x56}, 1, 0},
        {0xC6, (uint8_t[]){0x00}, 1, 0},
        {0xC7, (uint8_t[]){0x88}, 1, 0},
        {0xC8, (uint8_t[]){0x99}, 1, 0},
        {0xC9, (uint8_t[]){0x33}, 1, 0},
        {0xD0, (uint8_t[]){0x11}, 1, 0},
        {0xD1, (uint8_t[]){0xAA}, 1, 0},
        {0xD2, (uint8_t[]){0x65}, 1, 0},
        {0xD3, (uint8_t[]){0x74}, 1, 0},
        {0xD4, (uint8_t[]){0x47}, 1, 0},
        {0xD5, (uint8_t[]){0x56}, 1, 0},
        {0xD6, (uint8_t[]){0x00}, 1, 0},
        {0xD7, (uint8_t[]){0x88}, 1, 0},
        {0xD8, (uint8_t[]){0x99}, 1, 0},
        {0xD9, (uint8_t[]){0x33}, 1, 0},
        {0xF3, (uint8_t[]){0x01}, 1, 0},
        {0xF0, (uint8_t[]){0x00}, 1, 0},
        {0x21, (uint8_t[]){0x00}, 1, 0},
        {0x11, (uint8_t[]){0x00}, 1, 120},
        {0x29, (uint8_t[]){0x00}, 1, 0}};

static bool on_color_trans_done_cb(
    esp_lcd_panel_io_handle_t panel_io,
    esp_lcd_panel_io_event_data_t *edata,
    void *user_ctx
)
{
    lvgl_esp32_Display_obj_t *self = (lvgl_esp32_Display_obj_t *) user_ctx;

    if (self->transfer_done_cb != NULL)
    {
        self->transfer_done_cb(self->transfer_done_user_data);
    }

    return false;
}

void lvgl_esp32_Display_draw_bitmap(
    lvgl_esp32_Display_obj_t *self,
    int x_start,
    int y_start,
    int x_end,
    int y_end,
    const void *data
)
{
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(self->panel, x_start, y_start, x_end, y_end, data));
}
static void  brightness(lvgl_esp32_Display_obj_t *self,int percent){
    uint32_t duty_cycle = (BIT(self->ledc_timer.duty_resolution) * percent) / 100;
    ledc_channel_t channel = self->ledc_channel.channel;
    ledc_mode_t mode = self->ledc_channel.speed_mode;
    // 设置占空比
    ESP_ERROR_CHECK(ledc_set_duty(mode, channel, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(mode, channel));
}

static mp_obj_t lvgl_esp32_Display_mirrorX(mp_obj_t self_ptr,mp_obj_t en)
{
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_PANEL_CHECK_ERR_RET(esp_lcd_panel_mirror(self->panel, mp_obj_is_true(en), self->mirror_x), false, "Mirror X failed");
    self->mirror_x = mp_obj_is_true(en);

    return mp_const_none;
}

static mp_obj_t lvgl_esp32_Display_mirrorY(mp_obj_t self_ptr,mp_obj_t  en)
{
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_PANEL_CHECK_ERR_RET(esp_lcd_panel_mirror(self->panel, self->mirror_y, mp_obj_is_true(en)), false, "Mirror X failed");
    self->mirror_y = mp_obj_is_true(en);

    return mp_const_none;
}

static mp_obj_t lvgl_esp32_Display_swapXY(mp_obj_t self_ptr,mp_obj_t  en)
{
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_PANEL_CHECK_ERR_RET(esp_lcd_panel_swap_xy(self->panel, mp_obj_is_true(en)), false, "Swap XY failed");
    self->swap_xy = mp_obj_is_true(en);

    return mp_const_none;
}

static mp_obj_t lvgl_esp32_Display_brightness(mp_obj_t self_ptr,mp_obj_t percent){
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);
    int val=mp_obj_get_int(percent);
    brightness(self,val);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(lvgl_esp32_Display_brightness_obj, lvgl_esp32_Display_brightness);
static MP_DEFINE_CONST_FUN_OBJ_2(lvgl_esp32_Display_swapXY_obj, lvgl_esp32_Display_swapXY);
static MP_DEFINE_CONST_FUN_OBJ_2(lvgl_esp32_Display_mirrorX_obj, lvgl_esp32_Display_mirrorX);
static MP_DEFINE_CONST_FUN_OBJ_2(lvgl_esp32_Display_mirrorY_obj, lvgl_esp32_Display_mirrorY);

static void clear(lvgl_esp32_Display_obj_t *self)
{
    ESP_LOGI(TAG, "Clearing screen");

    // Create a temporary empty buffer of only one line of pixels so this will also work on memory-constrained devices
    size_t buf_size = self->width;
    uint8_t *buf = heap_caps_calloc(1, buf_size * 18, MALLOC_CAP_DMA);

    assert(buf);

    // Blit lines to the screen
    for (int line = 0; line < self->height; line++)
    {
        lvgl_esp32_Display_draw_bitmap(self, 0, line, self->width, line + 1, buf);
    }

    // Release the buffer
    heap_caps_free(buf);
}

static mp_obj_t lvgl_esp32_Display_init(mp_obj_t self_ptr)
{
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_LOGI(TAG, "Setting up panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = self->cs,
        .dc_gpio_num=-1,
        .pclk_hz = self->pixel_clock,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = on_color_trans_done_cb,
        .user_ctx = self,
        .flags = {
           .quad_mode=1
        }
    };
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_spi(
            (esp_lcd_spi_bus_handle_t) self->spi->spi_host_device,
            &io_config,
            &self->io_handle
        )
    );

    // HACK
    self->spi->device_count++;

    ESP_LOGI(TAG, "Setting up ST77916 panel driver");
    st77916_vendor_config_t vendor_config = {
             .init_cmds = lcd_init_cmds,         // Uncomment these line if use custom initialization commands
             .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st77916_lcd_init_cmd_t),

            .flags = {
                    .mirror_by_cmd=1,
                    .use_qspi_interface = 1,
            },
    };
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = self->reset,
        .rgb_ele_order = self->bgr ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 18,
        .vendor_config = &vendor_config
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st77916(self->io_handle, &panel_config, &self->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(self->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(self->panel));

    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(self->panel, self->invert));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(self->panel, self->swap_xy));
	ESP_ERROR_CHECK(esp_lcd_panel_mirror(self->panel, self->mirror_x, self->mirror_y));

    clear(self);

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(self->panel, true));

    // 配置LEDC（LED控制器）用于PWM
     ledc_channel_config_t channel_config={
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = self->blk,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE
    };
    self->ledc_channel =channel_config;
    ledc_timer_config_t timer_config={
            .duty_resolution = LEDC_TIMER_13_BIT, // PWM占空比分辨率
            .freq_hz         = 5000,              // PWM频率
            .speed_mode      = LEDC_LOW_SPEED_MODE,
            .timer_num       = LEDC_TIMER_0,
            .clk_cfg = LEDC_AUTO_CLK
    };
    self->ledc_timer =timer_config;
    ledc_timer_config(&self->ledc_timer);
    ledc_channel_config(&self->ledc_channel);

    // 设置初始背光亮度
    brightness(self,0);
    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_Display_init_obj, lvgl_esp32_Display_init);

static mp_obj_t lvgl_esp32_Display_deinit(mp_obj_t self_ptr)
{
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    if(self->panel != NULL)
    {
        ESP_LOGI(TAG, "Deinitializing ST7789 panel driver");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(self->panel, false));
        ESP_ERROR_CHECK(esp_lcd_panel_del(self->panel));
        self->panel = NULL;
    }

    if(self->io_handle != NULL)
    {
        ESP_LOGI(TAG, "Deinitializing panel IO");
        ESP_ERROR_CHECK(esp_lcd_panel_io_del(self->io_handle));
        self->io_handle = NULL;

        // HACK
        self->spi->device_count--;

        // We call deinit on spi in case it was (unsuccessfully) deleted earlier
        lvgl_esp32_QSPI_internal_deinit(self->spi);
    }

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_Display_deinit_obj, lvgl_esp32_Display_deinit);

static mp_obj_t lvgl_esp32_Display_make_new(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *all_args
)
{
    enum
    {
        ARG_width,          // width of the display
        ARG_height,         // height of the display
        ARG_spi,            // configured SPI instance
        ARG_reset,          // RESET pin number
        ARG_cs,             // CS pin number
        ARG_blk,             // BLK pin number
        ARG_pixel_clock,    // Pixel clock in Hz
        ARG_swap_xy,        // swap X and Y axis
        ARG_mirror_x,       // mirror on X axis
        ARG_mirror_y,       // mirror on Y axis
        ARG_invert,         // invert colors
        ARG_bgr,            // use BGR element order
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_spi, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_reset, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_cs, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_blk, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_pixel_clock, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 20 * 1000 * 1000 }},
        { MP_QSTR_swap_xy, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_mirror_x, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_mirror_y, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_invert, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_bgr, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    lvgl_esp32_Display_obj_t *self = mp_obj_malloc_with_finaliser(lvgl_esp32_Display_obj_t, &lvgl_esp32_Display_type);

    self->width = args[ARG_width].u_int;
    self->height = args[ARG_height].u_int;

    self->spi = (lvgl_esp32_QSPI_obj_t *) MP_OBJ_TO_PTR(args[ARG_spi].u_obj);
    self->reset = args[ARG_reset].u_int;
    self->cs = args[ARG_cs].u_int;
    self->blk = args[ARG_blk].u_int;
    self->pixel_clock = args[ARG_pixel_clock].u_int;

    self->swap_xy = args[ARG_swap_xy].u_bool;
    self->mirror_x = args[ARG_mirror_x].u_bool;
    self->mirror_y = args[ARG_mirror_y].u_bool;
    self->invert = args[ARG_invert].u_bool;
    self->bgr = args[ARG_bgr].u_bool;

    self->transfer_done_cb = NULL;
    self->transfer_done_user_data = NULL;

    self->panel = NULL;
    self->io_handle = NULL;

    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t lvgl_esp32_Display_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&lvgl_esp32_Display_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&lvgl_esp32_Display_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&lvgl_esp32_Display_deinit_obj) },
    {MP_ROM_QSTR(MP_QSTR_brightness),MP_ROM_PTR(&lvgl_esp32_Display_brightness_obj)}
};

static MP_DEFINE_CONST_DICT(lvgl_esp32_Display_locals, lvgl_esp32_Display_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    lvgl_esp32_Display_type,
    MP_QSTR_Display,
    MP_TYPE_FLAG_NONE,
    make_new,
    lvgl_esp32_Display_make_new,
    locals_dict,
    &lvgl_esp32_Display_locals
);
