#include <jni.h>
#include <string>
#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <common.h>
#include "enginebinder.h"
#include "androidcallservice.h"

typedef struct jni_context
{
	JavaVM *javaVM;
	jclass jniHelperClz;
	jobject jniHelperObj;
	jclass scriptServiceClz;
	jobject scriptServiceObj;
} JniContext;

static JniContext g_ctx;
static void *g_engineMod = nullptr;

static EngineBinder g_engineBinder;
static AndroidCallService g_androidCallService;

/*
 * processing one time initialization:
 *     Cache the javaVM into our context
 *     Find class ID for JniHelper
 *     Create an instance of JniHelper
 *     Make global reference since we are using them from a native thread
 * Note:
 *     All resources allocated here are never released by application
 *     we rely on system to free all global refs when it goes away;
 *     the pairing function JNI_OnUnload() never gets called at all.
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
	JNIEnv *env;
	memset(&g_ctx, 0, sizeof(g_ctx));

	g_ctx.javaVM = vm;
	if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK)
	{
		return JNI_ERR; // JNI version not supported.
	}

	jclass clz = env->FindClass("com/rainman/asf/core/JniHandler");
	g_ctx.jniHelperClz = (jclass) env->NewGlobalRef(clz);

	jmethodID jniHelperCtor = env->GetMethodID(g_ctx.jniHelperClz, "<init>", "()V");
	jobject handler = env->NewObject(g_ctx.jniHelperClz, jniHelperCtor);
	g_ctx.jniHelperObj = env->NewGlobalRef(handler);

	g_ctx.scriptServiceObj = nullptr;
	return JNI_VERSION_1_6;
}

void updateScriptState(int state, int flags)
{
	JNIEnv *env;
	g_ctx.javaVM->AttachCurrentThread(&env, nullptr);
	jmethodID id = env->GetMethodID(g_ctx.scriptServiceClz, "updateScriptState", "(II)V");
	env->CallVoidMethod(g_ctx.scriptServiceObj, id, state, flags);
	g_ctx.javaVM->DetachCurrentThread();
}

void outputScriptLog(uint32_t time, uint32_t color, const char *text)
{
	JNIEnv *env;
	g_ctx.javaVM->AttachCurrentThread(&env, nullptr);

	jstring javaText = env->NewStringUTF(text);
	jmethodID id = env->GetMethodID(g_ctx.scriptServiceClz, "outputScriptLog", "(IILjava/lang/String;)V");
	env->CallVoidMethod(g_ctx.scriptServiceObj, id, time, color, javaText);
	env->DeleteLocalRef(javaText);

	g_ctx.javaVM->DetachCurrentThread();
}

void updateCmdServerState(int state)
{
	JNIEnv *env;
	g_ctx.javaVM->AttachCurrentThread(&env, nullptr);
	jmethodID id = env->GetMethodID(g_ctx.scriptServiceClz, "updateCmdServerState", "(I)V");
	env->CallVoidMethod(g_ctx.scriptServiceObj, id, state);
	g_ctx.javaVM->DetachCurrentThread();
}

void engineDisconnectNotify()
{
	JNIEnv *env;
	g_ctx.javaVM->AttachCurrentThread(&env, nullptr);
	jmethodID id = env->GetMethodID(g_ctx.scriptServiceClz, "engineDisconnectNotify", "()V");
	env->CallVoidMethod(g_ctx.scriptServiceObj, id);
	g_ctx.javaVM->DetachCurrentThread();
}

void *createAndroidCaller()
{
	JNIEnv *env;
	g_ctx.javaVM->AttachCurrentThread(&env, nullptr);

	// new出一个AndroidCaller对象实例
	jmethodID id = env->GetMethodID(g_ctx.jniHelperClz, "createAndroidCaller",
	                                "(Lcom/rainman/asf/core/ScriptActuator;)Ljava/lang/Object;");
	jobject inst = env->CallObjectMethod(g_ctx.jniHelperObj, id, g_ctx.scriptServiceObj);
	inst = env->NewGlobalRef(inst);

	g_ctx.javaVM->DetachCurrentThread();
	return inst;
}

void destroyAndroidCaller(void *androidCaller)
{
	JNIEnv *env;
	g_ctx.javaVM->AttachCurrentThread(&env, nullptr);

	jmethodID id = env->GetMethodID(g_ctx.jniHelperClz, "destroyAndroidCaller", "(Ljava/lang/Object;)V");
	env->CallVoidMethod(g_ctx.jniHelperObj, id, (jobject) androidCaller);

	// 删除传入的AndroidCaller对象实例
	env->DeleteGlobalRef((jobject) androidCaller);

	g_ctx.javaVM->DetachCurrentThread();
}

// 用于转发脚本对Java层的Android API的调用
void forwardAndroidCall(void *androidCaller, CBuffer &request, CBuffer &response)
{
	JNIEnv *env;
	g_ctx.javaVM->AttachCurrentThread(&env, nullptr);

	jbyteArray arr = env->NewByteArray(request.GetSize());
	env->SetByteArrayRegion(arr, 0, request.GetSize(), (int8_t *) request.GetBuffer());

	jmethodID id = env->GetMethodID(g_ctx.jniHelperClz, "forwardAndroidCall", "(Ljava/lang/Object;[B)[B");
	auto barr = (jbyteArray) env->CallObjectMethod(g_ctx.jniHelperObj, id, (jobject) androidCaller, arr);

	jsize alen = env->GetArrayLength(barr);
	jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
	response.Write(ba, static_cast<size_t>(alen));
	env->ReleaseByteArrayElements(barr, ba, 0);

	g_ctx.javaVM->DetachCurrentThread();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_initNativeLib(JNIEnv *env, jobject instance)
{
	jclass clz = env->GetObjectClass(instance);
	g_ctx.scriptServiceClz = (jclass) env->NewGlobalRef(clz);
	g_ctx.scriptServiceObj = env->NewGlobalRef(instance);
	g_androidCallService.Start();       // 该服务用来转发脚本对Java层代码的调用
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_rainman_asf_core_ScriptActuator_connectEngine(JNIEnv *env, jobject instance, jstring connName_)
{
	const char *connName = env->GetStringUTFChars(connName_, nullptr);
	bool result = g_engineBinder.ConnectEngine(connName);
	env->ReleaseStringUTFChars(connName_, connName);
	return static_cast<jboolean>(result);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_disconnectEngine(JNIEnv *env, jobject instance)
{
	g_engineBinder.DisconnectEngine();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_rainman_asf_core_ScriptActuator_runScript(JNIEnv *env, jobject instance, jstring scriptCfg_,
                                                   jstring userVar_)
{
	const char *scriptCfg = env->GetStringUTFChars(scriptCfg_, nullptr);
	const char *userVar = env->GetStringUTFChars(userVar_, nullptr);

	bool result = g_engineBinder.RunScript(scriptCfg, userVar);

	env->ReleaseStringUTFChars(scriptCfg_, scriptCfg);
	env->ReleaseStringUTFChars(userVar_, userVar);
	return static_cast<jboolean>(result);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_abort(JNIEnv *env, jobject instance)
{
	g_engineBinder.Abort();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_reportDisplayInfo(JNIEnv *env, jobject instance, jint width, jint height,
                                                           jint rotation)
{
	g_engineBinder.SetDisplayInfo(width, height, rotation);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_enableCmdServer(JNIEnv *env, jobject instance, jboolean enable)
{
	g_engineBinder.EnableCmdServer(enable);
}

/**
 * libengine.so为lua脚本引擎所在模块，该模块采用动态加载方式，当需要脚本以ROOT权限运行时，该模块将由
 * 一个独立的ROOT权限进程来加载
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_nativeLoadEngine(JNIEnv *env, jobject instance, jstring workDir_)
{
	// 切换当前目录到指定的工作目录中
	const char *workDir = env->GetStringUTFChars(workDir_, nullptr);
	chdir(workDir);

	// 加载引擎模块
	std::string soPath = GetModulePath(-1, (void *) &GetModulePath);
	soPath = soPath.substr(0, soPath.find_last_of('/')) + "/libengine.so";
	g_engineMod = dlopen(soPath.c_str(), RTLD_NOW);
	if (g_engineMod == nullptr)
	{
		LOGE("%s", dlerror());
		return;
	}

	int (*start_service)();
	*(void **) &start_service = dlsym(g_engineMod, "start_service");
	if (start_service == nullptr)
	{
		LOGE("%s", dlerror());
		return;
	}

	// 启动引擎服务
	start_service();

	env->ReleaseStringUTFChars(workDir_, workDir);
}

void processServiceExit()
{
	if (g_engineMod)
	{
		void (*stop_service)();
		*(void **) &stop_service = dlsym(g_engineMod, "stop_service");
		if (stop_service == nullptr)
		{
			LOGE("%s", dlerror());
			return;
		}

		stop_service();

		dlclose(g_engineMod);
		g_engineMod = nullptr;
	}
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_nativeUnloadEngine(JNIEnv *env, jobject instance)
{
	g_engineBinder.Cleanup();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_sendEvent(JNIEnv *env, jobject instance, jstring event_)
{
	const char *event = env->GetStringUTFChars(event_, nullptr);
	g_engineBinder.SendEvent(event);
	env->ReleaseStringUTFChars(event_, event);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rainman_asf_core_ScriptActuator_setPluginDir(JNIEnv *env, jobject instance, jstring pluginDir_)
{
	const char *pluginDir = env->GetStringUTFChars(pluginDir_, 0);
	g_engineBinder.SetPluginDir(pluginDir);
	env->ReleaseStringUTFChars(pluginDir_, pluginDir);
}
