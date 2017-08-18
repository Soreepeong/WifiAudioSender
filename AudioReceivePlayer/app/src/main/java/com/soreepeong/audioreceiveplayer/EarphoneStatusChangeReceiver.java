package com.soreepeong.audioreceiveplayer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.IBinder;

public class EarphoneStatusChangeReceiver extends BroadcastReceiver {
	@Override
	public void onReceive(Context ctx, Intent intent) {
		if (intent.getAction().equals(AudioManager.ACTION_AUDIO_BECOMING_NOISY)) {
			IBinder binder = peekService(ctx, new Intent(ctx, WifiAudioPlayerService.class));
			if(binder == null)
				return;
			WifiAudioPlayerService service = ((WifiAudioPlayerService.LocalBinder) binder).getService();
			if (service.getAudio().isRunning())
				service.stopAudio();
		}
	}
}
