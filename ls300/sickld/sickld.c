#include <math.h>
#include "sickld.h"
#include <hd_timer_api.h>

static e_float64 sld_get_scan_resolution(sickld_t *sick);
static e_int32 sld_sync_driver_with_sick(sickld_t *sick);
static e_int32 sld_sort_scan_areas(e_float64 * const sector_start_angles,
		e_float64 * const sector_stop_angles, const e_uint32 num_sectors);
static e_float64 sld_compute_mean_pulse_frequency(
		const e_float64 active_scan_area, const e_float64 curr_motor_speed,
		const e_float64 curr_angular_resolution);
static e_float64 sld_compute_scan_area(const e_float64 sick_angle_step,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors);
static e_int32 sld_valid_active_sectors(
		const e_float64 * const sector_start_angles,
		const e_float64 * const sector_stop_angles, const e_uint32 num_sectors);
static e_int32 sld_set_temporary_scan_areas(sickld_t *sick,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors);
static e_uint32 sld_get_motor_speed(sickld_t *sick);
static e_int32 sld_valid_pulse_frequency_ex(const e_uint32 sick_motor_speed,
		const e_float64 sick_angle_step,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors);
static e_int32 sld_generate_sector_config(
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors, const e_float64 sick_angle_step,
		e_uint32 * const sector_functions, e_float64 * const sector_stop_angles,
		e_uint32 * num_sectors);
static e_int32 sld_set_sector_config(sickld_t *sick,
		const e_uint32 * const sector_functions,
		const e_float64 * const sector_stop_angles, const e_uint32 num_sectors,
		const e_bool write_to_flash);
static e_float64 sld_compute_max_pulse_frequency(
		const e_float64 total_scan_area, const e_float64 curr_motor_speed,
		const e_float64 curr_angular_resolution);
static e_int32 sld_set_sector_function(sickld_t *sick,
		const e_uint8 sector_number, const e_uint8 sector_function,
		const e_float64 sector_stop_angle, const e_bool write_to_flash);
static e_int32 sld_get_sector_config(sickld_t *sick);
static e_int32 sld_get_sector_function(sickld_t *sick, const e_uint8 sector_num,
		e_uint8 *sector_function, e_float64 *sector_stop_angle);
static e_int32 sld_send_message_and_getreply(sickld_t *sick,
		const sick_message_t *send_message, sick_message_t *recv_message);
static e_int32 sld_set_sensor_mode(sickld_t *sick,
		const e_uint8 new_sick_sensor_mode);
static e_int32 sld_set_sensor_mode_to_rotate(sickld_t *sick);
static e_int32 sld_set_sensor_mode_to_measure(sickld_t *sick);
static e_int32 sld_set_sensor_mode_to_idle(sickld_t *sick);
static e_float64 sld_ticks2angle(const e_uint16 ticks);
static e_uint16 sld_angle2ticks(const e_float64 angle);
static e_int32 sld_cancel_scan_profiles(sickld_t *sick);
static e_uint8 sld_sensor_mode_to_work_service_subcode(
		const e_uint8 sick_sensor_mode);
static e_int32 sld_recv_message(sickld_t *sick, sick_message_t *sick_message,
		const e_uint32 timeout_value);
static e_uint8* sld_sensor_mode_to_string(const e_uint8 sick_sensor_mode);
static e_int32 sld_set_signals(sickld_t *sick, const e_uint8 sick_signal_flags);
static e_int32 sld_teardown_connection(sickld_t *sick);
static e_int32 sld_quick_request(sickld_t *sick,
		e_uint8 *payload_buffer/*[in,out]*/, e_uint32 recv_len);
static e_int32 sld_get_status(sickld_t *sick);
static e_int32 sld_get_identity(sickld_t *sick);
static e_int32 sld_get_ethernet_config(sickld_t *sick);
static e_int32 sld_get_global_config(sickld_t *sick);
static e_int32 sld_print_init_footer(sickld_t *sick);

/***
 * \brief sickld constructor
 * \param[in,out] sickld return pointer to sickld_t
 * \param sick_ip_address sick's ipaddress
 * \param sick_tcp_port port
 * \return E_RESULT
 */
e_int32 sld_create(sickld_t **sickld, char* ip, e_uint16 port)
{
	e_int32 ret;
	sickld_t* sick = (sickld_t*) malloc(sizeof(sickld_t));
	e_assert(sick, E_ERROR_BAD_ALLOCATE);
	memset(sick, 0, sizeof(sickld_t));

	ret = sc_open_socket(&(*sickld)->sick_connect, ip, port);
	e_assert(ret>0, ret);

	sick->sensor_mode = SICK_SENSOR_MODE_UNKNOWN;
	sick->motor_mode = SICK_MOTOR_MODE_UNKNOWN;
	sick->streaming_range_data = false;
	sick->streaming_range_and_echo_data = false;

	(*sickld) = sick;

	return E_OK;
}

e_int32 sld_release(sickld_t **sickld)
{
	e_assert(sickld, E_ERROR_INVALID_HANDLER);
	free(*sickld);
	(*sickld) = NULL;
	return E_OK;
}

/***
 * \brief  Initializes the Sick LD unit (use scan areas defined in flash)
 * \param[in] sickld pointer to sickld_t
 * \return E_RESULT
 */
e_int32 sld_initialize(sickld_t* sick)
{
	e_int32 ret;
	DMSG((STDOUT,"\t*** Attempting to initialize the Sick LD...\r\n"));
	/*assert sick is not null*/
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	/* Ok, lets sync the driver with the Sick */
	DMSG((STDOUT,"\tAttempting to sync driver with Sick LD...\r\n"));
	ret = sld_sync_driver_with_sick(sick);
	if (ret <= 0)
		return ret;

	DMSG((STDOUT,"\t\tSynchronized!\r\n"));

	sick->initialized = true;
	sld_print_init_footer(sick);

	return ret;
}

/**
 * \brief Tear down the connection between the host and the Sick LD
 */
e_int32 sld_uninitialize(sickld_t* sick)
{
	e_int32 ret;

	DMSG((STDOUT,"\t*** Attempting to uninitialize the Sick LD...\r\n"));

	/*assert sick is not null*/
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Ensure the device has been initialized */

	if (e_check(sick->initialized,"Device NOT Initialized!!!"))
	{
		return E_ERROR_INVALID_STATUS;
	}

	/* If necessary, tell the Sick LD to stop streaming data */
	DMSG((STDOUT, "\tSetting Sick LD to idle mode...\r\n"));
	ret = sld_set_sensor_mode_to_idle(sick);
	e_assert(ret>0, ret);
	DMSG((STDOUT,"\t\tSick LD is now idle!\r\n"));

	/* Clear any signals that were set */
	ret = sld_set_signals(sick, DEFAULT_SICK_SIGNAL_SET);
	e_assert(ret>0, ret);

	/* Attempt to close the tcp connection */
	DMSG((STDOUT, "\tClosing connection to Sick LD...\r\n"));
	ret = sld_teardown_connection(sick);
	e_assert(ret>0, ret);

	DMSG((STDOUT, "\t\tConnection closed!\r\n"));
	DMSG( (STDOUT, "\t*** Uninit. complete - Sick LD is now offline!\r\n"));

	/* Mark the device as uninitialized */
	sick->initialized = false;

	return E_OK;
}

static e_int32 sld_sync_driver_with_sick(sickld_t *sick)
{
	e_int32 ret;
	/* Acquire current configuration */
	ret = sld_get_status(sick);
	e_assert(ret>0, ret);
	ret = sld_get_identity(sick);
	e_assert(ret>0, ret);
	ret = sld_get_ethernet_config(sick);
	e_assert(ret>0, ret);
	ret = sld_get_global_config(sick);
	e_assert(ret>0, ret);
	ret = sld_get_sector_config(sick);
	e_assert(ret>0, ret);

	/* Reset Sick signals */
	ret = sld_set_signals(sick, DEFAULT_SICK_SIGNAL_SET);
	e_assert(ret>0, ret);
	return E_OK;
}

e_int32 sld_set_temp_scan_areas(sickld_t *sick,
		const e_float64 * active_sector_start_angles,
		const e_float64 * active_sector_stop_angles,
		const e_uint32 num_active_sectors)
{
	int ret;
	/* Ensure the device has been initialized */
	e_assert(sick->initialized, E_ERROR_INVALID_STATUS);

	/* Do the standard initialization */

	/* Set the temporary scan configuration */
	DMSG((STDOUT,"\tAttempting to set desired scan config...\r\n"));
	ret = sld_set_temporary_scan_areas(sick, active_sector_start_angles,
			active_sector_stop_angles, num_active_sectors);
	e_assert(ret>0, ret);

	DMSG((STDOUT,"\t\tUsing desired scan area(s)!\r\n"));

	return E_OK;
}

/**
 * \brief Attempts to set the "temporary" (until a device reset) scan area config for the device
 * \param active_sector_start_angles Angles marking the beginning of each desired active sector/area
 * \param active_sector_stop_angles Angles marking the end of each desired active sector/area
 * \param num_active_sectors The number of active sectors
 */
static e_int32 sld_set_temporary_scan_areas(sickld_t *sick,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Define buffers to hold the device-ready configuration */
	e_uint32 num_sectors = 0;
	e_uint32 sector_functions[SICK_MAX_NUM_SECTORS] =
	{ 0 };
	e_float64 sector_stop_angles[SICK_MAX_NUM_SECTORS] =
	{ 0 };

	/* A few dummy buffers */
	e_float64 sorted_active_sector_start_angles[SICK_MAX_NUM_SECTORS] =
	{ 0 };
	e_float64 sorted_active_sector_stop_angles[SICK_MAX_NUM_SECTORS] =
	{ 0 };

	/* Begin by checking the num of active sectors */
	e_assert( (num_active_sectors <= SICK_MAX_NUM_SECTORS/2),
			E_ERROR_INVALID_PARAMETER);

	/* Copy the input arguments */
	memcpy(sorted_active_sector_start_angles, active_sector_start_angles,
			sizeof(sorted_active_sector_start_angles));
	memcpy(sorted_active_sector_stop_angles, active_sector_stop_angles,
			sizeof(sorted_active_sector_stop_angles));

	/* Ensure a proper ordering of the given sector angle sets */
	ret = sld_sort_scan_areas(sorted_active_sector_start_angles,
			sorted_active_sector_stop_angles, num_active_sectors);

	/* Check for an invalid configuration */
	ret = sld_valid_active_sectors(sorted_active_sector_start_angles,
			sorted_active_sector_stop_angles, num_active_sectors);
	if (e_failed(ret,"Invalid sector configuration!"))
		return ret;

	/* Ensure the resulting pulse frequency is valid for the device */
	ret = sld_valid_pulse_frequency_ex(sld_get_motor_speed(sick),
			sld_get_scan_resolution(sick), sorted_active_sector_start_angles,
			sorted_active_sector_stop_angles, num_active_sectors);
	if (e_failed(ret,"Invalid pulse frequency!"))
		return ret;

	/* Generate the corresponding device-ready sector config */
	ret = sld_generate_sector_config(sorted_active_sector_start_angles,
			sorted_active_sector_stop_angles, num_active_sectors,
			sld_get_scan_resolution(sick), sector_functions, sector_stop_angles,
			&num_sectors);
	e_assert(ret>0, E_ERROR_INVALID_PARAMETER);

	/* Set the new sector configuration */
	ret = sld_set_sector_config(sick, sector_functions, sector_stop_angles,
			num_sectors, 0);
	e_assert(ret>0, E_ERROR_INVALID_PARAMETER);
	return E_OK;
}

/**
 * \brief Check that the given profile format is supported by the current driver version.
 * \param profile_format The requested profile format.
 * \return True if the given scan profile is supported by the driver
 */
e_int32 sld_supported_scan_profile_format(const e_uint16 profile_format)
{
	/* Check the supplied scan profile format */
	switch (profile_format)
	{
	case SICK_SCAN_PROFILE_RANGE:
		return E_OK;
	case SICK_SCAN_PROFILE_RANGE_AND_ECHO:
		return E_OK;
	default:
		return E_ERROR;
	}
}

/**
 * \brief Converts the Sick LD numerical motor mode to a representative string
 * \param profile_format The profile format to be converted
 * \return The corresponding string
 */
e_uint8* sld_profile_format_to_string(const e_uint16 profile_format)
{
	switch (profile_format)
	{
	case SICK_SCAN_PROFILE_RANGE:
		return "RANGE ONLY";
	case SICK_SCAN_PROFILE_RANGE_AND_ECHO:
		return "RANGE + ECHO";
	default:
		return "UNRECOGNIZED!!!";
	}
}

/**
 * \brief Request n scan profiles from the Sick LD unit
 * \param profile_format The format for the requested scan profiles
 * \param num_profiles The number of profiles to request from Sick LD. (Default: 0)
 *                     (NOTE: When num_profiles = 0, the Sick LD continuously streams profile data)
 */
e_int32 sld_get_scan_profiles(sickld_t *sick, const e_uint16 profile_format,
		const e_uint16 num_profiles)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Ensure the device is in measurement mode */

	ret = sld_set_sensor_mode_to_measure(sick);
	e_assert(ret>0, ret);

	/* A quick check to ensure the requested format is supported by the driver */
	ret = sld_supported_scan_profile_format(profile_format);
	if (e_check(ret>0,"Unsupported profile format!"))
		return ret;
	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service code and subcode */
	payload_buffer[0] = SICK_MEAS_SERV_CODE;
	payload_buffer[1] = SICK_MEAS_SERV_GET_PROFILE;

	/* Write the number of profiles to request to the payload buffer */
	e_uint16 temp_buffer = host_to_sick_ld_byte_order16(num_profiles);
	memcpy(&payload_buffer[2], &temp_buffer, 2);

	/* Set the profile format mask (for now, we request everything from the Sick LD) */
	temp_buffer = profile_format;
	temp_buffer = host_to_sick_ld_byte_order16(temp_buffer);
	memcpy(&payload_buffer[4], &temp_buffer, 2);

	ret = sld_quick_request(sick, payload_buffer, 6);
	e_assert(ret>0, ret);

	/* Check to see if there was an error */
	/* Check to make sure the returned format is correct and there were no errors */
	memcpy(&temp_buffer, &payload_buffer[2], 2);
	temp_buffer = sick_ld_to_host_byte_order16(temp_buffer);

	/* Another sanity check */
	if (e_check(temp_buffer == profile_format,"Incorrect profile format was returned by the Sick LD!"))
		return E_ERROR;

	/* Check if the data stream flags need to be set */
	if (num_profiles == 0 && profile_format == SICK_SCAN_PROFILE_RANGE)
	{
		sick->streaming_range_data = true;
	}
	else if (num_profiles
			== 0&& profile_format == SICK_SCAN_PROFILE_RANGE_AND_ECHO)
	{
		sick->streaming_range_and_echo_data = true;
	}

	/* Show some output */
	if (num_profiles == 0)
	{
		DMSG((STDOUT,"\t\tData stream started!\r\n"));
	}
	else
	{
		DMSG((STDOUT,"\t\tSick LD sending %u scan profiles!\r\n",num_profiles));
	}

	return E_OK;
}

/**
 * \brief Parses a well-formed sequence of bytes into a corresponding scan profile
 * \param *src_buffer The source data buffer
 * \param &profile_data The destination data structure
 */
e_int32 sld_parse_scan_profile(e_uint8 * const src_buffer,
		sick_ld_scan_profile_t *profile_data)
{

	e_uint16 profile_format = 0;
	e_uint32 data_offset = 0, i, j;

	/* Extract the scan profile format from the buffer */
	memcpy(&profile_format, &src_buffer[data_offset], 2);
	profile_format = sick_ld_to_host_byte_order16(profile_format);
	data_offset += 2;

	/* Extract the number of sectors in the scan area */
	profile_data->num_sectors = src_buffer[data_offset + 1];
	data_offset += 2;

	/* NOTE: For the following field definitions see page 32 of the
	 *       Sick LD telegram listing.
	 */
	e_uint16 temp_buffer; // A temporary buffer

	/* Check if PROFILESENT is included */
	if (profile_format & 0x0001)
	{
		memcpy(&temp_buffer, &src_buffer[data_offset], 2);
		profile_data->profile_number =
				sick_ld_to_host_byte_order16( temp_buffer);
		data_offset += 2;
	}

	/* Check if PROFILECOUNT is included */
	if (profile_format & 0x0002)
	{
		memcpy(&temp_buffer, &src_buffer[data_offset], 2);
		profile_data->profile_counter =
				sick_ld_to_host_byte_order16( temp_buffer);
		data_offset += 2;
	}

	/* Check if LAYERNUM is included */
	if (profile_format & 0x0004)
	{
		memcpy(&temp_buffer, &src_buffer[data_offset], 2);
		profile_data->layer_num = sick_ld_to_host_byte_order16(temp_buffer);
		data_offset += 2;
	}

	/* The extraneous stuff is out of the way, now extract the data
	 * for each of the sectors in the scan area...
	 */
	for (i = 0; i < profile_data->num_sectors; i++)
	{

		/* Check if SECTORNUM is included */
		if (profile_format & 0x0008)
		{
			memcpy(&temp_buffer, &src_buffer[data_offset], 2);
			profile_data->sector_data[i].sector_num =
					sick_ld_to_host_byte_order16(temp_buffer);
			data_offset += 2;
		}
		else
		{
			profile_data->sector_data[i].sector_num = 0;
		}

		/* Check if DIRSTEP is included */
		if (profile_format & 0x0010)
		{
			memcpy(&temp_buffer, &src_buffer[data_offset], 2);
			profile_data->sector_data[i].angle_step =
					((e_float64) sick_ld_to_host_byte_order16(temp_buffer))
							/ 16;
			data_offset += 2;
		}
		else
		{
			profile_data->sector_data[i].angle_step = 0;
		}

		/* Check if POINTNUM is included */
		if (profile_format & 0x0020)
		{
			memcpy(&temp_buffer, &src_buffer[data_offset], 2);
			profile_data->sector_data[i].num_data_points =
					sick_ld_to_host_byte_order16(temp_buffer);
			data_offset += 2;
		}
		else
		{
			profile_data->sector_data[i].num_data_points = 0;
		}

		/* Check if TSTART is included */
		if (profile_format & 0x0040)
		{
			memcpy(&temp_buffer, &src_buffer[data_offset], 2);
			profile_data->sector_data[i].timestamp_start =
					sick_ld_to_host_byte_order16(temp_buffer);
			data_offset += 2;
		}
		else
		{
			profile_data->sector_data[i].timestamp_start = 0;
		}

		/* Check if STARTDIR is included */
		if (profile_format & 0x0080)
		{
			memcpy(&temp_buffer, &src_buffer[data_offset], 2);
			profile_data->sector_data[i].angle_start =
					((e_float64) sick_ld_to_host_byte_order16(temp_buffer))
							/ 16;
			data_offset += 2;
		}
		else
		{
			profile_data->sector_data[i].angle_start = 0;
		}

		/* Acquire the range and echo values for the sector */
		for (j = 0; j < profile_data->sector_data[i].num_data_points; j++)
		{

			/* Check if DISTANCE-n is included */
			if (profile_format & 0x0100)
			{
				memcpy(&temp_buffer, &src_buffer[data_offset], 2);
				profile_data->sector_data[i].range_values[j] =
						((e_float64) sick_ld_to_host_byte_order16(temp_buffer))
								/ 256;
				data_offset += 2;
			}
			else
			{
				profile_data->sector_data[i].range_values[j] = 0;
			}

			/* Check if DIRECTION-n is included */
			if (profile_format & 0x0200)
			{
				memcpy(&temp_buffer, &src_buffer[data_offset], 2);
				profile_data->sector_data[i].scan_angles[j] =
						((e_float64) sick_ld_to_host_byte_order16(temp_buffer))
								/ 16;
				data_offset += 2;
			}
			else
			{
				profile_data->sector_data[i].scan_angles[j] = 0;
			}

			/* Check if ECHO-n is included */
			if (profile_format & 0x0400)
			{
				memcpy(&temp_buffer, &src_buffer[data_offset], 2);
				profile_data->sector_data[i].echo_values[j] =
						sick_ld_to_host_byte_order16(temp_buffer);
				data_offset += 2;
			}
			else
			{
				profile_data->sector_data[i].echo_values[j] = 0;
			}

		}

		/* Check if TEND is included */
		if (profile_format & 0x0800)
		{
			memcpy(&temp_buffer, &src_buffer[data_offset], 2);
			profile_data->sector_data[i].timestamp_stop =
					sick_ld_to_host_byte_order16(temp_buffer);
			data_offset += 2;
		}
		else
		{
			profile_data->sector_data[i].timestamp_stop = 0;
		}

		/* Check if ENDDIR is included */
		if (profile_format & 0x1000)
		{
			memcpy(&temp_buffer, &src_buffer[data_offset], 2);
			profile_data->sector_data[i].angle_stop =
					((e_float64) sick_ld_to_host_byte_order16(temp_buffer))
							/ 16;
			data_offset += 2;
		}
		else
		{
			profile_data->sector_data[i].angle_stop = 0;
		}

	}

	/* Check if SENSTAT is included */
	if (profile_format & 0x2000)
	{
		profile_data->sensor_status = src_buffer[data_offset + 3] & 0x0F;
		profile_data->motor_status = (src_buffer[data_offset + 3] >> 4) & 0x0F;
	}
	else
	{
		profile_data->sensor_status = SICK_SENSOR_MODE_UNKNOWN;
		profile_data->motor_status = SICK_MOTOR_MODE_UNKNOWN;
	}
	return E_OK;
}

/**
 * \brief Acquires measurements and corresponding sector data from the Sick LD.
 * \param *range_measurements      A single array to hold ALL RANGE MEASUREMENTS from the current scan for all active
 *                                 sectors. Range values from each sector are stored block sequentially in this buffer.
 *                                 The respective index into the array marking the first data value returned by active
 *                                 sector i can be found by getting: range_measurement[sector_data_offsets[i]].
 * \param *echo_measurements       A single array to hold ALL ECHO MEASUREMENTS from the current scan for all active sectors.
 *                                 It is indexed the same as range_measurements. This argument is optional and if it is not
 *                                 provided the driver will request a RANGE ONLY data stream as opposed to a RANGE+ECHO thereby
 *                                 reducing consumed bandwidth (Default: NULL).
 * \param *num_measurements        An array where the ith element denotes the number of range/echo measurements obtained from
 *                                 active sector i (Default: NULL).
 * \param *sector_ids              An array where the ith element corresponds to the actual sector number/id of the ith active
 *                                 sector. (Default: NULL)
 * \param *sector_data_offsets     The index offsets mapping the ith active sector to the position in range_measurements
 *                                 where its first measured value can be found (Default: NULL).
 * \param *sector_step_angles      An array where the ith element corresponds to the angle step for the ith active sector.
 *                                 (Default: NULL)
 * \param *sector_start_angles     An array where the ith element corresponds to the starting scan angle of the ith active
 *                                 sector.
 * \param *sector_stop_angles      An array where the ith element corresponds to the stop scan angle of the ith active sector.
 * \param *sector_start_timestamps An array where the ith element denotes the time at which the first scan was taken for
 *                                 the ith active sector.
 * \param *sector_stop_timestamps  An array where the ith element denotes the time at which the last scan was taken for
 *                                 the ith active sector.
 *
 * ALERT: The user is responsible for ensuring that enough space is allocated for the return buffers to avoid overflow.
 *        See the example code for an easy way to do this.
 */
e_int32 sld_get_measurements(sickld_t *sick,
		e_float64 * const range_measurements,
		e_uint32 * const echo_measurements,
		e_float64 * const angle_measurements, e_uint32 * const num_measurements,
		e_uint32 * const sector_ids, e_uint32 * const sector_data_offsets,
		e_float64 * const sector_step_angles,
		e_float64 * const sector_start_angles,
		e_float64 * const sector_stop_angles,
		e_uint32 * const sector_start_timestamps,
		e_uint32 * const sector_stop_timestamps)
{
	e_int32 ret = E_OK;
	e_uint32 i, total_measurements;
	/* Ensure the device has been initialized */
	e_assert(sick&&sick->initialized, E_ERROR_INVALID_HANDLER);

	/* The following conditional holds true if the user wants a RANGE+ECHO data
	 * stream but already has an active RANGE-ONLY stream.
	 */
	if (sick->streaming_range_data && echo_measurements != NULL)
	{
		/* Cancel the current RANGE-ONLY data stream */
		ret = sld_cancel_scan_profiles(sick);
		e_assert(ret>0, ret);

		/* Request a RANGE+ECHO data stream */
		ret = sld_get_scan_profiles(sick, SICK_SCAN_PROFILE_RANGE_AND_ECHO, 0);
		e_assert(ret>0, ret);
	}

	/* The following conditional holds true if the user wants a RANGE-ONLY data
	 * stream but already has an active RANGE+ECHO stream.
	 */
	if (sick->streaming_range_and_echo_data && echo_measurements == NULL)
	{

		/* Cancel the current RANGE-ONLY data stream */
		ret = sld_cancel_scan_profiles(sick);
		e_assert(ret>0, ret);

		/* Request a RANGE+ECHO data stream */
		ret = sld_get_scan_profiles(sick, SICK_SCAN_PROFILE_RANGE, 0);
		e_assert(ret>0, ret);
	}

	/* If there aren't any active data streams, setup a new one */
	if (!sick->streaming_range_data && !sick->streaming_range_and_echo_data)
	{
		/* Determine the target data stream by checking the value of echo_measurements */
		if (echo_measurements != NULL)
		{
			/* Request a RANGE+ECHO data stream */
			ret = sld_get_scan_profiles(sick, SICK_SCAN_PROFILE_RANGE_AND_ECHO,
					0);
			e_assert(ret>0, ret);
		}
		else
		{
			/* Request a RANGE+ONLY data stream */
			ret = sld_get_scan_profiles(sick, SICK_SCAN_PROFILE_RANGE, 0);
			e_assert(ret>0, ret);
		}
	}

	/* Declare the receive message object */
	sick_message_t recv_message;
	ret = skm_create(&recv_message);
	e_assert(ret>0, ret);

	/* Acquire the most recently buffered message */
	ret = sld_recv_message(sick, &recv_message, (e_uint32) 1e6);
	if (e_failed(ret))
		goto OUT;

	/* A single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Get the message payload */
	ret = skm_get_payload(&recv_message, payload_buffer);
	if (e_failed(ret))
		goto OUT;

	/* Define the destination Sick LD scan profile struct */
	sick_ld_scan_profile_t profile_data;

	/* Extract the scan profile */
	ret = sld_parse_scan_profile(&payload_buffer[2], &profile_data);
	if (e_failed(ret))
		goto OUT;

	/* Update and check the returned sensor status */
	if ((sick->sensor_mode = profile_data.sensor_status)
			!= SICK_SENSOR_MODE_MEASURE)
	{
		DMSG(
				(STDOUT,"Unexpected sensor mode! %s \r\n", sld_sensor_mode_to_string(sick->sensor_mode)));
		ret = E_ERROR_INVALID_STATUS;
		goto OUT;
	}

	/* Update and check the returned motor status */
	if ((sick->motor_mode = profile_data.motor_status) != SICK_MOTOR_MODE_OK)
	{
		DMSG(
				(STDOUT,"Unexpected motor mode! (Are you using a valid motor speed!)"));
		ret = E_ERROR_INVALID_STATUS;
		goto OUT;

	}

	/* Everything is OK, so now populate the relevant return buffers */
	for (i = 0, total_measurements = 0;
			i < sick->sector_config.sick_num_active_sectors; i++)
	{

		/* Copy over the returned range values */
		memcpy(&range_measurements[total_measurements],
				profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].range_values,
				profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].num_data_points
						* sizeof(e_float64));

		/* Copy the returned echo values  if requested */
		if (echo_measurements != NULL)
		{
			memcpy(&echo_measurements[total_measurements],
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].echo_values,
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].num_data_points
							* sizeof(e_uint32));
		}

		//add
		if (angle_measurements != NULL)
		{
			memcpy(&angle_measurements[total_measurements],
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].scan_angles,
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].num_data_points
							* sizeof(e_float64));
		}

		/* Set the number of measurements */
		if (num_measurements != NULL)
		{
			num_measurements[i] =
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].num_data_points;
		}

		/* Set the associated sector's id if requested */
		if (sector_ids != NULL)
		{
			sector_ids[i] =
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].sector_num;
		}

		/* Set the associated sector's index into the range measurement buffer if requested */
		if (sector_data_offsets != NULL)
		{
			sector_data_offsets[i] = total_measurements;
		}

		/* Set the step angle if requested */
		if (sector_step_angles != NULL)
		{
			sector_step_angles[i] =
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].angle_step;
		}

		/* Set the sector start angle if requested */
		if (sector_start_angles != NULL)
		{
			sector_start_angles[i] =
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].angle_start;
		}

		/* Set the sector stop angle if requested */
		if (sector_stop_angles != NULL)
		{
			sector_stop_angles[i] =
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].angle_stop;
		}

		/* Set the sector start timestamp if requested */
		if (sector_start_timestamps != NULL)
		{
			sector_start_timestamps[i] =
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].timestamp_start;
		}

		/* Set the sector stop timestamp if requested */
		if (sector_stop_timestamps != NULL)
		{
			sector_stop_timestamps[i] =
					profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].timestamp_stop;
		}

		/* Update the total number of measurements */
		total_measurements +=
				profile_data.sector_data[sick->sector_config.sick_active_sector_ids[i]].num_data_points;
	}

	OUT: skm_release(&recv_message);
	return ret;
}

/**
 * \brief Sort the scan areas based on the given angles to place them in device "scan" order
 * \param sector_start_angles Array of angles (deg) defining the starting position of each active sector
 * \param sector_stop_angles Array of angles (deg) defining the stopping position of each active sector
 * \param num_active_sectors Number of active sectors
 */
static e_int32 sld_sort_scan_areas(e_float64 * const sector_start_angles,
		e_float64 * const sector_stop_angles, const e_uint32 num_sectors)
{

	/* A dummy temp variable */
	e_float64 temp = 0;
	e_uint32 i, j;

	/* Employ a simple bubblesort (NOTE: Only at most a handful of values will have to be sorted) */
	for (i = 0; i < num_sectors; i++)
	{
		for (j = (num_sectors - 1); j > i; j--)
		{
			if (sector_start_angles[j] < sector_start_angles[j - 1])
			{
				SWAP_VALUES(sector_start_angles[j], sector_start_angles[j-1],
						temp);
				SWAP_VALUES(sector_stop_angles[j], sector_stop_angles[j-1],
						temp);
			}
		}
	}
	return E_OK;
}

/**
 * \brief Determines wheter a given set of sector bounds are valid.
 * \param sector_start_angles Array of angles (deg) defining the starting position of each active sector
 * \param sector_stop_angles Array of angles (deg) defining the stopping position of each active sector
 * \param num_active_sectors Number of active sectors
 */
static e_int32 sld_valid_active_sectors(
		const e_float64 * const sector_start_angles,
		const e_float64 * const sector_stop_angles, const e_uint32 num_sectors)
{
	e_uint32 i;
	/* A sanity check to make sure all are in [0,360) */
	for (i = 0; i < num_sectors; i++)
	{
		if (e_check((sector_start_angles[i] >= 0 || sector_stop_angles[i] >= 0 || sector_start_angles[i] < 360 || sector_stop_angles[i] < 360),"Invalid sector config! (all degree values must be in [0,360))"))
		{
			return E_ERROR_INVALID_PARAMETER;
		}
	}

	/* If multiple sectors are defined */
	if (num_sectors > 1)
	{
		/* Check whether the given sector arrangement is overlapping */
		for (i = 0; i < (num_sectors - 1); i++)
		{
			if (e_check((sector_start_angles[i] <= sector_stop_angles[i] || sector_stop_angles[i] < sector_start_angles[i + 1]),"Invalid sector definitions! (check sector bounds)"))
			{
				return E_ERROR_INVALID_PARAMETER;
			}
		}
		/* Check the last sector against the first */
		if (e_check((sector_stop_angles[num_sectors - 1] > sector_start_angles[num_sectors - 1] || sector_stop_angles[num_sectors - 1] < sector_start_angles[0]),"Invalid sector definitions! (check sector bounds)"))
		{
			return E_ERROR_INVALID_PARAMETER;
		}
	}
	/* Valid! */
	return E_OK;
}

/**
 * \brief Checks whether the given sick motor speed is valid for the device.
 * \param sick_motor_speed The sick motor speed (Hz)
 */
e_int32 sld_valid_motor_speed(const e_uint32 sick_motor_speed)
{
	/* Check the validity of the new Sick LD motor speed */
	if (sick_motor_speed < SICK_MIN_MOTOR_SPEED
			|| sick_motor_speed > SICK_MAX_MOTOR_SPEED)
	{
		return E_ERROR;
	}
	/* Success */
	return E_OK;
}

/**
 * \brief Checks whether the given scan resolution is valid
 * \param sick_scan_resolution Scan resolution of the device
 * \param sector_start_angles An array of the sector start angles
 * \param sector_stop_angles An array of the sector stop angles
 */
e_int32 sld_valid_scan_resolution(const e_float64 sick_angle_step,
		const e_float64 * const sector_start_angles,
		const e_float64 * const sector_stop_angles, const e_uint32 num_sectors)
{
	e_uint32 i;
	/* Check the validity of the new Sick LD angular step */
	if (sick_angle_step < SICK_MAX_SCAN_ANGULAR_RESOLUTION
			|| fmod(sick_angle_step, SICK_MAX_SCAN_ANGULAR_RESOLUTION) != 0)
	{
		DMSG(
				(STDOUT, "Invalid scan resolution! (should be a positive multiple of %f)\r\n",SICK_MAX_SCAN_ANGULAR_RESOLUTION));
		return E_ERROR;
	}

	/* Ensure that the sector boundaries are divisible by the desired step angle */
	for (i = 0; i < num_sectors; i++)
	{

		/* Check both the sector start and stop angles */
		if (fmod(sector_start_angles[i], sick_angle_step) != 0
				|| fmod(sector_stop_angles[i], sick_angle_step) != 0)
		{
			DMSG(
					(STDOUT, "Invalid scan resolution! (sector boundaries must be evenly divisible by the step angle)\r\n"));
			return E_ERROR;
		}

	}

	/* Success */
	return E_OK;
}

/**
 * \brief Checks whether the given configuration yields a valid mean and max pulse frequency (uses current sector config)
 * \param sick_motor_speed Desired sick motor speed (Hz)
 * \param sick_angle_step Desired scan angular resolution (deg)
 */
static e_int32 sld_valid_pulse_frequency(sickld_t *sick,
		const e_uint32 sick_motor_speed, const e_float64 sick_angle_step)
{
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	/* Simply call the other function w/ the current sector config and the given motor and step angle values */
	return sld_valid_pulse_frequency_ex(sick_motor_speed, sick_angle_step,
			sick->sector_config.sick_sector_start_angles,
			sick->sector_config.sick_sector_stop_angles,
			sick->sector_config.sick_num_active_sectors);
}

/**
 * \brief Checks whether the given configuration yields a valid mean and max pulse frequency (uses given sector config)
 * \param sick_motor_speed Desired sick motor speed (Hz)
 * \param sick_angle_step Desired scan angular resolution (deg)
 * \param active_sector_start_angles Angles marking the beginning of each desired active sector/area
 * \param active_sector_stop_angles Angles marking the end of each desired active sector/area
 * \param num_active_sectors The number of active sectors/scan areas are given
 */
static e_int32 sld_valid_pulse_frequency_ex(const e_uint32 sick_motor_speed,
		const e_float64 sick_angle_step,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors)
{

	/* Compute the scan area */
	e_float64 scan_area = sld_compute_scan_area(sick_angle_step,
			active_sector_start_angles, active_sector_stop_angles,
			num_active_sectors);

	/* Check the mean pulse rate of the desired configuration */
	if (sld_compute_mean_pulse_frequency(scan_area, sick_motor_speed,
			sick_angle_step) > SICK_MAX_MEAN_PULSE_FREQUENCY)
	{
		DMSG(
				(STDOUT,"Max mean pulse frequency exceeded! (try a slower motor speed,"
				"a larger step angle and/or a smaller active scan area)\r\n"));
		return E_ERROR;
	}

	/* Check the maximum pulse rate of the desired configuration */
	if (sld_compute_max_pulse_frequency(SICK_MAX_SCAN_AREA, sick_motor_speed,
			sick_angle_step) > SICK_MAX_PULSE_FREQUENCY)
	{
		DMSG((STDOUT, "Max pulse frequency exceeded! (try a slower motor speed,"
		"a larger step angle and/or a smaller active scan area)\r\n"));
		return E_ERROR;
	}

	/* Valid! */
	return E_OK;
}

/**
 * \brief Computes the active scan area for the Sick given the current
 *        sector configuration
 * \param sick_angle_step The angular resolution of the Sick LD
 * \param active_sector_start_angles The start angles for the active scan sectors
 * \param active_sector_stop_angles The stop angles for the active scan sectors
 * \param num_active_sectors The number of active sectors
 * \return The Sick LD scan area corresponding to the given bounds
 *
 * NOTE: The Sick LD computes scan area by subtracting the end of the
 *       previous sector from the end of the current sector.  As such, we have
 *       to add a single step angle to our computation in order to get the scan
 *       area that is used by the Sick LD.  Unfortunately, this is how the device
 *       does its computation (as opposed to using the angle at which the first
 *       scan in the given sector is taken) so this is how we do it.
 *
 */
static e_float64 sld_compute_scan_area(const e_float64 sick_angle_step,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors)
{

	/* Define the current scan area */
	e_float64 total_scan_area = 0;
	e_float64 curr_sector_scan_area = 0;
	e_uint32 i;

	/* For each sector given sum the absolute scan area for it */
	for (i = 0; i < num_active_sectors; i++)
	{

		/* Compute the total scan area for this sector */
		curr_sector_scan_area = fabs(
				active_sector_start_angles[i] - active_sector_stop_angles[i]);

		/* Update the total scan area */
		total_scan_area += curr_sector_scan_area + sick_angle_step;
	}

	/* Return the computed area */
	return total_scan_area;

}

/**
 * \brief Compute the mean pulse frequency (see page 22 of the operator's manual)
 * \param active_scan_area The total area where the Sick is actively scanning (in deg) (i.e. total area where the laser isn't blanked)
 * \param curr_motor_speed The current motor speed (in Hz)
 * \param curr_angular_resolution The current angular resolution of the Sick (in deg)
 * \return The mean pulse frequency for the given configuration parameters
 */
static e_float64 sld_compute_mean_pulse_frequency(
		const e_float64 active_scan_area, const e_float64 curr_motor_speed,
		const e_float64 curr_angular_resolution)
{
	/* Compute the mean pulse frequency */
	return sld_compute_max_pulse_frequency(SICK_MAX_SCAN_AREA, curr_motor_speed,
			curr_angular_resolution)
			* (active_scan_area / ((e_float64) SICK_MAX_SCAN_AREA));
}

/**
 * \brief Compute the mean pulse frequency (see page 22 of the operator's manual)
 * \param active_scan_area The total scan area that can be covered by the Sick
 * \param curr_motor_speed The current motor speed (in Hz)
 * \param curr_angular_resolution The current angular resolution of the Sick (in deg)
 * \return The maximum pulse frequency for the given configuration parameters
 */
static e_float64 sld_compute_max_pulse_frequency(
		const e_float64 total_scan_area, const e_float64 curr_motor_speed,
		const e_float64 curr_angular_resolution)
{
	/* Compute the maximum pulse frequency */
	return total_scan_area * curr_motor_speed * (1 / curr_angular_resolution);
}

/**
 * \brief Acquire the Sick LD's current scan resolution
 * \return The Sick LD scan resolution
 */
static e_float64 sld_get_scan_resolution(sickld_t *sick)
{
	e_assert(sick&&sick->initialized, E_ERROR);
	return sick->global_config.sick_angle_step;
}

/**
 * \brief Sets the sector configuration for the device
 * \param sector_functions Angles marking the beginning of each desired active sector/area
 * \param sector_stop_angles Angles marking the end of each desired active sector/area
 * \param num_sectors The total number of sectors in the configuration
 * \param set_flash_flag Indicates whether to mark the sectors for writing to flash w/ the next SET_CONFIG (global)
 */
static e_int32 sld_set_sector_config(sickld_t *sick,
		const e_uint32 * const sector_functions,
		const e_float64 * const sector_stop_angles, const e_uint32 num_sectors,
		const e_bool write_to_flash)
{
	int ret;
	e_uint32 sector_id;
	/* Assign the new sector configuration to the device */
	for (sector_id = 0; sector_id < num_sectors; sector_id++)
	{
		/* Set the corresponding sector function */
		ret = sld_set_sector_function(sick, sector_id,
				sector_functions[sector_id], sector_stop_angles[sector_id],
				write_to_flash);
		e_assert(ret>0, ret);
		/* Resync the driver with the new sector configuration */
		ret = sld_get_sector_config(sick);
		e_assert(ret>0, ret);
	}

	return E_OK;
}

/**
 * \brief Query the Sick for its current sector configuration.
 *
 * NOTE: Here we only buffer the sector stop angle as this is the
 *       only value returned by the GET_FUNCTION command. Using the
 *       stop angle does not give enough information to extract the
 *       start angle of sectors that aren't "normal measurement".
 */
static e_int32 sld_get_sector_config(sickld_t *sick)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	e_uint32 i;
	/* Reset the sector config struct */
	memset(&sick->sector_config, 0, sizeof(sick_ld_config_sector_t));

	/* Get the configuration for all initialized sectors */
	for (i = 0; i < SICK_MAX_NUM_SECTORS; i++)
	{
		/* Query the Sick for the function of the ith sector */
		ret = sld_get_sector_function(sick, i,
				&sick->sector_config.sick_sector_functions[i],
				&sick->sector_config.sick_sector_stop_angles[i]);
		e_assert(ret>0, ret);

		/* Check if the sector is initialized */
		if (sick->sector_config.sick_sector_functions[i]
				!= SICK_CONF_SECTOR_NOT_INITIALIZED)
		{

			/* Check whether the sector is active (i.e. measuring) */
			if (sick->sector_config.sick_sector_functions[i]
					== SICK_CONF_SECTOR_NORMAL_MEASUREMENT)
			{
				sick->sector_config.sick_active_sector_ids[sick->sector_config.sick_num_active_sectors] =
						i;
				sick->sector_config.sick_num_active_sectors++;
			}

			/* Update the number of initialized sectors */
			sick->sector_config.sick_num_initialized_sectors++;
		}
		else
		{

			/* An uninitialized sector marks the end of the sector configuration */
			break;
		}

	}

	/* Compute the starting angle for each of the initialized sectors */
	for (i = 1; i < sick->sector_config.sick_num_initialized_sectors; i++)
	{
		sick->sector_config.sick_sector_start_angles[i] = fmod(
				sick->sector_config.sick_sector_stop_angles[i - 1]
						+ sick->global_config.sick_angle_step, 360);
	}

	/* Determine the starting angle for the first sector */
	if (sick->sector_config.sick_num_initialized_sectors > 1)
	{
		sick->sector_config.sick_sector_start_angles[0] =
				fmod(
						sick->sector_config.sick_sector_stop_angles[sick->sector_config.sick_num_initialized_sectors
								- 1] + sick->global_config.sick_angle_step,
						360);
	}

	return E_OK;
}

/**
 * \brief Sends a message to the Sick device
 * \param &sick_message A reference to the well-formed message that is to be sent to the Sick
 * \param byte_interval Minimum time in microseconds between transmitted bytes
 */
static e_int32 sld_send_message(sickld_t *sick,
		const sick_message_t * sick_message, const e_uint32 byte_interval)
{
	e_uint32 i;
	int ret = E_OK;
	e_uint32 len;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	e_uint8 message_buffer[MESSAGE_MAX_LENGTH] =
	{ 0 };

	/* Copy the given message and get the message length */
	skm_get_message(sick_message, message_buffer);
	e_uint32 message_length = skm_get_message_length(sick_message);

	/* Check whether a transmission delay between bytes is requested */
	if (byte_interval == 0)
	{

		/* Write the message to the stream */
		len = sc_send(&sick->sick_connect, message_buffer, message_length);
		if (e_check(len==message_length,"send failed."))
			goto OUT;
	}
	else
	{

		/* Write the message to the unit one byte at a time */
		for (i = 0; i < message_length; i++)
		{

			/* Write a single byte to the stream */
			len = sc_send(&sick->sick_connect, &message_buffer[i], 1);
			if (e_check(len==1,"send failed."))
				goto OUT;

			/* Some time between bytes (Sick LMS 2xx likes this) */
			Delay(byte_interval);
		}

	}
	OUT: return ret;
}

/*in us*/
static e_uint32 sld_compute_elapsed_time(const e_uint32 beg_time,
		e_uint32 end_time)
{
	return (end_time - beg_time);
}

/**
 * \brief Attempt to acquire the latest available message from the device
 * \param &sick_message A reference to the container that will hold the most recent message
 * \param timeout_value The time in microsecond to wait before throwing a timeout error
 * \return True if a new message was received, False otherwise
 */
static e_int32 sld_recv_message(sickld_t *sick, sick_message_t *sick_message,
		const e_uint32 timeout_value)
{
	int ret = E_OK;
	e_uint32 len;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Timeval structs for handling timeouts */
	e_uint32 beg_time, end_time;

	/* Acquire the elapsed time since epoch */
	beg_time = GetTickCount();

	/* Check the shared object */
#ifdef SICKLD_MOITOR_MULT_THREAD
	ret = sick_get_next_message_from_monitor(sick, sick_message);
#else
	ret = sld_get_next_message_from_datastream(sick, sick_message);
#endif

	while (ret <= 0)
	{

		/* Sleep a little bit */
		Delay(1000);

		/* Check whether the allowed time has expired */
		end_time = GetTickCount();
		len = sld_compute_elapsed_time(beg_time, end_time);
		if (e_check(len<=timeout_value,"Timeout occurred!"))
		{
			return E_ERROR_TIME_OUT;
		}
#ifdef SICKLD_MOITOR_MULT_THREAD
		ret = sick_get_next_message_from_monitor(sick, sick_message);
#else
		ret = sld_get_next_message_from_datastream(sick, sick_message);
#endif
	}

	return E_OK;
}

/**
 * \brief Attempt to acquire a message having a payload beginning w/ the given byte sequence
 * \param &sick_message A reference to the container that will hold the most recent message
 * \param *byte_sequence The byte sequence that is expected to lead off the payload in the packet (e.g. service codes, etc...)
 * \param byte_sequence_length The number of bytes in the given byte_sequence
 * \param timeout_value The time in ms to wait before throwing a timeout error
 * \return True if a new message was received, False otherwise
 *
 * NOTE: This method is intended to be a helper for _sendMessageAndGetReply
 */
e_int32 sld_recv_message_ex(sickld_t *sick, sick_message_t *sick_message,
		const e_uint8 * const byte_sequence,
		const e_uint32 byte_sequence_length, const e_uint32 timeout_value)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Define a buffer */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH];

	/* Timeval structs for handling timeouts */
	e_uint32 beg_time, end_time;

	/* A container for the message */
	sick_message_t curr_message;
	ret = skm_create(&curr_message);
	if (e_failed(ret))
	{
		skm_release(&curr_message);
		return ret;
	}

	/* Get the elapsed time since epoch */
	beg_time = GetTickCount();

	/* Check until it is found or a timeout */
	for (;;)
	{

		/* Attempt to acquire the message */
		e_uint32 i = 0;
#ifdef SICKLD_MOITOR_MULT_THREAD
		ret = sick_get_next_message_from_monitor(sick,
				&curr_message);
#else
		ret = sld_get_next_message_from_datastream(sick, &curr_message);
#endif
		if (ret > 0)
		{
			/* Extract the payload subregion */
			ret = skm_get_payload_subregion(&curr_message, payload_buffer, 0,
					byte_sequence_length - 1);
			e_assert(ret>0, ret);

			/* Match the byte sequence */
			for (i = 0;
					(i < byte_sequence_length)
							&& (payload_buffer[i] == byte_sequence[i]); i++)
				;

			/* Our message was found! */
			if (i == byte_sequence_length)
			{
				skm_copy(sick_message, &curr_message);
				break;
			}

		}

		/* Check whether the allowed time has expired */
		end_time = GetTickCount();
		DMSG(
				(STDOUT,"++++++++++++++TIME ELAPSED beg_time %u,end_time %u", (e_uint32)beg_time, (e_uint32)end_time));
		i = sld_compute_elapsed_time(beg_time, end_time);
		if (e_check(i<=timeout_value,"Timeout occurred!"))
		{
			return E_ERROR_TIME_OUT;
		}
		/* Sleep a little bit */
		Delay(1000);
	}

	skm_release(&curr_message);
	return E_OK;
}

/**
 * \param sick_send_frame A sick frame to be sent to the LMS
 * \param sick_receive_frame A sick frame to hold the response (expected or unexpected) of the LMS
 * \param num_tries The number of times to send the frame in the event the LMS fails to reply
 * \param timeout The epoch to wait before considering a sent frame lost
 * \return True if the message was sent and the expected reply was received
 */
static e_int32 sld_send_message_and_getreply_ex(sickld_t *sick,
		const sick_message_t *send_message, sick_message_t *recv_message,
		const e_uint8 * const byte_sequence,
		const e_uint32 byte_sequence_length, const e_uint32 byte_interval,
		const e_uint32 timeout_value, const e_uint32 num_tries)
{
	e_uint32 i;
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Send the message for at most num_tries number of times */
	for (i = 0; i < num_tries; i++)
	{
		/* Send the frame to the unit */
		ret = sld_send_message(sick, send_message, byte_interval);
		e_assert(ret>0, ret);
		/* Wait for the reply! */
		ret = sld_recv_message_ex(sick, recv_message, byte_sequence,
				byte_sequence_length, timeout_value);
		if (ret > 0)
		{
			return E_OK;/* message was found! */
		}
		else if (ret == E_ERROR_TIME_OUT)
		{/* Handle a timeout! */
			/* Check if it was found! */
			if (e_check(i<num_tries-1,"Attempted max number of tries w/o failed!"))
				return ret;
			/* Display the number of tries remaining! */
			DMSG((STDOUT,"%d tries remaining",(int)(num_tries - i - 1)));
		}
		else
		{
			return ret;
		}
	}
	return ret;
}

/**
 * \brief Send a message to the Sick LD and get its reply.
 * \param &send_message A reference to the well-formed message object that is to be sent.
 * \param &recv_message The destination message object for the received reply.
 * \param timeout_value The maximum time to allow for a response in seconds
 *
 * NOTE: This method also verifies the correct reply to the given send_message
 *       object is received.  So, if it fails, it may be due to an unexpected reply.
 */
static e_int32 sld_send_message_and_getreply(sickld_t *sick,
		const sick_message_t *send_message, sick_message_t *recv_message)
{
	e_uint8 code;
	e_uint8 byte_sequence[2] =
	{ 0 };
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	code = skm_get_service_code(send_message);
	e_assert(code!=0, code);
	byte_sequence[0] = code | 0x80;
	code = skm_get_service_subcode(send_message);
	e_assert(code!=0, code);
	byte_sequence[1] = code;

	/* Send message and get reply using full support method */
	ret = sld_send_message_and_getreply_ex(sick, send_message, recv_message,
			byte_sequence, 2, 0, DEFAULT_SICK_MESSAGE_TIMEOUT, 1);
	e_assert(ret>0, ret);

	return E_OK;
}

/** \brief Sets the function for a particular scan sector.
 *  \param sector_number The number of the sector (should be in [0,7])
 *  \param sector_function The function of the sector (e.g. no measurement, reserved, normal measurement, ...)
 *  \param sector_stop_angle The last angle of the sector (in odometer ticks)
 *  \param write_to_flash Indicates whether the sector configuration should be written to flash
 */
static e_int32 sld_set_sector_function(sickld_t *sick,
		const e_uint8 sector_number, const e_uint8 sector_function,
		const e_float64 sector_stop_angle, const e_bool write_to_flash)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Ensure the device is not measuring */
	if (sick->sensor_mode == SICK_SENSOR_MODE_MEASURE)
	{
		/* Set the Sick LD to rotate mode */
		ret = sld_set_sensor_mode_to_rotate(sick);
		e_assert(ret>0, ret);
	}

	/* Ensure a valid sector number */
	if (e_check(sector_number < SICK_MAX_NUM_SECTORS,"Invalid sector number!"))
		return E_ERROR;

	/* Check that a valid sector_function was given */
	if (e_check((sector_function == SICK_CONF_SECTOR_NOT_INITIALIZED || sector_function == SICK_CONF_SECTOR_NO_MEASUREMENT ||
					sector_function == SICK_CONF_SECTOR_RESERVED || sector_function == SICK_CONF_SECTOR_NORMAL_MEASUREMENT ||
					sector_function == SICK_CONF_SECTOR_REFERENCE_MEASUREMENT), "Invalid sector function code!"))
		return E_ERROR;

	/* Check that a valid stop angle was given */
	if (e_check(sector_stop_angle<=SICK_MAX_SCAN_AREA,"Invalid sector stop angle!"))
		return E_ERROR;

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* A temporary buffer for byte order conversion */
	e_uint16 temp_buff = 0;

	/* Set the service IDs */
	payload_buffer[0] = SICK_CONF_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_CONF_SERV_SET_FUNCTION; // Requested service subtype

	/* Assign the payload data */
	payload_buffer[3] = sector_number; // SECTORNUM
	payload_buffer[5] = sector_function; // SECTORFUNC

	/* Set the sector stop value */
	temp_buff =
			host_to_sick_ld_byte_order16( sld_angle2ticks(sector_stop_angle));
	memcpy(&payload_buffer[6], &temp_buff, 2); // SECTORSTOP

	/* Include the flash flag */
	payload_buffer[9] = (e_uint8) write_to_flash; // FLASHFLAG

	/* Create the Sick LD messages */
	ret = sld_quick_request(sick, payload_buffer, 10);
	e_assert(ret>0, ret);

	/* Check the response for an error */
	if (e_check((payload_buffer[2] != 0xFF || payload_buffer[3] != 0xFF),"Invalid request!"))
		return E_ERROR;

	return E_OK;
}

/**
 * \brief Acquires the function of the given sector
 * \param sector_num The target sector number
 * \param &sector_config The configuration word returned by the Sick LD
 * \param &sector_stop The stop angle of the given sector
 */
static e_int32 sld_get_sector_function(sickld_t *sick, const e_uint8 sector_num,
		e_uint8 *sector_function, e_float64 *sector_stop_angle)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Ensure the device is not measuring */
	if (sick->sensor_mode == SICK_SENSOR_MODE_MEASURE)
	{
		/* Set the Sick LD to rotate mode */
		ret = sld_set_sensor_mode_to_rotate(sick);
		e_assert(ret>0, ret);
	}

	/* Declare the message payload buffer */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_CONF_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_CONF_SERV_GET_FUNCTION; // Requested service subtype
	payload_buffer[3] = sector_num; // Sector number

	/* Create the Sick message */
	ret = sld_quick_request(sick, payload_buffer, 4);
	e_assert(ret>0, ret);

	/* Extract the returned sector number */
	e_uint16 temp_buffer = 0;
	memcpy(&temp_buffer, &payload_buffer[2], 2);
	temp_buffer = (e_uint8) sick_ld_to_host_byte_order16(temp_buffer);

	/* Check to make sure the returned sector number matches
	 * the requested sector number.
	 */
	if (e_check(temp_buffer == sector_num," Unexpected sector number returned by Sick LD!"))
		return E_ERROR;

	/* Extract the sector function */
	memcpy(&temp_buffer, &payload_buffer[4], 2);
	(*sector_function) = (e_uint8) sick_ld_to_host_byte_order16(temp_buffer);

	/* Extract the sector stop angle (in ticks) */
	memcpy(&temp_buffer, &payload_buffer[6], 2);
	(*sector_stop_angle) = sld_ticks2angle(
			sick_ld_to_host_byte_order16(temp_buffer));

	return E_OK;
}

/**
 * \brief Acquire the Sick LD's current motor speed in Hz
 * \return The Sick LD motor speed
 */
static e_uint32 sld_get_motor_speed(sickld_t *sick)
{
	e_assert(sick&&sick->initialized, E_ERROR);
	return sick->global_config.sick_motor_speed;
}

/**
 * \brief Generates a device-ready sector set given only an active sector spec.
 * \param *active_sector_start_angles Start angles for the active (measuring) sectors
 * \param *active_sector_stop_angles Stop angles for the active sectors
 * \param num_active_sectors The number of active sectors given
 * \param sick_angle_step The step angle (angular resolution) to be used in generating the config
 * \param *sector_functions An output buffer to hold the function assigned to each sector
 * \param *sector_stop_angles An output buffer to hold the stop angles associated w/ the generated sector set
 * \param &num_sectors The number of sectors in the final device-ready configuration
 */
static e_int32 sld_generate_sector_config(
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors, const e_float64 sick_angle_step,
		e_uint32 * const sector_functions, e_float64 * const sector_stop_angles,
		e_uint32 * num_sectors)
{

	(*num_sectors) = 0;
	e_uint32 i;

	/* Generate the sector configuration for multiple sectors */
	e_float64 final_diff = 0;
	if (num_active_sectors > 1)
	{

		/* Generate the actual sector configuration for the device */
		for (i = 0; i < num_active_sectors; i++)
		{

			/* Insert the measurement sector for the active area */
			sector_functions[(*num_sectors)] =
					SICK_CONF_SECTOR_NORMAL_MEASUREMENT;
			sector_stop_angles[(*num_sectors)] = active_sector_stop_angles[i];
			(*num_sectors)++;

			/* Check whether to insert a non-measurement sector */
			if ((i < num_active_sectors - 1)
					&& (active_sector_start_angles[i + 1]
							- active_sector_stop_angles[i]
							>= 2 * sick_angle_step))
			{

				/* Set the next sector function as non-measurement */
				sector_functions[(*num_sectors)] =
						SICK_CONF_SECTOR_NO_MEASUREMENT;
				sector_stop_angles[(*num_sectors)] =
						active_sector_start_angles[i + 1] - sick_angle_step;
				(*num_sectors)++;

			}

		}

		/* Compute the difference between the final stop angle and the first start angle*/
		if (active_sector_stop_angles[num_active_sectors - 1]
				< active_sector_start_angles[0])
		{
			final_diff = active_sector_start_angles[0]
					- active_sector_stop_angles[num_active_sectors - 1];
		}
		else
		{
			final_diff = active_sector_start_angles[0]
					+ (360 - active_sector_stop_angles[num_active_sectors - 1]);
		}

	}
	else
	{

		/* Insert the measurement sector for the active area */
		sector_functions[(*num_sectors)] = SICK_CONF_SECTOR_NORMAL_MEASUREMENT;
		sector_stop_angles[(*num_sectors)] = active_sector_stop_angles[0];
		(*num_sectors)++;

		/* Compute the difference between the final stop angle and the first start angle*/
		if (active_sector_stop_angles[0] <= active_sector_start_angles[0])
		{
			final_diff = active_sector_start_angles[0]
					- active_sector_stop_angles[num_active_sectors - 1];
		}
		else
		{
			final_diff = active_sector_start_angles[0]
					+ (360 - active_sector_stop_angles[num_active_sectors - 1]);
		}

	}

	/* Check whether to add a final non-measurement sector */
	if (final_diff >= 2 * sick_angle_step)
	{

		/* Include the final non-measurement sector */
		sector_functions[(*num_sectors)] = SICK_CONF_SECTOR_NO_MEASUREMENT;
		sector_stop_angles[(*num_sectors)] = active_sector_start_angles[0]
				- sick_angle_step
				+ 360 * (sick_angle_step > active_sector_start_angles[0]);
		(*num_sectors)++;

	}

	/* If necessary insert the non-initialized sector */
	if ((*num_sectors) < SICK_MAX_NUM_SECTORS)
	{

		/* Include the uninitialized sector */
		sector_functions[(*num_sectors)] = SICK_CONF_SECTOR_NOT_INITIALIZED;
		sector_stop_angles[(*num_sectors)] = 0;
		(*num_sectors)++;
	}

	return E_OK;
}

/**
 * \brief Sets the Sick LD sensor mode to ROTATE
 */
static e_int32 sld_set_sensor_mode_to_rotate(sickld_t *sick)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* If necessary adjust the operating mode of the sensor */
	if (sick->sensor_mode != SICK_SENSOR_MODE_ROTATE)
	{
		/* Switch the sensor's operating mode to ROTATE */
		ret = sld_set_sensor_mode(sick, SICK_SENSOR_MODE_ROTATE);
	}
	return ret;
}

/**
 * \brief Sets the Sick LD sensor mode to ROTATE
 */
static e_int32 sld_set_sensor_mode_to_measure(sickld_t *sick)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* If necessary adjust the operating mode of the sensor */
	if (sick->sensor_mode != SICK_SENSOR_MODE_MEASURE)
	{
		/* Switch the sensor's operating mode to ROTATE */
		ret = sld_set_sensor_mode(sick, SICK_SENSOR_MODE_MEASURE);
	}
	return ret;
}

/**
 * \brief Sets the Sick LD sensor mode to IDLE
 */
static e_int32 sld_set_sensor_mode_to_idle(sickld_t *sick)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* If necessary adjust the operating mode of the sensor */
	if (sick->sensor_mode != SICK_SENSOR_MODE_IDLE)
	{
		/* Switch the sensor's operating mode to IDLE */
		ret = sld_set_sensor_mode(sick, SICK_SENSOR_MODE_IDLE);
	}

	return ret;
}

/**
 * \brief Converts return value from TRANS_MEASURE to a representative string
 * \param return_value The TRANS_MEASURE numeric return code
 * \return The corresponding string
 */
static e_uint8* sld_trans_measure_return_to_string(const e_uint8 return_value)
{
	switch (return_value)
	{
	case SICK_WORK_SERV_TRANS_MEASURE_RET_OK:
		return (e_uint8*) ("LD-OEM/LD-LRS Measures");
	case SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_MAX_PULSE:
		return (e_uint8*) ("Max Pulse Frequency Too High");
	case SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_MEAN_PULSE:
		return (e_uint8*) ("Mean Pulse Frequency Too High");
	case SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_SECT_BORDER:
		return (e_uint8*) ("Sector Borders Not Configured Correctly");
	case SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_SECT_BORDER_MULT:
		return (e_uint8*) ("Sector Borders Not Multiple of Angle Step");
	default:
		return (e_uint8*) ("UNRECOGNIZED!!!");
	}
}

/**
 * \brief Converts the Sick LD numerical sensor mode to a representative string
 * \param sick_sensor_mode The sensor mode to be converted
 * \return The corresponding sensor_mode string
 */
static e_uint8* sld_sensor_mode_to_string(const e_uint8 sick_sensor_mode)
{

	switch (sick_sensor_mode)
	{
	case SICK_SENSOR_MODE_IDLE:
		return (e_uint8*) ("IDLE");
	case SICK_SENSOR_MODE_ROTATE:
		return (e_uint8*) ("ROTATE (laser is off)");
	case SICK_SENSOR_MODE_MEASURE:
		return (e_uint8*) ("MEASURE (laser is on)");
	case SICK_SENSOR_MODE_ERROR:
		return (e_uint8*) ("ERROR");
	case SICK_SENSOR_MODE_UNKNOWN:
		return (e_uint8*) ("UNKNOWN");
	default:
		return (e_uint8*) ("UNRECOGNIZED!!!");
	}
}

/**
 * \brief Sets the Sick LD to the requested sensor mode
 * \param new_sick_sensor_mode The desired sensor mode
 */
static e_int32 sld_set_sensor_mode(sickld_t *sick,
		const e_uint8 new_sick_sensor_mode)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* If the new mode matches the current mode then just return */
	if (sick->sensor_mode == new_sick_sensor_mode)
	{
		return E_OK;
	}

	/* If the current sensor mode is MEASURE and streaming data */
	if ((sick->sensor_mode == SICK_SENSOR_MODE_MEASURE)
			&& (sick->streaming_range_data
					|| sick->streaming_range_and_echo_data))
	{

		/* Cancel the current stream */
		ret = sld_cancel_scan_profiles(sick);
		e_assert(ret>0, ret);
	}

	/* The Sick LD must be in rotate mode before: going from IDLE to MEASURE or going from MEASURE to IDLE */
	if ((sick->sensor_mode == SICK_SENSOR_MODE_IDLE
			&& new_sick_sensor_mode == SICK_SENSOR_MODE_MEASURE)
			|| (sick->sensor_mode == SICK_SENSOR_MODE_MEASURE
					&& new_sick_sensor_mode == SICK_SENSOR_MODE_IDLE))
	{

		/* Set to rotate mode */
		ret = sld_set_sensor_mode_to_rotate(sick);
		e_assert(ret>0, ret);
	}

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* The payload length */
	e_uint32 payload_length = 2;

	/* Set the service IDs */
	payload_buffer[0] = SICK_WORK_SERV_CODE; // Requested service type
	payload_buffer[1] = sld_sensor_mode_to_work_service_subcode(
			new_sick_sensor_mode); // Requested service subtype

	/* If the target sensor mode is rotate then we add two more bytes
	 * to the payload length. Doing so adds two zero values to the payload
	 * which tells it to use the angular step and scan freqeuncy values
	 * stored in its flash.
	 */
	if (new_sick_sensor_mode == SICK_SENSOR_MODE_ROTATE)
	{
		payload_length += 2;
	}

	/* Create the Sick message */
	sick_message_t send_message, recv_message;
	ret = skm_create(&send_message);
	e_assert(ret>0, ret);
	ret = skm_create(&recv_message);
	e_assert(ret>0, ret);
	ret = skm_build_message(&send_message, payload_buffer, 4);
	e_assert(ret>0, ret);

	/* Send the message and get a response */
	ret = sld_send_message_and_getreply(sick, &send_message, &recv_message);
	if (e_failed(ret))
		goto OUT1;

	/* Reset the payload buffer */
	memset(payload_buffer, 0, payload_length);

	/* Extract the message payload */
	ret = skm_get_payload(&recv_message, payload_buffer);
	if (e_failed(ret))
		goto OUT1;

	/* Ensure the returned mode matches the requested mode */
	if ((sick->sensor_mode = (payload_buffer[5] & 0x0F))
			!= new_sick_sensor_mode)
	{
		/* Check whether there is an error code we can use */
		if (new_sick_sensor_mode == SICK_SENSOR_MODE_MEASURE)
		{
			e_uint16 return_code = 0;
			memcpy(&return_code, &payload_buffer[6], 2);
			return_code = sick_ld_to_host_byte_order16(return_code);
			/* Print the error code associated with the TRANS_MEASURE request */
			DMSG(
					(STDOUT, "Unexpected sensor mode returned from Sick LD! (TRANS_MEAS Error Code:  %s )", sld_trans_measure_return_to_string(return_code)));
			ret = E_ERROR;
			goto OUT1;
		}
	}

	/* Make sure the motor is Ok */
	if ((sick->motor_mode = ((payload_buffer[5] >> 4) & 0x0F))
			!= SICK_MOTOR_MODE_OK)
	{
		DMSG((STDOUT,"Unexpected motor mode returned from Sick LD!"));
		ret = E_ERROR;
		goto OUT1;
	}

	OUT1: skm_release(&send_message);
	skm_release(&recv_message);
	return ret;
}

/**
 * \brief Map Sick LD sensor modes to their equivalent service subcode representations
 * \param sick_sensor_mode The sensor mode to be converted
 * \return The corresponding work service subcode
 */
static e_uint8 sld_sensor_mode_to_work_service_subcode(
		const e_uint8 sick_sensor_mode)
{

	switch (sick_sensor_mode)
	{
	case SICK_SENSOR_MODE_IDLE:
		return SICK_WORK_SERV_TRANS_IDLE;
	case SICK_SENSOR_MODE_ROTATE:
		return SICK_WORK_SERV_TRANS_ROTATE;
	case SICK_SENSOR_MODE_MEASURE:
		return SICK_WORK_SERV_TRANS_MEASURE;
	default:
		DMSG((STDOUT,"Invalid sensor mode! (Returning 0)"));
		return E_ERROR; //Something is seriously wrong if we end up here!
	}
}

/**
 * \brief Kills the current data stream
 */
static e_int32 sld_cancel_scan_profiles(sickld_t *sick)
{
	int ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Ensure the device is in measurement mode */

	ret = sld_set_sensor_mode_to_measure(sick);
	e_assert(ret>0, ret);

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_MEAS_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_MEAS_SERV_CANCEL_PROFILE; // Requested service subtype
	DMSG((STDOUT,"\tStopping the data stream...\r\n"));
	ret = sld_quick_request(sick, payload_buffer, 2);
	e_assert(ret>0, ret);

	/* Extract and assign the sensor and motor status */
	sick->sensor_mode = payload_buffer[5] & 0x0F;
	sick->motor_mode = (payload_buffer[5] >> 4) & 0x0F;

	/* Since we just updated them, let's make sure everything s'ok */
	if (e_check(sick->sensor_mode != SICK_SENSOR_MODE_ERROR,"Sick LD returned sensor mode ERROR!"))
		return E_ERROR;
	/* Check the motor mode */
	if (e_check(sick->motor_mode != SICK_MOTOR_MODE_ERROR,"Sick LD returned motor mode ERROR!"))
		return E_ERROR;

	/* Set the stream flag for the driver */
	if (sick->streaming_range_data)
	{
		sick->streaming_range_data = false;
	}
	else
	{
		sick->streaming_range_and_echo_data = false;
	}

	DMSG((STDOUT,"\t\tStream stopped!\r\n"));

	return E_OK;
}

/**
 * \brief Converts encoder ticks to equivalent angle representation.
 * \param ticks The tick value to be converted
 * \return The corresponding angle value
 */
static e_float64 sld_ticks2angle(const e_uint16 ticks)
{
	return (((e_float64) ticks) / 16);
}

/**
 * \brief Converts encoder ticks to equivalent angle representation.
 * \param ticks The tick value to be converted
 * \return The corresponding ticks value
 *
 * NOTE: This function assumes that the angle value it receives is
 *       a multiple of 1/16 degree (which is the resolution of the
 *       encoder)
 */
static e_uint16 sld_angle2ticks(const e_float64 angle)
{
	return (e_uint16) (angle * 16);
}

/**
 * \brief Prints the initialization footer.
 */
e_int32 sld_print_init_footer(sickld_t *sick)
{
	e_assert(sick&&sick->initialized, E_ERROR_INVALID_HANDLER);
	DMSG((STDOUT,"\t*** Init. complete: Sick LD is online and ready!"));
	DMSG(
			(STDOUT,"\tNum. Active Sectors: %u", sick->sector_config.sick_num_active_sectors ));
	DMSG(
			(STDOUT,"\tMotor Speed: %u  (Hz)", sick->global_config.sick_motor_speed));
	DMSG(
			(STDOUT,"\tScan Resolution: %f (deg) \r\n", sick->global_config.sick_angle_step));
	return E_OK;
}

/**
 * \brief Converts the Sick LD numerical sector config to a representative string
 * \param sick_sector_function The numeric sector function to be converted
 * \return The corresponding string
 */
e_uint8* sld_sector_function_to_string(const e_uint16 sick_sector_function)
{
	switch (sick_sector_function)
	{
	case SICK_CONF_SECTOR_NOT_INITIALIZED:
		return "NOT INITIALIZED";
	case SICK_CONF_SECTOR_NO_MEASUREMENT:
		return "NOT MEASURING";
	case SICK_CONF_SECTOR_RESERVED:
		return "RESERVED";
	case SICK_CONF_SECTOR_NORMAL_MEASUREMENT:
		return "MEASURING";
	case SICK_CONF_SECTOR_REFERENCE_MEASUREMENT:
		return "REFERENCE";
	default:
		return "UNRECOGNIZED!!!";
	}

}

/**
 * \brief Acquire the Sick LD's sector config as a printable string
 * \return The Sick LD Sick LD's config as a well-formatted string
 */
e_int32 sld_print_sector_config(sickld_t *sick)
{
	e_uint32 i;
	e_assert(sick&&sick->initialized, E_ERROR_INVALID_HANDLER);
	DMSG((STDOUT, "\t=========== Sick Sector Config =========== \r\n" ));
	DMSG(
			(STDOUT,"\tNum. Active Sectors: %u\r\n", sick->sector_config.sick_num_active_sectors ));
	DMSG(
			(STDOUT,"\tNum. Initialized Sectors: %u \r\n", sick->sector_config.sick_num_initialized_sectors ));
	DMSG((STDOUT,"\tSector Configs:\r\n"));
	for (i = 0; i < sick->sector_config.sick_num_initialized_sectors; i++)
		DMSG(
				(STDOUT, "\t\t %d [%f,%f] ( %s )\r\n",(int)i,sick->sector_config.sick_sector_start_angles[i], sick->sector_config.sick_sector_stop_angles[i], sld_sector_function_to_string(sick->sector_config.sick_sector_functions[i])));
	return E_OK;

}

/**
 * \brief Sets the Sick LD signal LED's and switching outputs
 * \param sick_signal_flags Indicates the LEDs and switches to set/unset. ,default DEFAULT_SICK_SIGNAL_SET
 *
 * NOTE: This method does not preserve the previous state of the Sick's signals.
 *       In other words, only the signals flagged in sick_signal_flags will be
 *       set - all others will be off!
 */
static e_int32 sld_set_signals(sickld_t *sick, const e_uint8 sick_signal_flags)
{
	e_int32 ret = E_OK;
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_STAT_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_STAT_SERV_SET_SIGNAL; // Requested service subtype
	payload_buffer[3] = sick_signal_flags; // PORTVAL

	/* Create the Sick message */
	ret = sld_quick_request(sick, payload_buffer, 4);
	e_assert(ret>0, ret);
	/* Check to see if there was an error */
	if (e_check(payload_buffer[2] == 0))
		return E_ERROR;

	return E_OK;
}

/**
 * \brief Gets the Sick LD signal LED's and switching outputs.
 * \param &sick_signal_flags The destination buffer to hold the returned flags.
 */
static e_int32 sld_get_signals(sickld_t *sick, e_uint8 *sick_signal_flags)
{
	e_int32 ret = E_OK;

	/* Ensure the device has been initialized */
	e_assert(sick&&sick->initialized, E_ERROR_INVALID_HANDLER);

	/* Initialize the destination buffer */
	(*sick_signal_flags) = 0;

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_STAT_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_STAT_SERV_GET_SIGNAL; // Requested service subtype

	/* Create the Sick message */
	ret = sld_quick_request(sick, payload_buffer, 2);
	e_assert(ret>0, ret);

	/* Extract the Signal flags */
	(*sick_signal_flags) = payload_buffer[3];

	return E_OK;
}

/**
 * \brief Gets the internal clock time of the Sick LD unit.
 * \param &sick_time The sick clock time in milliseconds.
 */
static e_int32 sld_get_time(sickld_t *sick, e_uint16 *sick_time)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick&&sick->initialized, E_ERROR_INVALID_HANDLER);

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_CONF_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_CONF_SERV_GET_SYNC_CLOCK; // Requested service subtype

	/* Create the Sick message */
	ret = sld_quick_request(sick, payload_buffer, 2);
	e_assert(ret>0, ret);
	/* Extract actual time */
	e_uint16 current_time;
	memcpy(&current_time, &payload_buffer[2], 2);
	(*sick_time) = sick_ld_to_host_byte_order16(current_time);

	return E_OK;
}

/**
 * \brief Enables/disables nearfield suppression on the Sick LD
 * \param suppress_code Code indicating whether to enable or diable the nearfield suppression
 */
static e_int32 sld_set_filter(sickld_t *sick, const e_uint8 suppress_code)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	/* Ensure the device is not measuring */
	if (sick->sensor_mode == SICK_SENSOR_MODE_MEASURE)
	{
		/* Set the Sick LD to rotate mode */
		ret = sld_set_sensor_mode_to_rotate(sick);
		e_assert(ret>0, ret);
	}

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_CONF_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_CONF_SERV_SET_FILTER; // Requested service subtype
	payload_buffer[3] = SICK_CONF_SERV_SET_FILTER_NEARFIELD; // Setting nearfield suppression filter
	payload_buffer[5] = suppress_code; // Code telling whether to turn it on or off

	/* Create the Sick message */
	ret = sld_quick_request(sick, payload_buffer, 6);
	e_assert(ret>0, ret);

	/* Extract FILTERITEM */
	e_uint16 filter_item;
	memcpy(&filter_item, &payload_buffer[2], 2);
	filter_item = sick_ld_to_host_byte_order16(filter_item);

	/* Check that the returned filter item matches nearfiled suppression */
	if (e_check(filter_item == SICK_CONF_SERV_SET_FILTER_NEARFIELD,"Unexpected filter item returned from Sick LD!"))
		return E_ERROR;

	return E_OK;;
}

/**
 * \brief Enables nearfield suppressive filtering
 *
 * NOTE: This method writes this option to the Sick LD's flash, so there is no
 *       need to use it except when configuring the device.
 */
static e_int32 sld_enable_nearfield_suppression(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick&&sick->initialized, E_ERROR_INVALID_HANDLER);

	/* Tell the Sick LD to use nearfield suppression! */
	DMSG((STDOUT, "\tEnabling nearfield suppression...\r\n"));
	ret = sld_set_filter(sick, SICK_CONF_SERV_SET_FILTER_NEARFIELD_ON);
	e_assert(ret>0, ret);

	DMSG((STDOUT, "\t\tSuppression is enabled!\r\n"));

	return E_OK;
}

/**
 * \brief Disables nearfield suppressive filtering
 *
 * NOTE: This method writes this option to the Sick LD's flash, so there is no
 *       need to use it except when configuring the device.
 */
static e_int32 sld_disable_nearfield_suppression(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick&&sick->initialized, E_ERROR_INVALID_HANDLER);

	/* Tell the Sick LD to use nearfield suppression! */
	DMSG((STDOUT, "\tEnabling nearfield suppression...\r\n"));
	ret = sld_set_filter(sick, SICK_CONF_SERV_SET_FILTER_NEARFIELD_OFF);
	e_assert(ret>0, ret);

	DMSG((STDOUT, "\t\tSuppression is enabled!\r\n"));
	return E_OK;
}

/**
 * \brief Teardown TCP connection to Sick LD
 */
static e_int32 sld_teardown_connection(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	ret = sc_close(&sick->sick_connect);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get the status of the Sick LD
 */
static e_int32 sld_get_status(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_STAT_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_STAT_SERV_GET_STATUS; // Requested service subtype

	ret = sld_quick_request(sick, payload_buffer, 2);
	e_assert(ret>0, ret);

	/* Extract the Sick LD's current sensor mode */
	sick->sensor_mode = payload_buffer[5] & 0x0F;

	/* Extract the Sick LD's current motor mode */
	sick->motor_mode = (payload_buffer[5] >> 4) & 0x0F;

	return E_OK;
}

static e_int32 sld_quick_request(sickld_t *sick,
		e_uint8 *payload_buffer/*[in,out]*/, e_uint32 recv_len)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Create the Sick messages */
	sick_message_t send_message, recv_message;
	ret = skm_create(&send_message);
	e_assert(ret>0, ret);
	ret = skm_create(&recv_message);
	e_assert(ret>0, ret);
	ret = skm_build_message(&send_message, payload_buffer, recv_len);
	e_assert(ret>0, ret);

	/* Send the message and get a response */
	ret = sld_send_message_and_getreply(sick, &send_message, &recv_message);
	if (e_failed(ret))
		goto OUT1;

	/* Reset the payload buffer */
	memset(payload_buffer, 0, recv_len);

	/* Extract the message payload */
	ret = skm_get_payload(&recv_message, payload_buffer);
	if (e_failed(ret))
		goto OUT1;
	OUT1: skm_release(&send_message);
	skm_release(&recv_message);
	return ret;
}

/**
 * \brief Query the Sick LD for a particular ID string.
 * \param id_request_code The code indicating what ID string is being requested.
 * \param &id_return_string A reference to hold the string returned from the Sick LD.x
 */
e_int32 sld_get_identification_string(sickld_t *sick,
		const e_uint8 id_request_code, char *id_return_string)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_STAT_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_STAT_SERV_GET_ID; // Requested service subtype
	payload_buffer[3] = id_request_code; // ID information that is being requested

	ret = sld_quick_request(sick, payload_buffer, 4);
	e_assert(ret>0, ret);

	/* Assign the string */
	//id_return_string = (char *) &payload_buffer[2];
	strncpy(id_return_string, &payload_buffer[2], 4);

	return E_OK;
}

/**
 * \brief Get the Sick LD's part number
 */
e_int32 sld_get_sensor_part_number(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_SENSOR_PART_NUM,
			sick->identity.sick_part_number);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get the Sick LD's assigned sensor name
 */
e_int32 sld_get_sensor_name(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick, SICK_STAT_SERV_GET_ID_SENSOR_NAME,
			sick->identity.sick_name);

	e_assert(ret>0, ret);
	return E_OK;

}

/**
 * \brief Get the Sick LD's sensor version
 */
e_int32 sld_get_sensor_version(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_SENSOR_VERSION, sick->identity.sick_version);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get the Sick LD's serial number
 */
e_int32 sld_get_sensor_serial_number(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_SENSOR_SERIAL_NUM,
			sick->identity.sick_serial_number);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get sensor EDM serial number
 */
e_int32 sld_get_sensor_edm_serial_number(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_SENSOR_EDM_SERIAL_NUM,
			sick->identity.sick_edm_serial_number);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get firmware part number
 */
e_int32 sld_get_firmware_part_number(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_FIRMWARE_PART_NUM,
			sick->identity.sick_firmware_part_number);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get firmware name
 */
e_int32 sld_get_firmware_name(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_FIRMWARE_NAME,
			sick->identity.sick_firmware_name);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get firmware version number
 */
e_int32 sld_get_firmware_version(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_FIRMWARE_VERSION,
			sick->identity.sick_firmware_version);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get application software part number
 */
e_int32 sld_get_application_software_part_number(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick,
			SICK_STAT_SERV_GET_ID_APP_PART_NUM,
			sick->identity.sick_application_software_part_number);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get application software name
 */
e_int32 sld_get_application_software_name(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick, SICK_STAT_SERV_GET_ID_APP_NAME,
			sick->identity.sick_application_software_name);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get application software part number
 */
e_int32 sld_get_application_software_version(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Query the Sick LD */
	ret = sld_get_identification_string(sick, SICK_STAT_SERV_GET_ID_APP_VERSION,
			sick->identity.sick_application_software_version);

	e_assert(ret>0, ret);
	return E_OK;
}

/**
 * \brief Get the parameters that define the Sick LD's identity
 */
static e_int32 sld_get_identity(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	ret = sld_get_sensor_part_number(sick);
	e_assert(ret>0, ret);
	ret = sld_get_sensor_name(sick);
	e_assert(ret>0, ret);
	ret = sld_get_sensor_version(sick);
	e_assert(ret>0, ret);
	ret = sld_get_sensor_serial_number(sick);
	e_assert(ret>0, ret);
	ret = sld_get_sensor_edm_serial_number(sick);
	e_assert(ret>0, ret);
	ret = sld_get_firmware_part_number(sick);
	e_assert(ret>0, ret);
	ret = sld_get_firmware_name(sick);
	e_assert(ret>0, ret);
	ret = sld_get_firmware_version(sick);
	e_assert(ret>0, ret);
	ret = sld_get_application_software_part_number(sick);
	e_assert(ret>0, ret);
	ret = sld_get_application_software_name(sick);
	e_assert(ret>0, ret);
	ret = sld_get_application_software_version(sick);
	e_assert(ret>0, ret);

	return E_OK;
}

/**
 * \brief Get the Sick LD's Ethernet configuration.
 */
e_int32 sld_get_ethernet_config(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	ret = sld_set_sensor_mode_to_idle(sick);
	e_assert(ret>0, ret);

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_CONF_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_CONF_SERV_GET_CONFIGURATION; // Requested service subtype
	payload_buffer[3] = SICK_CONF_KEY_ETHERNET; // Configuration key

	ret = sld_quick_request(sick, payload_buffer, 4);
	e_assert(ret>0, ret);

	/* Extract the configuration key */
	e_uint16 temp_buffer = 0;
	e_uint32 data_offset = 2, i;
	memcpy(&temp_buffer, &payload_buffer[data_offset], 2);
	temp_buffer = sick_ld_to_host_byte_order16(temp_buffer);
	data_offset += 2;

	/* A quick sanity check */

	if (e_check(temp_buffer == SICK_CONF_KEY_ETHERNET, "Unexpected message contents!"))
		return E_ERROR;
	/* Extract the IP address of the Sick LD */
	for (i = 0; i < 4; i++, data_offset += 2)
	{
		memcpy(&sick->ethernet_config.sick_ip_address[i],
				&payload_buffer[data_offset], 2);
		sick->ethernet_config.sick_ip_address[i] = sick_ld_to_host_byte_order16(
				sick->ethernet_config.sick_ip_address[i]);
	}

	/* Extract the associated subnet mask */
	for (i = 0; i < 4; i++, data_offset += 2)
	{
		memcpy(&sick->ethernet_config.sick_subnet_mask[i],
				&payload_buffer[data_offset], 2);
		sick->ethernet_config.sick_subnet_mask[i] =
				sick_ld_to_host_byte_order16(
						sick->ethernet_config.sick_subnet_mask[i]);
	}

	/* Extract the default gateway */
	for (i = 0; i < 4; i++, data_offset += 2)
	{
		memcpy(&sick->ethernet_config.sick_gateway_ip_address[i],
				&payload_buffer[data_offset], 2);
		sick->ethernet_config.sick_gateway_ip_address[i] =
				sick_ld_to_host_byte_order16(
						sick->ethernet_config.sick_gateway_ip_address[i]);
	}

	/* Extract the sick node ID (NOTE: This value doesn't matter, but we buffer it anyways) */
	memcpy(&sick->ethernet_config.sick_node_id, &payload_buffer[data_offset],
			2);
	sick->ethernet_config.sick_node_id = sick_ld_to_host_byte_order16(
			sick->ethernet_config.sick_node_id);
	data_offset += 2;

	/* Extract the transparent TCP port (NOTE: The significance of this value is unclear as
	 * it doesn't affect the actual TCP port number that the Sick server is operating at.
	 * But, we buffer it anyways as it is included in the configuration.)
	 */
	memcpy(&sick->ethernet_config.sick_transparent_tcp_port,
			&payload_buffer[data_offset], 2);
	sick->ethernet_config.sick_transparent_tcp_port =
			sick_ld_to_host_byte_order16(
					sick->ethernet_config.sick_transparent_tcp_port);
	data_offset += 2;

	return E_OK;
}

/**
 * \brief Sets the Sick LD's global parameters (sensor id, motor speed, and angular step) in flash.
 * \param sick_sensor_id The sensor id to be assigned to the unit.
 * \param sick_motor_speed The speed of the motor in Hz (must be in [5,20])
 * \param sick_angular_step The difference between two laser pulses in 1/16 degrees (e.g. sick_angular_step = 4 => 0.25 deg step).
 *                          Also, this value must evenly divide into 5760 and be greater than 1 (see telegram listing page 21).
 *
 * ALERT: This method writes the parameters to the Sick LD's flash, so there is no
 *        need to use it except when configuring the device.
 *
 * ALERT: This method DOES NOT DO ERROR CHECKING on the given parameter values.  It is the responsibility
 *        of the caller function to do error checking beforehand.
 */
e_int32 sld_set_global_config(sickld_t *sick, const e_uint8 sick_sensor_id,
		const e_uint8 sick_motor_speed, const e_float64 sick_angle_step)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	ret = sld_set_sensor_mode_to_idle(sick);
	e_assert(ret>0, ret);

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_CONF_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_CONF_SERV_SET_CONFIGURATION; // Requested service subtype

	/* Set the configuration key */
	payload_buffer[3] = SICK_CONF_KEY_GLOBAL; // Use the global configuration key

	/* Set the message parameters */
	payload_buffer[5] = sick_sensor_id; // Include the given sensor ID
	payload_buffer[7] = sick_motor_speed; // Include the new Sick Motor speed value

	/* Set the angular step */
	e_uint16 temp_buffer = sld_angle2ticks(sick_angle_step);
	temp_buffer = host_to_sick_ld_byte_order16(temp_buffer);
	memcpy(&payload_buffer[8], &temp_buffer, 2);

	ret = sld_quick_request(sick, payload_buffer, 10);
	e_assert(ret>0, ret);

	/* Check to make sure there wasn't an error */
	if (e_check(payload_buffer[2] == 0 && payload_buffer[3] == 0," Configuration setting was NOT sucessful!"))
		return E_ERROR_IO;
	/* Update the device driver with the new values */
	sick->global_config.sick_sensor_id = sick_sensor_id;
	sick->global_config.sick_motor_speed = sick_motor_speed;
	sick->global_config.sick_angle_step = sick_angle_step;

	return E_OK;
}

/**
 * \brief Get the global configuration of the Sick LD.
 */
e_int32 sld_get_global_config(sickld_t *sick)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);

	ret = sld_set_sensor_mode_to_idle(sick);
	e_assert(ret>0, ret);

	/* Allocate a single buffer for payload contents */
	e_uint8 payload_buffer[MESSAGE_PAYLOAD_MAX_LENGTH] =
	{ 0 };

	/* Set the service IDs */
	payload_buffer[0] = SICK_CONF_SERV_CODE; // Requested service type
	payload_buffer[1] = SICK_CONF_SERV_GET_CONFIGURATION; // Requested service subtype
	payload_buffer[3] = SICK_CONF_KEY_GLOBAL; // Configuration key

	ret = sld_quick_request(sick, payload_buffer, 4);
	e_assert(ret>0, ret);

	/* Extract the configuration key */
	e_uint16 temp_buffer = 0;
	e_uint32 data_offset = 2;
	memcpy(&temp_buffer, &payload_buffer[data_offset], 2);
	temp_buffer = sick_ld_to_host_byte_order16(temp_buffer);
	data_offset += 2;

	/* A quick sanity check */
	if (e_check(temp_buffer == SICK_CONF_KEY_GLOBAL,"Unexpected message contents!"))
		return E_ERROR_IO;

	/* Extract the global sensor ID */
	memcpy(&sick->global_config.sick_sensor_id, &payload_buffer[data_offset],
			2);
	sick->global_config.sick_sensor_id = sick_ld_to_host_byte_order16(
			sick->global_config.sick_sensor_id);
	data_offset += 2;

	/* Extract the nominal motor speed */
	memcpy(&sick->global_config.sick_motor_speed, &payload_buffer[data_offset],
			2);
	sick->global_config.sick_motor_speed = sick_ld_to_host_byte_order16(
			sick->global_config.sick_motor_speed);
	data_offset += 2;

	/* Extract the angular step */
	memcpy(&temp_buffer, &payload_buffer[data_offset], 2);
	sick->global_config.sick_angle_step = sld_ticks2angle(
			sick_ld_to_host_byte_order16(temp_buffer));

	return E_OK;
}

/**
 * \brief Attempts to set the "permanent" (by writing flash) operating values for the device
 * \param sick_motor_speed Desired motor speed for the device (hz)
 * \param sick_angle_step Desired scan angular resolution (deg)
 * \param active_sector_start_angles Angles marking the beginning of each desired active sector/area
 * \param active_sector_stop_angles Angles marking the end of each desired active sector/area
 * \param num_active_sectors The number of active sectors
 */
e_int32 sld_set_global_params_and_scan_areas(sickld_t *sick,
		const e_uint32 sick_motor_speed, const e_float64 sick_angle_step,
		const e_float64 * const active_sector_start_angles,
		const e_float64 * const active_sector_stop_angles,
		const e_uint32 num_active_sectors)
{
	e_int32 ret = E_OK;
	/* Ensure the device has been initialized */
	e_assert(sick, E_ERROR_INVALID_HANDLER);
	/* Define buffers to hold the device-ready configuration */
	e_uint32 num_sectors = 0;
	e_uint32 sector_functions[SICK_MAX_NUM_SECTORS] =
	{ 0 };
	e_float64 sector_stop_angles[SICK_MAX_NUM_SECTORS] =
	{ 0 };

	/* A few dummy buffers */
	e_float64 sorted_active_sector_start_angles[SICK_MAX_NUM_SECTORS] =
	{ 0 };
	e_float64 sorted_active_sector_stop_angles[SICK_MAX_NUM_SECTORS] =
	{ 0 };

	/* Begin by checking the num of active sectors */
	if (e_check(num_active_sectors <= SICK_MAX_NUM_SECTORS / 2,"Invalid number of active scan sectors!"))
		return E_ERROR_INVALID_PARAMETER;
	/* Ensure the given motor speed is valid (within proper bounds, etc...) */
	ret = sld_valid_motor_speed(sick_motor_speed);
	if (e_check(ret>0,"Invalid motor speed!"))
		return E_ERROR_INVALID_PARAMETER;

	/* Ensure the scan resolution is valid (within proper bounds, etc...) */
	ret = sld_valid_scan_resolution(sick_angle_step, active_sector_start_angles,
			active_sector_stop_angles, num_active_sectors);
	if (e_check(ret>0," Invalid scan resolution!"))
		return E_ERROR_INVALID_PARAMETER;
	/* Copy the input arguments */
	memcpy(sorted_active_sector_start_angles, active_sector_start_angles,
			sizeof(sorted_active_sector_start_angles));
	memcpy(sorted_active_sector_stop_angles, active_sector_stop_angles,
			sizeof(sorted_active_sector_stop_angles));

	/* Ensure a proper ordering of the given sector angle sets */
	ret = sld_sort_scan_areas(sorted_active_sector_start_angles,
			sorted_active_sector_stop_angles, num_active_sectors);
	e_assert(ret>0, ret);

	/* Check for an invalid configuration */
	ret = sld_valid_active_sectors(sorted_active_sector_start_angles,
			sorted_active_sector_stop_angles, num_active_sectors);
	if (e_check(ret>0,"Invalid sector configuration!"))
		return E_ERROR_INVALID_PARAMETER;

	/* Ensure the resulting pulse frequency is valid for the device */
	ret = sld_valid_pulse_frequency_ex(sick_motor_speed, sick_angle_step,
			sorted_active_sector_start_angles, sorted_active_sector_stop_angles,
			num_active_sectors);
	if (e_check(ret>0," Invalid pulse frequency!"))
		return E_ERROR_INVALID_PARAMETER;

	/* Generate the corresponding device-ready sector config */
	ret = sld_generate_sector_config(sorted_active_sector_start_angles,
			sorted_active_sector_stop_angles, num_active_sectors,
			sick_angle_step, sector_functions, sector_stop_angles,
			&num_sectors);
	e_assert(ret>0, ret);

	/* Set the new sector configuration */
	ret = sld_set_sector_config(sick, sector_functions, sector_stop_angles,
			num_sectors, false);
	e_assert(ret>0, ret);

	/* Assign the new configuration in the flash
	 *
	 * NOTE: The following function must be called, even if the global parameters (motor speed,
	 *       and angular resolution) are the same, in order to write the sector configuration.
	 *       Why this is the case isn't exactly clear as the manual does not explain.
	 */
	ret = sld_set_global_config(sick, sick->global_config.sick_sensor_id,
			sick_motor_speed, sick_angle_step);
	e_assert(ret>0, ret);

	return E_OK;
}

static e_int32 sld_read_bytes(sickld_t *sick, e_uint8 * const dest_buffer,
		const int num_bytes_to_read, const e_uint32 timeout_value)
{

	/* Some helpful variables */
	int total_num_bytes_read = 0;
	e_int32 ret = 0;
//	DMSG((STDOUT,"+++++++++++sld_read_bytes total_num_bytes_read=%d num_bytes_to_read=%d",
//			total_num_bytes_read, num_bytes_to_read));
	/* Attempt to fetch the bytes */
	while (total_num_bytes_read < num_bytes_to_read)
	{

		/* Wait for the OS to tell us that data is waiting! */
		ret = sc_select(&sick->sick_connect, E_READ, timeout_value);

		//DMSG((STDOUT,"+++++++++++sld_read_bytes select"));
		/* Figure out what to do based on the output of select */
		e_assert((ret > 0), E_ERROR_IO);

		//DMSG((STDOUT,"+++++++++++sld_read_bytes CAN read"));

		/* A file is ready for reading!
		 *
		 * NOTE: The following conditional is just a sanity check. Since
		 *       the file descriptor set only contains the sick device fd,
		 *       it likely unnecessary to use FD_ISSET
		 */
		ret = sc_recv(&sick->sick_connect, &dest_buffer[total_num_bytes_read],
				1);
		e_assert((ret == 1), E_ERROR_IO);

		total_num_bytes_read++;
	}

//	DMSG((STDOUT,"+++++++++++sld_read_bytes recv data len:%d",total_num_bytes_read));

	return E_OK;
}

/**
 * \brief Acquires the next message from the SickLD byte stream
 * \param &sick_message The returned message object
 */
e_int32 sld_get_next_message_from_datastream(sickld_t *sick,
		sick_message_t *sick_message)
{
	e_int32 ret;
	e_uint32 i;
	/* Flush the input buffer */
	e_uint8 byte_buffer;

	e_assert(sick&&sick->initialized, E_ERROR_INVALID_PARAMETER);

//	DMSG((STDOUT,"+++++++++++sld_get_next_message_from_datastream"));

	/* A buffer to hold the current byte out of the stream */
	const e_uint8 sick_response_header[4] =
	{ 0x02, 'U', 'S', 'P' };

	e_uint8 checksum = 0;
	e_uint8 message_buffer[MESSAGE_MAX_LENGTH] =
	{ 0 };
	e_uint32 payload_length = 0;

	/* Search for the header in the byte stream */
	for (i = 0; i < sizeof(sick_response_header);)
	{
		/* Acquire the next byte from the stream */
		ret = sld_read_bytes(sick, &byte_buffer, 1, DEFAULT_SICK_BYTE_TIMEOUT);
		e_assert((ret>0), ret);

		/* Check if the current byte matches the expected header byte */
		if (byte_buffer == sick_response_header[i])
		{
			i++;
		}
		else
		{
			i = 0;
		}
	}

	/* Populate message buffer w/ response header */
	memcpy(message_buffer, sick_response_header, 4);

	/* Acquire the payload length! */
	ret = sld_read_bytes(sick, &message_buffer[4], 4,
			DEFAULT_SICK_BYTE_TIMEOUT);
	e_assert((ret>0), ret);

	/* Extract the payload size and adjust the byte order */
	memcpy(&payload_length, &message_buffer[4], 4);
	payload_length = sick_ld_to_host_byte_order32(payload_length);

	/* Read the packet payload */
	ret = sld_read_bytes(sick, &message_buffer[8], payload_length,
			DEFAULT_SICK_BYTE_TIMEOUT);
	e_assert((ret>0), ret);

	/* Read the checksum */
	ret = sld_read_bytes(sick, &checksum, 1, DEFAULT_SICK_BYTE_TIMEOUT);
	e_assert((ret>0), ret);

	/* Build the return message object based upon the received payload
	 * and compute the associated checksum.
	 *
	 * NOTE: In constructing this message we ignore the header bytes
	 *       buffered since the BuildMessage routine will insert the
	 *       correct header automatically and compute the payload's
	 *       checksum for us. We could probably get away with using
	 *       just ParseMessage here and not computing the checksum as
	 *       we are using TCP.  However, its safer this way.
	 */
	ret = skm_build_message(sick_message,
			&message_buffer[MESSAGE_HEADER_LENGTH], payload_length);
	e_assert((ret>0), ret);

	/* Verify the checksum is correct (this is probably unnecessary since we are using TCP/IP) */
	ret = skm_get_checksum(sick_message);
	e_assert((ret==checksum), ret);

	return E_OK;
}
