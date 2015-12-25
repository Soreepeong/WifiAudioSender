// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Windows Header Files:
#include<Windows.h>
#include<Audioclient.h>
#include<Mmdeviceapi.h>
#include<Endpointvolume.h>
#include<Shellapi.h>
#include <tchar.h>
#include <Functiondiscoverykeys_devpkey.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include<stdio.h>

#include "AudioProcesser.h"
HRESULT RecordAudioStream(AudioProcesser *pMySink, IMMDevice *pDevice);
IMMDeviceCollection* getAudioDevices();
IMMDevice* getDevice(TCHAR *pId);
#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
			                  { (punk)->Release(); (punk) = NULL; }