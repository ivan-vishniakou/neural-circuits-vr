// BallFootageTracking.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <BallTracking.h>

int main(int argc, char *argv[])
{
	std::string appName = boost::filesystem::basename(argv[0]);
	int add = 0;
	int like = 0;
	std::string input_filename, output_filename, config_filename;
	bool output_fit_quality = false;
	bool mode_calibration = false;

	/*
	Parsing command line options, heavilya adapted from
	http://www.radmangames.com/programming/how-to-use-boost-program_options
	*/
	namespace po = boost::program_options;
	po::variables_map vm;
	try
	{
		po::options_description desc("Options");
		desc.add_options()
			("help,h", "Print help messages")
			("input,i", po::value<std::string>(&input_filename)->required(), "Ball footage source file")
			("output,o", po::value<std::string>(&output_filename)->required(), "Track file output")
			("config,c", po::value<std::string>(&config_filename)->required(), "Path to config file to use")
			("quality,q", po::value<bool>(&output_fit_quality), "Output fit quality values");
		po::positional_options_description positionalOptions;
		positionalOptions.add("input", 1);
		positionalOptions.add("output", 1);

		po::store(po::command_line_parser(argc, argv).options(desc).positional(positionalOptions).run(), vm);
		po::notify(vm);
		std::cout << vm.count("help") << std::endl;
	}
	catch (std::exception& e)
	{
		std::cerr << "Unhandled Exception reached the top of main: "
			<< e.what() << ", application will now exit" << std::endl;
		std::cerr << "Check the command line parameters, specify input and output files." << std::endl;
		return ERROR_UNHANDLED_EXCEPTION;
	}
	std::cout << "Processing " << input_filename << std::endl;
	std::cout << "Writing output to " << output_filename << std::endl;

	/*
	Reading calibration config
	*/
	BallTracking ball_tracker(true, RotModel::MODE_CALIBRATION);
	if (vm.count("config")) {
		std::cout << "Using config: " << config_filename << std::endl;
		try {
			boost::property_tree::ptree tracker_settings;
			boost::property_tree::ini_parser::read_ini(config_filename, tracker_settings);

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
			return ERROR_UNHANDLED_EXCEPTION;
		}
	}
	else {
		std::cout << "No tracking config specified. Going default..." << std::endl;
	}

	/*
	Creating logger
	*/
	spdlog::filename_t log_filename = spdlog::filename_t(output_filename);
	auto file_logger = spdlog::rotating_logger_mt("calibration", log_filename, 1048576 * 5, 1000);
	spdlog::set_pattern("[%H:%M:%S.%e] %v");

	cv::Mat frame, track_data;

	Uint64 frame_count = 0;
	Uint64 time_count = 0;
	float fit_quality = 0.0;
	try {
		if (output_fit_quality) {
			file_logger->info("Tracking output is: [fit_quality, amplitude_rad, amplitude_tan, offset_tan, phase]");
		}
		else {
			file_logger->info("Tracking output is: [amplitude_rad, amplitude_tan, offset_tan, phase]");
		}
		/*
		Video capture and processing
		*/
		cv::VideoCapture cap(
			input_filename);

		if (!cap.isOpened()) {
			throw(std::exception("Error opening video stream or file"));

		
		}
		while (1) {
			cap >> frame;
			if (frame.empty())
				break;

			if (frame.channels() > 1) {
				cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
			}

			auto start = std::chrono::system_clock::now();

			track_data = ball_tracker.update(frame, fit_quality);
			cv::imshow("Frame", frame);
			cv::imshow("Debug", ball_tracker.debugPlot);

			auto end = std::chrono::system_clock::now();

			if (output_fit_quality) {
				file_logger->info("{} {} {} {} {}", fit_quality, track_data.at<double>(0), track_data.at<double>(1), track_data.at<double>(2), track_data.at<double>(3));
			}
			else {
				file_logger->info("{} {} {} {}", track_data.at<double>(0), track_data.at<double>(1), track_data.at<double>(2), track_data.at<double>(3));
			}
			auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

			frame_count += 1;
			time_count += elapsed.count();

			char c = (char)cv::waitKey(1);
			if (c == 27) break;
		}
		cap.release();
		cv::destroyAllWindows();

		std::cout << "Average processing time, us: " << time_count / frame_count << '\n';

	}
	catch (const std::exception &exc)
	{
		std::cerr << exc.what() << std::endl;
	}
	catch (...) {
		std::cerr << "Unknown exception!" << std::endl;
	}

	return 0;
}
