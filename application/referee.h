/**
 * @file referee.c
 * @brief RM裁判系统，单片机通过3pin的uart6连接到电源管理模块，获取裁判系统数据
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

/*-----------------------------------预处理-----------------------------------*/

#ifndef REFEREE_H
#define REFEREE_H

#include "main.h"
#include "protocol.h"
#include "struct_typedef.h"

/*-----------------------------------数据结构定义-----------------------------------*/

typedef enum
{
    RED_HERO = 1,
    RED_ENGINEER = 2,
    RED_STANDARD_1 = 3,
    RED_STANDARD_2 = 4,
    RED_STANDARD_3 = 5,
    RED_AERIAL = 6,
    RED_SENTRY = 7,
    BLUE_HERO = 11,
    BLUE_ENGINEER = 12,
    BLUE_STANDARD_1 = 13,
    BLUE_STANDARD_2 = 14,
    BLUE_STANDARD_3 = 15,
    BLUE_AERIAL = 16,
    BLUE_SENTRY = 17,
} robot_id_t;

typedef enum
{
    PROGRESS_UNSTART = 0,
    PROGRESS_PREPARE = 1,
    PROGRESS_SELFCHECK = 2,
    PROGRESS_5sCOUNTDOWN = 3,
    PROGRESS_BATTLE = 4,
    PROGRESS_CALCULATING = 5,
} game_progress_t;

typedef __packed struct
{
    uint8_t game_type : 4;      // 比赛类型（1：超级对抗赛；2：高校单项赛；3：人工智能挑战赛；4：RMUL3v3；5：RMUL步兵对抗赛）
    uint8_t game_progress : 4;  // 比赛阶段（0：未开始；1：准备阶段；2：15s裁判系统自检；3：5s倒计时；4：比赛中；5：比赛结算中）
    uint16_t stage_remain_time; // 当前阶段剩余时间
    // unit64_t SyncTimeStamp;     // 服务器同步时间戳，当机器人正确连接到裁判系统的NTP服务器后生效，此处未使用
} ext_game_state_t; // 0001 比赛状态数据

typedef __packed struct
{
    uint8_t winner;  // 0：平局；1：红方胜利；2：蓝方胜利
} ext_game_result_t; // 0002 比赛结果数据

typedef __packed struct
{
    uint16_t red_1_robot_HP;  // 红1英雄血量
    uint16_t red_2_robot_HP;  // 红2工程血量
    uint16_t red_3_robot_HP;  // 红3步兵血量
    uint16_t red_4_robot_HP;  // 红4步兵血量
    uint16_t reserved1;       // 保留位1
    uint16_t red_7_robot_HP;  // 红7哨兵血量
    uint16_t red_outpost_HP;  // 红方前哨站血量
    uint16_t red_base_HP;     // 红方基地血量
    uint16_t blue_1_robot_HP; // 蓝1英雄血量
    uint16_t blue_2_robot_HP; // 蓝2工程血量
    uint16_t blue_3_robot_HP; // 蓝3步兵血量
    uint16_t blue_4_robot_HP; // 蓝4步兵血量
    uint16_t reserved2;       // 保留位2
    uint16_t blue_7_robot_HP; // 蓝7哨兵血量
    uint16_t blue_outpost_HP; // 蓝方前哨站血量
    uint16_t blue_base_HP;    // 蓝方基地血量
} ext_game_robot_HP_t;        // 0003 比赛机器人血量数据

typedef __packed struct
{
    uint8_t non_overlapping_supply_area_occupation_status : 1; // bit 0：己方与兑换区不重叠的补给区占领状态，1为已占领
    uint8_t overlapping_supply_area_occupation_status : 1;     // bit 1：己方与兑换区重叠的补给区占领状态，1为已占领
    uint8_t supply_area_occupation_status : 1;                 // bit 2：己方补给区的占领状态，1为已占领（仅 RMUL 适用）
    uint8_t small_energy_chamber_activation_status : 1;        // bit 3：己方小能量机关的激活状态，1为已激活
    uint8_t big_energy_chamber_activation_status : 1;          // bit 4：己方大能量机关的激活状态，1为已激活
    uint8_t center_high_ground_occupation_status : 2;          // bit 5-6：己方中央高地的占领状态，1为被己方占领，2为被对方占领
    uint8_t tower_high_ground_occupation_status_1 : 1;         // bit 7-8：己方梯形高地的占领状态，1为已占领
    uint8_t tower_high_ground_occupation_status_2 : 1;         // 【由于临近单字节边缘，所以该变量需要拆为两个1bit变量存储】
    uint8_t dart_attack_time_1 : 7;                            // bit 9-17：对方飞镖最后一次击中己方前哨站或基地的时间（0-420，开局默认为0）
    uint8_t dart_attack_time_2 : 2;                            // 【由于临近单字节边缘，所以该变量的第9个bit需要单独拆为一个1bit变量存储】
    uint8_t dart_attack_target : 3;                            // bit 18-20：对方飞镖最后一次击中己方前哨站或基地的具体目标，开局默认为0，1为击中前哨站，2为击中基地固定目标，3为击中基地随机固定目标，4为击中基地随机移动目标
    uint8_t center_point_occupation_status : 2;                // bit 21-22：中心增益点的占领状态，0为未被占领，1为被己方占领，2为被对方占领，3为被双方占领。(仅RMUL适用)
    uint8_t reserved_1 : 1;
    uint8_t reserved_2 : 8; // 保留位
} ext_event_data_t;         // 0x0101 场地事件数据

typedef __packed struct
{
    uint8_t supply_projectile_id;
    uint8_t supply_robot_id;
    uint8_t supply_projectile_step;
    uint8_t supply_projectile_num;
} ext_supply_projectile_action_t; // 0x0102 已弃用

typedef __packed struct
{
    uint8_t supply_projectile_id;
    uint8_t supply_robot_id;
    uint8_t supply_num;
} ext_supply_projectile_booking_t; // 0x0103 已弃用

typedef __packed struct
{
    uint8_t level;
    uint8_t offending_robot_id;
    uint8_t count;
} ext_referee_warning_t; // 0x0104 裁判警告数据

typedef __packed struct
{
    uint8_t dart_remaining_time;
    uint16_t dart_info;
} dart_info_t; // 0x0105 飞镖发射相关数据

typedef __packed struct
{
    uint8_t robot_id;
    uint8_t robot_level;
    uint16_t remain_HP;
    uint16_t max_HP;
    uint16_t shooter_heat0_cooling_rate;
    uint16_t shooter_heat0_limit;
    uint16_t chassis_power_limit;
    uint8_t mains_power_gimbal_output : 1;
    uint8_t mains_power_chassis_output : 1;
    uint8_t mains_power_shooter_output : 1;
} ext_game_robot_state_t; // 0x0201 机器人性能体系数据

typedef __packed struct
{
    uint16_t chassis_volt;
    uint16_t chassis_current;
    float chassis_power;
    uint16_t chassis_power_buffer;
    uint16_t shooter_heat0;
    uint16_t shooter_heat1;
} ext_power_heat_data_t; // 0x0202 底盘缓冲能量和枪口热量数据

typedef __packed struct
{
    float x;            // 本机器人位置x坐标，单位m
    float y;            // 本机器人位置y坐标，单位m
    float angle;        // 本机器人测速模块的朝向，单位度（正北为0度）
} ext_game_robot_pos_t; // 0x0203 机器人位置数据

typedef __packed struct
{
    uint8_t recovery_buff;      // 机器人回血增益（百分比，值为10表示每秒恢复血量上限的10%）
    uint8_t cooling_buff;       // 机器人射击热量冷却倍率（直接值，值为5表示5倍冷却）
    uint8_t defence_buff;       // 机器人防御增益（百分比，值为50表示50%防御增益）
    uint8_t vulnerability_buff; // 机器人负防御增益（百分比，值为30表示-30%防御增益）
    uint16_t attack_buff;       // 机器人攻击增益（百分比，值为50表示50%攻击增益）
    uint8_t remaining_energy;   // bit 0-4：机器人剩余能量值反馈，以16进制标识机器人剩余能量值比例，仅在机器人剩余能量小于50%时反馈，其余默认反馈0x32。（bit 0：在剩余能量≥50%时为1，其余情况为0；bit 1：在剩余能量≥30%时为1，其余情况为0；bit 2：在剩余能量≥15%时为1，其余情况为0；bit 3：在剩余能量≥5%时为1，其余情况为0Bit4：在剩余能量≥1%时为1，其余情况为0）
} ext_buff_musk_t; // 0x0204 机器人增益数据

typedef __packed struct
{
    uint8_t energy_point;
    uint8_t attack_time;
} aerial_robot_energy_t; // 0x0205 已弃用

typedef __packed struct
{
    uint8_t armor_id : 4;            // bit 0-3：当扣血原因为装甲模块被弹丸攻击、受撞击、离线或测速模块离线时，该4 bit组成的数值为装甲模块或测速模块的ID编号；当其他原因导致扣血时，该数值为0
    uint8_t HP_deduction_reason : 4; // bit 4-7：血量变化类型（0-装甲模块被弹丸攻击导致扣血；1-裁判系统重要模块离线导致扣血；2-射击初速度超限导致扣血；3-射击热量超限导致扣血；4-底盘功率超限导致扣血；5-装甲模块受到撞击导致扣血）
} ext_robot_hurt_t; // 0x0206 伤害状态数据

typedef __packed struct
{
    uint8_t bullet_type;         // 弹丸类型（1：17mm；2：42mm）
    uint8_t shooter_number;      // 发射机构ID（1：第一个17mm发射机构，2：第二个17mm发射机构，3:42mm发射机构）
    uint8_t launching_frequency; // 弹丸射频，单位Hz
    float initial_speed;         // 弹丸初速度，单位m/s
} ext_shoot_data_t; // 0x0207 实时射击数据

typedef __packed struct
{
    uint16_t projectile_allowance_17mm;
    uint16_t projectile_allowance_42mm;
    uint16_t remaining_gold_coin;
} ext_bullet_remaining_t; // 0x0208 允许发弹量

typedef __packed struct
{
    // bit位值为1/0的含义：是否已检测到该增益点RFID卡
    uint8_t self_base_buff : 1;                   // bit 0：己方基地增益点
    uint8_t self_center_highland_buff : 1;        // bit 1：己方中央高地增益点
    uint8_t enemy_center_highland_buff : 1;       // bit 2：对方中央高地增益点
    uint8_t self_tower_highland_buff : 1;         // bit 3：己方梯形高地增益点
    uint8_t enemy_tower_highland_buff : 1;        // bit 4：对方梯形高地增益点
    uint8_t self_slope_span_buff : 1;             // bit 5：己方地形跨越增益点（飞坡）（靠近己方一侧飞坡前）
    uint8_t self_slope_span_buff_2 : 1;           // bit 6：己方地形跨越增益点（飞坡）（靠近己方一侧飞坡后）
    uint8_t enemy_slope_span_buff : 1;            // bit 7：对方地形跨越增益点（飞坡）（靠近对方一侧飞坡前）
    uint8_t enemy_slope_span_buff_2 : 1;          // bit 8：对方地形跨越增益点（飞坡）（靠近对方一侧飞坡后）
    uint8_t self_highland_span_buff : 1;          // bit 9：己方地形跨越增益点（中央高地下方）
    uint8_t self_highland_span_buff_2 : 1;        // bit 10：己方地形跨越增益点（中央高地上方）
    uint8_t enemy_highland_span_buff : 1;         // bit 11：对方地形跨越增益点（中央高地下方）
    uint8_t enemy_highland_span_buff_2 : 1;       // bit 12：对方地形跨越增益点（中央高地上方）
    uint8_t self_road_span_buff : 1;              // bit 13：己方地形跨越增益点（公路下方）
    uint8_t self_road_span_buff_2 : 1;            // bit 14：己方地形跨越增益点（公路上方）
    uint8_t enemy_road_span_buff : 1;             // bit 15：对方地形跨越增益点（公路下方）
    uint8_t enemy_road_span_buff_2 : 1;           // bit 16：对方地形跨越增益点（公路上方）
    uint8_t self_fortress_buff : 1;               // bit 17：己方堡垒增益点
    uint8_t self_outpost_buff : 1;                // bit 18：己方前哨站增益点
    uint8_t self_supply_area_buff : 1;            // bit 19：己方与兑换区不重叠的补给区/RMUL补给区
    uint8_t self_supply_area_buff_2 : 1;          // bit 20：己方与兑换区重叠的补给区
    uint8_t self_large_resource_island_buff : 1;  // bit 21：己方大资源岛增益点
    uint8_t enemy_large_resource_island_buff : 1; // bit 22：对方大资源岛增益点
    uint8_t center_point_buff : 1;                // bit 23：中心增益点（仅 RMUL 适用）
    uint8_t reserved : 8;                         // bit 24-31：保留
} ext_rfid_status_t; // 0x0209 机器人RFID数据

// 0x020A 飞镖选手端指令数据

// 0x020B 地面机器人位置数据

// 0x020C 雷达标记进度数据

// 0x020D 哨兵自主决策信息同步

// 0x020E 雷达自主决策信息同步

typedef __packed struct
{
    uint16_t data_cmd_id;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint8_t user_data[112];
} ext_student_interactive_data_t; // 0x0301 机器人交互数据，发送方触发发送

typedef __packed struct
{
    float data1;
    float data2;
    float data3;
    uint8_t data4;
} custom_data_t;

typedef __packed struct
{
    uint8_t data[64];
} ext_up_stream_data_t;

typedef __packed struct
{
    uint8_t data[32];
} ext_download_stream_data_t;

/*-----------------------------------外部函数声明-----------------------------------*/

extern void init_referee_struct_data(void);
extern void referee_data_solve(uint8_t *frame);
extern void get_chassis_power_and_buffer(fp32 *power, fp32 *buffer);
extern uint8_t get_robot_id(void);
extern void get_shoot_heat0_limit_and_heat0(uint16_t *heat0_limit, uint16_t *heat0);
extern void get_shoot_heat1_limit_and_heat1(uint16_t *heat1_limit, uint16_t *heat1);
extern ext_power_heat_data_t power_heat_data_t;
// 获取中心点状态
extern void get_center_gain_point_status(uint16_t *status);

#endif
