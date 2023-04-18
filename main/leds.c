#include "leds.h"

// Public vars
//-------------//

// Variable to hold current color data
rgb_s led_colors[CONFIG_NP_RGB_COUNT]       = {0};

// Current LED task status
led_anim_status_t led_anim_status = LA_STATUS_IDLE;

// Current mode color(s)
rgb_s mode_color    = {0};


// Store charge mode color
rgb_s charge_color  = COLOR_GREEN;


// Color preset defs
// preset order: right, down, up, left, start, select, dpad x4
rgb_s COLOR_PRESET_SFC[10] = {{.rgb = 0xC1121C}, {.rgb = 0xF7BA0B}, {.rgb = 0x00387b}, {.rgb = 0x007243},
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, 
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}};

rgb_s COLOR_PRESET_XBOX[10] = {{.rgb = 0xC1121C}, {.rgb = 0x007243}, {.rgb = 0xF7BA0B}, {.rgb = 0x00387b},
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, 
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}};

rgb_s COLOR_PRESET_SWITCH[10] = {{.rgb = 0xC1121C}, {.rgb = 0xF7BA0B}, {.rgb = 0x00387b}, {.rgb = 0x007243},
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, 
                              {.rgb = 0xC1121C}, {.rgb = 0xF7BA0B}, {.rgb = 0x00387b}, {.rgb = 0x007243}};

rgb_s COLOR_PRESET_DOLPHIN[10] = {{.rgb = 0x3ac9af}, {.rgb = 0xe63939}, {.rgb = 0x54585a}, {.rgb = 0x54585a},
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, 
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}};

rgb_s COLOR_PRESET_REALITY[10] = {{.rgb = 0x040fb4}, {.rgb = 0x008f2b}, {.rgb = 0xf5d400}, {.rgb = 0xf5d400},
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, 
                              {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}, {.rgb = 0x54585a}};

rgb_s *mode_color_array_ptr = COLOR_PRESET_SFC;
//-------------//
//-------------//

// Private vars
//-------------//

TaskHandle_t    _led_taskHandle;
QueueHandle_t   _led_xQueue;
// Use to store information passed through events
led_msg_s   _led_msg = {0};
rgb_s       _msg_colors[CONFIG_NP_RGB_COUNT] = {0};

// Store previous colors
rgb_s _previous_colors[CONFIG_NP_RGB_COUNT]  = {0};

//-------------//
//-------------//

// LED boot animation
void boot_anim()
{
    int back_forth = 0;
    bool toggle = false;
    bool colorflip = false;
    uint8_t color_idx = 0;
    uint8_t color_last_idx = 0;
    rgb_s colors[6] = {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE, COLOR_PURPLE};
    for(int i = 0; i < 36; i++)
    {
        memset(led_colors, 0x00, sizeof(led_colors));
        led_colors[back_forth] = colors[color_idx];
        
        if (!toggle)
        {
            if (back_forth > 0)
            {
                led_colors[back_forth-1] = colors[color_last_idx];
                color_last_idx = color_idx;
            }
            back_forth += 1;
            if (back_forth == CONFIG_NP_RGB_COUNT)
            {
                toggle = true;
                back_forth = CONFIG_NP_RGB_COUNT-1;
            }
        }
        else
        {
            if (back_forth < CONFIG_NP_RGB_COUNT-1)
            {
                led_colors[back_forth+1] = colors[color_last_idx];
                color_last_idx = color_idx;
            }
            back_forth -= 1;
            if (back_forth == -1)
            {
                toggle = false;
                back_forth = 0;
            }
        }

        if (!colorflip)
        {
            if (color_idx + 1 > 5)
            {
                colorflip = true;
                color_idx = 5;
            }
            else
            {
                color_idx += 1;
            }
        }
        else
        {
            if (color_idx - 1 < 0)
            {
                colorflip = false;
                color_idx = 0;
            }
            else
            {
                color_idx -= 1;
            }
        }

        rgb_show();
        vTaskDelay(35/portTICK_PERIOD_MS);
    }

}

void led_set_all(rgb_s * led_array, uint32_t color)
{
    for (uint8_t i = 0; i < CONFIG_NP_RGB_COUNT; i++)
    {
        led_array[i].rgb = color;
    }
}

void led_array_fade(rgb_s * blend_to, uint8_t speed)
{
    uint8_t fader = 0;
    uint8_t _speed = 10;
    if (speed != NULL)
    {
        _speed = speed;
    }
    
    memcpy(_previous_colors, led_colors, sizeof(led_colors));

    while(fader < 30)
    {
        uint8_t t = 0;
        if ((8 * fader) > 255)
        {
            t = 255;
        }
        else
        {
            t = (8 * fader);
        }
        for(uint8_t i = 0; i < CONFIG_NP_RGB_COUNT; i++)
        {
            rgb_blend(&led_colors[i], _previous_colors[i], blend_to[i], t);
        }
        rgb_show();
        fader += 1;
        vTaskDelay(_speed/portTICK_PERIOD_MS);
    }
    memcpy(led_colors, blend_to, sizeof(led_colors));
    rgb_show();
}

// Blinks to a color, then back to the original color.
void led_blink(rgb_s * led_array, uint8_t speed)
{
    uint8_t _speed = 10;
    if (speed != NULL)
    {
        _speed = speed;
    }

    rgb_s _origin_colors[CONFIG_NP_RGB_COUNT] = {0};
    memcpy(_origin_colors, led_colors, sizeof(led_colors));

    led_array_fade(led_array, _speed);
    led_array_fade(_origin_colors, _speed);
}   

void led_animator_task(void * params)
{
    led_msg_s _received_led_msg = {0};

    uint32_t    _blink_speed = 10;
    // Used to determine if blink should keep repeating.
    bool        _blinking_en = false;

    _led_xQueue = xQueueCreate(4, sizeof(led_msg_s));
    if (_led_xQueue != 0)
    {
        led_anim_status = LA_STATUS_READY;
    }

    for(;;)
    {
        // Check if we are supposed to be blinking color
        if (_blinking_en)
        {
            led_blink(_msg_colors, _blink_speed);
        }
        // If a message is received
        if (xQueueReceive(_led_xQueue, &(_received_led_msg), (TickType_t) 0))
        {
            // Disable blinking until we interpret message.

            _blinking_en = false;

            if (_received_led_msg.single)
            {
                led_set_all(_msg_colors, _received_led_msg.color_hex);
            }
            else
            {
                memcpy(_msg_colors, _received_led_msg.color_ptr, sizeof(_msg_colors));
            }

            switch(_received_led_msg.anim_type)
            {
                default:
                case LEDANIM_FADETO:
                    _blink_speed = 10;
                    led_array_fade(_msg_colors, _blink_speed);
                    break;

                case LEDANIM_BLINK_SLOW:
                    // Here we need to blink TO the desired color
                    // not before we store our original color
                    _blink_speed = 45;
                    _blinking_en = true;
                    led_blink(_msg_colors, _blink_speed);
                    break;

                case LEDANIM_BLINK_FAST:
                    _blink_speed = 10;
                    _blinking_en = true;
                    led_blink(_msg_colors, _blink_speed);
                    break;
            }
        }
        else if (!_blinking_en)
        {
            // Offload for 350
            vTaskDelay(150/portTICK_PERIOD_MS);
        }
    }
}

void led_animator_init(void)
{
    neopixel_init(led_colors, VSPI_HOST);
    rgb_setbrightness(75);

    xTaskCreatePinnedToCore(led_animator_task, "LED Animator", 2048, NULL, 3, &_led_taskHandle, HOJA_CORE_CPU);
}

// Animate to a single color
void led_animator_single(led_anim_t anim_type, rgb_s rgb_color)
{
    if (led_anim_status == LA_STATUS_IDLE)
    {
        return;
    }

    _led_msg.single = true;
    _led_msg.color_hex = rgb_color.rgb;
    _led_msg.anim_type = anim_type;

    if (_led_xQueue != 0)
    {
        xQueueSend(_led_xQueue, &_led_msg, (TickType_t) 0);
    }
}

// Animate to array of colors
void led_animator_array(led_anim_t anim_type, rgb_s * rgb_color_array)
{
    if (led_anim_status == LA_STATUS_IDLE)
    {
        return;
    }

    _led_msg.single = false;
    _led_msg.color_ptr = rgb_color_array;
    _led_msg.anim_type = anim_type;

    if (_led_xQueue != 0)
    {
        xQueueSend(_led_xQueue, &_led_msg, (TickType_t) 0);
    }
}
