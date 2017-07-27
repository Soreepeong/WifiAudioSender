package com.soreepeong.audioreceiveplayer;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;

public class EarphoneStatusChangeReceiver extends BroadcastReceiver {
	@Override
	public void onReceive(Context ctx, Intent intent) {
		if (intent.getAction().equals(AudioManager.ACTION_AUDIO_BECOMING_NOISY)) {
			if (WifiAudioPlayerService.isRunning()) {
				Intent i = new Intent(ctx, WifiAudioPlayerService.class);
				ctx.stopService(i);
			}
		}
	}
}
