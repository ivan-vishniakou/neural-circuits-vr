#pragma once

#include "stdafx.h"

#include "ImageAcquisition.h"


ImageAcquisition::ImageAcquisition()
	: aviInitialzed(false)
	, cameraInitialized(false)
{
}


std::vector<std::string> ImageAcquisition::listCameras() {
	std::vector<std::string> cameraNames;

	Pylon::PylonInitialize();
	try
	{
		Pylon::CTlFactory& tlFactory = Pylon::CTlFactory::GetInstance();
		Pylon::DeviceInfoList_t devices;

		if (tlFactory.EnumerateDevices(devices) == 0)
		{
			throw RUNTIME_EXCEPTION("No cameras found");
		}

		for (int i = 0; i < devices.size(); i++) {
			cameraNames.push_back(devices[i].GetUserDefinedName().c_str());
		}
	}
	catch (const GenICam::GenericException &e)
	{
		// Error handling.
		std::cerr << "An exception occurred." << std::endl
			<< e.GetDescription() << std::endl;
	}
	return cameraNames;
}

bool ImageAcquisition::softwareTrigger()
{
	try {
		pCamera->ExecuteSoftwareTrigger();
		return true;
	}
	catch (const GenICam::GenericException &e)
	{
		// Error handling.
		std::cerr << "An exception occurred." << std::endl
			<< e.GetDescription() << std::endl;
		return false;
	}
}

bool ImageAcquisition::waitTriggerReady()
{
	if (!pCamera->IsGrabbing()) {
		pCamera->StartGrabbing(Pylon::GrabStrategy_OneByOne);
		//std::cout << pCamera->GetNodeMap().GetNode("PylonIncludes") << std::endl;
	}
	//pCamera->GetNodeMap().GetNode(Pylon::acqui)
	return pCamera->WaitForFrameTriggerReady(1500);
}


bool ImageAcquisition::init(std::string cameraName)
{
	// Before using any pylon methods, the pylon runtime must be initialized. 
	Pylon::PylonInitialize();
	try
	{
		Pylon::CTlFactory& tlFactory = Pylon::CTlFactory::GetInstance();
		// Get all attached devices and exit application if no device is found.
		Pylon::DeviceInfoList_t devices;

		if (tlFactory.EnumerateDevices(devices) == 0)
		{
			throw RUNTIME_EXCEPTION("No camera present.");
		}
		// Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
		//devices[0].

		Pylon::String_t cam_name = Pylon::String_t(cameraName.c_str());


		for (int i = 0; i < devices.size(); i++) {
			if (devices[i].GetUserDefinedName() == cam_name)
			{
				std::cout << "Found camera \"" << cam_name << "\"" << std::endl;
				pCamera = new Pylon::CInstantCamera(Pylon::CTlFactory::GetInstance().CreateDevice(devices[i]));
			}
		}
		if (!pCamera) {
			std::cerr << "Camera \"" << cam_name << "\" not found. Terminating image acquisition." << std::endl;
			return 0;
		}

		pFormatConverter = new Pylon::CImageFormatConverter();
		pCamera->Open();
		std::cout << "Using device " << pCamera->GetDeviceInfo().GetModelName() << std::endl;
		pCamera->MaxNumBuffer = 10;
	}
	catch (const GenICam::GenericException &e)
	{
		// Error handling.
		std::cerr << "An exception occurred." << std::endl
			<< e.GetDescription() << std::endl;
		return false;
	}

	cameraInitialized = true;
	return true;
}
/*
bool ImageAcquisition::init(std::vector<std::string> cameraNames)
{
Pylon::PylonInitialize();
try
{
Pylon::CTlFactory& tlFactory = Pylon::CTlFactory::GetInstance();
// Get all attached devices and exit application if no device is found.
Pylon::DeviceInfoList_t devices;

if (tlFactory.EnumerateDevices(devices) == 0)
{
throw RUNTIME_EXCEPTION("No camera present.");
}

Pylon::CInstantCameraArray acquisitionCameras(cameraNames.size());

for (int cn = 0; cn < cameraNames.size(); cn++) {
Pylon::String_t cam_name;
for (int i = 0; i < devices.size(); i++) {
if (devices[i].GetUserDefinedName() == cam_name)
{
std::cout << "Found camera \"" << cam_name << "\"" << std::endl;
acquisitionCameras[cn].Attach(tlFactory.CreateDevice(devices[i]));
}
}
}
if (!pCamera) {
std::cerr << "Camera \"" << cam_name << "\" not found. Terminating image acquisition." << std::endl;
return 0;
}
acquisitionCameras[0].
pFormatConverter = new Pylon::CImageFormatConverter();
pCamera->Open();
std::cout << "Using device " << pCamera->GetDeviceInfo().GetModelName() << std::endl;
}
catch (const GenericException &e)
{
// Error handling.
std::cerr << "An exception occurred." << std::endl
<< e.GetDescription() << std::endl;
return false;
}

cameraInitialized = true;
return true;
}*/


bool ImageAcquisition::loadCameraSettings(std::string settingsFilename)
{
	/*
	Load cam parameters
	*/
	if (cameraInitialized) {
		try {
			std::ifstream infile(settingsFilename);
			if (infile.good()) {
				Pylon::CFeaturePersistence::Load(Pylon::String_t(settingsFilename.c_str()), &pCamera->GetNodeMap(), true);
				std::cout << "Camera setings loaded: " << settingsFilename << std::endl;
			}
			else {
				std::cerr << "Settings file not found" << std::endl;
				return false;
			}
		}
		catch (const GenICam::GenericException &e)
		{
			// Error handling.
			std::cerr << "An exception occurred." << std::endl
				<< e.GetDescription() << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "Can not load settings: camera not initialized" << std::endl;
		return false;
	}
	if (aviInitialzed) {
		/*
		TODO: fix video recording size if it has been initialized
		*/
	}
	return true;
}


bool ImageAcquisition::setupVideoRecording(char outputFileneame[])
{
	/*
	AVI writing stuff
	*/
	if (cameraInitialized)
	{
		GenApi::CIntegerPtr width(pCamera->GetNodeMap().GetNode("Width"));
		GenApi::CIntegerPtr height(pCamera->GetNodeMap().GetNode("Height"));
		GenApi::CEnumerationPtr pixelFormat(pCamera->GetNodeMap().GetNode("PixelFormat"));
		Pylon::EPixelType aviPixelType = Pylon::PixelType_BGR8packed;
		const int cFramesPerSecond = 10;
		Pylon::SAviCompressionOptions* pCompressionOptions = NULL;
		if (pixelFormat.IsValid())
		{
			// If the camera produces Mono8 images use Mono8 for the AVI file.
			if (pixelFormat->ToString() == "Mono8")
			{
				aviPixelType = Pylon::PixelType_Mono8;
			}
		}
		pAviWriter = new Pylon::CAviWriter();
		pAviWriter->Open(
			outputFileneame,
			cFramesPerSecond,
			aviPixelType,
			(uint32_t)width->GetValue(),
			(uint32_t)height->GetValue(),
			Pylon::ImageOrientation_BottomUp, // Some compression codecs will not work with top down oriented images.
			pCompressionOptions
		);
		aviInitialzed = true;
		return true;
	}
	else
	{
		return false;
	}
}


bool ImageAcquisition::grabOne(cv::OutputArray frame) {
	if (!pCamera->IsGrabbing()) {
		pCamera->StartGrabbing(Pylon::GrabStrategy_LatestImageOnly);
	}
	try
	{
		if (!pCamera->RetrieveResult(1500, ptrGrabResult, Pylon::TimeoutHandling_ThrowException)) {
			return false;
		}
		else {
			if (ptrGrabResult->GrabSucceeded())
			{
				pFormatConverter->Convert(pylonImage, ptrGrabResult);
				cv::Mat image = cv::Mat(ptrGrabResult->GetHeight(),
					ptrGrabResult->GetWidth(),
					CV_8UC1,
					(uint8_t *)pylonImage.GetBuffer());
				frame.assign(image);
				return true;
			}
		}
	}
	catch (Pylon::TimeoutException &e) {
		std::cerr << e.what() << std::endl;
		std::cout << "Frame timeout..." << std::endl;
		return false;
	}
}


ImageAcquisition::~ImageAcquisition()
{

	if (cameraInitialized) {
		if (pCamera->IsGrabbing()) pCamera->StopGrabbing();
		if (pCamera->IsOpen()) pCamera->Close();
	}
	if (aviInitialzed) pAviWriter->Close();
	// Releases all pylon resources. 
	delete pAviWriter;
	delete pCamera;
	delete pFormatConverter;

	Pylon::PylonTerminate();
}
