#include "stdafx.h"
extern const IID IID_IAudioClient;
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
AudioProcesser::AudioProcesser(){
	memset(&pwfx, 0, sizeof(pwfx));
	memset(&bRunning, 0, sizeof(bRunning));
	memset(&local, 0, sizeof(local));
	memset(&dest, 0, sizeof(dest));
	memset(&sSender, 0, sizeof(sSender));
	memset(&hProcessThread, 0, sizeof(hProcessThread));
	CoInitialize(NULL);
	CoCreateGuid(&gVolumeChange);
}
AudioProcesser::~AudioProcesser() {

}
HRESULT AudioProcesser::SetFormat(WAVEFORMATEX *p){
	memcpy(&pwfx, p, p->cbSize);
	nWaveType=pwfx.Format.wFormatTag;
	data.nSamplesPerSec=pwfx.Format.nSamplesPerSec;

	if (pTo != NULL){
		HRESULT hr;
		hr = pTo->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pToClient);
		EXIT_ON_ERROR(hr);
		hr = pToClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, p, NULL);
		EXIT_ON_ERROR(hr);
		hr = pToClient->GetService(IID_IAudioRenderClient, (void**)&pToRender);
		EXIT_ON_ERROR(hr);
		pToClient->Start();
	Exit:;
	}

	if(nWaveType==WAVE_FORMAT_EXTENSIBLE){
		memcpy(&pwfx, p, sizeof(pwfx));
		if(IsEqualGUID(pwfx.SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
			nWaveType=WAVE_FORMAT_IEEE_FLOAT;
		else if(IsEqualGUID(pwfx.SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
			nWaveType=WAVE_FORMAT_PCM;
		else
			return ERROR_UNKNOWN_FEATURE;
	}
	return ERROR_SUCCESS;
}

HRESULT AudioProcesser::CopyData(BYTE* pData, UINT32 numFramesAvailable, BOOL *bDone){
	*bDone=!bRunning;
	if(pData!=NULL){
		int i;
		int nChannels = pwfx.Format.nChannels;
		int nDestChannels = 2;
		data.nChannelConfiguration = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT; // pwfx.dwChannelMask;

		if (pToRender != NULL){
			BYTE *pTempData;
			HRESULT hr;
			hr = pToRender->GetBuffer(numFramesAvailable, &pTempData);
			EXIT_ON_ERROR(hr);
			memcpy(pTempData, pData, numFramesAvailable * pwfx.Format.nChannels * pwfx.Format.wBitsPerSample / 8);
			pToRender->ReleaseBuffer(numFramesAvailable, 0);
			Exit:;
		}

		for (i = numFramesAvailable*nChannels - 1; i >= 0; i--)
			if (pData[i] != 0)
				break;
		if (i >= 0) { // Not empty!
			int pass = 0;
			int bytes = pwfx.Format.wBitsPerSample / 8;
			data.nBufferLength = 0;
			for(i=0;i<(int)numFramesAvailable;i++){
				if (data.nBufferLength + nDestChannels*sizeof(short) > sizeof(AUDIOPROCESSER_DATA::wSend)) {
					data.index = nAudioIndex++;
					if (sendto(sSender, (char*)&data, data.nBufferLength + offsetof(AUDIOPROCESSER_DATA, wSend), 0, (sockaddr *)&dest, sizeof(sockaddr_in)) == -1)
						return WSAGetLastError();
					data.nBufferLength = 0;
				}
				for(int j=0;j<nChannels && j<nDestChannels;j++) {
					union {
						float dbl ;
						int nCurrentValue = 0;
					};
					memcpy(((char*)&dbl) + sizeof(dbl) - bytes, pData + (i*nChannels + j)*bytes, bytes);
					if(nWaveType==WAVE_FORMAT_PCM)
						dbl=(short)(nCurrentValue>>(pwfx.Format.wBitsPerSample-16))/32767.f;
					data.wSend[data.nBufferLength/2+j]=(short)(dbl*32767+0.5);
				}
				data.nBufferLength += nDestChannels*sizeof(short);
			}
			if (data.nBufferLength > 0) {
				data.index = nAudioIndex++;
				if (sendto(sSender, (char*)&data, data.nBufferLength + offsetof(AUDIOPROCESSER_DATA, wSend), 0, (sockaddr *)&dest, sizeof(sockaddr_in)) == -1)
					return WSAGetLastError();
			}
		}
	}
	return ERROR_SUCCESS;
}
DWORD WINAPI AudioProcesser::BackgroundProcess(LPVOID arg){
	CoInitialize(NULL);
	while (bRunning){
		if(RecordAudioStream(this, pFrom)==ERROR_SUCCESS) break;
		SAFE_RELEASE(pToClient);
		SAFE_RELEASE(pToRender);
		Sleep(1);
	}
	CoUninitialize();
	return ERROR_SUCCESS;
}
DWORD WINAPI AudioProcesser_BackgroundProcessStub(LPVOID *ptr) {
	AudioProcesser *ap=(AudioProcesser*)ptr;
	return ap->BackgroundProcess(ptr);
}
HRESULT AudioProcesser::StartSending(char *sRemoteAddr, TCHAR *selFrom, TCHAR *selTo){
	if(bRunning)
		return ERROR_ALREADY_INITIALIZED;
	pFrom = getDevice(selFrom);
	if (lstrcmp(selFrom, selTo) == 0)
		pTo = 0;
	else
		pTo = getDevice(selTo);
	__try{
		int d1, d2, d3, d4;
		WSADATA data;
		if(WSAStartup( MAKEWORD(2, 2), &data)!=0)
			return WSAGetLastError();

		memset(&local, 0, sizeof(local));
		local.sin_family=AF_INET;
		local.sin_port=htons(0); // Choose any
		local.sin_addr.S_un.S_addr=INADDR_ANY;
	
		memset(&dest, 0, sizeof(dest));
		dest.sin_family=AF_INET;
		dest.sin_port=htons(0x7d01);
		d1=d2=d3=d4=-1;
		sscanf_s(sRemoteAddr, "%d.%d.%d.%d", &d1, &d2, &d3, &d4);
		if(d1>=0 && d1<=255 && d2>=0 && d2<=255 && d3>=0 && d3<=255 && d4>=0 && d4<=255){
			dest.sin_addr.S_un.S_un_b.s_b1 = (unsigned char)d1;
			dest.sin_addr.S_un.S_un_b.s_b2 = (unsigned char)d2;
			dest.sin_addr.S_un.S_un_b.s_b3 = (unsigned char)d3;
			dest.sin_addr.S_un.S_un_b.s_b4 = (unsigned char)d4;
		}else{
			// TODO: Process address
		}
		if((sSender=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==NULL) return WSAGetLastError();
		if(bind(sSender, (sockaddr*)&local, sizeof(sockaddr_in))==-1) return WSAGetLastError();
		hProcessThread=CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)AudioProcesser_BackgroundProcessStub, this, NULL, NULL);
		bRunning=true;

		pFrom->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pFromVol);
		pTo->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pToVol);
		if (pFromVol != NULL && pToVol != NULL) {
			pFromVol->RegisterControlChangeNotify(this);
			pToVol->RegisterControlChangeNotify(this);
		}

		return ERROR_SUCCESS;
	}__finally{
		if(!bRunning){
			if(sSender) closesocket(sSender);
			sSender=NULL;
			WSACleanup();
		}
	}
	return ERROR_SUCCESS;
}
HRESULT AudioProcesser::StopSending(){
	if(!bRunning) return ERROR_NOT_READY;
	bRunning = false;
	if (pFromVol != NULL && pToVol != NULL) {
		pFromVol->UnregisterControlChangeNotify(this);
		pToVol->UnregisterControlChangeNotify(this);
	}
	SAFE_RELEASE(pFromVol);
	SAFE_RELEASE(pToVol);
	SAFE_RELEASE(pFrom);
	SAFE_RELEASE(pTo);
	CloseHandle(hProcessThread);
	return ERROR_SUCCESS;
}
BOOL AudioProcesser::isRuninng(){
	return bRunning;
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
		printf("%d %.4f\n", (int)pNotify->bMuted, pNotify->fMasterVolume);
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