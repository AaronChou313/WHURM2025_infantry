/**
 * @file referee.c
 * @brief RM裁判系统
 * ----------------------------------------------------------------------------
 * @version 1.0.0.0
 * @author RM
 * @date 2018-12-26
 * @remark 官步初始代码
 * ----------------------------------------------------------------------------
 * @version 1.0.0.1
 * @author 周明杨
 * @date 2024-12-30
 * @remark 优化整体架构
 */

/*-----------------------------------头文件引用-----------------------------------*/

#include "referee.h"
#include "string.h"
#include "stdio.h"
#include "CRC8_CRC16.h"
#include "protocol.h"

/*-----------------------------------内部函数声明-----------------------------------*/

// 空

/*-----------------------------------变量声明-----------------------------------*/

// 裁判系统数据接收头
frame_header_struct_t referee_receive_header;
// 裁判系统数据发送头
frame_header_struct_t referee_send_header;

ext_game_state_t game_state;            // 对局状态
ext_game_result_t game_result;          // 对局结果
ext_game_robot_HP_t game_robot_HP_t;    // 场上所有机器人血量

ext_event_data_t field_event;           // 场上所有事件

ext_referee_warning_t referee_warning_t;    // 裁判系统警告

ext_game_robot_state_t robot_state;         // 机器人状态
ext_power_heat_data_t power_heat_data_t;    // 功率与热量
ext_game_robot_pos_t game_robot_pos_t;      // 机器人坐标与yaw角
ext_buff_musk_t buff_musk_t;

ext_robot_hurt_t robot_hurt_t;
ext_shoot_data_t shoot_data_t;
ext_bullet_remaining_t bullet_remaining_t;

ext_rfid_status_t rfid_status;

ext_student_interactive_data_t student_interactive_data_t;

/*-----------------------------------函数实现-----------------------------------*/

void init_referee_struct_data(void)
{
    memset(&referee_receive_header, 0, sizeof(frame_header_struct_t));
    memset(&referee_send_header, 0, sizeof(frame_header_struct_t));

    memset(&game_state, 0, sizeof(ext_game_state_t));
    memset(&game_result, 0, sizeof(ext_game_result_t));
    memset(&game_robot_HP_t, 0, sizeof(ext_game_robot_HP_t));

    memset(&field_event, 0, sizeof(ext_event_data_t));

    memset(&referee_warning_t, 0, sizeof(ext_referee_warning_t));

    memset(&robot_state, 0, sizeof(ext_game_robot_state_t));
    memset(&power_heat_data_t, 0, sizeof(ext_power_heat_data_t));
    memset(&game_robot_pos_t, 0, sizeof(ext_game_robot_pos_t));
    memset(&buff_musk_t, 0, sizeof(ext_buff_musk_t));

    memset(&robot_hurt_t, 0, sizeof(ext_robot_hurt_t));
    memset(&shoot_data_t, 0, sizeof(ext_shoot_data_t));
    memset(&bullet_remaining_t, 0, sizeof(ext_bullet_remaining_t));

    memset(&student_interactive_data_t, 0, sizeof(ext_student_interactive_data_t));
}

void referee_data_solve(uint8_t *frame)
{
    uint16_t cmd_id = 0;

    uint8_t index = 0;

    memcpy(&referee_receive_header, frame, sizeof(frame_header_struct_t));

    index += sizeof(frame_header_struct_t);

    memcpy(&cmd_id, frame + index, sizeof(uint16_t));

    index += sizeof(uint16_t);

    switch (cmd_id)
    {
    case GAME_STATE_CMD_ID:
    {
        memcpy(&game_state, frame + index, sizeof(ext_game_state_t));
    }
    break;
    case GAME_RESULT_CMD_ID:
    {
        memcpy(&game_result, frame + index, sizeof(game_result));
    }
    break;
    case GAME_ROBOT_HP_CMD_ID:
    {
        memcpy(&game_robot_HP_t, frame + index, sizeof(ext_game_robot_HP_t));
    }
    break;
    case FIELD_EVENTS_CMD_ID:
    {
        memcpy(&field_event, frame + index, sizeof(field_event));
    }
    break;
    case REFEREE_WARNING_CMD_ID:
    {
        memcpy(&referee_warning_t, frame + index, sizeof(ext_referee_warning_t));
    }
    break;
    case ROBOT_STATE_CMD_ID:
    {
        memcpy(&robot_state, frame + index, sizeof(robot_state));
    }
    break;
    case POWER_HEAT_DATA_CMD_ID:
    {
        memcpy(&power_heat_data_t, frame + index, sizeof(power_heat_data_t));
    }
    break;
    case ROBOT_POS_CMD_ID:
    {
        memcpy(&game_robot_pos_t, frame + index, sizeof(game_robot_pos_t));
    }
    break;
    case BUFF_MUSK_CMD_ID:
    {
        memcpy(&buff_musk_t, frame + index, sizeof(buff_musk_t));
    }
    break;
    case ROBOT_HURT_CMD_ID:
    {
        memcpy(&robot_hurt_t, frame + index, sizeof(robot_hurt_t));
    }
    break;
    case SHOOT_DATA_CMD_ID:
    {
        memcpy(&shoot_data_t, frame + index, sizeof(shoot_data_t));
    }
    break;
    case BULLET_REMAINING_CMD_ID:
    {
        memcpy(&bullet_remaining_t, frame + index, sizeof(ext_bullet_remaining_t));
    }
    break;
    case RFID_STATUS_CMD_ID:
    {
        memcpy(&rfid_status, frame + index, sizeof(ext_rfid_status_t));
    }
    break;
    case STUDENT_INTERACTIVE_DATA_CMD_ID:
    {
        memcpy(&student_interactive_data_t, frame + index, sizeof(student_interactive_data_t));
    }
    break;
    default:
    {
        break;
    }
    }
}

void get_chassis_power_and_buffer(fp32 *power, fp32 *buffer)
{
    *power = power_heat_data_t.chassis_power;
    *buffer = power_heat_data_t.chassis_power_buffer;
}

uint8_t get_robot_id(void)
{
    return robot_state.robot_id;
}

void get_shoot_heat0_limit_and_heat0(uint16_t *heat0_limit, uint16_t *heat0)
{
    *heat0_limit = robot_state.shooter_heat0_limit;
    *heat0 = power_heat_data_t.shooter_heat0;
}

// void get_shoot_heat1_limit_and_heat1(uint16_t *heat1_limit, uint16_t *heat1)
// {
//     *heat1_limit = robot_state.shooter_heat1_cooling_limit;
//     *heat1 = power_heat_data_t.shooter_heat1;
// }