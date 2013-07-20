/*!
 * \file hd_data_manager.h
 * \brief 数据存取
 *
 * Code by Joy.you
 * Contact yjcpui(at)gmail(dot)com
 *
 * The hd ecore
 * Copyright (c) 2013, 海达数云
 * All rights reserved.
 *
 */

#include <ls300/hd_data_manager.h>

data_manager_t*
dm_alloc(char **files, int num, int width, int height, int mode)
{
	int i, ret;
	data_adapter_t* pda;
	data_manager_t*dm = malloc(sizeof(data_manager_t) + sizeof(data_adapter_t) * num);
	e_assert(dm, E_ERROR_BAD_ALLOCATE);

	pda = dm->adapters;

	for (i = 0; i < num; i++) {
		ret = da_open(&pda[i], files[i], width, height, mode);
		if (e_failed(ret)) {
			free(dm);
			return NULL;
		}
	}

	dm->num = num;
	dm->state = 1;
	return dm;
}
/*E_READ,E_WRITE*/
e_int32
dm_free(data_manager_t *dm)
{
	int i, ret;
	data_adapter_t* pda;
	e_assert(dm, E_ERROR_INVALID_HANDLER);

	pda = dm->adapters;

	for (i = 0; i < dm->num; i++) {
		ret = da_close(&pda[i]);
		e_check(ret<=0);
	}

	free(dm);
	return E_OK;
}

e_int32
dm_write_point(data_manager_t *dm, int x, int y, point_t* point, int file_right)
{
	int i, ret;
	data_adapter_t* pda;
	e_assert(dm, E_ERROR_INVALID_HANDLER);

	pda = dm->adapters;

	for (i = 0; i < dm->num; i++) {
		if (point->type != pda[i].pnt_type)
			continue;
		ret = da_write_point(&pda[i], x, y, point, file_right);
		e_assert(ret>0, ret);

	}

	return E_OK;
}

e_int32
dm_write_row(data_manager_t *dm, e_uint32 row_idx, point_t* point, int file_right)
{
	int i, ret;
	data_adapter_t* pda;
	e_assert(dm, E_ERROR_INVALID_HANDLER);

	pda = dm->adapters;

	for (i = 0; i < dm->num; i++) {
		if (point->type != pda[i].pnt_type)
			continue;
		ret = da_write_row(&pda[i], row_idx, point, file_right);
		//e_assert(ret>0, ret);
	}
	return E_OK;
}

e_int32
dm_write_column(data_manager_t *dm, e_uint32 column_idx, point_t* point, int file_right)
{
	int i, ret;
	data_adapter_t* pda;
	e_assert(dm, E_ERROR_INVALID_HANDLER);

	pda = dm->adapters;

	for (i = 0; i < dm->num; i++) {
		if (point->type != pda[i].pnt_type)
			continue;
		ret = da_write_column(&pda[i], column_idx, point, file_right);
		//e_assert(ret>0, ret);
	}
	return E_OK;
}

e_int32
dm_append_points(data_manager_t *dm, point_t* point, int pt_num, int file_right)
{
	int i, ret;
	data_adapter_t* pda;
	e_assert(dm, E_ERROR_INVALID_HANDLER);

	pda = dm->adapters;

	for (i = 0; i < dm->num; i++) {
		if (point->type != pda[i].pnt_type)
			continue;
		ret = da_append_points(&pda[i], point, pt_num, file_right);
		//e_assert(ret>0, ret);
	}
	return E_OK;
}

e_int32
dm_append_row(data_manager_t *dm, point_t* point, int file_right)
{
	int i, ret;
	data_adapter_t* pda;
	e_assert(dm, E_ERROR_INVALID_HANDLER);

	pda = dm->adapters;

	for (i = 0; i < dm->num; i++) {
		if (point->type != pda[i].pnt_type)
			continue;
		ret = da_append_row(&pda[i], point, file_right);
		//e_assert(ret>0, ret);
	}
	return E_OK;
}

e_int32
dm_append_column(data_manager_t *dm, point_t* point,
		int file_right)
{
	int i, ret;
	data_adapter_t* pda;
	e_assert(dm, E_ERROR_INVALID_HANDLER);

	pda = dm->adapters;

	for (i = 0; i < dm->num; i++) {
		if (point->type != pda[i].pnt_type)
			continue;
		ret = da_append_column(&pda[i], point, file_right);
		e_assert(ret>0, ret);
	}
	return E_OK;
}

