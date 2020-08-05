//
// Created by omer on 04/11/02017.
//

#ifndef CORIUM3D_CORIUM3DANDROIDACTIVITYLINK_H
#define CORIUM3D_CORIUM3DANDROIDACTIVITYLINK_H

#include <jni.h>
#include "Corium3D.h"

extern "C" {
    // Activity lifecycle callbacks
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnCreate(JNIEnv* env, jclass clazz, jobject assetManager, jstring internalDirPath);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnResume(JNIEnv* env, jclass clazz);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnPause(JNIEnv* env, jclass clazz);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnDestroy(JNIEnv* env, jclass clazz);

    // Android view callbacks
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nSurfaceCreated(JNIEnv* env, jobject obj, jobject surface);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nSurfaceChanged(JNIEnv* env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nSurfaceDestroyed(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nOnWindowFocusChanged(JNIEnv* env, jobject obj, jboolean hasFocus);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nOnDetachedFromWindow(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nOnTouchEvent(JNIEnv* env, jobject obj, jobject motionEvent);
}

#endif //CORIUM3D_CORIUM3DANDROIDACTIVITYLINK_H
