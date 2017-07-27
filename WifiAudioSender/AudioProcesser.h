#include"stdafx.h"
#include <vector>

class AudioProcesser : public IAudioEndpointVolumeCallback {
private:
	typedef struct _AUDIOPROCESSER_DATA{
		int index;
		int nChannelConfiguration;
		int nSamplesPerSec;
		int nBufferLength;
		short wSend[512];
	} AUDIOPROCESSER_DATA;
	AUDIOPROCESSER_DATA mSendBuffer;
	WAVEFORMATEXTENSIBLE mAudioFormat;
	BOOL mIsRunning;
	sockaddr_in mLocalAddress;
	std::vector<sockaddr_in> mDest;
	SOCKET mSocket;
	HANDLE hProcessThread;
	HANDLE hReceiverThread;
	int nWaveType;
	long nAudioIndex;
	int mFromMode;
	IMMDevice *pFrom, *pTo;
	IAudioClient *pToClient;
	IAudioRenderClient *pToRender;
	IAudioEndpointVolume *pFromVol, *pToVol;
	GUID gVolumeChange;
	long g_ref;

public:
	AudioProcesser();
	~AudioProcesser();
	DWORD BackgroundProcess();
	static DWORD WINAPI BackgroundProcessExternal(LPVOID *ptr);
	HRESULT SetFormat(WAVEFORMATEX *pwfx);
	HRESULT CopyData(BYTE* pData, UINT32 numFramesAvailable, BOOL *bDone);
	HRESULT StartSending(char *sRemoteAddr, TCHAR *selFrom, TCHAR *selTo);
	HRESULT StopSending();
	BOOL isRuninng();
	HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI QueryInterface(REFIID riid, VOID **ppvInterface);
};