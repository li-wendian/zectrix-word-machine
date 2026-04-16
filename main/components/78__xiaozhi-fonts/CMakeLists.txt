idf_component_register(
    SRCS
        "src/cbin_font.c"
        "src/DejaVuSans_Phonetic_16.c"
        "src/SourceHanSansSC_Medium_slim.c"
        "src/SourceHanSansSC_Regular_slim.c"
        "src/SourceHanSansSC_Vocab_18.c"
        "src/font_zectrix_16_1.c"
        "src/font_zectrix_48_1.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        "lvgl"
)

# 为组件添加编译宏定义
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)
