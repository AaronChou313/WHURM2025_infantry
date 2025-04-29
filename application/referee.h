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

// 机器人ID序列
typedef enum
{
    RED_HERO = 1,                      // 红方英雄机器人
    RED_ENGINEER = 2,                  // 红方工程机器人
    RED_STANDARD_3 = 3,                // 红方3号步兵机器人
    RED_STANDARD_4 = 4,                // 红方4号步兵机器人
    RED_STANDARD_5 = 5,                // 红方5号步兵机器人
    RED_AERIAL = 6,                    // 红方空中机器人
    RED_SENTRY = 7,                    // 红方哨兵机器人
    RED_DART = 8,                      // 红方飞镖
    RED_RADAR = 9,                     // 红方雷达
    RED_OUTPOST = 10,                  // 红方前哨站
    RED_BASE = 11,                     // 红方基地
    BLUE_HERO = 101,                   // 蓝方英雄机器人
    BLUE_ENGINEER = 102,               // 蓝方工程机器人
    BLUE_STANDARD_3 = 103,             // 蓝方3号步兵机器人
    BLUE_STANDARD_4 = 104,             // 蓝方4号步兵机器人
    BLUE_STANDARD_5 = 105,             // 蓝方5号步兵机器人
    BLUE_AERIAL = 106,                 // 蓝方空中机器人
    BLUE_SENTRY = 107,                 // 蓝方哨兵机器人
    BLUE_DART = 108,                   // 蓝方飞镖
    BLUE_RADAR = 109,                  // 蓝方雷达
    BLUE_OUTPOST = 110,                // 蓝方前哨站
    BLUE_BASE = 111,                   // 蓝方基地
    RED_HERO_OPERATOR = 0x0101,        // 红方英雄机器人选手端
    RED_ENGINEER_OPERATOR = 0x0102,    // ：红方工程机器人选手端
    RED_STANDARD_3_OPERATOR = 0x0103,  // 红方3号步兵机器人选手端
    RED_STANDARD_4_OPERATOR = 0x0104,  // 红方4号步兵机器人选手端
    RED_STANDARD_5_OPERATOR = 0x0105,  // 红方5号步兵机器人选手端
    RED_AERIAL_OPERATOR = 0x0106,      // 红方空中机器人选手端
    BLUE_AERIAL_OPERATOR = 0x016A,     // 蓝方空中机器人选手端
    BLUE_HERO_OPERATOR = 0x0165,       // 蓝方英雄机器人选手端
    BLUE_ENGINEER_OPERATOR = 0x0166,   // 蓝方工程机器人选手端
    BLUE_STANDARD_3_OPERATOR = 0x0167, // 蓝方3号步兵机器人选手端
    BLUE_STANDARD_4_OPERATOR = 0x0168, // 蓝方4号步兵机器人选手端
    BLUE_STANDARD_5_OPERATOR = 0x0169, // 蓝方5号步兵机器人选手端
    REFEREE_SERVER = 0x8080,           // 裁判系统服务器（用于哨兵和雷达自主决策指令）
} robot_id_t;

// 比赛阶段序列
typedef enum
{
    PROGRESS_UNSTART = 0,     // 未开始
    PROGRESS_PREPARE = 1,     // 准备阶段
    PROGRESS_SELFCHECK = 2,   // 裁判系统自检
    PROGRESS_5sCOUNTDOWN = 3, // 5秒倒计时
    PROGRESS_BATTLE = 4,      // 比赛中
    PROGRESS_CALCULATING = 5, // 结算
} game_progress_t;

/* 裁判系统消息结构体集合 */ 

// 0001 比赛状态数据
typedef __packed struct
{
    uint8_t game_type : 4;      // 比赛类型（1：超级对抗赛；2：高校单项赛；3：人工智能挑战赛；4：RMUL3v3；5：RMUL步兵对抗赛）
    uint8_t game_progress : 4;  // 比赛阶段（0：未开始；1：准备阶段；2：15s裁判系统自检；3：5s倒计时；4：比赛中；5：比赛结算中）
    uint16_t stage_remain_time; // 当前阶段剩余时间
    // unit64_t SyncTimeStamp;     // 服务器同步时间戳，当机器人正确连接到裁判系统的NTP服务器后生效，此处未使用
} ext_game_state_t;

// 0002 比赛结果数据
typedef __packed struct
{
    uint8_t winner; // 0：平局；1：红方胜利；2：蓝方胜利
} ext_game_result_t;

// 0003 比赛机器人血量数据
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
} ext_game_robot_HP_t;

// 0x0101 场地事件数据
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
} ext_event_data_t;

// 0x0104 裁判警告数据
typedef __packed struct
{
    uint8_t level;
    uint8_t offending_robot_id;
    uint8_t count;
} ext_referee_warning_t;

// 0x0105 飞镖发射相关数据
typedef __packed struct
{
    uint8_t dart_remaining_time;
    uint16_t dart_info;
} dart_info_t;

// 0x0201 机器人性能体系数据
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
} ext_game_robot_state_t;

// 0x0202 底盘缓冲能量和枪口热量数据
typedef __packed struct
{
    uint16_t chassis_volt;
    uint16_t chassis_current;
    float chassis_power;
    uint16_t chassis_power_buffer;
    uint16_t shooter_heat0;
    uint16_t shooter_heat1;
} ext_power_heat_data_t;

// 0x0203 机器人位置数据
typedef __packed struct
{
    float x;     // 本机器人位置x坐标，单位m
    float y;     // 本机器人位置y坐标，单位m
    float angle; // 本机器人测速模块的朝向，单位度（正北为0度）
} ext_game_robot_pos_t;

// 0x0204 机器人增益数据
typedef __packed struct
{
    uint8_t recovery_buff;      // 机器人回血增益（百分比，值为10表示每秒恢复血量上限的10%）
    uint8_t cooling_buff;       // 机器人射击热量冷却倍率（直接值，值为5表示5倍冷却）
    uint8_t defence_buff;       // 机器人防御增益（百分比，值为50表示50%防御增益）
    uint8_t vulnerability_buff; // 机器人负防御增益（百分比，值为30表示-30%防御增益）
    uint16_t attack_buff;       // 机器人攻击增益（百分比，值为50表示50%攻击增益）
    uint8_t remaining_energy;   // bit 0-4：机器人剩余能量值反馈，以16进制标识机器人剩余能量值比例，仅在机器人剩余能量小于50%时反馈，其余默认反馈0x32。（bit 0：在剩余能量≥50%时为1，其余情况为0；bit 1：在剩余能量≥30%时为1，其余情况为0；bit 2：在剩余能量≥15%时为1，其余情况为0；bit 3：在剩余能量≥5%时为1，其余情况为0Bit4：在剩余能量≥1%时为1，其余情况为0）
} ext_buff_musk_t;

// 0x0206 伤害状态数据
typedef __packed struct
{
    uint8_t armor_id : 4;            // bit 0-3：当扣血原因为装甲模块被弹丸攻击、受撞击、离线或测速模块离线时，该4 bit组成的数值为装甲模块或测速模块的ID编号；当其他原因导致扣血时，该数值为0
    uint8_t HP_deduction_reason : 4; // bit 4-7：血量变化类型（0-装甲模块被弹丸攻击导致扣血；1-裁判系统重要模块离线导致扣血；2-射击初速度超限导致扣血；3-射击热量超限导致扣血；4-底盘功率超限导致扣血；5-装甲模块受到撞击导致扣血）
} ext_robot_hurt_t;

// 0x0207 实时射击数据
typedef __packed struct
{
    uint8_t bullet_type;         // 弹丸类型（1：17mm；2：42mm）
    uint8_t shooter_number;      // 发射机构ID（1：第一个17mm发射机构，2：第二个17mm发射机构，3:42mm发射机构）
    uint8_t launching_frequency; // 弹丸射频，单位Hz
    float initial_speed;         // 弹丸初速度，单位m/s
} ext_shoot_data_t;

// 0x0208 允许发弹量
typedef __packed struct
{
    uint16_t projectile_allowance_17mm;
    uint16_t projectile_allowance_42mm;
    uint16_t remaining_gold_coin;
} ext_bullet_remaining_t;

// 0x0209 机器人RFID数据
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
} ext_rfid_status_t;

// 0x020A 飞镖选手端指令数据
typedef __packed struct
{
    uint8_t dart_launch_opening_status; // 当前飞镖发射站的状态：1-关闭，2-正在开启或关闭中，0-关闭
    uint8_t reserved;                   // 保留位
    uint16_t target_change_time;        // 切换击打目标时的比赛剩余时间，单位：秒。无/未切换动作，默认为0
    uint16_t latest_launch_cmd_time;    // 最后一次操作手确定发射指令时的比赛剩余时间，单位：秒，初始值为0
} dart_client_cmd_t;

// 0x020B 地面机器人位置数据
typedef __packed struct
{
    // 场地围挡在红方补给站附近的交点为坐标原点，沿场地长边向蓝方为 X 轴正方向，沿场地短边向红方停机坪为 Y 轴正方向
    float hero_x;       // 己方英雄机器人位置 x 轴坐标，单位：m
    float hero_y;       // 己方英雄机器人位置 y 轴坐标，单位：m
    float engineer_x;   // 己方工程机器人位置 x 轴坐标，单位：m
    float engineer_y;   // 己方工程机器人位置 y 轴坐标，单位：m
    float standard_3_x; // 己方 3 号步兵机器人位置 x 轴坐标，单位：m
    float standard_3_y; // 己方 3 号步兵机器人位置 y 轴坐标，单位：m
    float standard_4_x; // 己方 3 号步兵机器人位置 y 轴坐标，单位：m
    float standard_4_y; // 己方 4 号步兵机器人位置 y 轴坐标，单位：m
    float reserved1;     // 保留位
    float reserved2;     // 保留位
} ground_robot_position_t;

// 0x020C 雷达标记进度数据
typedef __packed struct
{
    // 在对应机器人被标记进度≥100 时发送 1，被标记进度<100 时发送 0
    uint8_t enemy_hero_mark_progress : 1;       // bit 0：对方 1 号英雄机器人易伤情况
    uint8_t enemy_engineer_mark_progress : 1;   // bit 1：对方 2 号工程机器人易伤情况
    uint8_t enemy_standard_3_mark_progress : 1; // bit 2：对方 3 号步兵机器人易伤情况
    uint8_t enemy_standard_4_mark_progress : 1; // bit 2：对方 3 号步兵机器人易伤情况
    uint8_t enemy_sentry_mark_progress : 1;     // bit 4：对方哨兵机器人易伤情况
} radar_mark_data_t;

// 0x020D 哨兵自主决策信息同步
typedef __packed struct
{
    // bit 0-10：除远程兑换外，哨兵机器人成功兑换的允许发弹量，开局为 0，在哨兵机器人成功兑换一定允许发弹量后，该值将变为哨兵机器人成功兑换的允许发弹量值。
    // bit 11-14：哨兵机器人成功远程兑换允许发弹量的次数，开局为 0，在哨兵机器人成功远程兑换允许发弹量后，该值将变为哨兵机器人成功远程兑换允许发弹量的次数。
    // bit 15-18：哨兵机器人成功远程兑换血量的次数，开局为 0，在哨兵机器人成功远程兑换血量后，该值将变为哨兵机器人成功远程兑换血量的次数。
    // bit 19：哨兵机器人当前是否可以确认免费复活，可以确认免费复活时值为1，否则为 0。
    // bit 20：哨兵机器人当前是否可以兑换立即复活，可以兑换立即复活时值为1，否则为 0。
    // bit 21-30：哨兵机器人当前若兑换立即复活需要花费的金币数。
    // bit 31：保留。
    uint32_t sentry_info;
    // bit 0：哨兵当前是否处于脱战状态，处于脱战状态时为 1，否则为 0。
    // bit 1-11：队伍 17mm 允许发弹量的剩余可兑换数。
    // bit 12-15：保留。
    uint16_t sentry_info_2;
} sentry_info_t;

// 0x020E 雷达自主决策信息同步
typedef __packed struct
{
    // bit 0-1：雷达是否拥有触发双倍易伤的机会，开局为 0，数值为雷达拥有触发双倍易伤的机会，至多为 2
    // bit 2：对方是否正在被触发双倍易伤，0-对方未被触发双倍易伤，1-对方正在被触发双倍易伤
    // bit 3-7：保留
    uint8_t radar_info;
} radar_info_t;

// 0x0301 机器人交互数据，发送方触发发送
typedef __packed struct
{
    uint16_t data_cmd_id;
    uint16_t sender_id;
    uint16_t receiver_id;
    uint8_t user_data[112];
} ext_student_interactive_data_t;

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
