//
// Created by RM UI Designer
//

#include "ui_default_left_dynamic_0.h"

#define FRAME_ID 0
#define GROUP_ID 5
#define START_ID 0
#define OBJ_NUM 7
#define FRAME_OBJ_NUM 7

CAT(ui_, CAT(FRAME_OBJ_NUM, _frame_t)) ui_default_left_dynamic_0;
ui_interface_round_t *ui_default_left_dynamic_rotate_state = (ui_interface_round_t *)&(ui_default_left_dynamic_0.data[0]);
ui_interface_round_t *ui_default_left_dynamic_fric_state = (ui_interface_round_t *)&(ui_default_left_dynamic_0.data[1]);
ui_interface_round_t *ui_default_left_dynamic_autoaim_state = (ui_interface_round_t *)&(ui_default_left_dynamic_0.data[2]);
ui_interface_round_t *ui_default_left_dynamic_fire_state = (ui_interface_round_t *)&(ui_default_left_dynamic_0.data[3]);
ui_interface_number_t *ui_default_left_dynamic_hp_value_i = (ui_interface_number_t *)&(ui_default_left_dynamic_0.data[4]);
ui_interface_number_t *ui_default_left_dynamic_now_pitch_value_f = (ui_interface_number_t *)&(ui_default_left_dynamic_0.data[5]);
ui_interface_number_t *ui_default_left_dynamic_now_yaw_value_f = (ui_interface_number_t *)&(ui_default_left_dynamic_0.data[6]);

void _ui_init_default_left_dynamic_0() {
    for (int i = 0; i < OBJ_NUM; i++) {
        ui_default_left_dynamic_0.data[i].figure_name[0] = FRAME_ID;
        ui_default_left_dynamic_0.data[i].figure_name[1] = GROUP_ID;
        ui_default_left_dynamic_0.data[i].figure_name[2] = i + START_ID;
        ui_default_left_dynamic_0.data[i].operate_tpyel = 1;
    }
    for (int i = OBJ_NUM; i < FRAME_OBJ_NUM; i++) {
        ui_default_left_dynamic_0.data[i].operate_tpyel = 0;
    }

    ui_default_left_dynamic_rotate_state->figure_tpye = 2;
    ui_default_left_dynamic_rotate_state->layer = 9;
    ui_default_left_dynamic_rotate_state->r = 15;
    ui_default_left_dynamic_rotate_state->start_x = 755;
    ui_default_left_dynamic_rotate_state->start_y = 815;
    ui_default_left_dynamic_rotate_state->color = 4;
    ui_default_left_dynamic_rotate_state->width = 10;

    ui_default_left_dynamic_fric_state->figure_tpye = 2;
    ui_default_left_dynamic_fric_state->layer = 9;
    ui_default_left_dynamic_fric_state->r = 15;
    ui_default_left_dynamic_fric_state->start_x = 755;
    ui_default_left_dynamic_fric_state->start_y = 765;
    ui_default_left_dynamic_fric_state->color = 4;
    ui_default_left_dynamic_fric_state->width = 10;

    ui_default_left_dynamic_autoaim_state->figure_tpye = 2;
    ui_default_left_dynamic_autoaim_state->layer = 9;
    ui_default_left_dynamic_autoaim_state->r = 15;
    ui_default_left_dynamic_autoaim_state->start_x = 755;
    ui_default_left_dynamic_autoaim_state->start_y = 715;
    ui_default_left_dynamic_autoaim_state->color = 4;
    ui_default_left_dynamic_autoaim_state->width = 10;

    ui_default_left_dynamic_fire_state->figure_tpye = 2;
    ui_default_left_dynamic_fire_state->layer = 0;
    ui_default_left_dynamic_fire_state->r = 15;
    ui_default_left_dynamic_fire_state->start_x = 755;
    ui_default_left_dynamic_fire_state->start_y = 665;
    ui_default_left_dynamic_fire_state->color = 4;
    ui_default_left_dynamic_fire_state->width = 10;

    ui_default_left_dynamic_hp_value_i->figure_tpye = 6;
    ui_default_left_dynamic_hp_value_i->layer = 0;
    ui_default_left_dynamic_hp_value_i->font_size = 20;
    ui_default_left_dynamic_hp_value_i->start_x = 658;
    ui_default_left_dynamic_hp_value_i->start_y = 883;
    ui_default_left_dynamic_hp_value_i->color = 6;
    ui_default_left_dynamic_hp_value_i->number = 12345;
    ui_default_left_dynamic_hp_value_i->width = 2;

    ui_default_left_dynamic_now_pitch_value_f->figure_tpye = 5;
    ui_default_left_dynamic_now_pitch_value_f->layer = 0;
    ui_default_left_dynamic_now_pitch_value_f->font_size = 20;
    ui_default_left_dynamic_now_pitch_value_f->start_x = 794;
    ui_default_left_dynamic_now_pitch_value_f->start_y = 272;
    ui_default_left_dynamic_now_pitch_value_f->color = 6;
    ui_default_left_dynamic_now_pitch_value_f->number = 12345;
    ui_default_left_dynamic_now_pitch_value_f->width = 2;

    ui_default_left_dynamic_now_yaw_value_f->figure_tpye = 5;
    ui_default_left_dynamic_now_yaw_value_f->layer = 0;
    ui_default_left_dynamic_now_yaw_value_f->font_size = 20;
    ui_default_left_dynamic_now_yaw_value_f->start_x = 794;
    ui_default_left_dynamic_now_yaw_value_f->start_y = 222;
    ui_default_left_dynamic_now_yaw_value_f->color = 6;
    ui_default_left_dynamic_now_yaw_value_f->number = 12345;
    ui_default_left_dynamic_now_yaw_value_f->width = 2;


    CAT(ui_proc_, CAT(FRAME_OBJ_NUM, _frame))(&ui_default_left_dynamic_0);
    SEND_MESSAGE((uint8_t *) &ui_default_left_dynamic_0, sizeof(ui_default_left_dynamic_0));
}

void _ui_update_default_left_dynamic_0() {
    for (int i = 0; i < OBJ_NUM; i++) {
        ui_default_left_dynamic_0.data[i].operate_tpyel = 2;
    }

    CAT(ui_proc_, CAT(FRAME_OBJ_NUM, _frame))(&ui_default_left_dynamic_0);
    SEND_MESSAGE((uint8_t *) &ui_default_left_dynamic_0, sizeof(ui_default_left_dynamic_0));
}

void _ui_remove_default_left_dynamic_0() {
    for (int i = 0; i < OBJ_NUM; i++) {
        ui_default_left_dynamic_0.data[i].operate_tpyel = 3;
    }

    CAT(ui_proc_, CAT(FRAME_OBJ_NUM, _frame))(&ui_default_left_dynamic_0);
    SEND_MESSAGE((uint8_t *) &ui_default_left_dynamic_0, sizeof(ui_default_left_dynamic_0));
}
