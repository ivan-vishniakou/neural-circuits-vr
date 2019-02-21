#include "BallTracking.h"
//#include <opencv2/opencv.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <chrono>

BallTracking::BallTracking(IConcurrentQueue* pResultQueue, bool enable_visualize = false, int mode = RotModel::MODE_TRACKING) :
	parameters(),
	pDhSolver(cv::DownhillSolver::create()),
	prevFit(),
	pRotModel(new RotModel(mode))
{
	ApplicationSettings& load_settings = ApplicationSettings::getInstance();

	visualize = enable_visualize;
	pResultOutputQueue = pResultQueue;

	/*
	Loading settings from config file
	*/
	parameters.polarCenter.x = load_settings.pt.get<float>("tracking settings.ball_center_x");
	parameters.polarCenter.y = load_settings.pt.get<float>("tracking settings.ball_center_y");
	parameters.visibleBallRadius = load_settings.pt.get<float>("tracking settings.ball_radius");

	parameters.calibrCXYrad = load_settings.pt.get<float>("tracking settings.c_xy_rad");
	parameters.calibrCXYtan = load_settings.pt.get<float>("tracking settings.c_xy_tan");
	parameters.calibrCZ = load_settings.pt.get<float>("tracking settings.c_z");

	/*
	Setting up solver and its model
	*/
	prevFit = cv::Mat::zeros(1, pRotModel->getDims(), CV_64FC1);
	cv::Mat step = (cv::Mat_<double>(1, pRotModel->getDims()) << 1.5, 1.0, 0.5, 0.5);

	pDhSolver->setInitStep(step);
	pDhSolver->setFunction(pRotModel);
	pDhSolver->setTermCriteria(cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 100, 0.001));

	std::cout << "Threads: " << cv::getNumThreads()
		<< ", CPUs: " << cv::getNumberOfCPUs() << std::endl;
}


float BallTracking::update(const cv::Mat& frame)
{
	try
	{
		cv::Mat currentFrame;
		cv::linearPolar(
			frame,
			currentFrame,
			parameters.polarCenter,
			parameters.visibleBallRadius,
			cv::WARP_FILL_OUTLIERS | cv::INTER_LINEAR
		);
		cv::resize(
			currentFrame.colRange(parameters.roiRhoMin, parameters.roiRhoMax),
			currentFrame,
			cv::Size(parameters.roiDownscaledWidth, frame.rows),
			0, 0, cv::INTER_LINEAR
		);

		if (prevFrame.empty())
		{
			// first call when previous frame is not availible
			prevFrame = currentFrame;

			debugPlot = cv::Mat(cv::Size(currentFrame.rows, 100), CV_8UC3);
			debugPlot.setTo(0);

			((TrackingQueue*)pResultOutputQueue)->push(cv::Vec3f(0, 0, 0));
			return 0.0;
		}
		else
		{
			cv::Mat flow(prevFrame.size(), CV_32FC2);
			cv::calcOpticalFlowFarneback(prevFrame, currentFrame, flow, 0.5, 2, 6, 2, 2, 1.7, 0);

			// average along radius in the ROI
			cv::reduce(flow.colRange(parameters.roiDownscaledRhoMin, parameters.roiDownscaledRhoMax), flow, 1, CV_REDUCE_AVG);
			cv::Mat uv[2];
			cv::split(flow, uv);
			uv[0] *= parameters.calibrCXYtan / parameters.calibrCXYrad;

			// set data for DownhillSolver
			pRotModel->setDataPoints(uv[0], uv[1]);
			// fit function using previous result as initial guess, returns MSE
			double res = pDhSolver->minimize(prevFit) / (float)currentFrame.rows;
			//prevFit is [amplitude, phase, offset_tan]

			debugPlot.setTo(0);
			for (int i = 0; i < 140; i++) {
				int y = (int)(50 - std::round(uv[0].at<float>(i, 0) * 20));
				y = std::max(0, y);
				debugPlot.at<cv::Vec3b>(y % 100, i) = cv::Vec3b(0, 0, 255);

				y = (int)(50 - std::round(uv[1].at<float>(i, 0) * 20));
				y = std::max(0, y);
				debugPlot.at<cv::Vec3b>(y % 100, i) = cv::Vec3b(255, 0, 0);

				y = (int)std::round(
					(50 - std::sin(2 * 3.1415 / 140.0*(float)i + prevFit.at<double>(1))*prevFit.at<double>(0) * 20)
				);
				y = std::max(0, y);
				debugPlot.at<cv::Vec3b>(y % 100, i) = cv::Vec3b(64, 64, 255);

				//[amplitude_rad, amplitude_tan, offset_tan, phase]
				y = (int)std::round(
					(50 - (std::cos(2 * 3.1415 / 140.0*(float)i + prevFit.at<double>(1))*prevFit.at<double>(0) + prevFit.at<double>(2)) * 20)
				);
				y = std::max(0, y);
				debugPlot.at<cv::Vec3b>(y % 100, i) = cv::Vec3b(255, 64, 64);

			}
			//cv::imshow("debug", debugPlot);


			cv::Point3f dir(
				std::cos(prevFit.at<double>(1)) * prevFit.at<double>(0) / parameters.calibrCXYtan,		// * 180 / 3.14,  //converts to degrees
				-std::sin(prevFit.at<double>(1)) * prevFit.at<double>(0) / parameters.calibrCXYtan,		// * 180 / 3.14,
				prevFit.at<double>(2) / parameters.calibrCZ		//*180/3.14
			);

			if (visualize) {

				// this outlines the optical flow ROI
				cv::circle(frame, parameters.polarCenter, parameters.visibleBallRadius, cv::Scalar(255));
				cv::circle(frame, parameters.polarCenter, parameters.visibleBallRadius*parameters.roiRhoMin / frame.size().width, cv::Scalar(100));
				cv::circle(frame, parameters.polarCenter, parameters.visibleBallRadius*parameters.roiRhoMax / frame.size().width, cv::Scalar(100));

				// this outlines the tracking ROI
				cv::circle(frame, parameters.polarCenter, parameters.visibleBallRadius*(parameters.roiRhoMin + parameters.roiDownscaledRhoMin*(parameters.roiRhoMax - parameters.roiRhoMin) / parameters.roiDownscaledWidth) / frame.size().width, cv::Scalar(256));
				cv::circle(frame, parameters.polarCenter, parameters.visibleBallRadius*(parameters.roiRhoMin + parameters.roiDownscaledRhoMax*(parameters.roiRhoMax - parameters.roiRhoMin) / parameters.roiDownscaledWidth) / frame.size().width, cv::Scalar(256));

				cv::line(frame, parameters.polarCenter, parameters.polarCenter + cv::Point2f(-dir.y, dir.x) * 3000, cv::Scalar(0));
			}

			((TrackingQueue*)pResultOutputQueue)->push(dir);
			//std::cout << dir << " - " << cv::norm(dir) << std::endl;
			//std::cout << prevFit.at<double>(0)/ prevFit.at<double>(1) << std::endl;

			prevFrame = currentFrame;
			return res;
		}
	}
	catch (const cv::Exception &e) {
		std::cout << e.msg << std::endl;
	}
	return 999.0; //error
}


BallTracking::~BallTracking()
{
	cv::destroyAllWindows();
}
