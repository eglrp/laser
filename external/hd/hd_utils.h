/*!
 * \file hd_utils.h
 * \brief 
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#ifndef HD_UTILS_H
#define HD_UTILS_H

/* Dependencies */
#include <hd_plat_base.h>
/*结构体定义*/


/*接口定义*/
#ifdef __cplusplus
extern "C"
{
#endif

/**
 * \brief Compute the message checksum (single-byte XOR).
 * \param data The address of the first data element in a sequence of bytes to be included in the sum
 * \param length The number of byte in the data sequence
 */
e_uint8 DEV_EXPORT hd_compute_xor(const e_uint8 * const data, const e_uint32 length);

#ifdef __cplusplus
}
#endif

#endif /*HD_UTILS_H*/
