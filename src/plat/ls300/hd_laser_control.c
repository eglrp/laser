/*!
 * \file hd_laser_control.c
 * \brief	控制板通讯
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd laser base
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#include <ls300/hd_laser_control.h>
#include <arch/hd_timer_api.h>
#include <comm/hd_utils.h>

//TODO:协议修改:合并同类协议的返回值
//TODO:角度修正做到控制板,温度修正做到控制板

//#define ANGEL_PARAM (1 + 1.2 / 180)
#define ANGEL_PARAM 1

enum CONTROL_STATE
{
	CONTROL_STATE_NONE = 0,
	CONTROL_STATE_OPEN = 1,
	CONTROL_STATE_WORK = 3,
	CONTROL_STATE_COMMAND = CONTROL_STATE_OPEN, //COMMAND是无状态的，由会话自动同步
	CONTROL_STATE_CLOSE = CONTROL_STATE_NONE,
	CONTROL_STATE_MAX = 4,
};
//VALID STATE (CONTROL_STATE_NONE,CONTROL_STATE_MAX)

typedef enum CONTROL_REQUEST
{
	CONTROL_REQUEST_OPEN,
	CONTROL_REQUEST_CONFIG,
	CONTROL_REQUEST_WORK,
	CONTROL_REQUEST_WORK_SICK,
	CONTROL_REQUEST_WORK_PHOTO,
	CONTROL_REQUEST_WORK_TURN,
	CONTROL_REQUEST_COMMAND,
	CONTROL_REQUEST_COMMAND_PHOTO,
	CONTROL_REQUEST_COMMAND_LED,
	CONTROL_REQUEST_COMMAND_ANGLE,
	CONTROL_REQUEST_COMMAND_TEMPERATURE,
	CONTROL_REQUEST_COMMAND_DIP,
	CONTROL_REQUEST_COMMAND_BATTERY,
	CONTROL_REQUEST_CLOSE,
} CONTROL_REQUEST;

//------------------------------------------------------------------
//inter function
static e_int32 inter_take_photo(laser_control_t *lc);
static e_int32 inter_turn(laser_control_t *lc, e_uint32 speed, float angle);
static e_int32 inter_turn_by_step(laser_control_t *lc, e_uint32 speed, int step);
//state machine:		  ↓￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣↑
//						 none -> open -> start/work -> stop  -> close

#define THREAD_SAFE 1

e_int32 static hl_push_state(laser_control_t *lc, CONTROL_REQUEST request)
{
	int ret = E_OK;
#ifdef THREAD_SAFE
	if (!lc)
		return 0;
	switch (request)
	{
	case CONTROL_REQUEST_OPEN:
		ret = lc->state <= CONTROL_STATE_NONE || lc->state >= CONTROL_STATE_MAX;
		break;
	case CONTROL_REQUEST_CONFIG:
		ret = lc->state == CONTROL_STATE_OPEN;
		break;
	case CONTROL_REQUEST_WORK_TURN:
		case CONTROL_REQUEST_WORK_SICK:
		case CONTROL_REQUEST_WORK_PHOTO:
		ret = lc->state == CONTROL_STATE_OPEN;
		if (ret)
			lc->state = CONTROL_STATE_WORK;
		break;
	case CONTROL_REQUEST_COMMAND:
		case CONTROL_REQUEST_COMMAND_PHOTO:
		case CONTROL_REQUEST_COMMAND_LED:
		case CONTROL_REQUEST_COMMAND_ANGLE:
		case CONTROL_REQUEST_COMMAND_TEMPERATURE:
		case CONTROL_REQUEST_COMMAND_DIP:
		case CONTROL_REQUEST_COMMAND_BATTERY:
		ret = lc->state == CONTROL_STATE_OPEN
				|| lc->state == CONTROL_STATE_WORK;
		break;
	case CONTROL_REQUEST_CLOSE:
		ret = lc->state == CONTROL_STATE_OPEN;
		if (ret)
			lc->state = CONTROL_STATE_CLOSE;
		break;
	default:
		return E_ERROR;
	}
#else
	return lc&&lc->state;
#endif //THREAD_SAFE
	return ret;
}

e_int32 static hl_pop_state(laser_control_t *lc, CONTROL_REQUEST request)
{
	int ret = E_OK;
#ifdef THREAD_SAFE
	if (!lc)
		return 0;
	switch (request)
	{
	case CONTROL_REQUEST_OPEN:
		lc->state = CONTROL_STATE_OPEN;
		break;
	case CONTROL_REQUEST_CONFIG:
		break;
	case CONTROL_REQUEST_WORK_TURN:
		case CONTROL_REQUEST_WORK_SICK:
		case CONTROL_REQUEST_WORK_PHOTO:
		lc->state = CONTROL_STATE_OPEN;
		break;
	case CONTROL_REQUEST_COMMAND:
		case CONTROL_REQUEST_COMMAND_PHOTO:
		case CONTROL_REQUEST_COMMAND_LED:
		case CONTROL_REQUEST_COMMAND_ANGLE:
		case CONTROL_REQUEST_COMMAND_TEMPERATURE:
		case CONTROL_REQUEST_COMMAND_DIP:
		case CONTROL_REQUEST_COMMAND_BATTERY:
		break;
	case CONTROL_REQUEST_CLOSE:
		break;
	default:
		return E_ERROR;
	}
#else
	return lc&&lc->state;
#endif //THREAD_SAFE
	return ret;
}

//打开设备
e_int32 hl_open(laser_control_t *lc, char *com_name, e_uint32 baudrate)
{
	int ret;
	e_assert(lc, E_ERROR_INVALID_HANDLER);
	memset(lc, 0, sizeof(laser_control_t));
	ret = hl_push_state(lc, CONTROL_REQUEST_OPEN);
	e_assert(ret, E_ERROR_INVALID_STATUS);

	ret = sc_open_serial(&lc->serial_port, com_name, baudrate);
	e_assert(ret>0, ret);
	ret = sc_connect(&lc->serial_port);
	e_assert(ret>0, ret);
	ret = mm_init(&lc->monitor, &lc->serial_port);
	e_assert(ret>0, ret);
	ret = mm_start(&lc->monitor);
	e_assert(ret>0, ret);

	lc->fs_turntable = mm_create_socket(&lc->monitor, "Turnable"); //转盘
	lc->fs_angle = mm_create_socket(&lc->monitor, "Angle"); //倾角
	lc->fs_temperature = mm_create_socket(&lc->monitor, "Temperature"); //温度
	lc->fs_camera = mm_create_socket(&lc->monitor, "Camera"); //相机
	lc->fs_led = mm_create_socket(&lc->monitor, "Led"); //指示灯
	lc->fs_info = mm_create_socket(&lc->monitor, "Info"); //取控制板信息

	e_assert(
				lc->fs_turntable&&lc->fs_angle&&lc->fs_temperature&& lc->fs_camera&&lc->fs_led&&lc->fs_info,
				E_ERROR);

	lc->step_delay = FAST_SPEED;

	hl_pop_state(lc, CONTROL_REQUEST_OPEN);
	return E_OK;
}

e_int32 hl_open_socket(laser_control_t *lc, e_uint8 *ip, e_uint32 port)
{
	int ret;
	e_assert(lc, E_ERROR_INVALID_HANDLER);
	memset(lc, 0, sizeof(laser_control_t));
	ret = hl_push_state(lc, CONTROL_REQUEST_OPEN);
	e_assert(ret, E_ERROR_INVALID_STATUS);

	ret = sc_open_socket(&lc->serial_port, ip, port);
	e_assert(ret>0, ret);
	ret = sc_connect(&lc->serial_port);
	e_assert(ret>0, ret);
	ret = mm_init(&lc->monitor, &lc->serial_port);
	e_assert(ret>0, ret);
	ret = mm_start(&lc->monitor);
	e_assert(ret>0, ret);

	lc->fs_turntable = mm_create_socket(&lc->monitor, "Turnable"); //转盘
	lc->fs_angle = mm_create_socket(&lc->monitor, "Angle"); //倾角
	lc->fs_temperature = mm_create_socket(&lc->monitor, "Temperature"); //温度
	lc->fs_camera = mm_create_socket(&lc->monitor, "Camera"); //相机
	lc->fs_led = mm_create_socket(&lc->monitor, "Led"); //指示灯
	lc->fs_info = mm_create_socket(&lc->monitor, "Info"); //取控制板信息

	e_assert(
				lc->fs_turntable&&lc->fs_angle&&lc->fs_temperature&& lc->fs_camera&&lc->fs_led&&lc->fs_info,
				E_ERROR);

	lc->step_delay = FAST_SPEED;

	hl_pop_state(lc, CONTROL_REQUEST_OPEN);
	return E_OK;
}

//关闭设备
e_int32 hl_close(laser_control_t *lc)
{
	int ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_CLOSE);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	mm_stop(&lc->monitor);
	sc_close(&lc->serial_port);
	memset(lc, 0, sizeof(laser_control_t));
	hl_pop_state(lc, CONTROL_REQUEST_CLOSE);
	return E_OK;
}

//开始工作
e_int32 hl_turntable_prepare(laser_control_t *lc, e_uint32 pre_start_angle)
{
	int ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_WORK_TURN);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	//先转到开始位置,阻塞的
	ret = inter_turn_by_step(lc, FAST_SPEED, lc->start_steps
										- ANGLE_TO_STEP(pre_start_angle)); //冗余处理
	e_assert(ret>0, ret);
	hl_pop_state(lc, CONTROL_REQUEST_WORK_TURN);
	return E_OK;
}

//开始工作
e_int32 hl_turntable_start(laser_control_t *lc)
{
	int ret;
	e_uint8 buf[20] =
			{ 0 };

	ret = hl_push_state(lc, CONTROL_REQUEST_WORK_TURN);
	e_assert(ret, E_ERROR_INVALID_STATUS);

	//开始工作前，由速度和总步数结合得到该具体发送的命令,认为只要消息发送成功便是启动成功
	sprintf(buf, MOTO_MSG_F, (int) lc->real_steps + 200, (int) lc->step_delay); //冗余处理
	DMSG((STDOUT,"REQUEST turn:%s",buf));

	//不检测超时,基于转台工作时间较长,采用异步
	ret = fsocket_send(lc->fs_turntable, buf, strlen(buf), TIMEOUT_TURNTABLE);
	e_assert(ret>0, ret);

	lc->state = CONTROL_STATE_WORK;
	return E_OK;
}

//停止工作
e_int32 hl_turntable_stop(laser_control_t *lc)
{
	int ret;
	ret = lc->state == CONTROL_STATE_WORK;
	e_assert(ret, E_ERROR_INVALID_STATUS);

	DMSG((STDOUT,"Try StopWork\r\n"));

	//重置socket状态
	fsocket_reset(lc->fs_turntable);

	ret = fsocket_command(lc->fs_turntable, STOP_WORK, sizeof(STOP_WORK),
							HALT_MSG, sizeof(HALT_MSG), TIMEOUT_TURNTABLE);
	e_assert(ret>0, ret);

	hl_pop_state(lc, CONTROL_REQUEST_WORK_TURN);

	DMSG((STDOUT,"LASER CONTROL Stoped\r\n"));
	return E_OK;
}

e_int32 hl_turntable_is_stopped(laser_control_t *lc)
{
	e_assert(lc&&lc->state, E_ERROR_INVALID_STATUS);
	//hl_turntable_get_angle(lc);
	return (STEP_TO_ANGLE(lc->current_angle) >= lc->end_steps);
}
//设置转台参数//设置旋转速度,区域
e_int32 hl_turntable_config(laser_control_t *lc, e_uint32 step_delay,
		e_float64 start, e_float64 end)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_CONFIG);
	e_assert(ret, E_ERROR_INVALID_STATUS);

	lc->step_delay = step_delay;

	//扫描区域换算出步数
	//0到180度是18000步
	lc->start_steps = (e_uint32) ANGLE_TO_STEP(start);
	lc->end_steps = (e_uint32) ANGLE_TO_STEP(end);
	lc->real_steps = lc->end_steps - lc->start_steps;

	hl_pop_state(lc, CONTROL_REQUEST_CONFIG);
	return E_OK;
}

static e_int32 inter_turn_by_step(laser_control_t *lc, e_uint32 speed, int step)
{
	e_uint8 bufSend[40] =
			{ 0 };
	e_int32 ret;

	if (step == 0)
			{
		return E_OK;
	}

	lc->end_steps = abs(step);

	//每转一步的时间,调整角度时以最快速度1毫秒，迅速转到起始角度
	if (step > 0)
		sprintf(bufSend, MOTO_MSG_F, step, (int) speed);
	else
		sprintf(bufSend, MOTO_MSG_R, -step, (int) speed);
	DMSG((STDOUT,"REQUEST turn:%s\r\n",bufSend));

	ret = fsocket_command(lc->fs_turntable, bufSend, strlen(bufSend),
							MOTO_SUCCESS, sizeof(MOTO_SUCCESS), TIMEOUT_TURNTABLE * 18); //3分钟最长等待
	return ret;
}

static e_int32 inter_turn(laser_control_t *lc, e_uint32 speed, float angle)
{
	return inter_turn_by_step(lc, speed, (int) ANGLE_TO_STEP(angle));
}

//根据实际传过来的水平旋转角度，调整水平台，以较快速度转到实际水平台的起始角度/转台回到起始原点
e_int32 hl_turntable_turn(laser_control_t *lc, e_float64 angle)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_WORK_TURN);
	e_assert(ret, E_ERROR_INVALID_STATUS);

	ret = inter_turn(lc, lc->step_delay, angle);

	hl_pop_state(lc, CONTROL_REQUEST_WORK_TURN);
	return ret;
}

static e_int32 inter_camera_scan(laser_control_t *lc,
		e_float64 camera_angle_start, e_float64 camera_angle_end)
{
	e_float64 start_angle;
	e_int32 ret;

//打印配置信息
	DMSG((STDOUT,"hl_camera_scan picture ["));
	for (start_angle = camera_angle_start; start_angle < camera_angle_end;
			start_angle += TAKEPHOTO_ANGLE)
			{
		DMSG((STDOUT,"%f° - ",start_angle));
	}
	DMSG((STDOUT,"]\r\n"));

	lc->camera_angle_start = camera_angle_start;
	lc->camera_angle_end = camera_angle_end;

//开始照相
	DMSG((STDOUT,"hl_camera_scan starting....\r\n"));
	Delay(5000); //装相机时间

	DMSG(
			(STDOUT,"hl_camera_scan goto start positon [%f]°\r\n",camera_angle_start));
	ret = inter_turn(lc, FAST_SPEED, lc->camera_angle_start); //转至起始角
	e_assert(ret>0, ret);
	Delay(300); //拍照时间停留增加（由0.1秒改为0.3秒），保证充分聚焦
	ret = inter_take_photo(lc);
	e_assert(ret>0, ret);
	lc->picture_num = 1;
	DMSG((STDOUT,"hl_camera_scan take [%d] photos.\r\n",(int)lc->picture_num));
//每隔一定角度拍照一次
	for (start_angle = camera_angle_start; start_angle < camera_angle_end;
			start_angle += TAKEPHOTO_ANGLE)
			{
		ret = inter_turn(lc, FAST_SPEED, TAKEPHOTO_ANGLE);
		e_assert(ret>0, ret);
		Delay(300);
		ret = inter_take_photo(lc);
		e_assert(ret>0, ret);
		lc->picture_num++;
		DMSG(
				(STDOUT,"hl_camera_scan take [%d] photos.\r\n",(int)lc->picture_num));
	}

	return E_OK;
}

//设置相机参数/设置是否开启相机彩色扫描
e_int32 hl_camera_scan(laser_control_t *lc, e_float64 camera_angle_start,
		e_float64 camera_angle_end)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_WORK_PHOTO);
	e_assert(ret, E_ERROR_INVALID_STATUS);

	ret = inter_camera_scan(lc, camera_angle_start, camera_angle_end);

	hl_pop_state(lc, CONTROL_REQUEST_WORK_PHOTO);
	return ret;
}

static e_int32 inter_take_photo(laser_control_t *lc)
{
	e_int32 ret;
	ret = fsocket_command(lc->fs_turntable, TAKEPHOTO, sizeof(TAKEPHOTO),
							PHOTO_SUCCESS, sizeof(PHOTO_SUCCESS), TIMEOUT_CAMERA);
	return ret;
}

//相机拍照
e_int32 hl_camera_take_photo(laser_control_t *lc)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_WORK_PHOTO);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_take_photo(lc);
	hl_pop_state(lc, CONTROL_REQUEST_WORK_PHOTO);
	return ret;
}

static e_int32 inter_get_angle(laser_control_t *lc, float *angle)
{
	e_int32 ret;
	int step = EINT_MIN;
	e_uint8 buf[MSG_MAX_LEN] =
			{ 0 };
	ret =
			fsocket_request_failed(lc->fs_turntable, GET_STEP, sizeof(GET_STEP),
									buf, sizeof(buf), GET_ERROR, sizeof(GET_ERROR), TIMEOUT_STEP);
	e_assert(ret>0, ret);

	sscanf(buf, MSG_RET_STEP, &step);
//	DMSG((STDOUT,"\tCURRENT ANGLE %f\n",STEP_TO_ANGLE(step)));
	(*angle) = STEP_TO_ANGLE(step) / ANGEL_PARAM; //水平角度补偿后，要求返回角度值不变
	return E_OK;
}

//获取当前水平转台的角度
e_float64 hl_turntable_get_angle(laser_control_t *lc)
{
	float angle = 0;
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND_ANGLE);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_get_angle(lc, &angle);
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND_ANGLE);
	e_assert(ret>0, E_ERROR);
	lc->current_angle = angle;
	return angle;
}

static e_int32 inter_get_temperature(laser_control_t *lc, float *value)
{
	e_int32 ret;
	e_uint8 buf[MSG_MAX_LEN] =
			{ 0 };
	ret = fsocket_request_failed(lc->fs_turntable, GET_TEMPERATURE,
									sizeof(GET_TEMPERATURE), buf, sizeof(buf), GET_ERROR,
									sizeof(GET_ERROR), TIMEOUT_TEMPERATURE);
	e_assert(ret>0, ret);

	sscanf(buf, MSG_RET_TEMPERATURE, value);
	return E_OK;
}

//获取温度
e_float64 hl_get_temperature(laser_control_t *lc)
{
	float value = 0;
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND_TEMPERATURE);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_get_temperature(lc, &value);
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND_TEMPERATURE);
	e_assert(ret>0, E_ERROR);
	return value;
}

static e_int32 inter_get_dip(laser_control_t *lc, float *v1, float *v2)
{
	e_int32 ret;
	int vi[2] =
			{ 0 };
	e_uint8 buf[MSG_MAX_LEN] =
			{ 0 };
	ret =
			fsocket_request_failed(lc->fs_turntable, GET_ANGLE, sizeof(GET_ANGLE),
									buf, sizeof(buf), GET_ERROR, sizeof(GET_ERROR), TIMEOUT_ANGLE);
	e_assert(ret>0, ret);

	sscanf(buf, MSG_RET_ANGLE, vi, vi + 1);

	(*v1) = VOLTATE_TO_DIP(vi[0]);
	(*v2) = VOLTATE_TO_DIP(vi[1]);

	return E_OK;
}

//获取倾斜度
e_int32 hl_get_dip(laser_control_t *lc, angle_t* angle)
{
	float value[2] =
			{ 0 };
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND_DIP);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_get_dip(lc, value, value + 1);
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND_DIP);
	e_assert(ret>0, E_ERROR);
	angle->dX = value[0];
	angle->dY = value[1];
	return E_OK;
}

static e_int32 inter_get_battery(laser_control_t *lc, float *value)
{
	e_int32 ret;
	e_uint8 buf[MSG_MAX_LEN] =
			{ 0 };
	ret =
			fsocket_request_failed(lc->fs_turntable, GET_BATTERY,
									sizeof(GET_BATTERY), buf, sizeof(buf), GET_ERROR, sizeof(GET_ERROR),
									TIMEOUT_INFO);
	e_assert(ret>0, ret);

	sscanf(buf, MSG_RET_BATTERY, value);
	(*value) *= 10; //返回的数据是/10过的
	return E_OK;
}

//获取状态
e_float64 hl_get_battery(laser_control_t *lc)
{
	float value = 0;
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND_BATTERY);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_get_battery(lc, &value);
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND_BATTERY);
	e_assert(ret>0, E_ERROR);
	return value;
}

static e_int32 inter_led(laser_control_t *lc, const e_uint8 *msg,
		e_uint32 msg_len)
{
	e_int32 ret;
	e_uint8 buf[MSG_MAX_LEN] =
			{ 0 };
	ret =
			fsocket_request_failed(lc->fs_turntable, msg, msg_len, buf,
									sizeof(buf), LED_ERROR, sizeof(LED_ERROR), TIMEOUT_LED);
	e_assert(ret>0, ret);
	return E_OK;
}

//亮红灯
e_int32 hl_led_red(laser_control_t *lc)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND_LED);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_led(lc, SET_LEDRED, sizeof(SET_LEDRED));
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND_LED);
	e_assert(ret>0, E_ERROR);
	return E_OK;
}

//亮绿灯
e_int32 hl_led_green(laser_control_t *lc)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND_LED);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_led(lc, SET_LEDGREEN, sizeof(SET_LEDGREEN));
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND_LED);
	e_assert(ret>0, E_ERROR);
	return E_OK;
}

//LED熄灭
e_int32 hl_led_off(laser_control_t *lc)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND_LED);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_led(lc, SET_LEDOFF, sizeof(SET_LEDOFF));
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND_LED);
	e_assert(ret>0, E_ERROR);
	return E_OK;
}

//搜索零点
static e_int32 inter_searchzero(laser_control_t *lc)
{
	e_int32 ret;
	e_uint8 buf[MSG_MAX_LEN] =
			{ 0 };
	ret = fsocket_request_success(lc->fs_turntable, SEARCH_ZERO,
									sizeof(SEARCH_ZERO), buf, sizeof(buf), SEARCH_SUCCESS,
									sizeof(SEARCH_SUCCESS), TIMEOUT_SEARCH_ZERO);
	e_assert(ret>0, ret);
	return E_OK;
}
//以最快速度调整到指定水平范围起始角
e_int32 hl_SearchZero(laser_control_t *lc)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_WORK_TURN);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_searchzero(lc);
	hl_pop_state(lc, CONTROL_REQUEST_WORK_TURN);
	e_assert(ret>0, E_ERROR);
	return E_OK;
}

//搜索零点
static e_int32 inter_getinfo(laser_control_t *lc, int idx, e_uint8 *buf,
		e_uint32 len)
{
	e_int32 ret;
	ret =
			fsocket_request_failed(lc->fs_turntable, GET_INFO[idx],
									strlen(GET_INFO[idx]), buf, len, GET_ERROR, sizeof(GET_ERROR),
									TIMEOUT_INFO);
	e_assert(ret>0, ret);
	return E_OK;
}

//硬件信息
e_int32 hl_get_info(laser_control_t *lc, e_uint32 idx, e_uint8* buffer,
		e_int32 blen)
{
	e_int32 ret;
	ret = hl_push_state(lc, CONTROL_REQUEST_COMMAND);
	e_assert(ret, E_ERROR_INVALID_STATUS);
	ret = inter_getinfo(lc, idx, buffer, blen);
	hl_pop_state(lc, CONTROL_REQUEST_COMMAND);
	e_assert(ret>0, E_ERROR);
	return E_OK;
}

