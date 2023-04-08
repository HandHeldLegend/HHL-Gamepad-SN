#ifndef LEDS_H
#define LEDS_H

#include "sdkconfig.h"
#include "esp32_neopixel.h"
#include "hoja_includes.h"

extern rgb_s led_colors[CONFIG_NP_RGB_COUNT];
extern rgb_s mode_color;
extern rgb_s *mode_color_array_ptr;
extern rgb_s charge_color;

// color presets
extern rgb_s COLOR_PRESET_SFC[10];
extern rgb_s COLOR_PRESET_XBOX[10];
extern rgb_s COLOR_PRESET_SWITCH[10];
extern rgb_s COLOR_PRESET_DOLPHIN[10];

typedef enum
{
    // Fade to a color once
    LEDANIM_FADETO,
    // Sync breathing fader
    LEDANIM_BLINK_FAST,
    // Battery breathing fader
    LEDANIM_BLINK_SLOW,
} led_anim_t;

typedef enum
{
    LA_STATUS_IDLE,
    LA_STATUS_READY,
} led_anim_status_t;

typedef struct
{
    led_anim_t  anim_type;
    bool        single;
    uint32_t    color_hex;
    rgb_s       *color_ptr;
} led_msg_s;

extern led_anim_status_t led_anim_status;

void boot_anim(void);

void led_animator_init(void);

void led_animator_single(led_anim_t anim_type, rgb_s rgb_color);
void led_animator_array(led_anim_t anim_type, rgb_s * rgb_color_array);

#endif