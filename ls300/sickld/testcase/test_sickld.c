/*!
 * \file main.cc
 * \brief A simple application illustrating the use of
 *        the Sick LD C++ driver using a single sector.
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
#if TEST_SICKLD

#include <stdlib.h>
#include "sickld.h"

int main(int argc, char *argv[])
{
	e_int32 ret, i;
	/* A string for the IP address */
	char* sick_ip_addr = DEFAULT_SICK_IP_ADDRESS;

	/* Check the num of args */
	if (argc > 2 || (argc == 2 && strcasecmp(argv[1], "--help") == 0))
	{
		DMSG((STDOUT,"Usage: ld_single_sector [SICK IP ADDRESS]\r\n"
		"Ex. ld_single_sector 192.168.1.11\r\n"));
		return -1;
	}

	/* Assign the IP address */
	if (argc == 2)
	{
		sick_ip_addr = argv[1];
	}

	/* Define the data buffers */
	/* Define the data buffers */
	double values[SICK_MAX_NUM_MEASUREMENTS] =
	{ 0 };
	e_uint32 num_values = 0;

	/* Define the bounds for a single sector */
	double sector_start_ang = 90;
	double sector_stop_ang = 270;

	/* Define the object */
	sickld_t* sickld;
	ret = sld_create(&sickld, sick_ip_addr, DEFAULT_SICK_TCP_PORT);
	if (e_failed(ret))
		goto OUT1;

	/*
	 * Initialize the Sick LD
	 */
	ret = sld_initialize(sickld);
	if (e_failed(ret))
		goto OUT2;

	DMSG((STDOUT, "\tInitialize end.\r\n"));

	/* Set the desired sector configuration */
	ret = sld_set_temp_scan_areas(sickld, &sector_start_ang, &sector_stop_ang,
			1);
	if (e_failed(ret))
		goto OUT2;

	/* Print the sector configuration */
	ret = sld_print_sector_config(sickld);
	if (e_failed(ret))
		goto OUT2;


	Delay(1000);

	/* Acquire some range measurements */
	for (i = 0; i < 10; i++)
	{
		/* Here we only want the range values so the second arg is NULL */
		ret = sld_get_measurements(sickld, values, NULL, NULL, &num_values,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		if (e_failed(ret))
			goto OUT2;
		DMSG((STDOUT,"\t Num. Valuse: %u",num_values));
	}

	OUT2: sld_uninitialize(sickld);
	OUT1: sld_release(&sickld);
	return ret;
}

#endif /*SICKLD*/
