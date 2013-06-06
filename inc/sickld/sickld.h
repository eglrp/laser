/*!
 * \file SickLD.h
 * \brief Defines the SickLD class for working with the
 *        Sick LD-OEM/LD-LRS long range LIDARs.
 *
 * Code by Jason C. Derenick and Thomas H. Miller.
 * Contact derenick(at)lehigh(dot)edu
 *
 * The Sick LIDAR Matlab/C++ Toolbox
 * Copyright (c) 2008, Jason C. Derenick and Thomas H. Miller
 * All rights reserved.
 *
 * This software is released under a BSD Open-Source License.
 * See http://sicktoolbox.sourceforge.net
 */

#ifndef SICK_LD_H
#define SICK_LD_H

#include "sickld_base.h"
#include "sick_message.h"
#include <ls300/hd_connect.h>

typedef struct sickld_t {
	hd_connect_t sick_connect;

	e_uint8 initialized;

	/** The current sensor mode */
	e_uint8 sensor_mode;

	/** The mode of the motor */
	e_uint8 motor_mode;

	/** Indicates whether the Sick LD is currently streaming range data */
	e_bool streaming_range_data;

	/** Indicates whether the Sick LD is currently streaming range and echo data */
	e_bool streaming_range_and_echo_data;

	/** The identity structure for the Sick */
	sick_ld_identity_t identity;

	/** The current global configuration for the unit */
	sick_ld_config_global_t global_config;

	/** The current Ethernet configuration for the unit */
	sick_ld_config_ethernet_t ethernet_config;

	/** The current sector configuration for the unit */
	sick_ld_config_sector_t sector_config;
} sickld_t;

/* 接口定义 */
#ifdef __cplusplus
extern "C" {
#endif

/** 构造函数 */
e_int32 DEV_EXPORT sld_create(sickld_t **sickld,char* ip,e_uint16 port);
e_int32 DEV_EXPORT sld_release(sickld_t **sick);

/** Initializes the Sick LD unit (use scan areas defined in flash) */
e_int32 DEV_EXPORT sld_initialize(sickld_t* sick);
e_int32 DEV_EXPORT sld_uninitialize(sickld_t* sick);

e_int32 DEV_EXPORT sld_set_temp_scan_areas(sickld_t *sick,
		const e_float64 * active_sector_start_angles,
		const e_float64 * active_sector_stop_angles,
		const e_uint32 num_active_sectors);

e_int32 DEV_EXPORT sld_set_global_params_and_scan_areas(sickld_t *sick,
		const e_uint32 sick_motor_speed, const e_float64 sick_angle_step,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors);

/** Acquires measurements and related data for all active sectors */
e_int32 DEV_EXPORT sld_get_measurements(sickld_t *sick,
		e_float64 * const range_measurements,
		e_uint32 * const echo_measurements,
		e_float64 * const angle_measurements,
		e_uint32 * const num_measurements,
		e_uint32 * const sector_ids,
		e_uint32 * const sector_data_offsets,
		e_float64 * const sector_step_angles,
		e_float64 * const sector_start_angles,
		e_float64 * const sector_stop_angles,
		e_uint32 * const sector_start_timestamps,
		e_uint32 * const sector_stop_timestamps);

e_int32 sld_get_next_message_from_datastream(sickld_t *sick,
		sick_message_t *sick_message);

e_int32 DEV_EXPORT sld_print_sector_config(sickld_t *sick);

/* 接口定义 */
#ifdef __cplusplus
}
#endif

#endif /* SICK_LD_H */
