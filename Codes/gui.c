#include <windows.h>
#include <string.h>
#include <stdio.h>
// Global variables
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND hSystemState, hEdit, hTable, hPortDropdown, hBaudrateDropdown;
HWND buttons[5]; 

// Serial Communication
HANDLE hSerial;
int InitSerialComm(LPCWSTR portName) {
    hSerial = CreateFileW(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) return 0;
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) return 0;
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.StopBits = ONESTOPBIT;
    return SetCommState(hSerial, &dcbSerialParams);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MyWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"MyWindowClass", L"Win32 GUI with Serial Communication", WS_OVERLAPPEDWINDOW | WS_THICKFRAME,
        100, 100, 800, 600, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void ResizeControls(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right;
    int height = rect.bottom;

    MoveWindow(hSystemState, 50, 50, width - 100, 50, TRUE);
    MoveWindow(hEdit, 50, 250, width - 100, 30, TRUE);
    MoveWindow(hTable, 50, 300, width - 100, height - 350, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hSystemState = CreateWindowW(L"STATIC", L"SYSTEM STATE", WS_VISIBLE | WS_CHILD,
            50, 50, 200, 50, hwnd, NULL, NULL, NULL);

        // Buttons
        buttons[0] = CreateWindowW(L"BUTTON", L"Play", WS_VISIBLE | WS_CHILD, 50, 150, 60, 30, hwnd, NULL, NULL, NULL);
        buttons[1] = CreateWindowW(L"BUTTON", L"Pause", WS_VISIBLE | WS_CHILD, 120, 150, 60, 30, hwnd, NULL, NULL, NULL);
        buttons[2] = CreateWindowW(L"BUTTON", L"Settings", WS_VISIBLE | WS_CHILD, 190, 150, 80, 30, hwnd, NULL, NULL, NULL);
        buttons[3] = CreateWindowW(L"BUTTON", L"TX-ON", WS_VISIBLE | WS_CHILD, 280, 150, 70, 30, hwnd, NULL, NULL, NULL);
        buttons[4] = CreateWindowW(L"BUTTON", L"TX-OFF", WS_VISIBLE | WS_CHILD, 360, 150, 70, 30, hwnd, NULL, NULL, NULL);
        
        // Dropdowns
        hPortDropdown = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
            50, 200, 150, 100, hwnd, NULL, NULL, NULL);
        SendMessageW(hPortDropdown, CB_ADDSTRING, 0, (LPARAM)L"COM1");
        SendMessageW(hPortDropdown, CB_ADDSTRING, 0, (LPARAM)L"COM2");
        SendMessageW(hPortDropdown, CB_ADDSTRING, 0, (LPARAM)L"COM3");
        
        hBaudrateDropdown = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
            220, 200, 150, 100, hwnd, NULL, NULL, NULL);
        SendMessageW(hBaudrateDropdown, CB_ADDSTRING, 0, (LPARAM)L"9600");
        SendMessageW(hBaudrateDropdown, CB_ADDSTRING, 0, (LPARAM)L"115200");
        
        hEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
            50, 250, 500, 30, hwnd, NULL, NULL, NULL);

        hTable = CreateWindowW(L"STATIC", L"Seq   DAF   H   for   fd   SNR_V   SNR_D   Isy", WS_VISIBLE | WS_CHILD,
            50, 300, 700, 200, hwnd, NULL, NULL, NULL);
        
        break;
    
    case WM_SIZE:
        ResizeControls(hwnd);
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
