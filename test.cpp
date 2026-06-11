#include <windows.h>
#include <jni.h>
#include <cmath>
#include <thread>
#include <atomic>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

HWND g_mainWnd = nullptr;
HWND g_overlay = nullptr;
HWND g_mcWnd = nullptr;
JavaVM* g_jvm = nullptr;
JNIEnv* g_env = nullptr;

std::atomic<bool> g_espOn = false;
std::atomic<bool> g_aimbotOn = false;
std::atomic<bool> g_aimbotActive = false;
bool g_running = true;

void ToggleFlight();
void ESPLoop();
void AimbotLoop();
bool AttachToJVM();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool AttachToJVM() {
    if (g_jvm && g_env) return true;
    jsize count;
    JavaVM* vms[1];
    if (JNI_GetCreatedJavaVMs(vms, 1, &count) != JNI_OK || count == 0) return false;
    g_jvm = vms[0];
    if (g_jvm->AttachCurrentThread((void**)&g_env, nullptr) != JNI_OK) return false;
    return true;
}

void ToggleFlight() {
    if (!AttachToJVM()) return;
    jclass mcClass = g_env->FindClass("ave");
    jobject mc = g_env->CallStaticObjectMethod(mcClass, g_env->GetStaticMethodID(mcClass, "A", "()Lave;"));
    jobject player = g_env->GetObjectField(mc, g_env->GetFieldID(mcClass, "h", "Lbew;"));
    if (!player) return;
    jobject cap = g_env->GetObjectField(player, g_env->GetFieldID(g_env->FindClass("bew"), "br", "Lvl;"));
    jclass capClass = g_env->FindClass("vl");
    jfieldID isFlying = g_env->GetFieldID(capClass, "g", "Z");
    jfieldID allowFlying = g_env->GetFieldID(capClass, "a", "Z");
    jboolean flying = g_env->GetBooleanField(cap, isFlying);
    bool newFly = !flying;
    g_env->SetBooleanField(cap, allowFlying, newFly);
    g_env->SetBooleanField(cap, isFlying, newFly);
    g_env->CallVoidMethod(player, g_env->GetMethodID(g_env->FindClass("bew"), "R", "()V"));
    MessageBoxA(nullptr, newFly ? "Flight ON" : "Flight OFF", "Fly", MB_OK);
}

void ESPLoop() {
    while (g_running) {
        if (g_espOn && g_overlay && g_mcWnd && AttachToJVM()) {
            HDC hdc = GetDC(g_overlay);
            RECT rect; GetClientRect(g_overlay, &rect);
            FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

            jclass mcClass = g_env->FindClass("ave");
            jobject mc = g_env->CallStaticObjectMethod(mcClass, g_env->GetStaticMethodID(mcClass, "A", "()Lave;"));
            jobject world = g_env->GetObjectField(mc, g_env->GetFieldID(mcClass, "f", "Ladm;"));
            if (!world) { ReleaseDC(g_overlay, hdc); Sleep(50); continue; }

            jobject entityList = g_env->GetObjectField(world, g_env->GetFieldID(g_env->FindClass("adm"), "g", "Ljava/util/List;"));
            if (!entityList) { ReleaseDC(g_overlay, hdc); Sleep(50); continue; }

            jclass listClass = g_env->FindClass("java/util/List");
            jint size = g_env->CallIntMethod(entityList, g_env->GetMethodID(listClass, "size", "()I"));
            jmethodID getMethod = g_env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

            jobject player = g_env->GetObjectField(mc, g_env->GetFieldID(mcClass, "h", "Lbew;"));
            if (!player) { ReleaseDC(g_overlay, hdc); continue; }

            double px = g_env->GetDoubleField(player, g_env->GetFieldID(g_env->FindClass("pk"), "s", "D"));
            double py = g_env->GetDoubleField(player, g_env->GetFieldID(g_env->FindClass("pk"), "t", "D"));
            double pz = g_env->GetDoubleField(player, g_env->GetFieldID(g_env->FindClass("pk"), "u", "D"));
            float yaw = g_env->GetFloatField(player, g_env->GetFieldID(g_env->FindClass("bew"), "v", "F"));
            float pitch = g_env->GetFloatField(player, g_env->GetFieldID(g_env->FindClass("bew"), "w", "F"));

            jclass entityPlayerClass = g_env->FindClass("bop");

            for (jint i = 0; i < size; i++) {
                jobject ent = g_env->CallObjectMethod(entityList, getMethod, i);
                if (!ent || g_env->IsSameObject(ent, player)) continue;
                if (!g_env->IsInstanceOf(ent, entityPlayerClass)) continue;

                double ex = g_env->GetDoubleField(ent, g_env->GetFieldID(g_env->FindClass("pk"), "s", "D"));
                double ey = g_env->GetDoubleField(ent, g_env->GetFieldID(g_env->FindClass("pk"), "t", "D"));
                double ez = g_env->GetDoubleField(ent, g_env->GetFieldID(g_env->FindClass("pk"), "u", "D"));

                double dx = ex - px;
                double dy = ey - (py + 1.62);
                double dz = ez - pz;

                double cosYaw = cos(yaw * 3.14159/180);
                double sinYaw = sin(yaw * 3.14159/180);
                double cosPitch = cos(pitch * 3.14159/180);
                double sinPitch = sin(pitch * 3.14159/180);

                double dx1 = cosYaw * dz + sinYaw * dx;
                double dy1 = dy;
                double dz1 = cosYaw * dx - sinYaw * dz;

                double dx2 = dx1;
                double dy2 = cosPitch * dy1 - sinPitch * dz1;
                double dz2 = sinPitch * dy1 + cosPitch * dz1;

                if (dz2 <= 0.1) continue;

                int w = rect.right - rect.left;
                int h = rect.bottom - rect.top;
                double fov = 70.0;
                double scale = (w / 2.0) / tan(fov * 0.5 * 3.14159/180);
                int sx = (int)((w/2.0) + (dx2/dz2) * scale);
                int sy = (int)((h/2.0) - (dy2/dz2) * scale);

                HPEN pen = CreatePen(PS_SOLID, 2, RGB(255,0,0));
                SelectObject(hdc, pen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, sx-10, sy-20, sx+10, sy);
                DeleteObject(pen);
            }
            ReleaseDC(g_overlay, hdc);
        }
        Sleep(16);
    }
}

void AimbotLoop() {
    while (g_running) {
        if (g_aimbotOn && g_aimbotActive && AttachToJVM()) {
            jclass mcClass = g_env->FindClass("ave");
            jobject mc = g_env->CallStaticObjectMethod(mcClass, g_env->GetStaticMethodID(mcClass, "A", "()Lave;"));
            jobject player = g_env->GetObjectField(mc, g_env->GetFieldID(mcClass, "h", "Lbew;"));
            if (!player) { Sleep(10); continue; }

            jobject world = g_env->GetObjectField(mc, g_env->GetFieldID(mcClass, "f", "Ladm;"));
            jobject entityList = g_env->GetObjectField(world, g_env->GetFieldID(g_env->FindClass("adm"), "g", "Ljava/util/List;"));
            if (!entityList) { Sleep(10); continue; }

            jclass listClass = g_env->FindClass("java/util/List");
            jint size = g_env->CallIntMethod(entityList, g_env->GetMethodID(listClass, "size", "()I"));
            jmethodID getMethod = g_env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

            double px = g_env->GetDoubleField(player, g_env->GetFieldID(g_env->FindClass("pk"), "s", "D"));
            double py = g_env->GetDoubleField(player, g_env->GetFieldID(g_env->FindClass("pk"), "t", "D")) + 1.62;
            double pz = g_env->GetDoubleField(player, g_env->GetFieldID(g_env->FindClass("pk"), "u", "D"));

            jobject target = nullptr;
            double minDist = 999.0, tx=0, ty=0, tz=0;
            jclass entityPlayerClass = g_env->FindClass("bop");

            for (jint i = 0; i < size; i++) {
                jobject ent = g_env->CallObjectMethod(entityList, getMethod, i);
                if (!ent || g_env->IsSameObject(ent, player)) continue;
                if (!g_env->IsInstanceOf(ent, entityPlayerClass)) continue;
                double ex = g_env->GetDoubleField(ent, g_env->GetFieldID(g_env->FindClass("pk"), "s", "D"));
                double ey = g_env->GetDoubleField(ent, g_env->GetFieldID(g_env->FindClass("pk"), "t", "D"));
                double ez = g_env->GetDoubleField(ent, g_env->GetFieldID(g_env->FindClass("pk"), "u", "D"));
                double dist = sqrt((ex-px)*(ex-px)+(ey-py)*(ey-py)+(ez-pz)*(ez-pz));
                if (dist < minDist) { minDist = dist; target = ent; tx=ex; ty=ey; tz=ez; }
            }

            if (target) {
                double dx = tx-px, dz = tz-pz;
                double yaw = atan2(dz, dx) * (180.0/3.14159) - 90.0;
                double pitch = -atan2(ty-py, sqrt(dx*dx+dz*dz)) * (180.0/3.14159);
                g_env->SetFloatField(player, g_env->GetFieldID(g_env->FindClass("bew"), "v", "F"), (float)yaw);
                g_env->SetFloatField(player, g_env->GetFieldID(g_env->FindClass("bew"), "w", "F"), (float)pitch);
            }
        }
        Sleep(5);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindow(L"BUTTON", L"Toggle Flight", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 10,10,180,30, hwnd, (HMENU)1, nullptr, nullptr);
            CreateWindow(L"BUTTON", L"ESP", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 10,50,80,20, hwnd, (HMENU)2, nullptr, nullptr);
            CreateWindow(L"BUTTON", L"Aimbot", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 100,50,90,20, hwnd, (HMENU)3, nullptr, nullptr);
            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wp) == 1) ToggleFlight();
            else if (LOWORD(wp) == 2) g_espOn = (SendDlgItemMessage(hwnd, 2, BM_GETCHECK, 0, 0) == BST_CHECKED);
            else if (LOWORD(wp) == 3) {
                g_aimbotOn = (SendDlgItemMessage(hwnd, 3, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_aimbotOn) g_aimbotActive = true;
            }
            break;
        }
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

void MakeWindow() {
    while (!g_mcWnd) { g_mcWnd = FindWindowA("LWJGL", nullptr); Sleep(500); }

    WNDCLASSEX wc = {sizeof(WNDCLASSEX)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"FlyGUI";
    RegisterClassEx(&wc);
    g_mainWnd = CreateWindowEx(WS_EX_TOPMOST, L"FlyGUI", L"Control Panel",
                               WS_POPUP|WS_CAPTION|WS_SYSMENU, 100,100,220,120,
                               nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(g_mainWnd, SW_SHOW);

    WNDCLASSEX wc2 = {sizeof(WNDCLASSEX)};
    wc2.lpfnWndProc = DefWindowProc;
    wc2.hInstance = GetModuleHandle(nullptr);
    wc2.lpszClassName = L"ESPOverlay";
    wc2.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    RegisterClassEx(&wc2);
    RECT r; GetClientRect(g_mcWnd, &r);
    g_overlay = CreateWindowEx(WS_EX_TOPMOST|WS_EX_TRANSPARENT|WS_EX_LAYERED,
                               L"ESPOverlay", L"", WS_POPUP,
                               r.left, r.top, r.right-r.left, r.bottom-r.top,
                               g_mcWnd, nullptr, GetModuleHandle(nullptr), nullptr);
    SetLayeredWindowAttributes(g_overlay, RGB(0,0,0), 0, LWA_COLORKEY);
    ShowWindow(g_overlay, SW_SHOW);

    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ESPLoop, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)AimbotLoop, nullptr, 0, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }

    g_running = false;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MakeWindow, nullptr, 0, nullptr);
    return TRUE;
}
