//
// Created by RM UI Designer
//

#include "ui_default_right_text_stable_0.h"
#include "string.h"

#define FRAME_ID 0
#define GROUP_ID 2
#define START_ID 0

ui_string_frame_t ui_default_right_text_stable_0;

ui_interface_string_t* ui_default_right_text_stable_bullet = &ui_default_right_text_stable_0.option;

void _ui_init_default_right_text_stable_0() {
    ui_default_right_text_stable_0.option.figure_name[0] = FRAME_ID;
    ui_default_right_text_stable_0.option.figure_name[1] = GROUP_ID;
    ui_default_right_text_stable_0.option.figure_name[2] = START_ID;
    ui_default_right_text_stable_0.option.operate_tpyel = 1;
    ui_default_right_text_stable_0.option.figure_tpye = 7;
    ui_default_right_text_stable_0.option.layer = 8;
    ui_default_right_text_stable_0.option.font_size = 20;
    ui_default_right_text_stable_0.option.start_x = 1134;
    ui_default_right_text_stable_0.option.start_y = 833;
    ui_default_right_text_stable_0.option.color = 1;
    ui_default_right_text_stable_0.option.str_length = 6;
    ui_default_right_text_stable_0.option.width = 2;
    strcpy(ui_default_right_text_stable_bullet->string, "BULLET");

    ui_proc_string_frame(&ui_default_right_text_stable_0);
    SEND_MESSAGE((uint8_t *) &ui_default_right_text_stable_0, sizeof(ui_default_right_text_stable_0));
}

void _ui_update_default_right_text_stable_0() {
    ui_default_right_text_stable_0.option.operate_tpyel = 2;

    ui_proc_string_frame(&ui_default_right_text_stable_0);
    SEND_MESSAGE((uint8_t *) &ui_default_right_text_stable_0, sizeof(ui_default_right_text_stable_0));
}

void _ui_remove_default_right_text_stable_0() {
    ui_default_right_text_stable_0.option.operate_tpyel = 3;

    ui_proc_string_frame(&ui_default_right_text_stable_0);
    SEND_MESSAGE((uint8_t *) &ui_default_right_text_stable_0, sizeof(ui_default_right_text_stable_0));
}