package com.soreepeong.audioreceiveplayer;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.ArrayList;
import java.util.Enumeration;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity implements OnClickListener {
	private TextView mViewIP;
	private Button mStartButton;
	private BroadcastReceiver mNetworkListener = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			updateIp();
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

	void updateIp(){
		StringBuilder sb = new StringBuilder();
		for (String s : getLocalIpAddress()) {
			sb.append(s).append("\n");
		}
		mViewIP.setText(sb);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		mViewIP = (TextView) findViewById(R.id.lblIPs);
		mStartButton = (Button) findViewById(R.id.cmdStartStop);
		mStartButton.setOnClickListener(this);
		registerReceiver(mNetworkListener, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
		registerReceiver(mNetworkListener, new IntentFilter(WifiManager.RSSI_CHANGED_ACTION));
		if (WifiAudioPlayerService.isRunning()) {
			mStartButton.setText("Stop");
		} else {
			mStartButton.setText("Start");
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		updateIp();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public void onClick(View v) {
		Intent i = new Intent(this, WifiAudioPlayerService.class);
		if (!WifiAudioPlayerService.isRunning()) {
			startService(i);
			mStartButton.setText("Stop");
		} else {
			stopService(i);
			mStartButton.setText("Start");
		}
	}

}
