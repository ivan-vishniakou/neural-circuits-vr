#pragma once

//#include <opencv2/core/core.hpp>
//#include <opencv2/core/optim.hpp>
//#include <iostream>
//#include <string>
//#include "spdlog/spdlog.h"
//#include <ConcurrentQueue.h>
//#include <omp.h>
//#include <boost/lockfree/queue.hpp>


class RotModel : public cv::MinProblemSolver::Function {
private:
	cv::Mat radialFlow;
	cv::Mat tangenFlow;

	uint nPoints;	//num of points to fit by function
	float freq;		//2PI/nPoints
	float ampRatio; //radialFlowAmplitude = ampRatio*tangenFlow;

public:
	int mode;
	static const int MODE_TRACKING = 3;
	static const int MODE_CALIBRATION = 4;

	RotModel(int fit_mode = MODE_TRACKING) {
		mode = fit_mode;
	}

	double calc(const double* x)const {
		/*
		Calculates SSE between actual and fitted optical flow distributions
		x = [amplitude, phase, offset_tan]
		*/
		float res = 0.0;

		if (mode == MODE_TRACKING) {
#pragma omp parallel for// reduction(+ : res)
			for (int i = 0; i < nPoints; i++) {
				float r = pow(
					radialFlow.at<float>(i) - (x[0] * sin(i*freq + x[1])),
					2
				);
				r += pow(
					tangenFlow.at<float>(i) - (x[2] + x[0] * cos(i*freq + x[1])),
					2
				);
				res += r;
			}
		}
		/*
		x = [amplitude_rad, amplitude_tan, offset_tan, phase ]
		*/
		else {
#pragma omp parallel for// reduction(+ : res)
			for (int i = 0; i < nPoints; i++) {
				float r = pow(
					radialFlow.at<float>(0, i) - (x[0] * sin(i*freq + x[3])),
					2
				);
				r += pow(
					tangenFlow.at<float>(0, i) - (x[2] + x[1] * cos(i*freq + x[3])),
					2
				);
				res += r;
			}
		}
		return res;
	}


	int getDims() const { return mode; };


	void setDataPoints(cv::Mat& radial, cv::Mat& tangential) {
		radialFlow = radial;
		tangenFlow = tangential;
		float ch = radial.channels();
		nPoints = radialFlow.size[0];
		freq = 3.14151926 * 2.0 / (float)nPoints;
	}
};

struct BallTrackingParameters {
	cv::Point2f polarCenter = cv::Point2f(112, 70);
	float visibleBallRadius = 116;	//px
	uint roiRhoMin = 40;			//px
	uint roiRhoMax = 100;			//px
	uint roiDownscaledWidth = 30;	//px
	uint roiDownscaledRhoMin = 2;	//px
	uint roiDownscaledRhoMax = 13;	//px

	float calibrCXYrad = 100.31;	//px/rad
	float calibrCXYtan = 76.85;		//px/rad
	float calibrCZ = 20.63;			//px/rad
};


class BallTracking
{
private:
	cv::Mat prevFrame;
	cv::Mat prevFit;
	bool visualize;
	cv::Ptr<cv::DownhillSolver> pDhSolver;
	cv::Ptr<RotModel> pRotModel;
public:
	BallTracking(bool enableVisualize, int mode);
	BallTrackingParameters parameters;
	cv::Mat debugPlot;
	cv::Mat update(const cv::Mat& frame, float& fit_quality);
	~BallTracking();
};