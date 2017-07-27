#include <jni.h>
#include "opensl_io.h"

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jlong JNICALL
Java_com_soreepeong_audioreceiveplayer_Native_OpenAudioDevice(JNIEnv *env, jclass jcls, jint sr, jint outchannels) {
	return (jlong) android_OpenAudioDevice(sr, outchannels);
}
JNIEXPORT void JNICALL
Java_com_soreepeong_audioreceiveplayer_Native_CloseAudioDevice(JNIEnv *env, jclass jcls, long long p) {
	android_CloseAudioDevice((OPENSL_STREAM *) p);
}
JNIEXPORT void JNICALL
Java_com_soreepeong_audioreceiveplayer_Native_AudioOut(JNIEnv *env, jclass jcls, jlong p, jbyteArray buffer, jint from, jint size) {
	jbyte *body = env->GetByteArrayElements(buffer, 0);
	android_AudioOut((OPENSL_STREAM *) p, body + from, size);
}
#ifdef __cplusplus
}
#endif

