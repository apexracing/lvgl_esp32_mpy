#include "qspi.h"

#include "py/runtime.h"

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "lvgl_esp32_QSPI";

static mp_obj_t lvgl_esp32_QSPI_init(mp_obj_t self_ptr)
{
    struct lvgl_esp32_QSPI_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_LOGI(TAG, "Initializing QSPI Bus");
    spi_bus_config_t bus_config = {
        .sclk_io_num = self->sck,
        .data0_io_num = self->data0,
        .data1_io_num = self->data1,
        .data2_io_num = self->data2,
        .data3_io_num = self->data3,
        .max_transfer_sz = self->max_transfer_s
    };

    ESP_ERROR_CHECK(spi_bus_initialize(self->spi_host_device, &bus_config, SPI_DMA_CH_AUTO));
    self->bus_initialized = true;

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_QSPI_init_obj, lvgl_esp32_QSPI_init);

static mp_obj_t lvgl_esp32_QSPI_deinit(mp_obj_t self_ptr)
{
    struct lvgl_esp32_QSPI_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    if (self->bus_initialized)
    {
        ESP_LOGI(TAG, "Deinitializing QSPI Bus");

        // HACK
        if (self->device_count > 0)
        {
            ESP_LOGW(TAG, "Could not deinitialize QSPI Bus (yet), still active devices");
            self->needs_deinit = true;
            return mp_obj_new_int_from_uint(0);
        }

        esp_err_t result = spi_bus_free(self->spi_host_device);

        if (result == ESP_ERR_INVALID_STATE)
        {
            // We can not predict the order in which MicroPython destroys objects, so we allow deinit to be called
            // multiple times from the bus users as they deinit() too.
            ESP_LOGW(TAG, "Could not deinitialize QSPI Bus (yet), still active devices");
            self->needs_deinit = true;
        }
        else
        {
            ESP_ERROR_CHECK(result);
            self->bus_initialized = false;
            self->needs_deinit = false;
        }
    }

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_QSPI_deinit_obj, lvgl_esp32_QSPI_deinit);

void lvgl_esp32_QSPI_internal_deinit(lvgl_esp32_QSPI_obj_t *self)
{
    if (self->needs_deinit)
    {
        lvgl_esp32_QSPI_deinit(self);
    }
}

static mp_obj_t lvgl_esp32_QSPI_make_new(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *all_args
)
{
    enum
    {
        ARG_spi_id,         // ID of SPI to use
        ARG_baudrate,       // Baudrate
        ARG_sck,            // SCK pin
        ARG_data0,           // data0 pin
        ARG_data1,           // data1 pin
        ARG_data2,           // data0 pin
        ARG_data3,           // data1 pin
        ARG_max_transfer_s,
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi_id, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_baudrate, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_sck, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_data0, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_data1, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_data2, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_data3, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_max_transfer_s, MP_ARG_INT | MP_ARG_REQUIRED },

    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    struct lvgl_esp32_QSPI_obj_t *self = mp_obj_malloc_with_finaliser(lvgl_esp32_QSPI_obj_t, &lvgl_esp32_QSPI_type);

    switch (args[ARG_spi_id].u_int)
    {
        case 1:
            self->spi_host_device = SPI1_HOST;
            break;
        case 2:
            self->spi_host_device = SPI2_HOST;
            break;
        case 3:
            self->spi_host_device = SPI3_HOST;
            break;
    }
    self->baudrate = args[ARG_baudrate].u_int;

    self->sck = args[ARG_sck].u_int;
    self->data0 = args[ARG_data0].u_int;
    self->data1 = args[ARG_data1].u_int;
    self->data2 = args[ARG_data2].u_int;
    self->data3 = args[ARG_data3].u_int;
    self->max_transfer_s = args[ARG_max_transfer_s].u_int;
    self->bus_initialized = false;
    self->needs_deinit = false;
    self->device_count = 0;

    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t lvgl_esp32_QSPI_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&lvgl_esp32_QSPI_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&lvgl_esp32_QSPI_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&lvgl_esp32_QSPI_deinit_obj) },
};

static MP_DEFINE_CONST_DICT(lvgl_esp32_QSPI_locals, lvgl_esp32_QSPI_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    lvgl_esp32_QSPI_type,
    MP_QSTR_QSPI,
    MP_TYPE_FLAG_NONE,
    make_new,
    lvgl_esp32_QSPI_make_new,
    locals_dict,
    &lvgl_esp32_QSPI_locals
);
