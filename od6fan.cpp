// Overdrive_Sample.cpp : Defines the entry point for the console application.
//

///
/// Copyright (c) 2013 Advanced Micro Devices, Inc.

/// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
/// EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
/// WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

/// \file Overdrive_Sample.cpp
/// \brief C/C++ Overdrive Color sample application, based on ADL_Sample project
///
/// A sample test application to test Overdrive get functions
/// Author: Ilia Blank

// Link with -ldl
// g++ od6fan.cpp -ldl -o od6fan

#if defined(__linux__)
#ifndef LINUX
#define LINUX 1 // morons
#endif
#include "adl_sdk.h"
#include "adl_structures.h"
#include <dlfcn.h> // dyopen, dlsym, dlclose
#include <stdlib.h>
#include <string.h> // memeset
#include <strings.h> // strcasecmp
#include <unistd.h> // sleep, getopt

#else

#include <windows.h>
#include <tchar.h>
#include "adl_sdk.h"
#endif

#include <stdio.h>

#define DEGC "\xc2\xb0" "C"
#define AMDVENDORID (1002)
#define ADL_WARNING_NO_DATA -100

static int adapter, fanspeed, coreclock, memclock, mincoreclock, minmemclock, ptune;

// Definitions of the used function pointers. Add more if you use other ADL APIs
typedef int ( *ADL_MAIN_CONTROL_CREATE )(ADL_MAIN_MALLOC_CALLBACK, int );
typedef int ( *ADL_MAIN_CONTROL_DESTROY )();
typedef int ( *ADL_ADAPTER_NUMBEROFADAPTERS_GET ) ( int* );
typedef int ( *ADL_ADAPTER_ADAPTERINFO_GET ) ( LPAdapterInfo, int );
typedef int ( *ADL_ADAPTER_ACTIVE_GET ) ( int, int* );
typedef int ( *ADL_OVERDRIVE_CAPS ) (int iAdapterIndex, int *iSupported, int *iEnabled, int *iVersion);
typedef int ( *ADL_OVERDRIVE5_THERMALDEVICES_ENUM ) (int iAdapterIndex, int iThermalControllerIndex, ADLThermalControllerInfo *lpThermalControllerInfo);
typedef int ( *ADL_OVERDRIVE5_ODPARAMETERS_GET ) ( int iAdapterIndex, ADLODParameters * lpOdParameters );
typedef int ( *ADL_OVERDRIVE5_TEMPERATURE_GET ) (int iAdapterIndex, int iThermalControllerIndex, ADLTemperature *lpTemperature);
typedef int ( *ADL_OVERDRIVE5_FANSPEED_GET ) (int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedValue *lpFanSpeedValue);
typedef int ( *ADL_OVERDRIVE5_FANSPEEDINFO_GET ) (int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedInfo *lpFanSpeedInfo);
typedef int ( *ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET ) (int iAdapterIndex, int iDefault, ADLODPerformanceLevels *lpOdPerformanceLevels);
typedef int ( *ADL_OVERDRIVE5_ODPARAMETERS_GET ) (int iAdapterIndex, ADLODParameters *lpOdParameters);
typedef int ( *ADL_OVERDRIVE5_CURRENTACTIVITY_GET ) (int iAdapterIndex, ADLPMActivity *lpActivity);
typedef int ( *ADL_OVERDRIVE5_FANSPEED_SET )(int iAdapterIndex, int iThermalControllerIndex, ADLFanSpeedValue *lpFanSpeedValue);
typedef int ( *ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET ) (int iAdapterIndex, ADLODPerformanceLevels *lpOdPerformanceLevels);
typedef int ( *ADL_OVERDRIVE5_POWERCONTROL_CAPS )(int iAdapterIndex, int *lpSupported);
typedef int ( *ADL_OVERDRIVE5_POWERCONTROLINFO_GET )(int iAdapterIndex, ADLPowerControlInfo *lpPowerControlInfo);
typedef int ( *ADL_OVERDRIVE5_POWERCONTROL_GET )(int iAdapterIndex, int *lpCurrentValue, int *lpDefaultValue);
typedef int ( *ADL_OVERDRIVE5_POWERCONTROL_SET )(int iAdapterIndex, int iValue);
typedef int ( *ADL_OVERDRIVE6_FANSPEED_GET )(int iAdapterIndex, ADLOD6FanSpeedInfo *lpFanSpeedInfo);
typedef int ( *ADL_OVERDRIVE6_THERMALCONTROLLER_CAPS )(int iAdapterIndex, ADLOD6ThermalControllerCaps *lpThermalControllerCaps);
typedef int ( *ADL_OVERDRIVE6_TEMPERATURE_GET )(int iAdapterIndex, int *lpTemperature);
typedef int ( *ADL_OVERDRIVE6_CAPABILITIES_GET ) (int iAdapterIndex, ADLOD6Capabilities *lpODCapabilities);
typedef int ( *ADL_OVERDRIVE6_STATEINFO_GET )(int iAdapterIndex, int iStateType, ADLOD6StateInfo *lpStateInfo);
typedef int ( *ADL_OVERDRIVE6_CURRENTSTATUS_GET )(int iAdapterIndex, ADLOD6CurrentStatus *lpCurrentStatus);
typedef int ( *ADL_OVERDRIVE6_POWERCONTROL_CAPS ) (int iAdapterIndex, int *lpSupported);
typedef int ( *ADL_OVERDRIVE6_POWERCONTROLINFO_GET )(int iAdapterIndex, ADLOD6PowerControlInfo *lpPowerControlInfo);
typedef int ( *ADL_OVERDRIVE6_POWERCONTROL_GET )(int iAdapterIndex, int *lpCurrentValue, int *lpDefaultValue);
typedef int ( *ADL_OVERDRIVE6_FANSPEED_SET )(int iAdapterIndex, ADLOD6FanSpeedValue *lpFanSpeedValue);
typedef int ( *ADL_OVERDRIVE6_STATE_SET )(int iAdapterIndex, int iStateType, ADLOD6StateInfo *lpStateInfo);
typedef int ( *ADL_OVERDRIVE6_POWERCONTROL_SET )(int iAdapterIndex, int iValue);

// Memory allocation function
static void* __stdcall ADL_Main_Memory_Alloc( int iSize )
{
	void* lpBuffer = malloc( iSize );
	return lpBuffer;
}

// Optional Memory de-allocation function
static void __stdcall ADL_Main_Memory_Free( void** lpBuffer )
{
	if ( NULL != *lpBuffer )
	{
		free( *lpBuffer );
		*lpBuffer = NULL;
	}
}

#if defined(__linux__)
// equivalent functions in linux
static void * GetProcAddress( void * pLibrary, const char * name)
{
	return dlsym( pLibrary, name);
}
#endif


#if defined(__linux__)
static void Overdrive5Control(int adapterId, void* hDLL)
#else
static void Overdrive5Control(int adapterId, HINSTANCE hDLL)
#endif
{
	ADL_OVERDRIVE5_THERMALDEVICES_ENUM ADL_Overdrive5_ThermalDevices_Enum;
	ADL_OVERDRIVE5_ODPARAMETERS_GET ADL_Overdrive5_ODParameters_Get;
	ADL_OVERDRIVE5_TEMPERATURE_GET ADL_Overdrive5_Temperature_Get;
	ADL_OVERDRIVE5_FANSPEED_GET ADL_Overdrive5_FanSpeed_Get;
	ADL_OVERDRIVE5_FANSPEEDINFO_GET ADL_Overdrive5_FanSpeedInfo_Get;
	ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET ADL_Overdrive5_ODPerformanceLevels_Get;
	ADL_OVERDRIVE5_CURRENTACTIVITY_GET ADL_Overdrive5_CurrentActivity_Get;
	ADL_OVERDRIVE5_FANSPEED_SET ADL_Overdrive5_FanSpeed_Set;
	ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET ADL_Overdrive5_ODPerformanceLevels_Set;
	ADL_OVERDRIVE5_POWERCONTROL_CAPS ADL_Overdrive5_PowerControl_Caps;
	ADL_OVERDRIVE5_POWERCONTROLINFO_GET ADL_Overdrive5_PowerControlInfo_Get;
	ADL_OVERDRIVE5_POWERCONTROL_GET ADL_Overdrive5_PowerControl_Get;
	ADL_OVERDRIVE5_POWERCONTROL_SET ADL_Overdrive5_PowerControl_Set;
	ADL_OVERDRIVE6_STATE_SET ADL_Overdrive6_State_Set;

	LPAdapterInfo lpAdapterInfo = NULL;
	int ADL_Err = ADL_ERR;
	int iNumberAdapters = 0;
	int iOverdriveSupported = 0;
	int iOverdriveEnabled = 0;
	ADLFanSpeedInfo fanSpeedInfo = {0};
	int iOverdriveVersion = 0;
	int fanSpeedReportingMethod = 0;
	int maxThermalControllerIndex = 0;
	ADLODPerformanceLevels* performanceLevels = 0;
	int powerControlSupported = 0;
	ADLPowerControlInfo powerControlInfo = {0};
	int powerControlCurrent = 0;
	int powerControlDefault = 0;


	ADL_Overdrive5_ThermalDevices_Enum = (ADL_OVERDRIVE5_THERMALDEVICES_ENUM) GetProcAddress (hDLL, "ADL_Overdrive5_ThermalDevices_Enum");
	ADL_Overdrive5_ODParameters_Get = (ADL_OVERDRIVE5_ODPARAMETERS_GET) GetProcAddress (hDLL, "ADL_Overdrive5_ODParameters_Get");
	ADL_Overdrive5_Temperature_Get = (ADL_OVERDRIVE5_TEMPERATURE_GET) GetProcAddress (hDLL, "ADL_Overdrive5_Temperature_Get");
	ADL_Overdrive5_FanSpeed_Get = (ADL_OVERDRIVE5_FANSPEED_GET) GetProcAddress (hDLL, "ADL_Overdrive5_FanSpeed_Get");
	ADL_Overdrive5_FanSpeedInfo_Get = (ADL_OVERDRIVE5_FANSPEEDINFO_GET ) GetProcAddress (hDLL, "ADL_Overdrive5_FanSpeedInfo_Get");
	ADL_Overdrive5_ODPerformanceLevels_Get = (ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET) GetProcAddress (hDLL, "ADL_Overdrive5_ODPerformanceLevels_Get");
	ADL_Overdrive5_CurrentActivity_Get = (ADL_OVERDRIVE5_CURRENTACTIVITY_GET) GetProcAddress (hDLL, "ADL_Overdrive5_CurrentActivity_Get");
	ADL_Overdrive5_FanSpeed_Set = (ADL_OVERDRIVE5_FANSPEED_SET)GetProcAddress (hDLL, "ADL_Overdrive5_FanSpeed_Set");
	ADL_Overdrive5_ODPerformanceLevels_Set = (ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET ) GetProcAddress(hDLL, "ADL_Overdrive5_ODPerformanceLevels_Set");
	ADL_Overdrive5_PowerControl_Caps = (ADL_OVERDRIVE5_POWERCONTROL_CAPS) GetProcAddress (hDLL, "ADL_Overdrive5_PowerControl_Caps");
	ADL_Overdrive5_PowerControlInfo_Get = (ADL_OVERDRIVE5_POWERCONTROLINFO_GET) GetProcAddress (hDLL, "ADL_Overdrive5_PowerControlInfo_Get");
	ADL_Overdrive5_PowerControl_Get = (ADL_OVERDRIVE5_POWERCONTROL_GET ) GetProcAddress (hDLL, "ADL_Overdrive5_PowerControl_Get");
	ADL_Overdrive5_PowerControl_Set = (ADL_OVERDRIVE5_POWERCONTROL_SET) GetProcAddress (hDLL, "ADL_Overdrive5_PowerControl_Set");
	ADL_Overdrive6_State_Set = (ADL_OVERDRIVE6_STATE_SET) GetProcAddress (hDLL, "ADL_Overdrive6_State_Set");

	if (
		NULL == ADL_Overdrive5_ThermalDevices_Enum ||
		NULL == ADL_Overdrive5_Temperature_Get ||
		NULL == ADL_Overdrive5_FanSpeedInfo_Get ||
		NULL == ADL_Overdrive5_ODPerformanceLevels_Get ||
		NULL == ADL_Overdrive5_ODParameters_Get ||
		NULL == ADL_Overdrive5_CurrentActivity_Get ||
		NULL == ADL_Overdrive5_FanSpeed_Set ||
		NULL == ADL_Overdrive5_ODPerformanceLevels_Set ||
		NULL == ADL_Overdrive5_PowerControl_Caps ||
		NULL == ADL_Overdrive5_PowerControlInfo_Get ||
		NULL == ADL_Overdrive5_PowerControl_Get ||
		NULL == ADL_Overdrive5_PowerControl_Set)
	{
		printf("ADL's API is missing!\n");
		exit(1);
	}

	ADLThermalControllerInfo termalControllerInfo = {0};
	termalControllerInfo.iSize = sizeof(ADLThermalControllerInfo);

	for (int iThermalControllerIndex = 0; iThermalControllerIndex < 10; iThermalControllerIndex++)
	{
		ADL_Err = ADL_Overdrive5_ThermalDevices_Enum(adapterId, iThermalControllerIndex, &termalControllerInfo);

		if (ADL_Err == ADL_WARNING_NO_DATA)
		{
			maxThermalControllerIndex = iThermalControllerIndex - 1;
			break;
		}

		if (ADL_Err == ADL_WARNING_NO_DATA)
		{
			printf("Failed to enumerate thermal devices\n");
			exit(1);
		}

		if (termalControllerInfo.iThermalDomain == ADL_DL_THERMAL_DOMAIN_GPU)
		{
			ADLTemperature adlTemperature = {0};
			adlTemperature.iSize = sizeof(ADLTemperature);
			if (ADL_OK != ADL_Overdrive5_Temperature_Get( adapterId, iThermalControllerIndex, &adlTemperature ))
			{
				printf("Failed to get thermal devices temperature\n");
				exit(1);
			}
			int temperatureInDegreesCelsius = adlTemperature.iTemperature / 1000; // The temperature is returned in millidegrees Celsius.


			fanSpeedInfo.iSize = sizeof(ADLFanSpeedInfo);
			if ( ADL_OK != ADL_Overdrive5_FanSpeedInfo_Get(adapterId, iThermalControllerIndex, &fanSpeedInfo))
			{
				printf("Failed to get fan caps\n");
				exit(1);
			}

			ADLFanSpeedValue fanSpeedValue = {0};
			fanSpeedReportingMethod = ((fanSpeedInfo.iFlags & ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ) == ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ) ?
				ADL_DL_FANCTRL_SPEED_TYPE_PERCENT : ADL_DL_FANCTRL_SPEED_TYPE_RPM;
			//Set to ADL_DL_FANCTRL_SPEED_TYPE_RPM or to ADL_DL_FANCTRL_SPEED_TYPE_PERCENT to request fan speed to be returned in rounds per minute or in percentage points.
			//Note that the call might fail if requested fan speed reporting method is not supported by the GPU.
			fanSpeedValue.iSpeedType = fanSpeedReportingMethod;
			if ( ADL_OK != ADL_Overdrive5_FanSpeed_Get(adapterId, iThermalControllerIndex, &fanSpeedValue))
			{
				printf("Failed to get fan speed\n");
				exit(1);
			}

			printf("Thermal controller id:%d\n", iThermalControllerIndex);
			printf("Current temperature: %d" DEGC "\n", temperatureInDegreesCelsius);
			if (fanSpeedReportingMethod == ADL_DL_FANCTRL_SPEED_TYPE_RPM)
			{
				printf("Current fan speed: %d rpm\n", fanSpeedValue.iFanSpeed);
				printf("Minimum fan speed: %d rpm\n", fanSpeedInfo.iMinRPM);
				printf("Maximum fan speed: %d rpm\n", fanSpeedInfo.iMaxRPM);
			}
			else
			{
				printf("Current fan speed: %d%%\n", fanSpeedValue.iFanSpeed);
				printf("Minimum fan speed: %d%%\n", fanSpeedInfo.iMinPercent);
				printf("Maximum fan speed: %d%%\n", fanSpeedInfo.iMaxPercent);
			}

			if (((fanSpeedInfo.iFlags & ADL_DL_FANCTRL_SUPPORTS_RPM_WRITE) == ADL_DL_FANCTRL_SUPPORTS_RPM_WRITE) ||
			    ((fanSpeedInfo.iFlags & ADL_DL_FANCTRL_SUPPORTS_PERCENT_WRITE) == ADL_DL_FANCTRL_SUPPORTS_PERCENT_WRITE))
			{
				if (fanspeed >= 0)
				{
					int fanSpeed;

					if ((fanSpeedInfo.iFlags & ADL_DL_FANCTRL_SUPPORTS_PERCENT_WRITE) == ADL_DL_FANCTRL_SUPPORTS_PERCENT_WRITE)
					{
						if (fanspeed > fanSpeedInfo.iMaxPercent || fanspeed < fanSpeedInfo.iMinPercent) {
							printf("Error: Fan speed range %d - %d%% :\n", fanSpeedInfo.iMinPercent, fanSpeedInfo.iMaxPercent);
							exit(1);
						}
						fanSpeed = fanspeed;
					}
					else
					{
						if (fanspeed > fanSpeedInfo.iMaxRPM || fanspeed < fanSpeedInfo.iMinRPM) {
							printf("Error: Fan speed range %d - %d%% :\n", fanSpeedInfo.iMinRPM, fanSpeedInfo.iMaxRPM);
							exit(1);
						}
						fanSpeed = fanspeed;
					}

					ADLFanSpeedValue newFanSpeed = {0};
					newFanSpeed.iSpeedType = fanSpeedReportingMethod;
					newFanSpeed.iFanSpeed = fanSpeed;

					if (ADL_OK != ADL_Overdrive5_FanSpeed_Set(adapterId, iThermalControllerIndex, &newFanSpeed))
					{
						printf("Failed to set new fan speed.\n");
						exit(1);
					}
					if ( ADL_OK != ADL_Overdrive5_FanSpeed_Get(adapterId, iThermalControllerIndex, &fanSpeedValue))
					{
						printf("Failed to get fan speed\n");
						exit(1);
					}
					if (fanSpeedReportingMethod == ADL_DL_FANCTRL_SPEED_TYPE_RPM)
						printf("New fan speed: %d rpm\n", fanSpeedValue.iFanSpeed);
					else
						printf("New fan speed: %d%%\n", fanSpeedValue.iFanSpeed);
				}
			}
		}
	}

	if (ADL_OK != ADL_Overdrive5_PowerControl_Caps(adapterId, &powerControlSupported))
	{
		printf("Failed to get Power Controls support\n");
		exit(1);
	}

	if (powerControlSupported)
	{
		if (ADL_OK != ADL_Overdrive5_PowerControlInfo_Get(adapterId, &powerControlInfo))
		{
			printf("Failed to get Power Controls Info\n");
			exit(1);
		}

		if (ADL_OK != ADL_Overdrive5_PowerControl_Get(adapterId, &powerControlCurrent, &powerControlDefault))
		{
			printf("Failed to get Power Control current value\n");
			exit(1);
		}

		printf("The Power Control threshold range is %d to %d with step of %d\n", powerControlInfo.iMinValue, powerControlInfo.iMaxValue, powerControlInfo.iStepValue);
		printf("Current value of Power Control threshold is %d\n", powerControlCurrent);
	}

	ADLODParameters overdriveParameters = {0};
	overdriveParameters.iSize = sizeof(ADLODParameters);

	if (ADL_OK != ADL_Overdrive5_ODParameters_Get(adapterId, &overdriveParameters))
	{
		printf("Failed to get overdrive parameters\n");
		exit(1);
	}

	printf("The GPU Engine clock range is %d - %d MHz with step of %d MHz\n",
	       overdriveParameters.sEngineClock.iMin / 100,
	       overdriveParameters.sEngineClock.iMax / 100,
	       overdriveParameters.sEngineClock.iStep / 100);

	printf("The GPU Memory clock range is %d - %d MHz with step of %d MHz\n",
	       overdriveParameters.sMemoryClock.iMin / 100,
	       overdriveParameters.sMemoryClock.iMax / 100,
	       overdriveParameters.sMemoryClock.iStep);


	printf("The GPU Core Voltage range is %d - %d mV with step of %d mV\n",
	       overdriveParameters.sVddc.iMin,
	       overdriveParameters.sVddc.iMax,
	       overdriveParameters.sVddc.iStep);


	if (overdriveParameters.iNumberOfPerformanceLevels > 0)
	{
		//Overdrive5 GPUs support few performance levels. Application can set distinct engine clocks, memory clocks, core voltage
		//for each of the performance levels. Performance level with index 0 corresponds to lowest performance system state.
		//Performance level with highest index corresponds to highest performance system state - 3D game playing for example.
		//Users are usually interested in overclocking highest index performance level.

		printf("The GPU supports %d performance levels:\n", overdriveParameters.iNumberOfPerformanceLevels);

		int size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * (overdriveParameters.iNumberOfPerformanceLevels - 1);
		void* performanceLevelsBuffer = malloc(size);
		memset(performanceLevelsBuffer, 0, size);
		performanceLevels = (ADLODPerformanceLevels*)performanceLevelsBuffer;
		performanceLevels->iSize = size;

		if (ADL_OK != ADL_Overdrive5_ODPerformanceLevels_Get(adapterId, 1/*Getting default values first*/ , performanceLevels))
		{
			printf("Failed to get information about supported performance levels.\n");
			exit(1);
		}

		for (int i = 0; i < overdriveParameters.iNumberOfPerformanceLevels; i++)
		{
			printf("Performance level %d\n\tDefault Engine Clock:%d MHz\n\tDefault Memory Clock:%d MHz\n\tDefault Core Voltage:%d mV\n",
			       i,
			       performanceLevels->aLevels[i].iEngineClock / 100,
			       performanceLevels->aLevels[i].iMemoryClock / 100,
			       performanceLevels->aLevels[i].iVddc);
		}

		memset(performanceLevelsBuffer, 0, size);
		performanceLevels->iSize = size;

		if (ADL_OK != ADL_Overdrive5_ODPerformanceLevels_Get(adapterId, 0/*Getting current values first*/ , performanceLevels))
		{
			printf("Failed to get information about supported performance levels.\n");
			exit(1);
		}

		for (int i = 0; i < overdriveParameters.iNumberOfPerformanceLevels; i++)
		{
			printf("Performance level %d\n\tCurrent Engine Clock:%d MHz\n\tCurrent Memory Clock:%d MHz\n\tCurrent Core Voltage:%d mV\n",
			       i,
			       performanceLevels->aLevels[i].iEngineClock / 100,
			       performanceLevels->aLevels[i].iMemoryClock / 100,
			       performanceLevels->aLevels[i].iVddc);
		}
	}

	//Getting real current values for clocks, performance levels, voltage effective in the system.
	ADLPMActivity activity = {0};
	activity.iSize = sizeof(ADLPMActivity);
	if (ADL_OK != ADL_Overdrive5_CurrentActivity_Get(adapterId, &activity))
	{
		printf("Failed to get current GPU activity.\n");
		exit(1);
	}
	printf("Current Engine Clock: %d MHz\n", activity.iEngineClock / 100);
	printf("Current Memory Clock: %d MHz\n", activity.iMemoryClock / 100);
	printf("Current Core Voltage: %d mV\n", activity.iVddc);
	printf("Current Performance Level: %d\n", activity.iCurrentPerformanceLevel - 1);

	if (overdriveParameters.iActivityReportingSupported)
	{
		printf("Current Engine Clock: %d%%\n", activity.iActivityPercent);
	}

	if (overdriveParameters.iNumberOfPerformanceLevels > 0)
	{
		//High performance level is the one that affects 3d graphics during game playing.
		//In most of the cases the applications will be interested in changing only high performance level attributes.

		if (coreclock >= 0)
		{
			if ((coreclock < overdriveParameters.sEngineClock.iMin / 100) ||
			    (coreclock > overdriveParameters.sEngineClock.iMax / 100)) {
				printf("Error: Engine Clock range %d - %d MHz\n",
				       overdriveParameters.sEngineClock.iMin / 100,
				       overdriveParameters.sEngineClock.iMax / 100);
				exit(1);
			}

			int clocks = coreclock * 100;

			performanceLevels->aLevels[overdriveParameters.iNumberOfPerformanceLevels - 1].iEngineClock = clocks;

			if (ADL_OK != ADL_Overdrive5_ODPerformanceLevels_Set(adapterId, performanceLevels) )
			{
				printf("Failed to set new Engine Clock.\n");
				exit(1);
			}
			if (overdriveParameters.iNumberOfPerformanceLevels > 0) {
				int size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * (overdriveParameters.iNumberOfPerformanceLevels - 1);
				void* performanceLevelsBuffer = malloc(size);
				memset(performanceLevelsBuffer, 0, size);
				performanceLevels->iSize = size;

				if (ADL_OK != ADL_Overdrive5_ODPerformanceLevels_Get(adapterId, 0 , performanceLevels))
				{
					printf("Failed to get information about new performance levels.\n");
					exit(1);
				}

				for (int i = 1; i < overdriveParameters.iNumberOfPerformanceLevels; i++)
				{
					printf("Performance level %d - New Engine Clock:%d MHz\n",
					       i,
					       performanceLevels->aLevels[i].iEngineClock / 100);
				}
			}
		}
		if (memclock >= 0)
		{
			if ((memclock < overdriveParameters.sMemoryClock.iMin / 100) ||
			    (memclock > overdriveParameters.sMemoryClock.iMax / 100)) {
				printf("Error: Memory Clock range %d - %d MHz\n",
				       overdriveParameters.sMemoryClock.iMin / 100,
				       overdriveParameters.sMemoryClock.iMax / 100);
				exit(1);
			}

			int clocks = memclock * 100;

			performanceLevels->aLevels[overdriveParameters.iNumberOfPerformanceLevels - 1].iMemoryClock = clocks;

			if (ADL_OK != ADL_Overdrive5_ODPerformanceLevels_Set(adapterId, performanceLevels) )
			{
				printf("Failed to set new Memory Clock.\n");
				exit(1);
			}
			if (overdriveParameters.iNumberOfPerformanceLevels > 0) {
				int size = sizeof(ADLODPerformanceLevels) + sizeof(ADLODPerformanceLevel) * (overdriveParameters.iNumberOfPerformanceLevels - 1);
				void* performanceLevelsBuffer = malloc(size);
				memset(performanceLevelsBuffer, 0, size);
				performanceLevels->iSize = size;

				if (ADL_OK != ADL_Overdrive5_ODPerformanceLevels_Get(adapterId, 0 , performanceLevels))
				{
					printf("Failed to get information about new performance levels.\n");
					exit(1);
				}

				for (int i = 1; i < overdriveParameters.iNumberOfPerformanceLevels; i++)
				{
					printf("Performance level %d - New Memory Clock:%d MHz\n",
					       i,
					       performanceLevels->aLevels[i].iMemoryClock / 100);
				}
			}
		}
		if (powerControlSupported)
		{
			if (ptune > -9999)
			{
				if (ptune < powerControlInfo.iMinValue || ptune > powerControlInfo.iMaxValue) {
					printf("Error: Power Control threshold range %d - %d:\n", powerControlInfo.iMinValue, powerControlInfo.iMaxValue);
					exit(1);
				}
				int newPowerControlTreshold = ptune;
				if (ADL_OK != ADL_Overdrive5_PowerControl_Set(adapterId, newPowerControlTreshold))
				{
					printf("Failed to set new Power Control treshold.\n");
					exit(1);
				}
				if (ADL_OK != ADL_Overdrive5_PowerControl_Get(adapterId, &powerControlCurrent, &powerControlDefault))
				{
					printf("Failed to get Power Control current value\n");
					exit(1);
				}
				printf("New value of Power Control threshold is %d\n", powerControlCurrent);
			}
		}
	}
	return;
}


#if defined(__linux__)
static void Overdrive6Control(int adapterId, void* hDLL)
#else
static void Overdrive6Control(int adapterId, HINSTANCE hDLL)
#endif
{
	ADLOD6FanSpeedInfo fanSpeedInfo = {0};
	ADLOD6ThermalControllerCaps thermalControllerCaps = {0};
	ADLOD6Capabilities od6Capabilities = {0};
	int temperature = 0;
	ADLOD6CurrentStatus currentStatus = {0};
	int powerControlSupported = 0;
	ADLOD6PowerControlInfo powerControlInfo = {0};
	int powerControlCurrent = 0;
	int powerControlDefault = 0;
	ADLOD6FanSpeedValue fanSpeedValue = {0};

	ADL_OVERDRIVE6_FANSPEED_GET ADL_Overdrive6_FanSpeed_Get;
	ADL_OVERDRIVE6_THERMALCONTROLLER_CAPS ADL_Overdrive6_ThermalController_Caps;
	ADL_OVERDRIVE6_TEMPERATURE_GET ADL_Overdrive6_Temperature_Get;
	ADL_OVERDRIVE6_CAPABILITIES_GET ADL_Overdrive6_Capabilities_Get;
	ADL_OVERDRIVE6_STATEINFO_GET ADL_Overdrive6_StateInfo_Get;
	ADL_OVERDRIVE6_CURRENTSTATUS_GET ADL_Overdrive6_CurrentStatus_Get;
	ADL_OVERDRIVE6_POWERCONTROL_CAPS ADL_Overdrive6_PowerControl_Caps;
	ADL_OVERDRIVE6_POWERCONTROLINFO_GET ADL_Overdrive6_PowerControlInfo_Get;
	ADL_OVERDRIVE6_POWERCONTROL_GET ADL_Overdrive6_PowerControl_Get;
	ADL_OVERDRIVE6_FANSPEED_SET ADL_Overdrive6_FanSpeed_Set;
	ADL_OVERDRIVE6_STATE_SET ADL_Overdrive6_State_Set;
	ADL_OVERDRIVE6_POWERCONTROL_SET ADL_Overdrive6_PowerControl_Set;



	ADL_Overdrive6_FanSpeed_Get = (ADL_OVERDRIVE6_FANSPEED_GET) GetProcAddress(hDLL,"ADL_Overdrive6_FanSpeed_Get");
	ADL_Overdrive6_ThermalController_Caps = (ADL_OVERDRIVE6_THERMALCONTROLLER_CAPS)GetProcAddress (hDLL, "ADL_Overdrive6_ThermalController_Caps");
	ADL_Overdrive6_Temperature_Get = (ADL_OVERDRIVE6_TEMPERATURE_GET)GetProcAddress (hDLL, "ADL_Overdrive6_Temperature_Get");
	ADL_Overdrive6_Capabilities_Get = (ADL_OVERDRIVE6_CAPABILITIES_GET)GetProcAddress(hDLL, "ADL_Overdrive6_Capabilities_Get");
	ADL_Overdrive6_StateInfo_Get = (ADL_OVERDRIVE6_STATEINFO_GET)GetProcAddress(hDLL, "ADL_Overdrive6_StateInfo_Get");
	ADL_Overdrive6_CurrentStatus_Get = (ADL_OVERDRIVE6_CURRENTSTATUS_GET)GetProcAddress(hDLL, "ADL_Overdrive6_CurrentStatus_Get");
	ADL_Overdrive6_PowerControl_Caps = (ADL_OVERDRIVE6_POWERCONTROL_CAPS)GetProcAddress(hDLL, "ADL_Overdrive6_PowerControl_Caps");
	ADL_Overdrive6_PowerControlInfo_Get = (ADL_OVERDRIVE6_POWERCONTROLINFO_GET)GetProcAddress(hDLL, "ADL_Overdrive6_PowerControlInfo_Get");
	ADL_Overdrive6_PowerControl_Get = (ADL_OVERDRIVE6_POWERCONTROL_GET)GetProcAddress(hDLL, "ADL_Overdrive6_PowerControl_Get");
	ADL_Overdrive6_FanSpeed_Set = (ADL_OVERDRIVE6_FANSPEED_SET)GetProcAddress(hDLL, "ADL_Overdrive6_FanSpeed_Set");
	ADL_Overdrive6_State_Set = (ADL_OVERDRIVE6_STATE_SET)GetProcAddress(hDLL, "ADL_Overdrive6_State_Set");
	ADL_Overdrive6_PowerControl_Set = (ADL_OVERDRIVE6_POWERCONTROL_SET) GetProcAddress(hDLL, "ADL_Overdrive6_PowerControl_Set");


	if (NULL == ADL_Overdrive6_FanSpeed_Get ||
	    NULL == ADL_Overdrive6_ThermalController_Caps ||
	    NULL == ADL_Overdrive6_Temperature_Get ||
	    NULL == ADL_Overdrive6_Capabilities_Get ||
	    NULL == ADL_Overdrive6_StateInfo_Get ||
	    NULL == ADL_Overdrive6_CurrentStatus_Get ||
	    NULL == ADL_Overdrive6_PowerControl_Caps ||
	    NULL == ADL_Overdrive6_PowerControlInfo_Get ||
	    NULL == ADL_Overdrive6_PowerControl_Get ||
	    NULL == ADL_Overdrive6_FanSpeed_Set ||
	    NULL == ADL_Overdrive6_State_Set ||
	    NULL == ADL_Overdrive6_PowerControl_Set)
	{
		printf("ADL's API is missing!\n");
		exit(1);
	}

	if (ADL_OK != ADL_Overdrive6_ThermalController_Caps(adapterId, &thermalControllerCaps))
	{
		printf("Failed to get thermal controller capabilities\n");
		exit(1);
	}

	if ( ADL_OD6_TCCAPS_FANSPEED_CONTROL ==(thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_CONTROL) ) //Verifies that fan speed controller exists on the GPU.
	{
		if (ADL_OD6_TCCAPS_FANSPEED_PERCENT_READ == (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_PERCENT_READ ) ||
		    ADL_OD6_TCCAPS_FANSPEED_RPM_READ == (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_RPM_READ ) )
		{
			/*fanSpeedInfo.iSpeedType = ((thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_RPM_READ) == ADL_OD6_TCCAPS_FANSPEED_RPM_READ)?
			  ADL_OD6_FANSPEED_TYPE_RPM :
			  ADL_OD6_FANSPEED_TYPE_PERCENT;*/

			if (ADL_OK != ADL_Overdrive6_FanSpeed_Get(adapterId, &fanSpeedInfo))
			{
				printf("Failed to get fan speed info\n");
				exit(1);
			}

			if (ADL_OD6_TCCAPS_FANSPEED_PERCENT_READ ==(thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_PERCENT_READ ))
				printf("Fan speed range: %d - %d%%\n", thermalControllerCaps.iFanMinPercent, thermalControllerCaps.iFanMaxPercent);
			else
				printf("Fan speed range: %d - %d rpm\n", thermalControllerCaps.iFanMinRPM, thermalControllerCaps.iFanMaxRPM);

			if (ADL_OD6_FANSPEED_TYPE_PERCENT == (fanSpeedInfo.iSpeedType & ADL_OD6_FANSPEED_TYPE_PERCENT))
				printf("Current fan speed: %d%%\n", fanSpeedInfo.iFanSpeedPercent);
			else
				printf("Current fan speed: %d rpm\n", fanSpeedInfo.iFanSpeedRPM);
		}
	}

	if ( ADL_OD6_TCCAPS_THERMAL_CONTROLLER == (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_THERMAL_CONTROLLER) ) //Verifies that thermal controller exists on the GPU.
	{
		if (ADL_OK != ADL_Overdrive6_Temperature_Get(adapterId, &temperature))
		{
			printf("Failed to get GPU temperature\n");
			exit(1);
		}

		printf("GPU temperature is %d" DEGC "\n", temperature / 1000); //The temperature is returned in mili-degree of Celsius
	}

	if (ADL_OK != ADL_Overdrive6_Capabilities_Get(adapterId, &od6Capabilities))
	{
		printf("Failed to get Overdrive capabilities\n");
		exit(1);
	}

	//Overdrive 6 uses performance levels to communicate effective minimum and maximum values for memory and GPU clocks
	//either requested by user or preconfigured by the driver defaults. Thus only 2 performance levels should be returned.
	if (od6Capabilities.iNumberOfPerformanceLevels != 2)
	{
		printf("Unexpected number of performance levels\n");
		exit(1);
	}

	//Calculating buffer needed to contain performance levels information returned by ADL_Overdrive6_StateInfo_Get.
	//The buffer contains instance of ADLOD6StateInfo at the beginning and trailing space needed to include extra ADLOD6PerformanceLevel structures.
	//Note that single ADLOD6PerformanceLevel structure is included in ADLOD6StateInfo itself. So the trailing space is needed to be large enough
	//to hold only one extra ADLOD6PerformanceLevel
	int size = sizeof(ADLOD6StateInfo) + sizeof(ADLOD6PerformanceLevel);
	ADLOD6StateInfo * defaultStateInfo = (ADLOD6StateInfo*) malloc(size);
	memset((void*)defaultStateInfo, 0, size);
	defaultStateInfo->iNumberOfPerformanceLevels = 2;

	//Getting default effective minimum and maximum values for memory and GPU clocks.
	//The only state supported by Overdrive6 is "Performance".
	if (ADL_OK != ADL_Overdrive6_StateInfo_Get(adapterId, ADL_OD6_GETSTATEINFO_DEFAULT_PERFORMANCE, defaultStateInfo))
	{
		printf("Failed to get default performance levels info\n");
		exit(1);
	}

	ADLOD6StateInfo * customStateInfo = (ADLOD6StateInfo*) malloc(size);
	memset((void*)customStateInfo, 0, size);
	customStateInfo->iNumberOfPerformanceLevels = 2;

	//Getting default effective minimum and maximum values for memory and GPU clocks.
	//The only state supported by Overdrive6 is "Performance".
	if (ADL_OK != ADL_Overdrive6_StateInfo_Get(adapterId, ADL_OD6_GETSTATEINFO_CUSTOM_PERFORMANCE, customStateInfo))
	{
		printf("Failed to get custom performance levels info\n");
		exit(1);
	}

	if (ADL_OD6_CAPABILITY_SCLK_CUSTOMIZATION == (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_SCLK_CUSTOMIZATION))
	{
		printf("Range of clock supported by GPU core: %d - %d MHz with step of %d MHz\n",
		       od6Capabilities.sEngineClockRange.iMin / 100,
		       od6Capabilities.sEngineClockRange.iMax / 100,
		       od6Capabilities.sEngineClockRange.iStep / 100);

		printf("Default effective range of GPU core clock: %d - %d MHz\n", defaultStateInfo->aLevels[0].iEngineClock / 100, defaultStateInfo->aLevels[1].iEngineClock / 100);
		printf("Custom effective range of GPU core clock: %d - %d MHz\n", customStateInfo->aLevels[0].iEngineClock / 100, customStateInfo->aLevels[1].iEngineClock / 100);
	}

	if (ADL_OD6_CAPABILITY_MCLK_CUSTOMIZATION == (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_MCLK_CUSTOMIZATION))
	{
		printf("Range of supported memory clock: %d - %d MHz with step of %d MHz\n",
		       od6Capabilities.sMemoryClockRange.iMin / 100,
		       od6Capabilities.sMemoryClockRange.iMax / 100,
		       od6Capabilities.sMemoryClockRange.iStep / 100);

		printf("Default effective range of GPU memory clock: %d - %d MHz\n", defaultStateInfo->aLevels[0].iMemoryClock / 100, defaultStateInfo->aLevels[1].iMemoryClock / 100);
		printf("Custom effective range of GPU memory clock: %d - %d MHz\n", customStateInfo->aLevels[0].iMemoryClock / 100, customStateInfo->aLevels[1].iMemoryClock / 100);
	}

	if (ADL_OK != ADL_Overdrive6_CurrentStatus_Get(adapterId, &currentStatus))
	{
		printf("Failed to get custom performance levels info\n");
		exit(1);
	}

	printf("Current GPU core clock: %d MHz\n" , currentStatus.iEngineClock/ 100);
	printf("Current GPU memory clock: %d MHz\n" , currentStatus.iMemoryClock/ 100);
	printf("Current number of PCI bus lanes: %d\n" , currentStatus.iCurrentBusLanes);
	printf("Current PCI bus speed: %d\n" , currentStatus.iCurrentBusSpeed);


	//First we need to verify that ASIC supports monitoring of its current activities before we attempt to retrieve its current clock
	if ((od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR) == ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR)
	{
		printf("Current GPU activity level: %d%%\n" , currentStatus.iActivityPercent);
	}

	if (ADL_OK != ADL_Overdrive6_PowerControl_Caps(adapterId, &powerControlSupported))
	{
		printf("Failed to get power control capabilities\n");
		exit(1);
	}

	if (powerControlSupported)
	{
		if (ADL_OK != ADL_Overdrive6_PowerControlInfo_Get(adapterId, &powerControlInfo))
		{
			printf("Failed to get power control information\n");
			exit(1);
		}

		if (ADL_OK != ADL_Overdrive6_PowerControl_Get(adapterId, &powerControlCurrent, &powerControlDefault))
		{
			printf("Failed to get power control current and default settings\n");
			exit(1);
		}

		printf("Power Control range: %d - %d with step of %d\n", powerControlInfo.iMinValue, powerControlInfo.iMaxValue, powerControlInfo.iStepValue);
		printf("Power Control current level: %d\n", powerControlCurrent);
		printf("Power Control default level: %d\n", powerControlDefault);
	}


	if ( ADL_OD6_TCCAPS_FANSPEED_CONTROL == (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_CONTROL) &&
	     (
		     ((thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_PERCENT_WRITE) == ADL_OD6_TCCAPS_FANSPEED_PERCENT_WRITE)
		     ||
		     ((thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_RPM_WRITE) == ADL_OD6_TCCAPS_FANSPEED_RPM_WRITE)
		     )
		)
	{
		if (fanspeed >= 0) {
			if ((thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_PERCENT_WRITE) == ADL_OD6_TCCAPS_FANSPEED_PERCENT_WRITE) {
				if (fanspeed < thermalControllerCaps.iFanMinPercent || fanspeed > thermalControllerCaps.iFanMaxPercent) {
					printf("Error: Fan speed range %d - %d%%\n", thermalControllerCaps.iFanMinPercent, thermalControllerCaps.iFanMaxPercent);
					exit(1);
				}
			} else {
				if (fanspeed < thermalControllerCaps.iFanMinRPM || fanspeed > thermalControllerCaps.iFanMaxRPM) {
					printf("Error: Fan speed range %d - %d RPM\n", thermalControllerCaps.iFanMinRPM, thermalControllerCaps.iFanMaxRPM);
					exit(1);
				}
			}

			fanSpeedValue.iFanSpeed = fanspeed;
			fanSpeedValue.iSpeedType = ((thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_PERCENT_WRITE) == ADL_OD6_TCCAPS_FANSPEED_PERCENT_WRITE)?
				ADL_OD6_FANSPEED_TYPE_PERCENT :
				ADL_OD6_FANSPEED_TYPE_RPM;

			if (ADL_OK != ADL_Overdrive6_FanSpeed_Set(adapterId, &fanSpeedValue))
			{
				printf("Failed to set new fan speed\n");
				exit(1);
			}

			if ( ADL_OD6_TCCAPS_FANSPEED_CONTROL ==(thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_CONTROL) ) //Verifies that fan speed controller exists on the GPU.
			{
				if (ADL_OD6_TCCAPS_FANSPEED_PERCENT_READ == (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_PERCENT_READ ) ||
				    ADL_OD6_TCCAPS_FANSPEED_RPM_READ == (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_RPM_READ ) )
				{
					/*fanSpeedInfo.iSpeedType = ((thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_RPM_READ) == ADL_OD6_TCCAPS_FANSPEED_RPM_READ)?
					  ADL_OD6_FANSPEED_TYPE_RPM :
					  ADL_OD6_FANSPEED_TYPE_PERCENT;*/

					if (ADL_OK != ADL_Overdrive6_FanSpeed_Get(adapterId, &fanSpeedInfo))
					{
						printf("Failed to get new fan speed info\n");
						exit(1);
					}

					if (ADL_OD6_FANSPEED_TYPE_PERCENT == (fanSpeedInfo.iSpeedType & ADL_OD6_FANSPEED_TYPE_PERCENT))
						printf("New fan speed: %d%%\n", fanSpeedInfo.iFanSpeedPercent);
					else
						printf("New fan speed: %d rpm\n", fanSpeedInfo.iFanSpeedRPM);
				}
			}
		}
	}

	if (ADL_OD6_CAPABILITY_SCLK_CUSTOMIZATION == (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_SCLK_CUSTOMIZATION))
	{
		if (coreclock >= 0 || mincoreclock >= 0)
		{
			if (mincoreclock < 0) mincoreclock = customStateInfo->aLevels[0].iEngineClock / 100;
			if (coreclock < 0) coreclock = customStateInfo->aLevels[1].iEngineClock / 100;
			//ADL_Overdrive6_State_Set allows changing the effective range of memory and core clocks by setting performance level 0 clocks to desired
			//minimum value and setting performance level 1 clocks to desired maximum. In reality user rarely needs to change minimum value.
			//Therefore this example demonstrates only changing the upper range boundary. The upper range value has to be greater then current
			//minimum value (stored in performance level 0), but less than highest clock supported by the ASIC
			if (mincoreclock < od6Capabilities.sEngineClockRange.iMin / 100 || coreclock > od6Capabilities.sEngineClockRange.iMax / 100) {
				printf("Error: Core clock range %d - %d MHz\n", od6Capabilities.sEngineClockRange.iMin / 100, od6Capabilities.sEngineClockRange.iMax / 100);
				exit(1);
			}
			customStateInfo->aLevels[0].iEngineClock = mincoreclock * 100;
			customStateInfo->aLevels[1].iEngineClock = coreclock * 100;

			if (ADL_OK != ADL_Overdrive6_State_Set(adapterId, ADL_OD6_SETSTATE_PERFORMANCE, customStateInfo)) // This is the only value that is currently accepted
			{
				printf("Failed to set new effective core clock\n");
				exit(1);
			}

			if (ADL_OD6_CAPABILITY_SCLK_CUSTOMIZATION == (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_SCLK_CUSTOMIZATION))
				printf("New effective range of GPU core clock: %d - %d MHz\n", customStateInfo->aLevels[0].iEngineClock / 100, customStateInfo->aLevels[1].iEngineClock / 100);
		}
	}

	if (ADL_OD6_CAPABILITY_MCLK_CUSTOMIZATION == (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_MCLK_CUSTOMIZATION))
	{
		if (memclock >= 0 || minmemclock >= 0)
		{
			if (minmemclock < 0) minmemclock = customStateInfo->aLevels[0].iMemoryClock / 100;
			if (memclock < 0) memclock = customStateInfo->aLevels[1].iMemoryClock / 100;
			//ADL_Overdrive6_State_Set allows changing the effective range of memory and core clocks by setting performance level 0 clocks to desired
			//minimum value and setting performance level 1 clocks to desired maximum. In reality user rarely needs to change minimum value.
			//Therefore this example demonstrates only changing the upper range boundary. The upper range value has to be greater then current
			//minimum value (stored in performance level 0), but less than highest clock supported by the ASIC
			if (minmemclock < od6Capabilities.sMemoryClockRange.iMin / 100 || memclock > od6Capabilities.sMemoryClockRange.iMax / 100) {
				printf("Error: Memory clock range %d - %d MHz\n", od6Capabilities.sMemoryClockRange.iMin / 100, od6Capabilities.sMemoryClockRange.iMax / 100);
				exit(1);
			}
			customStateInfo->aLevels[0].iMemoryClock = minmemclock * 100;
			customStateInfo->aLevels[1].iMemoryClock = memclock * 100;

			if (ADL_OK != ADL_Overdrive6_State_Set(adapterId, ADL_OD6_SETSTATE_PERFORMANCE, customStateInfo)) // This is the only value that is currently accepted
			{
				printf("Failed to set new effective memory clock\n");
				exit(1);
			}
			if (ADL_OD6_CAPABILITY_MCLK_CUSTOMIZATION == (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_MCLK_CUSTOMIZATION))
				printf("New effective range of GPU memory clock: %d - %d MHz\n", customStateInfo->aLevels[0].iMemoryClock / 100, customStateInfo->aLevels[1].iMemoryClock / 100);
		}
	}

	if (powerControlSupported)
	{
		if (ptune > -9999)
		{
			if (ptune < powerControlInfo.iMinValue || ptune > powerControlInfo.iMaxValue) {
				printf("Error: Power Control threshold range %d - %d:\n", powerControlInfo.iMinValue, powerControlInfo.iMaxValue);
				exit(1);
			}
			int newPowerControlTreshold = ptune;
			if (ADL_OK != ADL_Overdrive6_PowerControl_Set(adapterId, newPowerControlTreshold))
			{
				printf("Failed to set new Power Control treshold.\n");
				exit(1);
			}
			if (ADL_OK != ADL_Overdrive6_PowerControlInfo_Get(adapterId, &powerControlInfo))
			{
				printf("Failed to get power control information\n");
				exit(1);
			}

			if (ADL_OK != ADL_Overdrive6_PowerControl_Get(adapterId, &powerControlCurrent, &powerControlDefault))
			{
				printf("Failed to get power control new and default settings\n");
				exit(1);
			}

			printf("Power Control new level: %d\n", powerControlCurrent);
		}
	}
	return;
}

static void usage(char *name) {
	printf("Get/set AMD GPU overdrive5/overdrive6 parameters. If -a is omitted, setting(s)\nwill apply to ALL compatible adapters.\n"
	       "Usage: %s [option <arg>] [...]\n"
	       "\t-a <adapter>\n"
	       "\t-f <fanspeed>\n"
	       "\t-C <min core clock>\n"
	       "\t-c <max core clock>\n"
	       "\t-M <min mem clock>\n"
	       "\t-m <max mem clock>\n"
	       "\t-p <powertune level>\n", name);
	exit(0);
}

#if defined(__linux__)
int main(int argc, char* argv[])
#else
int main(int argc, _TCHAR* argv[])
#endif
{
#if defined(__linux__)
	void *hDLL; // Handle to .so library
#else
	HINSTANCE hDLL; // Handle to DLL
#endif

	ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create;
	ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy;
	ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
	ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get;
	ADL_ADAPTER_ACTIVE_GET ADL_Adapter_Active_Get;
	ADL_OVERDRIVE_CAPS ADL_Overdrive_Caps;
	LPAdapterInfo lpAdapterInfo = NULL;
	int i;
	int ADL_Err = ADL_ERR;
	int iNumberAdapters = 0;
	int iOverdriveSupported = 0;
	int iOverdriveEnabled = 0;
	int iOverdriveVersion = 0;
	char c;

	adapter = fanspeed = coreclock = memclock = mincoreclock = minmemclock = ptune = -9999;

	while ((c = getopt(argc, argv, "a:f:c:C:m:M:p:h")) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			break;
		case 'a':
			adapter = atoi(optarg);
			break;
		case 'f':
			fanspeed = atoi(optarg);
			break;
		case 'c':
			coreclock = atoi(optarg);
			break;
		case 'm':
			memclock = atoi(optarg);
			break;
		case 'C':
			mincoreclock = atoi(optarg);
			break;
		case 'M':
			minmemclock = atoi(optarg);
			break;
		case 'p':
			ptune = atoi(optarg);
			break;
		}
	}
	argc -= optind;

	if (argc > 0)
		usage(argv[0]);

#if defined(__linux__)
	hDLL = dlopen( "libatiadlxx.so", RTLD_LAZY|RTLD_GLOBAL);
#else
	hDLL = LoadLibrary("atiadlxx.dll");
	if (hDLL == NULL)
		// A 32 bit calling application on 64 bit OS will fail to LoadLIbrary.
		// Try to load the 32 bit library (atiadlxy.dll) instead
		hDLL = LoadLibrary("atiadlxy.dll");
#endif

	if (NULL == hDLL)
	{
		printf("ADL library not found!\n");
		return 0;
	}

	ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE) GetProcAddress(hDLL,"ADL_Main_Control_Create");
	ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY) GetProcAddress(hDLL,"ADL_Main_Control_Destroy");
	ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET) GetProcAddress(hDLL,"ADL_Adapter_NumberOfAdapters_Get");
	ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET) GetProcAddress(hDLL,"ADL_Adapter_AdapterInfo_Get");
	ADL_Adapter_Active_Get = (ADL_ADAPTER_ACTIVE_GET)GetProcAddress(hDLL, "ADL_Adapter_Active_Get");
	ADL_Overdrive_Caps = (ADL_OVERDRIVE_CAPS)GetProcAddress(hDLL, "ADL_Overdrive_Caps");

	if ( NULL == ADL_Main_Control_Create ||
	     NULL == ADL_Main_Control_Destroy ||
	     NULL == ADL_Adapter_NumberOfAdapters_Get ||
	     NULL == ADL_Adapter_AdapterInfo_Get ||
	     NULL == ADL_Adapter_Active_Get ||
	     NULL == ADL_Overdrive_Caps
		)
	{
		printf("ADL's API is missing!\n");
		return 0;
	}

	// Initialize ADL. The second parameter is 1, which means:
	// retrieve adapter information only for adapters that are physically present and enabled in the system
	if ( ADL_OK != ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1) )
	{
		printf("ADL Initialization Error!\n");
		return 0;
	}

	// Obtain the number of adapters for the system
	if ( ADL_OK != ADL_Adapter_NumberOfAdapters_Get( &iNumberAdapters ) )
	{
		printf("Cannot get the number of adapters!\n");
		return 0;
	}

	if ( 0 < iNumberAdapters )
	{
		lpAdapterInfo = (LPAdapterInfo)malloc( sizeof(AdapterInfo) * iNumberAdapters );
		memset( lpAdapterInfo,'\0', sizeof(AdapterInfo) * iNumberAdapters );

		// Get the AdapterInfo structure for all adapters in the system
		ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters);
	}

	// Look for present and active adapters in the system
	int adapterId = -1;
	for ( i = 0; i < iNumberAdapters; i++ )
	{
		int adapterActive = 0;
		AdapterInfo adapterInfo = lpAdapterInfo[ i ];
		ADL_Adapter_Active_Get(adapterInfo.iAdapterIndex , &adapterActive);
		if ( adapterActive && (adapter < 0 || i == adapter))
		{
			adapterId = adapterInfo.iAdapterIndex;

			printf("\nAdapter %d (%d) ", i, adapterId);

			//Overdrive 5 APIs should be used if returned version indicates 5. Overdrive 6 APIs are used if 6 is returned.
			//Overdrive 5 is supported on legacy ASICs. Newer ASICs (CIK+) should report Overdrive 6
			if ( ADL_OK != ADL_Overdrive_Caps(adapterId, &iOverdriveSupported, &iOverdriveEnabled, &iOverdriveVersion) ) {
				printf("Can't get Overdrive capabilities\n");
				return 1;
			}

			if (!iOverdriveSupported) {
				printf("Overdrive is not supported\n");
				return 1;
			}

			if (iOverdriveVersion == 5) {
				printf("Overdrive5\n");
				Overdrive5Control(adapterId, hDLL);
			} else if(iOverdriveVersion == 6) {
				printf("Overdrive6\n");
				Overdrive6Control(adapterId, hDLL);
			} else
				printf("Unsupported Overdrive version %d\n", iOverdriveVersion);
		}
	}

	if (-1 == adapterId)
	{
		printf("Cannot find any active AMD adapter\n");
		return 1;
	}
	return 0;
}
