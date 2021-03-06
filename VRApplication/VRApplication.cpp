// VRApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BallTracking.h"
#include "ImageAcquisition.h"
#include "Environment3D.h"
#include "ConcurrentQueue.h"



int vr_thread_function(int* control_flags, TrackingQueue* tracking_queue, std::string arena_filename) {
	/*
	Game engine thread
	*/
	Urho3D::SharedPtr<Urho3D::Context> context(new Urho3D::Context());
	Urho3D::SharedPtr<Environment3D> application(new Environment3D(context));

	const char* startup_script = arena_filename.c_str();
	application->Configure((char*)startup_script, control_flags, tracking_queue);
	int r = application->Run();
	*control_flags = 1;
	return r;
}


int tracking_thread_function(int* control_flags, TrackingQueue* tracking_queue) {
	
	BallTracking ball_tracker(true, RotModel::MODE_TRACKING);

	bool load_camera_settings;
	std::string camera_name, camera_settings_file;

	try {
		boost::property_tree::ptree tracker_settings;
		boost::property_tree::ini_parser::read_ini("Config\\tracking.cfg", tracker_settings);

		camera_name = tracker_settings.get<std::string>("camera settings.TrackingCameraName", "tracking_cam");
		camera_settings_file = tracker_settings.get<std::string>("camera settings.CameraSettingsFile", "Config/camera_settings.pfs");

		ball_tracker.parameters.polarCenter.x = tracker_settings.get<float>("tracking settings.BallCenterX");
		ball_tracker.parameters.polarCenter.y = tracker_settings.get<float>("tracking settings.BallCenterY");
		ball_tracker.parameters.visibleBallRadius = tracker_settings.get<float>("tracking settings.BallRadius");

		ball_tracker.parameters.calibrCXYrad = tracker_settings.get<float>("tracking settings.CxyRad");
		ball_tracker.parameters.calibrCXYtan = tracker_settings.get<float>("tracking settings.CxyTan");
		ball_tracker.parameters.calibrCZ = tracker_settings.get<float>("tracking settings.Cz");
	}
	catch (std::exception& e)
	{
		std::cerr << boost::current_exception_diagnostic_information() << std::endl;
		std::cout << "Error reading tracking configuration file" << std::endl;
		std::cout << e.what() << std::endl;
		*control_flags = 1;
		return ERROR_UNHANDLED_EXCEPTION;
	}

	ImageAcquisition imageAcquisition;
	
	if (imageAcquisition.init(camera_name)) {
		imageAcquisition.loadCameraSettings(camera_settings_file);
	}
	else {
		std::cerr << "Failed to open camera " << camera_name << std::endl;
		return -1;
	}

	cv::Mat timings_plot = cv::Mat::zeros(cv::Size(1000, 100), CV_8UC1);
	cv::Mat frame, track_data;
	int c = 0;
	std::vector<int> runtimes;
	float fit_quality;
	std::chrono::microseconds prev_iter_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());

	uint64 frame_timestamp;

	while (!*control_flags) {
		c++;
		imageAcquisition.grabOne(frame, frame_timestamp);  // waits for newest frame and returns it
														  // comment out to get full frame period, uncommented for tracking time only:
														 // prev_iter_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
		track_data = ball_tracker.update(frame, fit_quality);
		//cv::imshow(camera_name, frame);

		std::chrono::microseconds current_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
		std::chrono::microseconds dt = current_time - prev_iter_time;
		prev_iter_time = current_time;
		runtimes.push_back(dt.count());
		if (c % 10 == 0 && !(*control_flags)) {
			cv::imshow("Tracking camera", frame);
			//cv::imshow("Tracking timings", timings_plot);

			int average = std::accumulate(runtimes.begin(), runtimes.end(), 0.0) / runtimes.size();
			runtimes.clear();
			//std::cout << "\r" << average << "               " << std::flush;
		}
		cv::Vec4d data;
		std::memcpy(&data[0], &frame_timestamp, sizeof(double));	// TODO: find a better way to pass integer timestamp to the queue...
		data[1] = std::cos(track_data.at<double>(2))*track_data.at<double>(0) / ball_tracker.parameters.calibrCXYtan;
		data[2] = -std::sin(track_data.at<double>(2))*track_data.at<double>(0) / ball_tracker.parameters.calibrCXYtan;
		data[3] = track_data.at<double>(1) / ball_tracker.parameters.calibrCZ;
		tracking_queue->push(data);
	}
	std::cerr << "Tracking thread shutting down" << std::endl;
	return 0;
}


int main(int argc, char *argv[])
{
	std::string arena_filename;
	namespace po = boost::program_options;
	po::variables_map vm;

	//auto file_logger = spdlog::rotating_logger_mt("calibration", log_filename, 1048576 * 5, 1000);
	//spdlog::set_pattern("[%H:%M:%S.%e] %v");
	try
	{
		po::options_description desc("Options");
		desc.add_options()
			("arena,a", po::value<std::string>(&arena_filename)->required(), "Arena to load");
		po::positional_options_description positionalOptions;
		positionalOptions.add("arena", 1);
		po::store(po::command_line_parser(argc, argv).options(desc).positional(positionalOptions).run(), vm);
		po::notify(vm);
	}
	catch (std::exception& e)
	{
		std::cerr << "Unhandled Exception reached the top of main: "
			<< e.what() << ", application will now exit" << std::endl;
		std::cerr << "Check the command line parameters, specify the arena file." << std::endl;
		return ERROR_UNHANDLED_EXCEPTION;
	}

	/*
	Setting up logging;
	*/
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::ostringstream oss;
	oss << "Logs//";
	oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
	oss << " " << arena_filename.substr(0, arena_filename.length() - 3);

	boost::filesystem::path log_path(boost::filesystem::current_path());
	log_path /= oss.str().c_str();

	arena_filename = "Data/VirtualEnvironments/" + arena_filename;
	/*
	TODO: copy arena and config file to the LOG for reproducibility
	*/
	boost::filesystem::create_directories(log_path.c_str());
	
	std::cout << "Starting log in " << log_path.c_str() << std::endl;
	log_path /= "//track.txt";

	auto file_logger = spdlog::rotating_logger_mt("track", log_path.string(), 1048576 * 5, 1000);
	spdlog::set_pattern("%v");
	file_logger->info("0 Output will be [cam_timestamp, ball_x, ball_y, ball_z, arena_x, arena_y, arena_theta]");
	
	TrackingQueue q;
	cv::namedWindow("Tracking camera");
	int control_flags = 0;
	std::thread vr_thread(vr_thread_function, &control_flags, &q, arena_filename);
	std::thread tracking_thread(tracking_thread_function, &control_flags, &q);

	while (!control_flags) {
		char k = cv::waitKey(1);
		if (k) {
			//control_flags |= 32;
		}
		if (k == 'q' || k==27)
			break;
	}

	control_flags |= 1;

	cv::destroyAllWindows();
	tracking_thread.join();
	vr_thread.join();

    return 0;
}

