package com.soreepeong.audioreceiveplayer;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.ArrayList;
import java.util.Enumeration;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

public class MainActivity extends Activity implements OnClickListener, WifiAudioPlayerService.OnAudioStatusChangeListener, SeekBar.OnSeekBarChangeListener {
	private TextView mViewIP, mBufferSizeText;
	private Button mStartButton;
	private SeekBar mBufferSizeBar;
	private BroadcastReceiver mNetworkListener = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			updateIp();
		}
	};
	private WifiAudioPlayerService mBoundService;
	private String mIpInfo = "";
	private Handler mHandler = new Handler();
	private final Runnable mUpdateBufferInfo = new Runnable() {
		@Override
		public void run() {
			if(mBoundService == null) {
				mViewIP.setText(mIpInfo);
				return;
			}
			mViewIP.setText(getString(R.string.status,
					mBoundService.getAudio().getBufferCount(),
					mBoundService.getAudio().getDropped(), mIpInfo));
			mHandler.removeCallbacks(this);
			if (mBoundService != null && mBoundService.getAudio().isRunning())
				mHandler.postDelayed(this, 100);
		}
	};

	private ServiceConnection mConnection = new ServiceConnection() {
		public void onServiceConnected(ComponentName className, IBinder service) {
			mBoundService = ((WifiAudioPlayerService.LocalBinder) service).getService();
			mBoundService.addStatusCallback(MainActivity.this);
			OnAudioStatusChanged(mBoundService.getAudio().isRunning());
			mBufferSizeBar.setProgress(mBoundService.getAudio().getMaxData() - 1);
		}

		public void onServiceDisconnected(ComponentName className) {
			mBoundService = null;
		}
	};

	public String[] getLocalIpAddress() {
		ArrayList<String> addresses = new ArrayList<>();
		try {
			for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
				NetworkInterface intf = en.nextElement();
				for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
					InetAddress inetAddress = enumIpAddr.nextElement();
					if (!inetAddress.isLoopbackAddress()) {
						addresses.add(inetAddress.getHostAddress());
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return addresses.toArray(new String[addresses.size()]);
	}

	void updateIp() {
		StringBuilder sb = new StringBuilder();
		for (String s : getLocalIpAddress()) {
			sb.append(s).append("\n");
		}
		mIpInfo = sb.toString();
		mUpdateBufferInfo.run();
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		mViewIP = findViewById(R.id.ip_list_label);
		mStartButton = findViewById(R.id.start_stop_button);
		mBufferSizeBar = findViewById(R.id.buffer_size);
		mBufferSizeText = findViewById(R.id.buffer_size_text);
		mStartButton.setOnClickListener(this);
		mBufferSizeBar.setMax(AudioReceiver.MAX_DATA - 1);
		mBufferSizeBar.setOnSeekBarChangeListener(this);
		registerReceiver(mNetworkListener, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
		registerReceiver(mNetworkListener, new IntentFilter(WifiManager.RSSI_CHANGED_ACTION));

		startService(new Intent(MainActivity.this, WifiAudioPlayerService.class));
		bindService(new Intent(MainActivity.this, WifiAudioPlayerService.class), mConnection, Context.BIND_AUTO_CREATE);
	}

	@Override
	protected void onResume() {
		super.onResume();
		updateIp();
	}

	@Override
	protected void onDestroy() {
		boolean run = mBoundService.getAudio().isRunning();
		mBoundService.removeStatusCallback(this);
		unregisterReceiver(mNetworkListener);
		unbindService(mConnection);
		if(!run)
			stopService(new Intent(MainActivity.this, WifiAudioPlayerService.class));
		super.onDestroy();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public void onClick(View v) {
		if (mBoundService == null)
			return;
		Intent i = new Intent(this, WifiAudioPlayerService.class);
		if (!mBoundService.getAudio().isRunning()) {
			mBoundService.startAudio();
		} else {
			mBoundService.stopAudio();
		}
	}

	@Override
	public void OnAudioStatusChanged(boolean running) {
		mUpdateBufferInfo.run();
		if (running) {
			mStartButton.setText(R.string.btn_stop);
		} else {
			mStartButton.setText(R.string.btn_start);
		}
	}

	@Override
	public void onProgressChanged(SeekBar seekBar, int i, boolean fromUser) {
		mBufferSizeText.setText(getString(R.string.buffer_count, mBufferSizeBar.getProgress() + 1));
		if (fromUser) {
			mBoundService.getAudio().setMaxData(i + 1);
		}
	}

	@Override
	public void onStartTrackingTouch(SeekBar seekBar) {
	}

	@Override
	public void onStopTrackingTouch(SeekBar seekBar) {
	}
}
