#pragma once

#include <stdint.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// From docs/zectrix_fonts/demo.html (IcoMoon)

// Battery icons (icon-b*)
#define FONT_ZECTRIX_BATTERY_EMPTY "\xee\xa4\x85"      // icon-b1 (e905)
#define FONT_ZECTRIX_BATTERY_25     "\xee\xa4\x82"         // icon-b25 (e902)
#define FONT_ZECTRIX_BATTERY_50     "\xee\xa4\x83"         // icon-b50 (e903)
#define FONT_ZECTRIX_BATTERY_75     "\xee\xa4\x84"         // icon-b75 (e904)
#define FONT_ZECTRIX_BATTERY_FULL   "\xee\xa4\x81"       // icon-b2 (e901)
#define FONT_ZECTRIX_BATTERY_CHARGING "\xee\xa4\x8c"     // icon-charging (e90c)

// Wi-Fi icons (icon-Wi-Fi-*)
#define FONT_ZECTRIX_WIFI_FULL "\xee\xa4\x8e"             // icon-Wi-Fi-1 (e90e)
#define FONT_ZECTRIX_WIFI_FAIR "\xee\xa4\x8f"             // icon-Wi-Fi-2 (e90f)
#define FONT_ZECTRIX_WIFI_WEAK "\xee\xa4\x90"             // icon-Wi-Fi-3 (e910)
#define FONT_ZECTRIX_WIFI_SLASH "\xee\xa4\x86"            // icon-w3 (e906)


// Status / UI icons
#define FONT_ZECTRIX_ICON_MUTE "\xee\xa4\x8b"          // icon-mute (e90b)
#define FONT_ZECTRIX_ICON_SPEAKER "\xee\xa4\x89"          // icon-speaker (e909)
#define FONT_ZECTRIX_ICON_CHECKBOX "\xee\xa4\x91"      // icon-checkbox (e911)
#define FONT_ZECTRIX_ICON_CHECKBOX_OK "\xee\xa4\x8a"   // icon-checkboxok (e90a)
#define FONT_ZECTRIX_ICON_MIC "\xee\xa4\x80"           // icon-mic (e900)
#define FONT_ZECTRIX_ICON_SETTING "\xee\xa4\x8d"       // icon-setting (e90d)
#define FONT_ZECTRIX_ICON_POWER "\xee\xa4\x92"         // icon-power (e912)
#define FONT_ZECTRIX_ICON_SYNC "\xee\xa4\x93"          // icon-sync (e913)
#define FONT_ZECTRIX_ICON_REBOOT "\xee\xa4\x94"        // icon-reboot (e914)

#define FONT_ZECTRIX_ICON_TODO "\xee\xa4\x87"             // icon-todo (e907)

// Unclassified (kept by icon name)
#define FONT_ZECTRIX_ICON_COLON "\xee\xa4\x95"           // icon-mao (e915)
#define FONT_ZECTRIX_ICON_0 "\xee\xa4\x9f"          // icon-0
#define FONT_ZECTRIX_ICON_1 "\xee\xa4\x88"             // icon-1 (e908)
#define FONT_ZECTRIX_ICON_2 "\xee\xa4\x97"             // icon-2 (e917)
#define FONT_ZECTRIX_ICON_3 "\xee\xa4\x98"             // icon-3 (e918)
#define FONT_ZECTRIX_ICON_4 "\xee\xa4\x9e"            // icon-4 (e91e)
#define FONT_ZECTRIX_ICON_5 "\xee\xa4\x99"             // icon-5 (e919)
#define FONT_ZECTRIX_ICON_6 "\xee\xa4\x9a"             // icon-6 (e91a)
#define FONT_ZECTRIX_ICON_7 "\xee\xa4\x9b"             // icon-7 (e91b)
#define FONT_ZECTRIX_ICON_8 "\xee\xa4\x9c"             // icon-8 (e91c)
#define FONT_ZECTRIX_ICON_9 "\xee\xa4\x9d"             // icon-9 (e91d)


#ifdef __cplusplus
}
#endif
