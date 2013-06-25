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
void concatenate_fields()
{
	std::cout << "concatenate_fields" << std::endl;
	/*
	 * 我们定义了连接点云会用到的五个点云对象：三个输入（cloud_a、cloud_b和n_cloud_b），
	 * 两个输出（cloud_c和p_n_cloud_c）。然后我们为两个输入点云（cloud_a和cloud_b或者
	 * cloud_a和n_cloud_b）填充数据。然后，下面几行：
	 */
	pcl::PointCloud<pcl::PointXYZ> cloud_a, cloud_b, cloud_c;
	pcl::PointCloud<pcl::Normal> n_cloud_b; //存储进行连接时需要的normal点云
	pcl::PointCloud<pcl::PointNormal> p_n_cloud_c; //存储连接xyz与normal后的点云
	// 创建点云数据
	cloud_a.width = 5; //设置cloud_a点个数为5，
	cloud_a.height = cloud_b.height = n_cloud_b.height = 1; //设置都为无序点云
	cloud_a.points.resize(cloud_a.width * cloud_a.height);

	n_cloud_b.width = 5; //如果是连接xyz与normal则生成3个法线
	n_cloud_b.points.resize(n_cloud_b.width * n_cloud_b.height);

	//以下循环生成无序点云，填充上面定义的两种类型点云对象
	for (size_t i = 0; i < cloud_a.points.size(); ++i)
	{ //cloud_a始终产生3个点
		cloud_a.points[i].x = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_a.points[i].y = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_a.points[i].z = 1024 * rand() / (RAND_MAX + 1.0f);
	}

	for (size_t i = 0; i < n_cloud_b.points.size(); ++i)
	{ //如果连接xyz+normal=xyznormal则n_cloud_b用3个点作为normal数据
		n_cloud_b.points[i].normal[0] = 1024 * rand() / (RAND_MAX + 1.0f);
		n_cloud_b.points[i].normal[1] = 1024 * rand() / (RAND_MAX + 1.0f);
		n_cloud_b.points[i].normal[2] = 1024 * rand() / (RAND_MAX + 1.0f);
	}

	std::cerr << "Cloud A: " << std::endl;
	for (size_t i = 0; i < cloud_a.points.size(); ++i)
		std::cerr << "    " << cloud_a.points[i].x << " " << cloud_a.points[i].y
				<< " " << cloud_a.points[i].z << std::endl;
	std::cerr << "Cloud B: " << std::endl;

	for (size_t i = 0; i < n_cloud_b.points.size(); ++i)
		std::cerr << "    " << n_cloud_b.points[i].normal[0] << " "
				<< n_cloud_b.points[i].normal[1] << " "
				<< n_cloud_b.points[i].normal[2] << std::endl;

	/*把cloud_a和cloud_b或n_cloud_b（取决于命令行参数）的数据打印在标准输出上。
	 * 如果我们需要连接点云，那么下面的代码：*/
	cloud_c = cloud_a;
	cloud_c += cloud_b;
	//把cloud_a和cloud_b连接在一起创建了cloud_c。
	// 通过把cloud_a和n_cloud_b字段连接在一起创建了p_n_cloud_c。最后：
	std::cerr << "Cloud C: " << std::endl;

	//另外如果要连接字段，那么下面的代码：
	pcl::concatenateFields(cloud_a, n_cloud_b, p_n_cloud_c);
	for (size_t i = 0; i < p_n_cloud_c.points.size(); ++i)
		std::cerr << "    " << p_n_cloud_c.points[i].x << " "
				<< p_n_cloud_c.points[i].y << " " << p_n_cloud_c.points[i].z
				<< " " << p_n_cloud_c.points[i].normal[0] << " "
				<< p_n_cloud_c.points[i].normal[1] << " "
				<< p_n_cloud_c.points[i].normal[2] << std::endl;
	pcl::io::savePCDFileASCII("c_fields.pcd", cloud_c);
}

void concatenate_points()
{
	std::cout << "concatenate_points" << std::endl;
	/*
	 * 我们定义了连接点云会用到的五个点云对象：三个输入（cloud_a、cloud_b和n_cloud_b），
	 * 两个输出（cloud_c和p_n_cloud_c）。然后我们为两个输入点云（cloud_a和cloud_b或者
	 * cloud_a和n_cloud_b）填充数据。然后，下面几行：
	 */
	pcl::PointCloud<pcl::PointXYZ> cloud_a, cloud_b, cloud_c;
	pcl::PointCloud<pcl::Normal> n_cloud_b; //存储进行连接时需要的normal点云
	pcl::PointCloud<pcl::PointNormal> p_n_cloud_c; //存储连接xyz与normal后的点云
	// 创建点云数据
	cloud_a.width = 5; //设置cloud_a点个数为5，
	cloud_a.height = cloud_b.height = n_cloud_b.height = 1; //设置都为无序点云
	cloud_a.points.resize(cloud_a.width * cloud_a.height);

	cloud_b.width = 3;
	cloud_b.points.resize(cloud_b.width * cloud_b.height);

	//以下循环生成无序点云，填充上面定义的两种类型点云对象
	for (size_t i = 0; i < cloud_a.points.size(); ++i)
	{ //cloud_a始终产生3个点
		cloud_a.points[i].x = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_a.points[i].y = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_a.points[i].z = 1024 * rand() / (RAND_MAX + 1.0f);
	}

	for (size_t i = 0; i < cloud_b.points.size(); ++i)
	{ //如果连接a+b=c则cloud_b用2个点作为xyz数据
		cloud_b.points[i].x = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_b.points[i].y = 1024 * rand() / (RAND_MAX + 1.0f);
		cloud_b.points[i].z = 1024 * rand() / (RAND_MAX + 1.0f);
	}

	std::cerr << "Cloud A: " << std::endl;
	for (size_t i = 0; i < cloud_a.points.size(); ++i)
		std::cerr << "    " << cloud_a.points[i].x << " " << cloud_a.points[i].y
				<< " " << cloud_a.points[i].z << std::endl;
	std::cerr << "Cloud B: " << std::endl;

	for (size_t i = 0; i < cloud_b.points.size(); ++i)
		std::cerr << "    " << cloud_b.points[i].x << " " << cloud_b.points[i].y
				<< " " << cloud_b.points[i].z << std::endl;

	/*把cloud_a和cloud_b或n_cloud_b（取决于命令行参数）的数据打印在标准输出上。
	 * 如果我们需要连接点云，那么下面的代码：*/
	cloud_c = cloud_a;
	cloud_c += cloud_b;
	//把cloud_a和cloud_b连接在一起创建了cloud_c。
	// 通过把cloud_a和n_cloud_b字段连接在一起创建了p_n_cloud_c。最后：
	std::cerr << "Cloud C: " << std::endl;

	for (size_t i = 0; i < cloud_c.points.size(); ++i)
		std::cerr << "    " << cloud_c.points[i].x << " " << cloud_c.points[i].y
				<< " " << cloud_c.points[i].z << " " << std::endl;
	pcl::io::savePCDFileASCII("a.pcd", cloud_a);
	pcl::io::savePCDFileASCII("b.pcd", cloud_b);
	pcl::io::savePCDFileASCII("c_points.pcd", cloud_c);
}

#endif
