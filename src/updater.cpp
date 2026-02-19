#include <jni.h>
#include <string>
#include <thread>
#include <android/log.h>

#define LOG_TAG "AUPD_NATIVE"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef __aarch64__
const char* APK_URL = "https://github.com/theakres/flare/releases/download/latest/flare-latest-v8a.apk";
#else
const char* APK_URL = "https://github.com/theakres/flare/releases/download/latest/flare-latest-v7a.apk";
#endif

const char* VERSION_URL = "https://github.com/theakres/flare/raw/main/version.txt";

JavaVM* g_vm = nullptr;

void RunUpdateCheck() {
    JNIEnv* env;
    if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jmethodID currentAppMethod = env->GetStaticMethodID(activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    jobject context = env->CallStaticObjectMethod(activityThreadClass, currentAppMethod);

    if (!context) {
        LOGE("Failed to get context, retrying in 2 seconds...");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        g_vm->DetachCurrentThread();
        std::thread(RunUpdateCheck).detach();
        return;
    }

    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPkgMethod = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
    jstring pkgName = (jstring)env->CallObjectMethod(context, getPkgMethod);

    jmethodID getPMMethod = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject pm = env->CallObjectMethod(context, getPMMethod);

    jclass pmClass = env->GetObjectClass(pm);
    jmethodID getPkgInfoMethod = env->GetMethodID(pmClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    jobject pkgInfo = env->CallObjectMethod(pm, getPkgInfoMethod, pkgName, 0);

    jclass piClass = env->GetObjectClass(pkgInfo);
    jfieldID verNameField = env->GetFieldID(piClass, "versionName", "Ljava/lang/String;");
    jstring currentVerJ = (jstring)env->GetObjectField(pkgInfo, verNameField);
    
    const char* cVer = env->GetStringUTFChars(currentVerJ, nullptr);
    std::string currentVersion(cVer);
    env->ReleaseStringUTFChars(currentVerJ, cVer);

    g_vm->DetachCurrentThread();
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    std::thread(RunUpdateCheck).detach();
    return JNI_VERSION_1_6;
}
