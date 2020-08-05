//
// Created by omer on 04/11/02017.
//

#include "Corium3DAndroidLink.h"
#include <android/asset_manager_jni.h>
#include <string>
#include <pthread.h>
#include <android/native_window_jni.h>
#include "ServiceLocator.h"
#include "Corium3D.h"
#include "Renderer.h"

inline void extractAsset(AAssetManager* apkAssetManager, const char* assetName, const char* assetFullPath);

Corium3D *corium3D = NULL;
ANativeWindow *window = NULL;

//////////////////////////////////
// Activity lifecycle callbacks //
//////////////////////////////////
JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnCreate(JNIEnv* env, jclass clazz, jobject assetManager, jstring internalDirPath){
    const char* assetNameCube = "cube.dae";
    const char* assetNameTxtTexAtlas = "txtTexAtlas.png";

    const char *cPathToInternalDir = env->GetStringUTFChars(internalDirPath, NULL );
    std::string apkInternalPath = std::string(cPathToInternalDir);
    env->ReleaseStringUTFChars(internalDirPath, cPathToInternalDir);
    std::string assetCubeFullPath = apkInternalPath + "/" + assetNameCube;
    std::string assetTxtTexAtlasFullPath = apkInternalPath + "/" + assetNameTxtTexAtlas;
    AAssetManager*  apkAssetManager = AAssetManager_fromJava(env, assetManager);
    extractAsset(apkAssetManager, assetNameCube, assetCubeFullPath.c_str());
    extractAsset(apkAssetManager, assetNameTxtTexAtlas, assetTxtTexAtlasFullPath.c_str());

    corium3D = new Corium3D(assetCubeFullPath.c_str(), assetTxtTexAtlasFullPath.c_str());
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnResume(JNIEnv* env, jclass clazz){
    corium3D->signalResume();
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnPause(JNIEnv* env, jclass clazz){
    corium3D->signalPause();
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DActivity_nOnDestroy(JNIEnv* env, jclass clazz) {

}

////////////////////////////
// Android view callbacks //
////////////////////////////
JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nSurfaceCreated(JNIEnv* env, jobject obj, jobject surface) {
    if (surface != NULL) {
        window = ANativeWindow_fromSurface(env, surface);
        ServiceLocator::getLogger().logd("Corium3DAndroidLink", "Got window %p", window);
        corium3D->signalSurfaceCreated(window);
    } else {
        ServiceLocator::getLogger().logd("Corium3DAndroidLink", "Releasing window");
        ANativeWindow_release(window);
    }
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nSurfaceChanged(JNIEnv* env, jobject obj, jint width, jint height) {
    corium3D->signalSurfaceSzChanged(width, height);
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nSurfaceDestroyed(JNIEnv* env, jobject obj){
    corium3D->signalSurfaceDestroyed();
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nOnWindowFocusChanged(JNIEnv* env, jobject obj, jboolean hasFocus){
    corium3D->signalWindowFocusChanged(hasFocus);
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nOnDetachedFromWindow(JNIEnv* env, jobject obj) {
    corium3D->signalDetachedFromWindow();
    delete corium3D;
    ANativeWindow_release(window);
}

JNIEXPORT void JNICALL Java_com_corium_corium3d_Corium3DView_nOnTouchEvent(JNIEnv* env, jobject obj, jobject motionEvent) {

}

inline void extractAsset(AAssetManager* apkAssetManager, const char* assetName, const char* assetFullPath) {
    pthread_mutex_t threadMutex;
    pthread_mutex_init(&threadMutex, NULL);
    FILE* file = fopen(assetFullPath, "rb");
    //if (file) {
    //    ServiceLocator::getLogger().logd("Corium3DAndroidLink", "Found extracted file in assets: %s", "cube.dae");
    //} else {
        pthread_mutex_lock(&threadMutex);
        AAsset *asset = AAssetManager_open(apkAssetManager, assetName, AASSET_MODE_STREAMING);
        char buf[BUFSIZ];
        int readBytesNr = 0;
        if (asset != NULL) {
            FILE *extractedAsset = fopen(assetFullPath, "w");
            while ((readBytesNr = AAsset_read(asset, buf, BUFSIZ)) > 0) {
                fwrite(buf, readBytesNr, 1, extractedAsset);
            }
            fclose(extractedAsset);
            AAsset_close(asset);
            pthread_mutex_unlock(&threadMutex);

            ServiceLocator::getLogger().logd("Corium3DAndroidLink", "Asset extracted: %s", assetFullPath);
        } else {
            ServiceLocator::getLogger().logd("Corium3DAndroidLink", "Asset not found: %s", assetFullPath);
            return;
        }
    //}
}