#include <windows.h>
#include <jni.h>

#pragma comment(lib, "user32.lib")

HWND g_hwnd = nullptr;
JavaVM* g_jvm = nullptr;
JNIEnv* g_env = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_COMMAND && LOWORD(wp) == 1) {
        if (!g_jvm) {
            JavaVM* vms[1]; jsize count;
            JNI_GetCreatedJavaVMs(vms, 1, &count);
            if (count > 0) g_jvm = vms[0];
        }
        if (!g_jvm) {
            MessageBoxA(nullptr, "No JVM found. Start Minecraft first!", "Error", MB_ICONERROR);
            return 0;
        }
        if (!g_env) {
            if (g_jvm->AttachCurrentThread((void**)&g_env, nullptr) != JNI_OK) return 0;
        }

        jclass mcClass = g_env->FindClass("ave");
        jmethodID getMinecraft = g_env->GetStaticMethodID(mcClass, "A", "()Lave;");
        jobject mc = g_env->CallStaticObjectMethod(mcClass, getMinecraft);

        jfieldID playerField = g_env->GetFieldID(mcClass, "h", "Lbew;");
        jobject player = g_env->GetObjectField(mc, playerField);
        if (!player) {
            MessageBoxA(nullptr, "You must be in a world!", "Error", MB_ICONERROR);
            return 0;
        }

        jclass playerClass = g_env->FindClass("bew");
        jfieldID capField = g_env->GetFieldID(playerClass, "br", "Lvl;");
        jobject cap = g_env->GetObjectField(player, capField);
        if (!cap) return 0;

        jclass capClass = g_env->FindClass("vl");
        jfieldID flyingField = g_env->GetFieldID(capClass, "g", "Z");
        jfieldID allowField = g_env->GetFieldID(capClass, "a", "Z");
        jboolean flying = g_env->GetBooleanField(cap, flyingField);

        bool newFly = !flying;
        g_env->SetBooleanField(cap, allowField, newFly ? JNI_TRUE : JNI_FALSE);
        g_env->SetBooleanField(cap, flyingField, newFly ? JNI_TRUE : JNI_FALSE);

        jmethodID send = g_env->GetMethodID(playerClass, "R", "()V");
        g_env->CallVoidMethod(player, send);

        MessageBoxA(nullptr, newFly ? "Flight ON" : "Flight OFF", "Success", MB_OK);
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void MakeWindow() {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"FlyGUI";
    RegisterClassEx(&wc);

    g_hwnd = CreateWindowEx(WS_EX_TOPMOST, L"FlyGUI", L"Fly Toggle",
                            WS_POPUP, 100, 100, 200, 60,
                            nullptr, nullptr, wc.hInstance, nullptr);
    CreateWindow(L"BUTTON", L"Toggle Flight",
                 WS_CHILD | WS_VISIBLE, 10, 10, 180, 40,
                 g_hwnd, (HMENU)1, wc.hInstance, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) DispatchMessage(&msg);
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID) {
    if (r == DLL_PROCESS_ATTACH)
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MakeWindow, nullptr, 0, nullptr);
    return TRUE;
}