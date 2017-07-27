package com.soreepeong.audioreceiveplayer;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;

import android.media.AudioFormat;
import android.media.AudioTrack;

public class AudioReceiver implements Runnable {
	private static final int AUDIO_TRACK_BUFFER_SIZE = 0x10000; // About stutters
	private DatagramSocket mReceiverSocket;
	private Thread mReceiverThread = null;
	private boolean mRunning = false;
	private final ArrayList<ByteBuffer> mData = new ArrayList<>();

	@Override
	public void run() {
		ByteBuffer buffer = ByteBuffer.allocate(2048);
		DatagramPacket receivePacket = new DatagramPacket(buffer.array(), buffer.capacity());
		Thread reader = new Thread() {
			@Override
			public void run() {
				int mSampleRate = 44100;
				int mChannelOut = AudioFormat.CHANNEL_OUT_STEREO;

				// AudioTrack mAudio = new AudioTrack(0x3, mSampleRate, mChannelOut, AudioFormat.ENCODING_PCM_16BIT, AUDIO_TRACK_BUFFER_SIZE, AudioTrack.MODE_STREAM);
				// mAudio.play();
				long mAH = Native.OpenAudioDevice(mSampleRate, 2);
				android.util.Log.d("AudioReceiver", "OpenAudioDevice 44100/2");

				try {
					while (mRunning) {
						try {
							ByteBuffer buffer;
							synchronized (mData) {
								if (mData.isEmpty())
									mData.wait();
								buffer = mData.remove(0);
							}
							buffer.order(ByteOrder.LITTLE_ENDIAN);
							buffer.rewind();
							int nIndex = buffer.getInt();
							int nChannelConf = buffer.getInt();
							int nSamplesPerSec = buffer.getInt();
							int nBufferLength = buffer.getInt();
							if (nSamplesPerSec != mSampleRate) {
								mSampleRate = nSamplesPerSec;
								if (mAH != 0) Native.CloseAudioDevice(mAH);
								mAH = Native.OpenAudioDevice(mSampleRate, 2);
								android.util.Log.d("AudioReceiver", "OpenAudioDevice "+mSampleRate+"/2");
								// mAudio.setPlaybackRate(mSampleRate);
							}
							// mAudio.write(buffer.array(), buffer.position(), nBufferLength);
							Native.AudioOut(mAH, buffer.array(), buffer.position(), nBufferLength);
						} catch (InterruptedException ie) {
							break;
						} catch (Exception e) {
							e.printStackTrace();
						}
					}
				}finally{
					// if (mAudio != null) mAudio.release();
					if (mAH != 0) Native.CloseAudioDevice(mAH);
				}
			}
		};
		try {
			mReceiverSocket = new DatagramSocket(0x7d01 /* PORT */);
			mRunning = true;
			reader.start();
			while (mRunning && reader.isAlive()) {
				mReceiverSocket.receive(receivePacket);
				synchronized (mData) {
					if (mData.isEmpty() || mData.get(0).getInt(0) < buffer.getInt(0)) {
						ByteBuffer temp = ByteBuffer.allocate(buffer.capacity());
						buffer.rewind();
						temp.put(buffer);
						mData.add(temp);
						// if(mData.size() > 8) mData.subList(0, 4).clear();
						mData.notify();
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (mReceiverSocket != null)
				mReceiverSocket.close();
			mReceiverThread = null;
			if (mRunning) {
				reader.interrupt();
				mRunning = false;
				start();
			}
		}
	}

	public boolean start() {
		if (mReceiverThread != null) return false;
		mReceiverThread = new Thread(this);
		mReceiverThread.start();
		return true;
	}

	public boolean stop() {
		if (mReceiverThread == null) return false;
		mRunning = false;
		mReceiverThread.interrupt();
		return true;
	}
}