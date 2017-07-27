package com.soreepeong.audioreceiveplayer;

public class Native {
	static {
		System.loadLibrary("AudioReceivePlayerNative");
	}
	public static native long OpenAudioDevice(int sr, int outch);
	public static native void CloseAudioDevice(long handle);
	public static native void AudioOut(long handle, byte[] buffer, int from, int size);
}
