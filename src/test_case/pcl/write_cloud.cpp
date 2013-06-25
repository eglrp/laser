/*
 * write_point.cpp
 *
 *  Created on: Jun 14, 2013
 *      Author: uplusplus
 */
 #if TEST_PCL

#include <iostream>           //标准C++库中的输入输出类相关头文件。
#include <pcl/io/pcd_io.h>   //pcd 读写类相关的头文件。
#include <pcl/point_types.h> //PCL中支持的点类型头文件。

void write_cloud()
{
	std::cout << "\rwrite_cloud" << std::endl;
	pcl::PointCloud<pcl::PointXYZ> cloud;

	// 创建点云
	cloud.width = 5;
	cloud.height = 1;
	cloud.is_dense = false;
	cloud.points.resize(cloud.width * cloud.height);
	for (size_t i = 0; i < cloud.points.size(); ++i)
	{
		cloud.points[i].x = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud.points[i].y = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud.points[i].z = 1024 * rand() / (RAND_MAX + 1.0f);
	}

	pcl::io::savePCDFileASCII("test_pcd.pcd", cloud);
	std::cerr << "Saved " << cloud.points.size()
			<< " data points to test_pcd.pcd." << std::endl;
	for (size_t i = 0; i < cloud.points.size(); ++i)
		std::cerr << "    " << cloud.points[i].x << " " << cloud.points[i].y
				<< " " << cloud.points[i].z << std::endl;
}

#endif

