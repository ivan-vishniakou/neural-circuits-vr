#pragma once
#include "stdafx.h"

class ImageAcquisition
{
private:
	Pylon::CInstantCamera* pCamera;
	Pylon::CImageFormatConverter* pFormatConverter;
	Pylon::CAviWriter* pAviWriter;

	Pylon::CGrabResultPtr ptrGrabResult;
	Pylon::CPylonImage pylonImage;

	bool aviInitialzed;
	bool cameraInitialized;

public:
	ImageAcquisition();
	static std::vector<std::string> listCameras();
	bool softwareTrigger();
	bool waitTriggerReady();
	bool init(std::string cameraName);
	bool loadCameraSettings(std::string settingsFilename);
	bool setupVideoRecording(char outputFileneame[]);
	bool grabOne(cv::OutputArray frame, uint64& timestamp);
	~ImageAcquisition();
};

