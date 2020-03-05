package com.soreepeong.audioreceiveplayer;

import android.app.*;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import androidx.core.app.NotificationCompat;

import java.util.ArrayList;
import java.util.WeakHashMap;

public class WifiAudioPlayerService extends Service {
	public static final int NOTIFICATION = 1;
	private final AudioReceiver mAudio = new AudioReceiver();
	private final IBinder mBinder = new LocalBinder();
	private final WeakHashMap<OnAudioStatusChangeListener, Object> mListeners = new WeakHashMap<>();

	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}

	public void addStatusCallback(OnAudioStatusChangeListener l){
		mListeners.put(l, null);
	}

	public void removeStatusCallback(OnAudioStatusChangeListener l){
		mListeners.remove(l);
	}

	public class LocalBinder extends Binder {
		WifiAudioPlayerService getService() {
			return WifiAudioPlayerService.this;
		}
	}

	public AudioReceiver getAudio(){
		return mAudio;
	}

	@Override
	public void onCreate() {
		super.onCreate();
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			ArrayList<NotificationChannel> channels = new ArrayList<>();
			NotificationChannel channel;

			channel = new NotificationChannel(
					"WAR",
					getString(R.string.app_name),
					NotificationManager.IMPORTANCE_DEFAULT);
			channel.setDescription(getString(R.string.app_name));
			channels.add(channel);

			NotificationManager notificationManager = (NotificationManager) getSystemService(
					NOTIFICATION_SERVICE);
			assert notificationManager != null;
			notificationManager.createNotificationChannels(channels);
		}
	}

	public void startAudio(){
		mAudio.start();
		Notification mNotification;
		NotificationCompat.Builder bd = new NotificationCompat.Builder(this, "WAR");
		bd.setSmallIcon(R.drawable.ic_launcher);
		bd.setPriority(NotificationCompat.PRIORITY_MIN);
		bd.setAutoCancel(false);
		bd.setOngoing(true);
		Intent i = new Intent(this, MainActivity.class);
		i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
		bd.setContentIntent(PendingIntent.getActivity(this, 0, i, PendingIntent.FLAG_UPDATE_CURRENT));
		bd.setContentTitle(getString(R.string.app_name));
		bd.setContentText(getString(R.string.noti_desc));
		mNotification = bd.build();
		startForeground(NOTIFICATION, mNotification);
		for(OnAudioStatusChangeListener l : mListeners.keySet())
			l.OnAudioStatusChanged(true);
	}

	public void stopAudio(){
		mAudio.stop();
		SharedPreferences.Editor mPref = getSharedPreferences("service_config", 0).edit();
		mPref.putInt("max", mAudio.getMaxData());
		mPref.apply();
		stopForeground(true);
		for(OnAudioStatusChangeListener l : mListeners.keySet())
			l.OnAudioStatusChanged(false);
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		SharedPreferences mPref = getSharedPreferences("service_config", 0);
		mAudio.setMaxData(mPref.getInt("max", 8));
		return START_NOT_STICKY;
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
	}

	public interface OnAudioStatusChangeListener{
		void OnAudioStatusChanged(boolean running);
	}
}
