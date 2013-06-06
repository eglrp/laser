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

#include <comm/hd_utils.h>

/**
 * \brief Compute the checksum (single-byte XOR).
 * \param data The address of the first data element in a sequence of bytes to be included in the sum
 * \param length The number of byte in the data sequence
 */
e_uint8 hd_compute_xor(const e_uint8 * const data, const e_uint32 length) {
	e_uint32 i;
	/* Compute the XOR by summing all of the bytes */
	e_uint8 checksum = 0;
	for (i = 0; i < length; i++) {
		checksum ^= data[i]; // NOTE: this is equivalent to simply summing all of the bytes
	}

	/* done */
	return checksum;
}
