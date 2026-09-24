#include <jni.h>

#ifndef _Included_Distorm3
#define _Included_Distorm3
#ifdef __cplusplus
extern "C" {
#endif

#define PACKAGE_PREFIX "diStorm3/"

JNIEXPORT void JNICALL Java_diStorm3_distorm3_Decompose
  (JNIEnv *, jclass, jobject, jobject);

JNIEXPORT void JNICALL Java_diStorm3_distorm3_Decode
  (JNIEnv *, jclass, jobject, jobject);

JNIEXPORT jobject JNICALL Java_diStorm3_distorm3_Format
  (JNIEnv *, jclass, jobject, jobject);

#ifdef __cplusplus
}
#endif
#endif
