# WifiAudioSender & AudioReceivePlayer

![Windows Sender/Receiver](https://raw.githubusercontent.com/Soreepeong/WifiAudioSender/master/readme-images/Win.png)
![Android Receiver](https://raw.githubusercontent.com/Soreepeong/WifiAudioSender/master/readme-images/Android.jpg)

WifiAudioSender captures your Windows system's audio device's output, and copies it to another audio device or sends it to specific client by UDP.

By using USB Tether with your Android device's native sampling rate as your PC's sound device's sampling rate, you can *probably* achieve &lt;10ms latency, provided that your phone supports [Pro Audio](https://developer.android.com/ndk/guides/audio/audio-latency.html).

# What it can do
  - Capture audio output
  - Copy to another audio device
  - Send it to another PC or Android device via UDP
  - (Android side) Uses OpenSLES latency output (if the device supports current sampling rate)

# What it can't do (=TODO)
  - Resample
  - Reset the task when audio device's configuration changes
  - Encrypt audio stream

# Referenced codes
  - [Android OpenSL input/output module by Victor Lazzarini](https://audioprograming.wordpress.com/2012/10/29/lock-free-audio-io-with-opensl-es-on-android/)
  - [MSDN Article "Capturing a Stream"](https://msdn.microsoft.com/en-us/library/windows/desktop/dd370800.aspx)