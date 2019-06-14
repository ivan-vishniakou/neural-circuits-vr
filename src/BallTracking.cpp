#pragma once
#include <stdafx.h>
#include <BallTracking.h>
#include <ApplicationSettings.h>

BallTracking::BallTracking(bool enable_visualize = false, int mode = RotModel::MODE_TRACKING) :
	parameters(),
	pDhSolver(cv::DownhillSolver::create()),
	prevFit(),
	pRotModel(new RotModel(mode))
{
	visualize = enable_visualize;
	/*
	Setting up solver and its model
	*/
	prevFit = cv::Mat::zeros(1, pRotModel->getDims(), CV_64FC1);
	cv::Mat step = (cv::Mat_<double>(1, pRotModel->getDims()) << 1.5, 1.0, 0.5, 0.5);

	pDhSolver->setInitStep(step);
	pDhSolver->setFunction(pRotModel);
	pDhSolver->setTermCriteria(cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 100, 0.001));
	//cv::setNumThreads(10);
	std::cout << "Threads: " << cv::getNumThreads() << ", CPUs: " << cv::getNumberOfCPUs() << std::endl;
}


cv::Mat BallTracking::update(const cv::Mat& frame, float& fit_quality)
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

			return prevFit;
		}
		else
		{
			cv::Mat flow(prevFrame.size(), CV_32FC2);
			flow.setTo(0.0);
			cv::calcOpticalFlowFarneback(prevFrame, currentFrame, flow, 0.5, 2, 6, 2, 2, 1.7, 0);
			// average along radius in the ROI
			cv::reduce(flow.colRange(parameters.roiDownscaledRhoMin, parameters.roiDownscaledRhoMax), flow, 1, cv::ReduceTypes::REDUCE_AVG);
			cv::Mat uv[2];
			cv::split(flow, uv);
			uv[0] *= parameters.calibrCXYtan / parameters.calibrCXYrad;

			// set data for DownhillSolver
			pRotModel->setDataPoints(uv[0], uv[1]);
			// fit function using previous result as initial guess, returns MSE
			double res = pDhSolver->minimize(prevFit) / (float)currentFrame.rows;
			fit_quality = res;

			//prevFit is [amplitude, offset_tan, phase]
			cv::Point3f dir(
				std::cos(prevFit.at<double>(2)) * prevFit.at<double>(0) / parameters.calibrCXYtan,
				-std::sin(prevFit.at<double>(2)) * prevFit.at<double>(0) / parameters.calibrCXYtan,
				prevFit.at<double>(1) / parameters.calibrCZ
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
			prevFrame = currentFrame;
			return prevFit;
		}
	}
	catch (const cv::Exception &e) {
		std::cout << e.what() << std::endl;
	}
	fit_quality = 999.9;
	return prevFit;
}


BallTracking::~BallTracking()
{
	cv::destroyAllWindows();
}
