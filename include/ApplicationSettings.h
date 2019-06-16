#pragma once

//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/ini_parser.hpp>
//#include <boost/lockfree/spsc_queue.hpp>
//#include <boost/lexical_cast.hpp>
//#include <iostream>
//#include <string>
//#include <opencv2/core.hpp>
//#include <ConcurrentQueue.h>

//typedef ConcurrentQueue<cv::Vec3f> TrackingQueue;		//used in tracking
//typedef ConcurrentQueue<cv::Vec4f> CalibrationQueue;	//used in calibration


template<typename T>
std::vector<T> ini_string_to_array(const std::string& s)
{
	std::vector<T> result;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, ',')) result.push_back(boost::lexical_cast<T>(item));
	return result;
}

class ApplicationSettings
{
public:
	static ApplicationSettings& getInstance(std::string filename = "default.ini") {
		static ApplicationSettings instance(filename);
		return instance;
	}

private:

	ApplicationSettings(std::string filename)
	{
		try {
			boost::property_tree::ini_parser::read_ini(filename, pt);
			loaded = true;
			settings_filename = filename;
		}
		catch (...) {
			std::cout << "Could not load" << filename << std::endl;
			loaded = false;
		}
	}

public:
	std::string settings_filename;
	bool loaded;
	boost::property_tree::ptree pt;
	std::string work_dir;
	ApplicationSettings(ApplicationSettings const&) = delete;
	void operator=(ApplicationSettings const&) = delete;
};