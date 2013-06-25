/*
 * main.cpp
 *
 *  Created on: Jun 14, 2013
 *      Author: uplusplus
 */
#if TEST_PCL

#include <iostream>           //标准C++库中的输入输出类相关头文件。
#include <pcl/io/pcd_io.h>   //pcd 读写类相关的头文件。
#include <pcl/point_types.h> //PCL中支持的点类型头文件。
void write_cloud();
void read_cloud();
void concatenate_fields();
void concatenate_points();
int view_cloud();

int main()
{
	std::cout << "PCL TEST" << std::endl;
	write_cloud();
	read_cloud();
#if TEST_PCL_VIEW
	view_cloud();
#endif
	concatenate_fields();
	concatenate_points();

	std::ifstream fs;
	fs.open("a.txt");
	fs.seekg(1);

	return 0;
}

#endif
