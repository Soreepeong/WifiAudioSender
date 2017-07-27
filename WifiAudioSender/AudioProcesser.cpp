#include "stdafx.h"
extern const IID IID_IAudioClient;
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
AudioProcesser::AudioProcesser() {
	memset(&mAudioFormat, 0, sizeof(mAudioFormat));
	memset(&mIsRunning, 0, sizeof(mIsRunning));
	memset(&mLocalAddress, 0, sizeof(mLocalAddress));
	memset(&mSocket, 0, sizeof(mSocket));
	memset(&hProcessThread, 0, sizeof(hProcessThread));
	CoInitialize(NULL);
	CoCreateGuid(&gVolumeChange);
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
}
AudioProcesser::~AudioProcesser() {

}
HRESULT AudioProcesser::SetFormat(WAVEFORMATEX *p) {
	memcpy(&mAudioFormat, p, sizeof(WAVEFORMATEX) + p->cbSize);
	nWaveType = mAudioFormat.Format.wFormatTag;
	mSendBuffer.nSamplesPerSec = mAudioFormat.Format.nSamplesPerSec;

	if (pTo != NULL) {
		HRESULT hr;
		hr = pTo->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**) &pToClient);
		EXIT_ON_ERROR(hr);
		hr = pToClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, p, NULL);
		EXIT_ON_ERROR(hr);
		hr = pToClient->GetService(IID_IAudioRenderClient, (void**) &pToRender);
		EXIT_ON_ERROR(hr);
		pToClient->Start();
		goto suc;
Exit:;
		SAFE_RELEASE(pToClient);
		SAFE_RELEASE(pToRender);
suc:;
	}

	if (nWaveType == WAVE_FORMAT_EXTENSIBLE) {
		memcpy(&mAudioFormat, p, sizeof(mAudioFormat));
		if (IsEqualGUID(mAudioFormat.SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
			nWaveType = WAVE_FORMAT_IEEE_FLOAT;
		else if (IsEqualGUID(mAudioFormat.SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
			nWaveType = WAVE_FORMAT_PCM;
		else
			return ERROR_UNKNOWN_FEATURE;
	}
	return ERROR_SUCCESS;
}

HRESULT AudioProcesser::CopyData(BYTE* pData, UINT32 numFramesAvailable, BOOL *bDone) {
	if (bDone)
		*bDone = !mIsRunning;
	if (pData != NULL) {
		int i;
		int nChannels = mAudioFormat.Format.nChannels;
		int nDestChannels = 2;
		mSendBuffer.nChannelConfiguration = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT; // pwfx.dwChannelMask;

		if (pToRender != NULL) {
			BYTE *pTempData;
			HRESULT hr;
			hr = pToRender->GetBuffer(numFramesAvailable, &pTempData);
			EXIT_ON_ERROR(hr);
			memcpy(pTempData, pData, numFramesAvailable * mAudioFormat.Format.nChannels * mAudioFormat.Format.wBitsPerSample / 8);
			pToRender->ReleaseBuffer(numFramesAvailable, 0);
Exit:;
		}

		for (i = numFramesAvailable*nChannels - 1; i >= 0; i--)
			if (pData[i] != 0)
				break;
		if (i >= 0) { // Not empty!
			int pass = 0;
			int bytes = mAudioFormat.Format.wBitsPerSample / 8;
			mSendBuffer.nBufferLength = 0;
			for (i = 0; i < (int) numFramesAvailable; i++) {
				if (mSendBuffer.nBufferLength + nDestChannels * sizeof(short) > sizeof(AUDIOPROCESSER_DATA::wSend)) {
					mSendBuffer.index = nAudioIndex++;
					for (auto dest = mDest.cbegin(); dest != mDest.cend(); ++dest)
						if (sendto(mSocket, (char*) &mSendBuffer, mSendBuffer.nBufferLength + offsetof(AUDIOPROCESSER_DATA, wSend), 0, (sockaddr *) &*dest, sizeof(sockaddr_in)) == -1)
							continue;
					mSendBuffer.nBufferLength = 0;
				}
				for (int j = 0; j < nChannels && j < nDestChannels; j++) {
					union {
						float dbl;
						int nCurrentValue = 0;
					};
					memcpy(((char*) &dbl) + sizeof(dbl) - bytes, pData + (i*nChannels + j)*bytes, bytes);
					if (nWaveType == WAVE_FORMAT_PCM)
						dbl = (short) (nCurrentValue >> (mAudioFormat.Format.wBitsPerSample - 16)) / 32767.f;
					mSendBuffer.wSend[mSendBuffer.nBufferLength / 2 + j] = (short) (dbl * 32767 + 0.5);
				}
				mSendBuffer.nBufferLength += nDestChannels * sizeof(short);
			}
			if (mSendBuffer.nBufferLength > 0) {
				mSendBuffer.index = nAudioIndex++;
				for (auto dest = mDest.cbegin(); dest != mDest.cend(); ++dest)
					if (sendto(mSocket, (char*) &mSendBuffer, mSendBuffer.nBufferLength + offsetof(AUDIOPROCESSER_DATA, wSend), 0, (sockaddr *) &*dest, sizeof(sockaddr_in)) == -1)
						continue;
			}
		}
	}
	return ERROR_SUCCESS;
}
DWORD AudioProcesser::BackgroundProcess() {
	CoInitialize(NULL);
	while (mIsRunning) {
		switch (mFromMode) {
			case 0:
			{
				// Empty audio
				WAVEFORMATEX ex = { sizeof(WAVEFORMATEX) };
				ex.wFormatTag = WAVE_FORMAT_PCM;
				ex.nChannels = 2;
				ex.nSamplesPerSec = 44100;
				ex.nAvgBytesPerSec = 176400;
				ex.nBlockAlign = 4;
				ex.wBitsPerSample = 16;
				SetFormat(&ex);

				while (mIsRunning) {
					BYTE *pTempData;
					HRESULT hr;
					int availableFrames = 448;
					hr = pToRender->GetBuffer(availableFrames, &pTempData);
					EXIT_ON_ERROR(hr);
					memset(pTempData, 0, availableFrames * mAudioFormat.Format.nChannels * mAudioFormat.Format.wBitsPerSample / 8);
					pToRender->ReleaseBuffer(availableFrames, 0);
				}
Exit:;
				break;
			}
			case 1:
			{
				AUDIOPROCESSER_DATA data;
				sockaddr out;
				int out_len = sizeof(sockaddr);
				while (mIsRunning) {
					int read = recvfrom(mSocket, (char*) &data, sizeof(data), 0, &out, &out_len);
					if (read <= 0) {
						read = WSAGetLastError();
						if(read)
							break;
					}
					if (mAudioFormat.Format.nSamplesPerSec != data.nSamplesPerSec) {
						WAVEFORMATEX ex = { 0 };
						ex.wFormatTag = WAVE_FORMAT_PCM;
						ex.nChannels = 2;
						ex.wBitsPerSample = 16;
						ex.nBlockAlign = 4;
						ex.nSamplesPerSec = data.nSamplesPerSec;
						ex.nAvgBytesPerSec = ex.nSamplesPerSec * ex.nBlockAlign;
						ex.cbSize = 0;
						SetFormat(&ex);
					}
					CopyData((BYTE*) data.wSend, data.nBufferLength / mAudioFormat.Format.nBlockAlign, 0);
				}
				break;
			}
			default:
			{
				if (RecordAudioStream(this, pFrom) == ERROR_SUCCESS)
					mIsRunning = false;
			}
		}
		SAFE_RELEASE(pToClient);
		SAFE_RELEASE(pToRender);
		Sleep(1);
	}
	CoUninitialize();
	return ERROR_SUCCESS;
}
DWORD WINAPI AudioProcesser::BackgroundProcessExternal(LPVOID *ptr) {
	AudioProcesser *ap = (AudioProcesser*) ptr;
	return ap->BackgroundProcess();
}
HRESULT AudioProcesser::StartSending(char *sRemoteAddr, TCHAR *selFrom, TCHAR *selTo) {
	if (mIsRunning)
		return ERROR_ALREADY_INITIALIZED;
	memset(&mAudioFormat, 0, sizeof(mAudioFormat));
	mFromMode = (int) selFrom;
	pFrom = (mFromMode == 0 || mFromMode == 1) ? 0 : getDevice(selFrom);
	pTo = selTo == NULL || (pFrom && lstrcmp(selFrom, selTo) == 0) ? NULL : getDevice(selTo);
	__try {
		mDest.clear();
		if (strlen(sRemoteAddr)) {
			int d1, d2, d3, d4;

			do {
				d1 = d2 = d3 = d4 = -1;
				if (sscanf_s(sRemoteAddr, "%d.%d.%d.%d", &d1, &d2, &d3, &d4) < 4)
					continue;

				sockaddr_in dest;
				memset(&dest, 0, sizeof(dest));
				dest.sin_family = AF_INET;
				dest.sin_port = htons(0x7d01);
				if (d1 >= 0 && d1 <= 255 && d2 >= 0 && d2 <= 255 && d3 >= 0 && d3 <= 255 && d4 >= 0 && d4 <= 255) {
					dest.sin_addr.S_un.S_un_b.s_b1 = (unsigned char) d1;
					dest.sin_addr.S_un.S_un_b.s_b2 = (unsigned char) d2;
					dest.sin_addr.S_un.S_un_b.s_b3 = (unsigned char) d3;
					dest.sin_addr.S_un.S_un_b.s_b4 = (unsigned char) d4;
					mDest.push_back(dest);
				} else {
					// TODO: Process address
				}
			} while (sRemoteAddr = strstr(sRemoteAddr + 1, " "));
		}

		if (!mDest.empty() || mFromMode == 1) {
			memset(&mLocalAddress, 0, sizeof(mLocalAddress));
			mLocalAddress.sin_family = AF_INET;
			mLocalAddress.sin_port = htons(0x7d01); // Choose any
			mLocalAddress.sin_addr.S_un.S_addr = INADDR_ANY;
			if ((mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == NULL) return WSAGetLastError();
			if (bind(mSocket, (sockaddr*) &mLocalAddress, sizeof(sockaddr_in)) == -1) return WSAGetLastError();
		}

		hProcessThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) BackgroundProcessExternal, this, NULL, NULL);
		mIsRunning = true;

		if (pFrom)
			pFrom->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**) &pFromVol);
		if (pTo)
			pTo->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**) &pToVol);

		if (pFromVol != NULL && pToVol != NULL) {
			pFromVol->RegisterControlChangeNotify(this);
			pToVol->RegisterControlChangeNotify(this);
		}

		return ERROR_SUCCESS;
	} __finally {
		if (!mIsRunning) {
			if (mSocket) closesocket(mSocket);
			mSocket = NULL;
		}
	}
	return ERROR_SUCCESS;
}
HRESULT AudioProcesser::StopSending() {
	if (!mIsRunning) return ERROR_NOT_READY;
	mIsRunning = false;
	if (pFromVol != NULL && pToVol != NULL) {
		pFromVol->UnregisterControlChangeNotify(this);
		pToVol->UnregisterControlChangeNotify(this);
	}
	if (mSocket) closesocket(mSocket);
	mSocket = NULL;
	SAFE_RELEASE(pFromVol);
	SAFE_RELEASE(pToVol);
	SAFE_RELEASE(pFrom);
	SAFE_RELEASE(pTo);
	CloseHandle(hReceiverThread);
	CloseHandle(hProcessThread);
	return ERROR_SUCCESS;
}
BOOL AudioProcesser::isRuninng() {
	return mIsRunning;
}
HRESULT STDMETHODCALLTYPE AudioProcesser::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) {
	if (pNotify == NULL)
		return E_INVALIDARG;
	if (pNotify->guidEventContext != gVolumeChange) {
		BOOL mute;
		float vol;
		if (pFromVol->GetMute(&mute) == S_OK && mute != pNotify->bMuted)
			pFromVol->SetMute(pNotify->bMuted, &gVolumeChange);
		if (pToVol->GetMute(&mute) == S_OK && mute != pNotify->bMuted)
			pToVol->SetMute(pNotify->bMuted, &gVolumeChange);
		if (pFromVol->GetMasterVolumeLevelScalar(&vol) == S_OK && vol != pNotify->fMasterVolume)
			pFromVol->SetMasterVolumeLevelScalar(pNotify->fMasterVolume, &gVolumeChange);
		if (pToVol->GetMasterVolumeLevelScalar(&vol) == S_OK && vol != pNotify->fMasterVolume)
			pToVol->SetMasterVolumeLevelScalar(pNotify->fMasterVolume, &gVolumeChange);
		printf("%d %.4f\n", (int) pNotify->bMuted, pNotify->fMasterVolume);
	}
	return S_OK;
}

ULONG WINAPI AudioProcesser::AddRef() {
	return InterlockedIncrement(&g_ref);
}
ULONG WINAPI AudioProcesser::Release() {
	return InterlockedDecrement(&g_ref);
}
HRESULT WINAPI AudioProcesser::QueryInterface(REFIID riid, VOID **ppvInterface) {
	if (IID_IUnknown == riid) {
		AddRef();
		*ppvInterface = (IUnknown*)this;
	} else if (__uuidof(IAudioEndpointVolumeCallback) == riid) {
		AddRef();
		*ppvInterface = (IAudioEndpointVolumeCallback*)this;
	} else {
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}