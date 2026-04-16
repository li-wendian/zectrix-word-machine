set(SOURCES
    "application.cc"
    "main.cc"
    "settings.cc"
    "system_info.cc"
    "audio/audio_codec.cc"
    "audio/audio_service.cc"
    "audio/codecs/es8311_audio_codec.cc"
    "audio/processors/audio_debugger.cc"
    "audio/processors/no_audio_processor.cc"
    "display/display.cc"
    "display/lcd_display.cc"
    "display/ui_page_registry.cc"
    "display/pages/factory_test_page_adapter.cc"
    "display/pages/vocabulary_page_adapter.cc"
    "display/lvgl_display/lvgl_display.cc"
    "display/lvgl_display/lvgl_theme.cc"
    "display/lvgl_display/lvgl_font.cc"
    "vocabulary/deck_store.cc"
    "vocabulary/progress_store.cc"
    "vocabulary/review_scheduler.cc"
    "vocabulary/sync_client.cc"
    "vocabulary/vocabulary_app.cc"
    "common/sleep_manager.cc"
    "boards/common/board.cc"
    "boards/common/button.cc"
    "boards/common/i2c_bus_lock.cc"
    "boards/common/i2c_device.cc"
    "boards/zectrix/zectrix_nfc.cc"
    "boards/zectrix-s3-epaper-4.2/board_power_bsp.cc"
    "boards/zectrix-s3-epaper-4.2/charge_status.cc"
    "boards/zectrix-s3-epaper-4.2/custom_lcd_display.cc"
    "boards/zectrix-s3-epaper-4.2/i2c_power_hook.cc"
    "boards/zectrix-s3-epaper-4.2/rtc_pcf8563.cc"
    "boards/zectrix-s3-epaper-4.2/zectrix-s3-epaper-4.2.cc"
    "boards/zectrix-s3-epaper-4.2/FT/acoustic_selftest.cc"
    "boards/zectrix-s3-epaper-4.2/FT/factory_test_service.cc"
)

set(INCLUDE_DIRS
    "."
    "audio"
    "display"
    "display/lvgl_display"
    "vocabulary"
    "protocols"
    "boards/common"
)

if(NOT DEFINED BOARD_TYPE)
    set(BOARD_TYPE "zectrix-s3-epaper-4.2")
endif()

if(NOT DEFINED BOARD_NAME)
    set(BOARD_NAME "${BOARD_TYPE}")
endif()

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)

target_compile_definitions(
    ${COMPONENT_LIB}
    PRIVATE
    BOARD_TYPE=\"${BOARD_TYPE}\"
    BOARD_NAME=\"${BOARD_NAME}\"
    BUILTIN_TEXT_FONT=SourceHanSansSC_Regular_slim
    BUILTIN_ICON_FONT=font_zectrix_16_1
)
