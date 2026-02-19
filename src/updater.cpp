#include <jni.h>
#include <string>
#include <thread>
#include <fstream>
#include <vector>

#ifdef __aarch64__
const char* APK_URL = "https://github.com/theakres/flare/releases/download/latest/flare-latest-v8a.apk";
#else
const char* APK_URL = "https://github.com/theakres/flare/releases/download/latest/flare-latest-v7a.apk";
#endif

const char* VERSION_URL = "https://github.com/theakres/flare/raw/main/version.txt";

JavaVM* g_vm = nullptr;
jobject g_context = nullptr;

void ShowUpdateDialog(JNIEnv* env, const std::string& currentVer, const std::string& newVer, bool isRu);
void DownloadAndInstall(JNIEnv* env, bool isRu);

std::string JStringToString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string ret(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return ret;
}

jstring StringToJString(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

void RunUpdateCheck() {
    JNIEnv* env;
    g_vm->AttachCurrentThread(&env, nullptr);

    jclass localeClass = env->FindClass("java/util/Locale");
    jmethodID getDefaultMethod = env->GetStaticMethodID(localeClass, "getDefault", "()Ljava/util/Locale;");
    jobject localeObj = env->CallStaticObjectMethod(localeClass, getDefaultMethod);
    jmethodID getLanguageMethod = env->GetMethodID(localeClass, "getLanguage", "()Ljava/lang/String;");
    jstring langStr = (jstring)env->CallObjectMethod(localeObj, getLanguageMethod);
    std::string lang = JStringToString(env, langStr);
    bool isRu = (lang == "ru");

    jclass contextClass = env->GetObjectClass(g_context);
    jmethodID getPackageNameMethod = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
    jstring packageName = (jstring)env->CallObjectMethod(g_context, getPackageNameMethod);

    jmethodID getPackageManagerMethod = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject packageManager = env->CallObjectMethod(g_context, getPackageManagerMethod);

    jclass pmClass = env->GetObjectClass(packageManager);
    jmethodID getPackageInfoMethod = env->GetMethodID(pmClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfoMethod, packageName, 0);

    jclass piClass = env->GetObjectClass(packageInfo);
    jfieldID versionNameField = env->GetFieldID(piClass, "versionName", "Ljava/lang/String;");
    jstring currentVersionJStr = (jstring)env->GetObjectField(packageInfo, versionNameField);
    std::string currentVersion = JStringToString(env, currentVersionJStr);

    jclass urlClass = env->FindClass("java/net/URL");
    jmethodID urlInit = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
    jobject urlObj = env->NewObject(urlClass, urlInit, StringToJString(env, VERSION_URL));

    jmethodID openConnectionMethod = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");
    jobject urlConnection = env->CallObjectMethod(urlObj, openConnectionMethod);

    jclass scannerClass = env->FindClass("java/util/Scanner");
    jmethodID getInputStreamMethod = env->GetMethodID(env->GetObjectClass(urlConnection), "getInputStream", "()Ljava/io/InputStream;");
    jobject inputStream = env->CallObjectMethod(urlConnection, getInputStreamMethod);

    jmethodID scannerInit = env->GetMethodID(scannerClass, "<init>", "(Ljava/io/InputStream;)V");
    jobject scannerObj = env->NewObject(scannerClass, scannerInit, inputStream);

    jmethodID useDelimiterMethod = env->GetMethodID(scannerClass, "useDelimiter", "(Ljava/lang/String;)Ljava/util/Scanner;");
    scannerObj = env->CallObjectMethod(scannerObj, useDelimiterMethod, StringToJString(env, "\\A"));

    jmethodID hasNextMethod = env->GetMethodID(scannerClass, "hasNext", "()Z");
    std::string newVersion = "";
    if (env->CallBooleanMethod(scannerObj, hasNextMethod)) {
        jmethodID nextMethod = env->GetMethodID(scannerClass, "next", "()Ljava/lang/String;");
        jstring newVersionJStr = (jstring)env->CallObjectMethod(scannerObj, nextMethod);
        newVersion = JStringToString(env, newVersionJStr);
    }

    if (!newVersion.empty() && newVersion.back() == '\n') {
        newVersion.pop_back();
    }

    if (!newVersion.empty() && currentVersion != newVersion) {
        ShowUpdateDialog(env, currentVersion, newVersion, isRu);
    }

    g_vm->DetachCurrentThread();
}

void ShowUpdateDialog(JNIEnv* env, const std::string& currentVer, const std::string& newVer, bool isRu) {
    jclass handlerClass = env->FindClass("android/os/Handler");
    jclass looperClass = env->FindClass("android/os/Looper");
    jmethodID getMainLooperMethod = env->GetStaticMethodID(looperClass, "getMainLooper", "()Landroid/os/Looper;");
    jobject mainLooper = env->CallStaticObjectMethod(looperClass, getMainLooperMethod);
    jmethodID handlerInit = env->GetMethodID(handlerClass, "<init>", "(Landroid/os/Looper;)V");
    jobject handler = env->NewObject(handlerClass, handlerInit, mainLooper);

    std::string title = isRu ? "Вышло Обновление" : "New Version Available!";
    std::string message = currentVer + " ---> " + newVer;
    std::string btnUpdate = isRu ? "Обновить" : "Update";
    std::string btnLater = isRu ? "Позже" : "Later";

}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jmethodID currentApplicationMethod = env->GetStaticMethodID(activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    jobject application = env->CallStaticObjectMethod(activityThreadClass, currentApplicationMethod);
    g_context = env->NewGlobalRef(application);

    std::thread checkThread(RunUpdateCheck);
    checkThread.detach();

    return JNI_VERSION_1_6;
}
