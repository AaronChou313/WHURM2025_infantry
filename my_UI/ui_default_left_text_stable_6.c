//
// Created by RM UI Designer
//

#include "ui_default_left_text_stable_6.h"
#include "string.h"

#define FRAME_ID 0
#define GROUP_ID 1
#define START_ID 6

ui_string_frame_t ui_default_left_text_stable_6;

ui_interface_string_t* ui_default_left_text_stable_now_yaw = &ui_default_left_text_stable_6.option;

void _ui_init_default_left_text_stable_6() {
    ui_default_left_text_stable_6.option.figure_name[0] = FRAME_ID;
    ui_default_left_text_stable_6.option.figure_name[1] = GROUP_ID;
    ui_default_left_text_stable_6.option.figure_name[2] = START_ID;
    ui_default_left_text_stable_6.option.operate_tpyel = 1;
    ui_default_left_text_stable_6.option.figure_tpye = 7;
    ui_default_left_text_stable_6.option.layer = 0;
    ui_default_left_text_stable_6.option.font_size = 20;
    ui_default_left_text_stable_6.option.start_x = 588;
    ui_default_left_text_stable_6.option.start_y = 222;
    ui_default_left_text_stable_6.option.color = 1;
    ui_default_left_text_stable_6.option.str_length = 7;
    ui_default_left_text_stable_6.option.width = 2;
    strcpy(ui_default_left_text_stable_now_yaw->string, "NOW_YAW");

    ui_proc_string_frame(&ui_default_left_text_stable_6);
    SEND_MESSAGE((uint8_t *) &ui_default_left_text_stable_6, sizeof(ui_default_left_text_stable_6));
}

void _ui_update_default_left_text_stable_6() {
    ui_default_left_text_stable_6.option.operate_tpyel = 2;

    ui_proc_string_frame(&ui_default_left_text_stable_6);
    SEND_MESSAGE((uint8_t *) &ui_default_left_text_stable_6, sizeof(ui_default_left_text_stable_6));
}

void _ui_remove_default_left_text_stable_6() {
    ui_default_left_text_stable_6.option.operate_tpyel = 3;

    ui_proc_string_frame(&ui_default_left_text_stable_6);
    SEND_MESSAGE((uint8_t *) &ui_default_left_text_stable_6, sizeof(ui_default_left_text_stable_6));
}