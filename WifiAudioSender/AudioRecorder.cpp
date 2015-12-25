#include"stdafx.h"
//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond


const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

IMMDeviceCollection* getAudioDevices(){
	HRESULT hr;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDeviceCollection *pDevices;
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
	EXIT_ON_ERROR(hr);
	hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
	EXIT_ON_ERROR(hr);
	return pDevices;
Exit:
	SAFE_RELEASE(pEnumerator);
	SAFE_RELEASE(pDevices);
	return 0;
}

IMMDevice* getDevice(TCHAR *pId){
	IMMDeviceCollection* pDevices = getAudioDevices();
	IMMDevice *pDevice;
	WCHAR *pName;
	IPropertyStore *pProps = NULL;
	UINT count;
	HRESULT hr = NULL;
	if (pDevices == NULL)
		return 0;
	pDevices->GetCount(&count);
	for (UINT i = 0; i < count; i++){
		hr = pDevices->Item(i, &pDevice);
		EXIT_ON_ERROR(hr);
		hr = pDevice->GetId(&pName);
		EXIT_ON_ERROR(hr);
		if (lstrcmp(pName, pId) == 0){
			goto Exit;
		}
		CoTaskMemFree(pName);
		SAFE_RELEASE(pDevice);
	}
Exit:
	SAFE_RELEASE(pDevices);
	return pDevice;
}

HRESULT RecordAudioStream(AudioProcesser *pMySink, IMMDevice *pDevice) {
    HRESULT hr;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
    WAVEFORMATEX *pwfx = NULL;
    UINT32 packetLength = 0;
    BOOL bDone = FALSE;
    BYTE *pData;
    DWORD flags;

    // hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    // EXIT_ON_ERROR(hr)

    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pwfx, NULL);
    EXIT_ON_ERROR(hr)

    // Get the size of the allocated buffer.
    hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    EXIT_ON_ERROR(hr)

    hr = pMySink->SetFormat(pwfx);
    EXIT_ON_ERROR(hr)

    hr = pAudioClient->Start();  // Start recording.
    EXIT_ON_ERROR(hr)

    // Each loop fills about half of the shared buffer.
    while (bDone == FALSE) {
        Sleep(1);
        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        EXIT_ON_ERROR(hr)

        while (packetLength != 0) {
            // Get the available data in the shared buffer.
            hr = pCaptureClient->GetBuffer(
                                   &pData,
                                   &numFramesAvailable,
                                   &flags, NULL, NULL);
            EXIT_ON_ERROR(hr)

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                pData = NULL;  // Tell CopyData to write silence.
            }

            // Copy the available capture data to the audio sink.
            hr = pMySink->CopyData(pData, numFramesAvailable, &bDone);

            EXIT_ON_ERROR(hr)

            hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
            EXIT_ON_ERROR(hr)

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            EXIT_ON_ERROR(hr)
        }
    }

    hr = pAudioClient->Stop();  // Stop recording.
    EXIT_ON_ERROR(hr)

Exit:
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(pCaptureClient)

    return hr;
}
