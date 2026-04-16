#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

#define AUDIO_I2S_GPIO_MCLK  GPIO_NUM_14
#define AUDIO_I2S_GPIO_WS    GPIO_NUM_38
#define AUDIO_I2S_GPIO_BCLK  GPIO_NUM_15
#define AUDIO_I2S_GPIO_DIN   GPIO_NUM_16
#define AUDIO_I2S_GPIO_DOUT  GPIO_NUM_45

#define AUDIO_CODEC_PA_PIN       GPIO_NUM_46
#define AUDIO_CODEC_I2C_SDA_PIN  GPIO_NUM_47
#define AUDIO_CODEC_I2C_SCL_PIN  GPIO_NUM_48
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR

#define BOOT_BUTTON_GPIO        GPIO_NUM_0


#define TODO_UP_BUTTON_GPIO     GPIO_NUM_39
#define TODO_DOWN_BUTTON_GPIO   GPIO_NUM_18
//开机电源键与下键复用
#define VBAT_PWR_GPIO           GPIO_NUM_18
#define TODO_CONFIRM_BUTTON_GPIO GPIO_NUM_0
#define CHARGE_DETECT_GPIO      GPIO_NUM_2
#define CHARGE_FULL_GPIO        GPIO_NUM_1
// CHARGE_DETECT charging level definition: 0 means low=charging, 1 means high=charging.
#define CHARGE_DETECT_CHARGING_LEVEL 0
// 1: charging GPIO blocks sleep; 0: ignore charging GPIO for sleep decision.
#define CHARGE_GPIO_AFFECT_SLEEP 1

// RTC (PCF8563T/5)
#define RTC_INT_GPIO            GPIO_NUM_5
#define RTC_I2C_ADDR            0x51

// NFC (GT23SC6699)
#define NFC_I2C_ADDR            0x55
#define NFC_FD_GPIO             GPIO_NUM_7
#define NFC_PWR_GPIO            GPIO_NUM_21
#define NFC_FD_ACTIVE_LEVEL     0
/*EPD port Init*/
#define EPD_SPI_NUM        SPI3_HOST

#define EPD_DC_PIN    GPIO_NUM_10
#define EPD_CS_PIN    GPIO_NUM_11
#define EPD_SCK_PIN   GPIO_NUM_12
#define EPD_MOSI_PIN  GPIO_NUM_13
#define EPD_RST_PIN   GPIO_NUM_9
#define EPD_BUSY_PIN  GPIO_NUM_8

#define EXAMPLE_LCD_WIDTH   400
#define EXAMPLE_LCD_HEIGHT  300

/*DEV POWER init*/
#define EPD_PWR_PIN     GPIO_NUM_6
#define Audio_PWR_PIN   GPIO_NUM_42
#define AUDIO_PWR_FORCE_LEVEL 1
#define Audio_AMP_PIN   GPIO_NUM_46
#define VBAT_PWR_PIN    GPIO_NUM_17
 
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY  false

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0



#endif // _BOARD_CONFIG_H_
