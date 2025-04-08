//
// Created by RM UI Designer
//

#include "ui_default_left_text_stable_5.h"
#include "string.h"

#define FRAME_ID 0
#define GROUP_ID 1
#define START_ID 5

ui_string_frame_t ui_default_left_text_stable_5;

ui_interface_string_t* ui_default_left_text_stable_now_pitch = &ui_default_left_text_stable_5.option;

void _ui_init_default_left_text_stable_5() {
    ui_default_left_text_stable_5.option.figure_name[0] = FRAME_ID;
    ui_default_left_text_stable_5.option.figure_name[1] = GROUP_ID;
    ui_default_left_text_stable_5.option.figure_name[2] = START_ID;
    ui_default_left_text_stable_5.option.operate_tpyel = 1;
    ui_default_left_text_stable_5.option.figure_tpye = 7;
    ui_default_left_text_stable_5.option.layer = 0;
    ui_default_left_text_stable_5.option.font_size = 20;
    ui_default_left_text_stable_5.option.start_x = 588;
    ui_default_left_text_stable_5.option.start_y = 272;
    ui_default_left_text_stable_5.option.color = 1;
    ui_default_left_text_stable_5.option.str_length = 9;
    ui_default_left_text_stable_5.option.width = 2;
    strcpy(ui_default_left_text_stable_now_pitch->string, "NOW_PITCH");

    ui_proc_string_frame(&ui_default_left_text_stable_5);
    SEND_MESSAGE((uint8_t *) &ui_default_left_text_stable_5, sizeof(ui_default_left_text_stable_5));
}

void _ui_update_default_left_text_stable_5() {
    ui_default_left_text_stable_5.option.operate_tpyel = 2;

    ui_proc_string_frame(&ui_default_left_text_stable_5);
    SEND_MESSAGE((uint8_t *) &ui_default_left_text_stable_5, sizeof(ui_default_left_text_stable_5));
}

void _ui_remove_default_left_text_stable_5() {
    ui_default_left_text_stable_5.option.operate_tpyel = 3;

    ui_proc_string_frame(&ui_default_left_text_stable_5);
    SEND_MESSAGE((uint8_t *) &ui_default_left_text_stable_5, sizeof(ui_default_left_text_stable_5));
}