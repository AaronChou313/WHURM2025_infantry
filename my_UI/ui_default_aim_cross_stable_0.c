//
// Created by RM UI Designer
//

#include "ui_default_aim_cross_stable_0.h"

#define FRAME_ID 0
#define GROUP_ID 0
#define START_ID 0
#define OBJ_NUM 5
#define FRAME_OBJ_NUM 5

CAT(ui_, CAT(FRAME_OBJ_NUM, _frame_t)) ui_default_aim_cross_stable_0;
ui_interface_line_t *ui_default_aim_cross_stable_aim_cross_horizon = (ui_interface_line_t *)&(ui_default_aim_cross_stable_0.data[0]);
ui_interface_line_t *ui_default_aim_cross_stable_aim_cross_vertical = (ui_interface_line_t *)&(ui_default_aim_cross_stable_0.data[1]);
ui_interface_line_t *ui_default_aim_cross_stable_aim_cross_true = (ui_interface_line_t *)&(ui_default_aim_cross_stable_0.data[2]);
ui_interface_line_t *ui_default_aim_cross_stable_box_cross_1 = (ui_interface_line_t *)&(ui_default_aim_cross_stable_0.data[3]);
ui_interface_line_t *ui_default_aim_cross_stable_box_cross_2 = (ui_interface_line_t *)&(ui_default_aim_cross_stable_0.data[4]);

void _ui_init_default_aim_cross_stable_0() {
    for (int i = 0; i < OBJ_NUM; i++) {
        ui_default_aim_cross_stable_0.data[i].figure_name[0] = FRAME_ID;
        ui_default_aim_cross_stable_0.data[i].figure_name[1] = GROUP_ID;
        ui_default_aim_cross_stable_0.data[i].figure_name[2] = i + START_ID;
        ui_default_aim_cross_stable_0.data[i].operate_tpyel = 1;
    }
    for (int i = OBJ_NUM; i < FRAME_OBJ_NUM; i++) {
        ui_default_aim_cross_stable_0.data[i].operate_tpyel = 0;
    }

    ui_default_aim_cross_stable_aim_cross_horizon->figure_tpye = 0;
    ui_default_aim_cross_stable_aim_cross_horizon->layer = 2;
    ui_default_aim_cross_stable_aim_cross_horizon->start_x = 620;
    ui_default_aim_cross_stable_aim_cross_horizon->start_y = 540;
    ui_default_aim_cross_stable_aim_cross_horizon->end_x = 1299;
    ui_default_aim_cross_stable_aim_cross_horizon->end_y = 540;
    ui_default_aim_cross_stable_aim_cross_horizon->color = 2;
    ui_default_aim_cross_stable_aim_cross_horizon->width = 2;

    ui_default_aim_cross_stable_aim_cross_vertical->figure_tpye = 0;
    ui_default_aim_cross_stable_aim_cross_vertical->layer = 2;
    ui_default_aim_cross_stable_aim_cross_vertical->start_x = 960;
    ui_default_aim_cross_stable_aim_cross_vertical->start_y = 201;
    ui_default_aim_cross_stable_aim_cross_vertical->end_x = 960;
    ui_default_aim_cross_stable_aim_cross_vertical->end_y = 880;
    ui_default_aim_cross_stable_aim_cross_vertical->color = 2;
    ui_default_aim_cross_stable_aim_cross_vertical->width = 2;

    ui_default_aim_cross_stable_aim_cross_true->figure_tpye = 0;
    ui_default_aim_cross_stable_aim_cross_true->layer = 0;
    ui_default_aim_cross_stable_aim_cross_true->start_x = 920;
    ui_default_aim_cross_stable_aim_cross_true->start_y = 490;
    ui_default_aim_cross_stable_aim_cross_true->end_x = 1000;
    ui_default_aim_cross_stable_aim_cross_true->end_y = 490;
    ui_default_aim_cross_stable_aim_cross_true->color = 2;
    ui_default_aim_cross_stable_aim_cross_true->width = 2;

    ui_default_aim_cross_stable_box_cross_1->figure_tpye = 0;
    ui_default_aim_cross_stable_box_cross_1->layer = 0;
    ui_default_aim_cross_stable_box_cross_1->start_x = 780;
    ui_default_aim_cross_stable_box_cross_1->start_y = 286;
    ui_default_aim_cross_stable_box_cross_1->end_x = 1221;
    ui_default_aim_cross_stable_box_cross_1->end_y = 644;
    ui_default_aim_cross_stable_box_cross_1->color = 6;
    ui_default_aim_cross_stable_box_cross_1->width = 2;

    ui_default_aim_cross_stable_box_cross_2->figure_tpye = 0;
    ui_default_aim_cross_stable_box_cross_2->layer = 0;
    ui_default_aim_cross_stable_box_cross_2->start_x = 780;
    ui_default_aim_cross_stable_box_cross_2->start_y = 644;
    ui_default_aim_cross_stable_box_cross_2->end_x = 1221;
    ui_default_aim_cross_stable_box_cross_2->end_y = 286;
    ui_default_aim_cross_stable_box_cross_2->color = 6;
    ui_default_aim_cross_stable_box_cross_2->width = 2;


    CAT(ui_proc_, CAT(FRAME_OBJ_NUM, _frame))(&ui_default_aim_cross_stable_0);
    SEND_MESSAGE((uint8_t *) &ui_default_aim_cross_stable_0, sizeof(ui_default_aim_cross_stable_0));
}

void _ui_update_default_aim_cross_stable_0() {
    for (int i = 0; i < OBJ_NUM; i++) {
        ui_default_aim_cross_stable_0.data[i].operate_tpyel = 2;
    }

    CAT(ui_proc_, CAT(FRAME_OBJ_NUM, _frame))(&ui_default_aim_cross_stable_0);
    SEND_MESSAGE((uint8_t *) &ui_default_aim_cross_stable_0, sizeof(ui_default_aim_cross_stable_0));
}

void _ui_remove_default_aim_cross_stable_0() {
    for (int i = 0; i < OBJ_NUM; i++) {
        ui_default_aim_cross_stable_0.data[i].operate_tpyel = 3;
    }

    CAT(ui_proc_, CAT(FRAME_OBJ_NUM, _frame))(&ui_default_aim_cross_stable_0);
    SEND_MESSAGE((uint8_t *) &ui_default_aim_cross_stable_0, sizeof(ui_default_aim_cross_stable_0));
}
