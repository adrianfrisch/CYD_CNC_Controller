#pragma once
// =============================================================================
// LovyanGFX Configuration — ESP32-8048S070C (7.0" 800x480 RGB parallel)
// =============================================================================

#ifdef USE_LOVYANGFX

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_RGB _panel_instance;
    lgfx::Bus_RGB _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX(void) {
        // =====================================================================
        // Bus configuration (16-bit RGB parallel)
        // =====================================================================
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;

            cfg.pin_d0  = GPIO_NUM_6;   // B0
            cfg.pin_d1  = GPIO_NUM_7;   // B1
            cfg.pin_d2  = GPIO_NUM_15;  // B2
            cfg.pin_d3  = GPIO_NUM_16;  // B3
            cfg.pin_d4  = GPIO_NUM_4;   // B4
            cfg.pin_d5  = GPIO_NUM_8;   // G0
            cfg.pin_d6  = GPIO_NUM_3;   // G1
            cfg.pin_d7  = GPIO_NUM_46;  // G2
            cfg.pin_d8  = GPIO_NUM_9;   // G3
            cfg.pin_d9  = GPIO_NUM_1;   // G4
            cfg.pin_d10 = GPIO_NUM_5;   // G5
            cfg.pin_d11 = GPIO_NUM_45;  // R0
            cfg.pin_d12 = GPIO_NUM_48;  // R1
            cfg.pin_d13 = GPIO_NUM_47;  // R2
            cfg.pin_d14 = GPIO_NUM_21;  // R3
            cfg.pin_d15 = GPIO_NUM_14;  // R4

            cfg.pin_henable = GPIO_NUM_41;  // DE
            cfg.pin_vsync   = GPIO_NUM_40;  // VSYNC
            cfg.pin_hsync   = GPIO_NUM_39;  // HSYNC
            cfg.pin_pclk    = GPIO_NUM_42;  // PCLK

            cfg.freq_write = 16000000;  // Pixel clock (16MHz)

            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 40;
            cfg.hsync_pulse_width = 48;
            cfg.hsync_back_porch  = 40;

            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 1;
            cfg.vsync_pulse_width = 1;
            cfg.vsync_back_porch  = 13;

            cfg.pclk_active_neg = 1;
            cfg.de_idle_high    = 0;
            cfg.pclk_idle_high  = 0;

            _bus_instance.config(cfg);
        }

        // =====================================================================
        // Panel configuration
        // =====================================================================
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = 800;
            cfg.memory_height = 480;
            cfg.panel_width   = 800;
            cfg.panel_height  = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;

            _panel_instance.config(cfg);
        }

        _panel_instance.setBus(&_bus_instance);
        // Use PSRAM for framebuffer (default use_psram=2 means PSRAM only)
        // Reduce chunk size for large panels to avoid DMA issues
        {
            auto cfg = _panel_instance.config_detail();
            cfg.use_psram = 2;
            _panel_instance.config_detail(cfg);
        }

        // =====================================================================
        // Backlight configuration
        // =====================================================================
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = GPIO_NUM_2;
            cfg.invert = false;
            cfg.freq   = 44100;
            cfg.pwm_channel = 7;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

#endif // USE_LOVYANGFX

