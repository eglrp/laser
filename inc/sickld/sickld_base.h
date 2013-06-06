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

#ifndef SICK_LD_BASE_H
#define SICK_LD_BASE_H

#include "sick_base.h"

/* Macros */
#define DEFAULT_SICK_IP_ADDRESS      "192.168.1.10"  ///< Default Sick LD INet 4 address
#define DEFAULT_SICK_TCP_PORT       (49152)  ///< Default TCP port
#define DEFAULT_SICK_MESSAGE_TIMEOUT                (e_uint32)(5e6)  ///< The max time to wait for a message reply (usecs)
#define DEFAULT_SICK_CONNECT_TIMEOUT                (e_uint32)(1e6)  ///< The max time to wait before considering a connection attempt as failed (usecs)
#define DEFAULT_SICK_BYTE_TIMEOUT         (35000)  ///< Max allowable time between consecutive bytes
#define DEFAULT_SICK_MSG_RECV_SLEEP 		(100*1000) //100 MS
#define DEFAULT_SICK_NUM_SCAN_PROFILES  (0)  ///< Setting this value to 0 will tell the Sick LD to stream measurements when measurement data is requested (NOTE: A profile is a single scans worth of range measurements)
#define DEFAULT_SICK_SIGNAL_SET         (0)  ///< Default Sick signal configuration
/*消息相关定义*/
#define SICK_LD_MSG_HEADER_LEN             (8)  ///< Sick LD message header length in bytes
#define SICK_LD_MSG_PAYLOAD_MAX_LEN     (5816)  ///< Sick LD maximum payload length
#define SICK_LD_MSG_TRAILER_LEN            (1)  ///< Sick LD length of the message trailer
/**
 * \def SWAP_VALUES(x,y,t)
 * \brief A simple macro for swapping two values.
 */
#define SWAP_VALUES(x,y,t) (t=x,x=y,y=t);

/* Some constants for the developer/end-user */
#define SICK_MAX_NUM_MEASUREMENTS (e_uint16)(2881) ///< Maximum number of measurements per sector
#define SICK_MAX_NUM_SECTORS        (e_uint16)(8)        ///< Maximum number of scan sectors (NOTE: This value must be even)
#define SICK_MAX_NUM_MEASURING_SECTORS ((e_uint16)4)                           ///< Maximum number of active/measuring scan sectors
#define	SICK_MAX_SCAN_AREA	((e_uint16)360)         ///< Maximum area that can be covered in a single scan (deg)
#define	SICK_MIN_MOTOR_SPEED	((e_uint16)5)         ///< Minimum motor speed in Hz
#define	SICK_MAX_MOTOR_SPEED	((e_uint16)20)        ///< Maximum motor speed in Hz
#define	SICK_MIN_VALID_SENSOR_ID	((e_uint16)1)     ///< The lowest value the Sick will accept as a Sensor ID
#define	SICK_MAX_VALID_SENSOR_ID	((e_uint16)254)   ///< The largest value the Sick will accept as a Sensor ID
#define	SICK_MAX_MEAN_PULSE_FREQUENCY	((e_uint16)10800)                        ///< Max mean pulse frequence of the current device configuration (in Hz) (see page 22 of the operator's manual)
#define	SICK_MAX_PULSE_FREQUENCY	((e_uint16)14400) ///< Max pulse frequency of the device (in Hz) (see page 22 of the operator's manual)
#define	SICK_NUM_TICKS_PER_MOTOR_REV	((e_uint16)5760)                          ///< Odometer ticks per revolution of the Sick LD scan head
static const e_float64 SICK_MAX_SCAN_ANGULAR_RESOLUTION = 0.125; ///< Minimum valid separation between laser pulses in active scan ares (deg)
static const e_float64 SICK_DEGREES_PER_MOTOR_STEP = 0.0625; ///< Each odometer tick is equivalent to rotating the scan head this many degrees

/* Sick LD sensor modes of operation */
#define	SICK_SENSOR_MODE_IDLE	((e_uint8)0x01)      ///< The Sick LD is powered but idle
#define	SICK_SENSOR_MODE_ROTATE	((e_uint8)0x02)    ///< The Sick LD prism is rotating, but laser is off
#define	SICK_SENSOR_MODE_MEASURE	((e_uint8)0x03)   ///< The Sick LD prism is rotating, and the laser is on
#define	SICK_SENSOR_MODE_ERROR	((e_uint8)0x04)     ///< The Sick LD is in error mode
#define	SICK_SENSOR_MODE_UNKNOWN	((e_uint8)0xFF)   ///< The Sick LD is in an unknown state
/* Sick LD motor modes */
#define	SICK_MOTOR_MODE_OK	((e_uint8)0x00)         ///< Motor is functioning properly
#define	SICK_MOTOR_MODE_SPIN_TOO_HIGH	((e_uint8)0x09)                          ///< Motor spin too low (i.e. rotational velocity too low)
#define	SICK_MOTOR_MODE_SPIN_TOO_LOW	((e_uint8)0x04)                           ///< Motor spin too high (i.e. rotational velocity too fast)
#define	SICK_MOTOR_MODE_ERROR	((e_uint8)0x0B)      ///< Motor stops or coder error
#define	SICK_MOTOR_MODE_UNKNOWN	((e_uint8)0xFF)    ///< Motor is in an unknown state
/* Sick LD service codes */
#define	SICK_STAT_SERV_CODE	((e_uint8)0x01)        ///< Status service code
#define	SICK_CONF_SERV_CODE	((e_uint8)0x02)        ///< Configuration service code
#define	SICK_MEAS_SERV_CODE	((e_uint8)0x03)        ///< Measurement service code
#define	SICK_WORK_SERV_CODE	((e_uint8)0x04)        ///< Working service code
#define	SICK_ROUT_SERV_CODE	((e_uint8)0x06)        ///< Routing service code
#define	SICK_FILE_SERV_CODE	((e_uint8)0x07)        ///< File service code
#define	SICK_MONR_SERV_CODE	((e_uint8)0x08)        ///< Monitor service code
/* Sick LD status services (service code 0x01) */
#define	SICK_STAT_SERV_GET_ID	((e_uint8)0x01)      ///< Request the Sick LD ID
#define	SICK_STAT_SERV_GET_STATUS	((e_uint8)0x02)  ///< Request status information
#define	SICK_STAT_SERV_GET_SIGNAL	((e_uint8)0x04)  ///< Reads the value of the switch and LED port
#define	SICK_STAT_SERV_SET_SIGNAL	((e_uint8)0x05)  ///< Sets the switches and LEDs
#define	SICK_STAT_SERV_LD_REGISTER_APPLICATION	((e_uint8)0x06)                 ///< Registers the ID data for the application firmware
/* Sick LD status service GET_IDENTIFICATION request codes */
#define	SICK_STAT_SERV_GET_ID_SENSOR_PART_NUM	((e_uint8)0x00)                  ///< Request the sensor's part number
#define	SICK_STAT_SERV_GET_ID_SENSOR_NAME	((e_uint8)0x01)                      ///< Request the sensor's name
#define	SICK_STAT_SERV_GET_ID_SENSOR_VERSION	((e_uint8)0x02)                   ///< Request the sensor's version
#define	SICK_STAT_SERV_GET_ID_SENSOR_SERIAL_NUM	((e_uint8)0x03)                ///< Request the sensor's serial number
#define	SICK_STAT_SERV_GET_ID_SENSOR_EDM_SERIAL_NUM	((e_uint8)0x04)            ///< Request the edm??? serial number
#define	SICK_STAT_SERV_GET_ID_FIRMWARE_PART_NUM	((e_uint8)0x10)                ///< Requess the firmware's part number
#define	SICK_STAT_SERV_GET_ID_FIRMWARE_NAME	((e_uint8)0x11)                    ///< Request the firmware's name
#define	SICK_STAT_SERV_GET_ID_FIRMWARE_VERSION	((e_uint8)0x12)                 ///< Request the firmware's version
#define	SICK_STAT_SERV_GET_ID_APP_PART_NUM	((e_uint8)0x20)                     ///< Request the application part number
#define	SICK_STAT_SERV_GET_ID_APP_NAME	((e_uint8)0x21)                         ///< Request the application name
#define	SICK_STAT_SERV_GET_ID_APP_VERSION	((e_uint8)0x22)                      ///< Request the application version
/* Sick LD configuration services (service code 0x02) */
#define	SICK_CONF_SERV_SET_CONFIGURATION	((e_uint8)0x01)                       ///< Set the Sick LD configuration
#define	SICK_CONF_SERV_GET_CONFIGURATION	((e_uint8)0x02)                       ///< Read the Sick LD configuration information
#define	SICK_CONF_SERV_SET_TIME_ABSOLUTE	((e_uint8)0x03)                       ///< Set the internal clock to a timestamp value
#define	SICK_CONF_SERV_SET_TIME_RELATIVE	((e_uint8)0x04)                       ///< Correct the internal clock by some value
#define	SICK_CONF_SERV_GET_SYNC_CLOCK	((e_uint8)0x05)                          ///< Read the internal time of the LD-OEM/LD-LRS
#define	SICK_CONF_SERV_SET_FILTER	((e_uint8)0x09)  ///< Set the filter configuration
#define	SICK_CONF_SERV_SET_FUNCTION	((e_uint8)0x0A)                            ///< Assigns a measurement function to an angle range
#define	SICK_CONF_SERV_GET_FUNCTION	((e_uint8)0x0B)                            ///< Returns the configuration of the given sector
/* Sick LD configuration filter codes */
#define	SICK_CONF_SERV_SET_FILTER_NEARFIELD	((e_uint8)0x01)                    ///< Code for identifying filter type: nearfield suppression
/* Sick LD nearfield suppression configuration codes */
#define	SICK_CONF_SERV_SET_FILTER_NEARFIELD_OFF	((e_uint8)0x00)                ///< Used to set nearfield suppression off
#define	SICK_CONF_SERV_SET_FILTER_NEARFIELD_ON	((e_uint8)0x01)                 ///< Used to set nearfield suppression on
/* Sick LD measurement services (service code 0x03) */
#define	SICK_MEAS_SERV_GET_PROFILE	((e_uint8)0x01) ///< Requests n profiles of a defined format
#define	SICK_MEAS_SERV_CANCEL_PROFILE	((e_uint8)0x02)                          ///< Stops profile output
/* Sick LD working services (service code 0x04) */
#define	SICK_WORK_SERV_RESET	((e_uint8)0x01)       ///< Sick LD enters a reset sequence
#define	SICK_WORK_SERV_TRANS_IDLE	((e_uint8)0x02)  ///< Sick LD enters IDLE mode (motor stops and laser is turned off)
#define	SICK_WORK_SERV_TRANS_ROTATE	((e_uint8)0x03)                            ///< Sick LD enters ROTATE mode (motor starts and rotates with a specified speed in Hz, laser is off)
#define	SICK_WORK_SERV_TRANS_MEASURE	((e_uint8)0x04)                           ///< Sick LD enters MEASURE mode (laser starts with next revolution)
/* Sick LD working service DO_RESET request codes */
#define	SICK_WORK_SERV_RESET_INIT_CPU	((e_uint8)0x00)                          ///< Sick LD does a complete reset (Reinitializes the CPU)
#define	SICK_WORK_SERV_RESET_KEEP_CPU	((e_uint8)0x01)                          ///< Sick LD does a partial reset (CPU is not reinitialized)
#define	SICK_WORK_SERV_RESET_HALT_APP	((e_uint8)0x02)                          ///< Sick LD does a minimal reset (Application is halted and device enters IDLE state)
/* Sick LD working service TRANS_MEASURE return codes */
#define SICK_WORK_SERV_TRANS_MEASURE_RET_OK  ((e_uint8)0x00)                  ///< Sick LD is ready to stream/obtain scan profiles
#define	SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_MAX_PULSE	((e_uint8)0x01)         ///< Sick LD reports config yields a max laser pulse frequency that is too high
#define	SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_MEAN_PULSE	((e_uint8)0x02)        ///< Sick LD reports config yields a max mean pulse frequency that is too high
#define	SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_SECT_BORDER	((e_uint8)0x03)       ///< Sick LD reports sector borders are not configured correctly
#define	SICK_WORK_SERV_TRANS_MEASURE_RET_ERR_SECT_BORDER_MULT	((e_uint8)0x04)  ///< Sick LD reports sector borders are not a multiple of the step angle
/* Sick LD interface routing services (service code 0x06) */
#define	SICK_ROUT_SERV_COM_ATTACH	((e_uint8)0x01)  ///< Attach a master (host) communications interface
#define	SICK_ROUT_SERV_COM_DETACH	((e_uint8)0x02)  ///< Detach a master (host) communications interface
#define	SICK_ROUT_SERV_COM_INITIALIZE	((e_uint8)0x03)                          ///< Initialize the interface (Note: using this may not be necessary for some interfaces, e.g. Ethernet)
#define	SICK_ROUT_SERV_COM_OUTPUT	((e_uint8)0x04)  ///< Output data to the interface
#define	SICK_ROUT_SERV_COM_DATA	((e_uint8)0x05)    ///< Forward data received on specified interface to master interface
/* Sick LD file services (service code 0x07) */
#define	SICK_FILE_SERV_DIR	((e_uint8)0x01)         ///< List the stored files in flash memory
#define	SICK_FILE_SERV_SAVE	((e_uint8)0x02)        ///< Saves the data into flash memory
#define	SICK_FILE_SERV_LOAD	((e_uint8)0x03)        ///< Recalls a file from the flash
#define	SICK_FILE_SERV_DELETE	((e_uint8)0x04)      ///< Deletes a file from the flash
/* Sick LD monitor services (service code 0x08) */
#define	SICK_MONR_SERV_MONITOR_RUN	((e_uint8)0x01) ///< Enable/disable monitor services
#define	SICK_MONR_SERV_MONITOR_PROFILE_LOG	((e_uint8)0x02)                     ///< Enable/disable profile logging
/* Sick LD configuration keys */
#define	SICK_CONF_KEY_RS232_RS422	((e_uint8)0x01)  ///< Key for configuring RS-232/RS-422
#define	SICK_CONF_KEY_CAN	((e_uint8)0x02)          ///< Key for configuring CAN
#define	SICK_CONF_KEY_ETHERNET	((e_uint8)0x05)     ///< Key for configuring Ethernet
#define	SICK_CONF_KEY_GLOBAL	((e_uint8)0x10)       ///< Key for global configuration
/* Sick LD sector configuration codes */
#define	SICK_CONF_SECTOR_NOT_INITIALIZED	((e_uint8)0x00)                       ///< Sector is uninitialized
#define	SICK_CONF_SECTOR_NO_MEASUREMENT	((e_uint8)0x01)                        ///< Sector has no measurements
#define	SICK_CONF_SECTOR_RESERVED	((e_uint8)0x02)  ///< Sector is reserved by Sick LD
#define	SICK_CONF_SECTOR_NORMAL_MEASUREMENT	((e_uint8)0x03)                    ///< Sector is returning measurements
#define	SICK_CONF_SECTOR_REFERENCE_MEASUREMENT	((e_uint8)0x04)                 ///< Sector can be used as reference measurement
/* Sick LD profile formats */
#define	SICK_SCAN_PROFILE_RANGE	((e_uint16)0x39FF) ///< Request sector scan data w/o any echo data
/*
 * SICK_SCAN_PROFILE_RANGE format (0x39FF) interpretation:
 * (See page 32 of telegram listing for fieldname definitions)
 *
 * Field Name   | Send
 * --------------------
 * PROFILESENT  | YES
 * PROFILECOUNT | YES
 * LAYERNUM     | YES
 * SECTORNUM    | YES
 * DIRSTEP      | YES
 * POINTNUM     | YES
 * TSTART       | YES
 * STARTDIR     | YES
 * DISTANCE-n   | YES
 * DIRECTION-n  | NO
 * ECHO-n       | NO
 * TEND         | YES
 * ENDDIR       | YES
 * SENSTAT      | YES
 */

/* Sick LD profile formats */
#define	SICK_SCAN_PROFILE_RANGE_AND_ECHO	((e_uint16)0x3DFF)                    ///< Request sector scan data w/ echo data
/*
 * SICK_SCAN_PROFILE_RANGE format (0x3DFF) interpretation:
 * (See page 32 of telegram listing for fieldname definitions)
 *
 * Field Name   | Send
 * --------------------
 * PROFILESENT  | YES
 * PROFILECOUNT | YES
 * LAYERNUM     | YES
 * SECTORNUM    | YES
 * DIRSTEP      | YES
 * POINTNUM     | YES
 * TSTART       | YES
 * STARTDIR     | YES
 * DISTANCE-n   | YES
 * DIRECTION-n  | NO
 * ECHO-n       | YES
 * TEND         | YES
 * ENDDIR       | YES
 * SENSTAT      | YES
 */

/* Masks for working with the Sick LD signals
 *
 * NOTE: Although the Sick LD manual defines the flag
 *       values for red and green LEDs the operation
 *       of these LEDs are reserved. So they can't
 *       be set by the device driver.
 */
#define	SICK_SIGNAL_LED_YELLOW_A	((e_uint8)0x01)   ///< Mask for first yellow LED
#define	SICK_SIGNAL_LED_YELLOW_B	((e_uint8)0x02)   ///< Mask for second yellow LED
#define	SICK_SIGNAL_LED_GREEN	((e_uint8)0x04)      ///< Mask for green LED
#define	SICK_SIGNAL_LED_RED	((e_uint8)0x08)        ///< Mask for red LED
#define	SICK_SIGNAL_SWITCH_0	((e_uint8)0x10)       ///< Mask for signal switch 0
#define	SICK_SIGNAL_SWITCH_1	((e_uint8)0x20)       ///< Mask for signal switch 1
#define	SICK_SIGNAL_SWITCH_2	((e_uint8)0x40)       ///< Mask for signal switch 2
#define	SICK_SIGNAL_SWITCH_3	((e_uint8)0x80)       ///< Mask for signal switch 3
/**
 * \struct sick_ld_config_global_tag
 * \brief A structure to aggregate the data used to configure the
 *        Sick LD global parameter values.
 */
/**
 * \typedef sick_ld_config_global_t
 * \brief Adopt c-style convention
 */
typedef struct sick_ld_config_global_tag
{
	e_uint16 sick_sensor_id; ///< The single word sensor ID for the Sick unit
	e_uint16 sick_motor_speed; ///< Nominal motor speed value: 0x0005 to 0x0014 (5 to 20)
	e_float64 sick_angle_step; ///< Difference between two laser pulse positions in 1/16th deg.
	//(NOTE: this value must be a divisor of 5760 and be greater than 1)
} sick_ld_config_global_t;

/**
 * \struct sick_ld_config_ethernet_tag
 * \brief A structure to aggregate the data used to configure
 *        the Sick LD unit for Ethernet.
 *
 * \todo Eventually add similar config structures for the other protocols.
 */
/**
 * \typedef sick_ld_config_ethernet_t
 * \brief Adopt c-style convention
 */
typedef struct sick_ld_config_ethernet_tag
{
	e_uint16 sick_ip_address[4]; ///< IP address in numerical form w/ leftmost part at sick_ip_address[0]
	e_uint16 sick_subnet_mask[4]; ///< Subnet mask for the network to which the Sick LD is assigned
	e_uint16 sick_gateway_ip_address[4]; ///< The address of the local gateway
	e_uint16 sick_node_id; ///< Single word address of the Sick LD
	e_uint16 sick_transparent_tcp_port; ///< The TCP/IP transparent port associated with the Sick LD
} sick_ld_config_ethernet_t;

/**
 * \struct sick_ld_config_sector_tag
 * \brief A structure to aggregate data used to define the
 *        Sick LD's sector configuration.
 */
/**
 * \typedef sick_ld_config_sector_t
 * \brief Adopt c-style convention
 */
typedef struct sick_ld_config_sector_tag
{
	e_uint8 sick_num_active_sectors; ///< Number of active sectors (sectors that are actually being scanned)
	e_uint8 sick_num_initialized_sectors; ///< Number of sectors configured w/ a function other than "not initialized"
	e_uint8 sick_active_sector_ids[SICK_MAX_NUM_SECTORS]; ///< IDs of all active sectors
	e_uint8 sick_sector_functions[SICK_MAX_NUM_SECTORS]; ///< Function values associated w/ each of the Sick LD's sectors
	e_float64 sick_sector_start_angles[SICK_MAX_NUM_SECTORS]; ///< Start angles for each initialized sector (deg)
	e_float64 sick_sector_stop_angles[SICK_MAX_NUM_SECTORS]; ///< Stop angles for each sector (deg)
} sick_ld_config_sector_t;

/**
 * \struct sick_ld_identity_tag
 * \brief A structure to aggregate the fields that collectively
 *        define the identity of a Sick LD unit.
 */
/**
 * \typedef sick_ld_identity_t
 * \brief Adopt c-style convention
 */
typedef struct sick_ld_identity_tag
{
	e_uint8 sick_part_number[256]; ///< The Sick LD's part number
	e_uint8 sick_name[256]; ///< The name assigned to the Sick
	e_uint8 sick_version[256]; ///< The Sick LD's version number
	e_uint8 sick_serial_number[256]; ///< The Sick LD's serial number
	e_uint8 sick_edm_serial_number[256]; ///< The Sick LD's edm??? serial number
	e_uint8 sick_firmware_part_number[256]; ///< The Sick LD's firmware part number
	e_uint8 sick_firmware_name[256]; ///< The Sick LD's firmware name
	e_uint8 sick_firmware_version[256]; ///< The Sick LD's firmware version
	e_uint8 sick_application_software_part_number[256]; ///< The Sick LD's app. software part number
	e_uint8 sick_application_software_name[256]; ///< The Sick LD's app. software name
	e_uint8 sick_application_software_version[256]; ///< The Sick LD's app. software version
} sick_ld_identity_t;

/**
 * \struct sick_ld_sector_data_tag
 * \brief A structure to aggregate the fields that collectively
 *        define a sector in the scan area of the Sick LD unit.
 */
/**
 * \typedef sick_ld_sector_data_t
 * \brief Adopt c-style convention
 */
typedef struct sick_ld_sector_data_tag
{
	e_uint32 sector_num; ///< The sector number in the scan area
	e_uint32 num_data_points; ///< The number of data points in the scan area
	e_uint32 timestamp_start; ///< The timestamp (in ms) corresponding to the time the first measurement in the sector was taken
	e_uint32 timestamp_stop; ///< The timestamp (in ms) corresponding to the time the last measurement in the sector was taken
	e_uint32 echo_values[SICK_MAX_NUM_MEASUREMENTS]; ///< The corresponding echo/reflectivity values
	e_float64 angle_step; ///< The angle step used for the given sector (this should be the same for all sectors)
	e_float64 angle_start; ///< The angle at which the first measurement in the sector was acquired
	e_float64 angle_stop; ///< The angle at which the last measurement in the sector was acquired
	e_float64 range_values[SICK_MAX_NUM_MEASUREMENTS]; ///< The corresponding range values (NOTE: The size of this array is intended to be large enough to accomodate various sector configs.)
	e_float64 scan_angles[SICK_MAX_NUM_MEASUREMENTS]; ///< The scan angles corresponding to the respective measurements
} sick_ld_sector_data_t;

/**
 * \struct sick_ld_scan_profile_tag
 * \brief A structure to aggregate the fields that collectively
 *        define the profile of a single scan acquired from the
 *        Sick LD unit.
 */
/**
 * \typedef sick_ld_scan_profile_t
 * \brief Adopt c-style convention
 */
typedef struct sick_ld_scan_profile_tag
{
	e_uint32 profile_number; ///< The number of profiles sent to the host (i.e. the current profile number)
	e_uint32 profile_counter; ///< The number of profiles gathered by the Sick LD
	e_uint32 layer_num; ///< The layer number associated with a scan (this will always be 0)
	e_uint32 sensor_status; ///< The status of the Sick LD sensor
	e_uint32 motor_status; ///< The status of the Sick LD motor
	e_uint32 num_sectors; ///< The number of sectors returned in the profile
	sick_ld_sector_data_t sector_data[SICK_MAX_NUM_SECTORS]; ///< The sectors associated with the scan profile
} sick_ld_scan_profile_t;

#endif /* SICK_LD_BASE_H */
