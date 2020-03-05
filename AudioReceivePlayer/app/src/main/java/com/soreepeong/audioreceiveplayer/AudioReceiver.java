package com.soreepeong.audioreceiveplayer;

import androidx.annotation.NonNull;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.PriorityBlockingQueue;
import java.util.concurrent.TimeUnit;

public class AudioReceiver implements Runnable {
    private DatagramSocket mReceiverSocket;
    private Thread mReceiverThread = null;
    private boolean mRunning = false;
    private final PriorityBlockingQueue<AudioBuffer> mData = new PriorityBlockingQueue<>();
    public final static int MAX_DATA = 48;
    private int mMaxData = 8;
    private int mDropped;

    @Override
    public void run() {
        ByteBuffer buffer = ByteBuffer.allocate(2048);
        DatagramPacket receivePacket = new DatagramPacket(buffer.array(), buffer.capacity());
        Thread reader = new Thread() {
            @Override
            public void run() {
                int mSampleRate = 48000;
                long mAH = Native.OpenAudioDevice(mSampleRate, 2);
                try {
                    while (mRunning && !Thread.interrupted()) {
                        if (mData.size() >= mMaxData) {
                            while(!mData.isEmpty()) {
                                mData.poll().recycle();
                                mDropped++;
                            }
                        }
                        AudioBuffer buf = mData.poll(60, TimeUnit.SECONDS);
                        if (buf == null) continue;
                        try {
                            if (buf.nSamplesPerSec != mSampleRate) {
                                mSampleRate = buf.nSamplesPerSec;
                                if (mAH != 0) Native.CloseAudioDevice(mAH);
                                mAH = Native.OpenAudioDevice(mSampleRate, 2);
                            }
                            Native.AudioOut(mAH, buf.mBuffer.array(), buf.mBuffer.position(), buf.nBufferLength);
                        } catch (Exception e) {
                            e.printStackTrace();
                        } finally {
                            buf.recycle();
                        }
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                } finally {
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
                mData.add(AudioBuffer.getBuffer(buffer));
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (mReceiverSocket != null)
                mReceiverSocket.close();
            mReceiverThread = null;
            reader.interrupt();
            if (mRunning) {
                mRunning = false;
                start();
            }
        }
    }

    public int getDropped() {
        return mDropped;
    }

    public int getBufferCount() {
        return mData.size();
    }

    public boolean start() {
        if (mReceiverThread != null) return false;
        while(!mData.isEmpty())
            mData.poll().recycle();
        mDropped = 0;
        mReceiverThread = new Thread(this);
        mReceiverThread.start();
        return true;
    }

    public boolean stop() {
        if (mReceiverThread == null) return false;
        mRunning = false;
        if (mReceiverSocket != null)
            mReceiverSocket.close();
        mReceiverThread.interrupt();
        return true;
    }

    public void setMaxData(int n) {
        mMaxData = Math.max(1, Math.min(MAX_DATA, n));
    }

    public int getMaxData() {
        return mMaxData;
    }

    public boolean isRunning() {
        return mRunning || mReceiverThread != null;
    }

    private static class AudioBuffer implements Comparable<AudioBuffer> {
        private final static AudioBuffer[] mAudioBuffers = new AudioBuffer[128];
        int nIndex, nChannelConf, nSamplesPerSec, nBufferLength;
        final ByteBuffer mBuffer = ByteBuffer.allocate(2048);

        public static AudioBuffer getBuffer(ByteBuffer buffer) {
            synchronized (mAudioBuffers) {
                for(int i = 0; i < mAudioBuffers.length; i++){
                    AudioBuffer buf = mAudioBuffers[i];
                    if(buf == null)
                        continue;
                    mAudioBuffers[i] = null;
                    buf.initialize(buffer);
                    return buf;
                }
                return new AudioBuffer(buffer);
            }
        }

        public void recycle() {
            synchronized (mAudioBuffers) {
                for(int i = 0; i < mAudioBuffers.length; i++)
                    if(mAudioBuffers[i] == null) {
                        mAudioBuffers[i] = this;
                        return;
                    }
            }
        }

        private AudioBuffer(ByteBuffer buffer) {
            initialize(buffer);
        }

        private void initialize(ByteBuffer buffer) {
            buffer.rewind();
            mBuffer.rewind();
            mBuffer.put(buffer);
            mBuffer.order(ByteOrder.LITTLE_ENDIAN);
            mBuffer.rewind();
            nIndex = mBuffer.getInt();
            nChannelConf = mBuffer.getInt();
            nSamplesPerSec = mBuffer.getInt();
            nBufferLength = mBuffer.getInt();
        }

        @Override
        public int compareTo(@NonNull AudioBuffer audioBuffer) {
            return nIndex - audioBuffer.nIndex;
        }
    }
}