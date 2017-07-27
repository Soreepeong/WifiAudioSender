package com.soreepeong.audioreceiveplayer;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class WifiAudioPlayerService extends Service {
	public static final int NOTIFICATION = 1;
	private AudioReceiver mAudio = new AudioReceiver();
	private Notification mNotification;
	private static boolean bRunning = false;

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		mAudio.start();
		Notification.Builder bd = new Notification.Builder(this);
		bd.setSmallIcon(R.drawable.ic_launcher);
		bd.setPriority(Notification.PRIORITY_MIN);
		bd.setAutoCancel(false);
		bd.setOngoing(true);
		Intent i = new Intent(this, MainActivity.class);
		i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
		bd.setContentIntent(PendingIntent.getActivity(this, 0, i, PendingIntent.FLAG_UPDATE_CURRENT));
		bd.setContentTitle("Wifi Audio Receiver");
		bd.setContentText("Tap to change settings");
		mNotification = bd.build();
		startForeground(NOTIFICATION, mNotification);
		bRunning = true;
		return START_NOT_STICKY;
	}

	@Override
	public void onDestroy() {
		mAudio.stop();
		stopForeground(true);
		bRunning = false;
		super.onDestroy();
	}

	public static boolean isRunning() {
		return bRunning;
	}
}
