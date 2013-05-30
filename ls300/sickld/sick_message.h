/*!
 * \file sick_message.h
 * \brief Defines the abstract parent class for all Sick messages.
 *
 * \brief 定义了到sick扫描仪连接的消息类
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */
#ifndef SICK_MESSAGE_H
#define SICK_MESSAGE_H

/* Dependencies */
#include "sickld_base.h"

#define MESSAGE_HEADER_LENGTH ((e_uint32)SICK_LD_MSG_HEADER_LEN)
#define MESSAGE_TRAILER_LENGTH ((e_uint32)SICK_LD_MSG_TRAILER_LEN)
#define MESSAGE_PAYLOAD_MAX_LENGTH ((e_uint32)SICK_LD_MSG_PAYLOAD_MAX_LEN)
#define MESSAGE_MAX_LENGTH  (MESSAGE_HEADER_LENGTH + MESSAGE_PAYLOAD_MAX_LENGTH + MESSAGE_TRAILER_LENGTH)

/*消息结构体定义*/
typedef struct {
	/** The length of the message payload in bytes */
	e_uint32 payload_length;
	/** The length of the message in bytes */
	e_uint32 message_length;
	/** The message as a raw sequence of bytes */
	e_uint8 message_buffer[MESSAGE_MAX_LENGTH];
	/** Indicates whether the message container/object is populated */
	e_bool populated;
	e_bool state;
} sick_message_t;

#ifdef __cplusplus
extern "C" {
#endif

/*创建*/
e_int32 DEV_EXPORT skm_create(sick_message_t *skm);
/*释放*/
void DEV_EXPORT skm_release(sick_message_t *skm);
/*可用*/
e_int32 DEV_EXPORT skm_state(sick_message_t *skm);
/*清空消息*/
void DEV_EXPORT skm_clear(sick_message_t *skm);
/*拷贝消息*/
void DEV_EXPORT skm_copy(sick_message_t *skm_to,
		const sick_message_t *skm_from);
/*创建消息*/
e_int32 DEV_EXPORT skm_build_message(sick_message_t *skm,
		const e_uint8 * const payload_buffer, const e_uint32 payload_length);
/*查询请求号*/
e_uint8 DEV_EXPORT skm_get_service_code(const sick_message_t *skm);
/*查询请求子号*/
e_uint8 DEV_EXPORT skm_get_service_subcode(const sick_message_t *skm);
/*查询校验值*/
e_uint8 DEV_EXPORT skm_get_checksum(const sick_message_t *skm);
/*填充状态*/
e_uint8 DEV_EXPORT skm_get_populated(const sick_message_t *skm);
/*导出消息*/
e_int32 DEV_EXPORT skm_get_message(const sick_message_t *skm,
		e_uint8* const message_buffer);

/*消息长度*/
e_int32 DEV_EXPORT skm_get_message_length(const sick_message_t *skm);

/*导出消息本体*/
e_int32 DEV_EXPORT skm_get_payload(const sick_message_t *skm,
		e_uint8 * const payload_buffer);
/*导出指定位置本体*/
e_int32 DEV_EXPORT skm_get_payload_subregion(const sick_message_t *skm,
		e_uint8 * const payload_sub_buffer, const e_uint32 start_idx,
		const e_uint32 stop_idx);

#ifdef __cplusplus
}
#endif

#endif /* SICK_MESSAGE_H */
