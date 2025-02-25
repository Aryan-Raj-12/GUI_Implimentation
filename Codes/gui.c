#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.lib")

// Rest of your code
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
// Screen IDs
#define SCREEN_INPUT    0
#define SCREEN_PLOT     1
#define SCREEN_SETTINGS 2
#define SCREEN_ABOUT    3
#define NUM_SCREENS     4

// Plot constants
#define MAX_DATA_POINTS 100
#define PLOT_MARGIN_X   50
#define PLOT_MARGIN_Y   50

// Global variables for all screens
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND g_hwndScreens[NUM_SCREENS];  // Array to hold screen handles
int g_currentScreen = SCREEN_INPUT;
HWND g_hwndNavButtons[NUM_SCREENS]; // Navigation buttons

// Input screen controls
HWND hSystemState, hEdit, hTable, hPortDropdown, hBaudrateDropdown;
HWND buttons[5]; 

// Plot screen data
typedef struct {
    double time;
    double value;
} DataPoint;

DataPoint g_plotData[MAX_DATA_POINTS];
int g_dataCount = 0;
HWND g_hwndPlotCanvas;
HWND g_hwndAddDataButton;
HWND g_hwndClearDataButton;
HWND g_hwndDataValueEdit;

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

// Function prototypes
void CreateInputScreen(HWND hwndParent);
void CreatePlotScreen(HWND hwndParent);
void CreateSettingsScreen(HWND hwndParent);
void CreateAboutScreen(HWND hwndParent);
void CreateNavigationControls(HWND hwndParent);
void SwitchToScreen(int screenId);
void ResizeScreen(int screenId, int width, int height);
void DrawPlot(HDC hdc, RECT rect);
void AddDataPoint(double value);

// WinMain - application entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MultiScreenAppClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // Register plot canvas class
    WNDCLASSW wcPlot = { 0 };
    wcPlot.lpfnWndProc = DefWindowProc;
    wcPlot.hInstance = hInstance;
    wcPlot.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcPlot.lpszClassName = L"PlotCanvasClass";
    wcPlot.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wcPlot);

    // Create main window
    HWND hwnd = CreateWindowW(
        L"MultiScreenAppClass", 
        L"Multi-Screen Application", 
        WS_OVERLAPPEDWINDOW | WS_THICKFRAME,
        100, 100, 900, 700, 
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Message loop
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // Create all screens
        CreateInputScreen(hwnd);
        CreatePlotScreen(hwnd);
        CreateSettingsScreen(hwnd);
        CreateAboutScreen(hwnd);
        
        // Create navigation buttons
        CreateNavigationControls(hwnd);
        
        // Show initial screen
        SwitchToScreen(SCREEN_INPUT);
        break;

    case WM_COMMAND:
        // Handle command messages (button clicks, etc.)
        if (LOWORD(wParam) >= 1000 && LOWORD(wParam) < 1000 + NUM_SCREENS) {
            // Navigation button clicked
            SwitchToScreen(LOWORD(wParam) - 1000);
            return 0;
        }
        
        // Plot screen commands
        if (g_currentScreen == SCREEN_PLOT) {
            if (LOWORD(wParam) == 2000) { // Add Data button
                // Get data from edit control
                WCHAR buffer[32];
                GetWindowTextW(g_hwndDataValueEdit, buffer, 32);
                double value = wcstod(buffer, NULL);
                AddDataPoint(value);
                
                // Redraw plot
                InvalidateRect(g_hwndPlotCanvas, NULL, TRUE);
                return 0;
            }
            else if (LOWORD(wParam) == 2001) { // Clear Data button
                g_dataCount = 0;
                InvalidateRect(g_hwndPlotCanvas, NULL, TRUE);
                return 0;
            }
        }
        break;
        
    case WM_SIZE:
        {
            // Get client area size
            RECT rect;
            GetClientRect(hwnd, &rect);
            int width = rect.right;
            int height = rect.bottom;
            
            // Position the navigation controls
            for (int i = 0; i < NUM_SCREENS; i++) {
                MoveWindow(g_hwndNavButtons[i], 
                    i * 120 + 10, 10, 110, 30, TRUE);
            }
            
            // Resize active screen
            ResizeScreen(g_currentScreen, width, height - 50);
        }
        break;
        
    case WM_PAINT:
        if (g_currentScreen == SCREEN_PLOT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(g_hwndPlotCanvas, &ps);
            RECT rect;
            GetClientRect(g_hwndPlotCanvas, &rect);
            DrawPlot(hdc, rect);
            EndPaint(g_hwndPlotCanvas, &ps);
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Create the Input Screen (Original GUI)
void CreateInputScreen(HWND hwndParent) {
    // Create container for the screen
    g_hwndScreens[SCREEN_INPUT] = CreateWindowW(
        L"STATIC", NULL, 
        WS_CHILD | WS_VISIBLE | SS_NOTIFY,
        0, 50, 800, 600, 
        hwndParent, NULL, NULL, NULL
    );

    // System state label
    hSystemState = CreateWindowW(
        L"STATIC", L"SYSTEM STATE", 
        WS_VISIBLE | WS_CHILD,
        50, 20, 200, 50, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );

    // Buttons
    buttons[0] = CreateWindowW(
        L"BUTTON", L"Play", 
        WS_VISIBLE | WS_CHILD, 
        50, 80, 60, 30, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
    
    buttons[1] = CreateWindowW(
        L"BUTTON", L"Pause", 
        WS_VISIBLE | WS_CHILD, 
        120, 80, 60, 30, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
    
    buttons[2] = CreateWindowW(
        L"BUTTON", L"Settings", 
        WS_VISIBLE | WS_CHILD, 
        190, 80, 80, 30, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
    
    buttons[3] = CreateWindowW(
        L"BUTTON", L"TX-ON", 
        WS_VISIBLE | WS_CHILD, 
        280, 80, 70, 30, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
    
    buttons[4] = CreateWindowW(
        L"BUTTON", L"TX-OFF", 
        WS_VISIBLE | WS_CHILD, 
        360, 80, 70, 30, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
    
    // Dropdowns
    hPortDropdown = CreateWindowW(
        L"COMBOBOX", NULL, 
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        50, 130, 150, 100, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
    
    SendMessageW(hPortDropdown, CB_ADDSTRING, 0, (LPARAM)L"COM1");
    SendMessageW(hPortDropdown, CB_ADDSTRING, 0, (LPARAM)L"COM2");
    SendMessageW(hPortDropdown, CB_ADDSTRING, 0, (LPARAM)L"COM3");
    
    hBaudrateDropdown = CreateWindowW(
        L"COMBOBOX", NULL, 
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        220, 130, 150, 100, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
    
    SendMessageW(hBaudrateDropdown, CB_ADDSTRING, 0, (LPARAM)L"9600");
    SendMessageW(hBaudrateDropdown, CB_ADDSTRING, 0, (LPARAM)L"115200");
    
    // Edit control
    hEdit = CreateWindowW(
        L"EDIT", L"", 
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        50, 180, 500, 30, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );

    // Table
    hTable = CreateWindowW(
        L"STATIC", L"Seq   DAF   H   for   fd   SNR_V   SNR_D   Isy", 
        WS_VISIBLE | WS_CHILD,
        50, 230, 700, 200, 
        g_hwndScreens[SCREEN_INPUT], NULL, NULL, NULL
    );
}

// Create the Plot Screen
void CreatePlotScreen(HWND hwndParent) {
    // Create container for the screen
    g_hwndScreens[SCREEN_PLOT] = CreateWindowW(
        L"STATIC", NULL, 
        WS_CHILD | SS_NOTIFY,
        0, 50, 800, 600, 
        hwndParent, NULL, NULL, NULL
    );

    // Plot title
    CreateWindowW(
        L"STATIC", L"Data Plot (Time vs Value)", 
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 10, 800, 30, 
        g_hwndScreens[SCREEN_PLOT], NULL, NULL, NULL
    );

    // Plot canvas (where we'll draw the chart)
    g_hwndPlotCanvas = CreateWindowW(
        L"PlotCanvasClass", NULL, 
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        50, 50, 700, 400, 
        g_hwndScreens[SCREEN_PLOT], NULL, NULL, NULL
    );

    // Data input controls
    CreateWindowW(
        L"STATIC", L"Add new data point:", 
        WS_VISIBLE | WS_CHILD,
        50, 470, 150, 25, 
        g_hwndScreens[SCREEN_PLOT], NULL, NULL, NULL
    );

    g_hwndDataValueEdit = CreateWindowW(
        L"EDIT", L"0.0", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        200, 470, 100, 25, 
        g_hwndScreens[SCREEN_PLOT], NULL, NULL, NULL
    );

    g_hwndAddDataButton = CreateWindowW(
        L"BUTTON", L"Add Data", 
        WS_VISIBLE | WS_CHILD,
        320, 470, 100, 25, 
        g_hwndScreens[SCREEN_PLOT], (HMENU)2000, NULL, NULL
    );

    g_hwndClearDataButton = CreateWindowW(
        L"BUTTON", L"Clear Data", 
        WS_VISIBLE | WS_CHILD,
        430, 470, 100, 25, 
        g_hwndScreens[SCREEN_PLOT], (HMENU)2001, NULL, NULL
    );
}

// Create the Settings Screen (Placeholder)
void CreateSettingsScreen(HWND hwndParent) {
    // Create container for the screen
    g_hwndScreens[SCREEN_SETTINGS] = CreateWindowW(
        L"STATIC", NULL, 
        WS_CHILD | SS_NOTIFY,
        0, 50, 800, 600, 
        hwndParent, NULL, NULL, NULL
    );

    // Settings content
    CreateWindowW(
        L"STATIC", L"Settings Screen (Placeholder)", 
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 100, 800, 30, 
        g_hwndScreens[SCREEN_SETTINGS], NULL, NULL, NULL
    );

    // Add a few mock settings
    CreateWindowW(
        L"STATIC", L"Display Options:", 
        WS_VISIBLE | WS_CHILD,
        100, 150, 200, 25, 
        g_hwndScreens[SCREEN_SETTINGS], NULL, NULL, NULL
    );

    CreateWindowW(
        L"BUTTON", L"Show Grid Lines", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        100, 180, 200, 25, 
        g_hwndScreens[SCREEN_SETTINGS], NULL, NULL, NULL
    );

    CreateWindowW(
        L"BUTTON", L"Dark Mode", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        100, 210, 200, 25, 
        g_hwndScreens[SCREEN_SETTINGS], NULL, NULL, NULL
    );

    CreateWindowW(
        L"STATIC", L"Data Sampling Rate:", 
        WS_VISIBLE | WS_CHILD,
        100, 250, 200, 25, 
        g_hwndScreens[SCREEN_SETTINGS], NULL, NULL, NULL
    );

    CreateWindowW(
        L"COMBOBOX", NULL, 
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        100, 280, 200, 100, 
        g_hwndScreens[SCREEN_SETTINGS], NULL, NULL, NULL
    );

    // Add items to the combo box
    HWND hCombo = FindWindowExW(g_hwndScreens[SCREEN_SETTINGS], NULL, L"COMBOBOX", NULL);
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"1 Hz");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"10 Hz");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"100 Hz");
    SendMessageW(hCombo, CB_SETCURSEL, 1, 0);
}

// Create the About Screen (Placeholder)
void CreateAboutScreen(HWND hwndParent) {
    // Create container for the screen
    g_hwndScreens[SCREEN_ABOUT] = CreateWindowW(
        L"STATIC", NULL, 
        WS_CHILD | SS_NOTIFY,
        0, 50, 800, 600, 
        hwndParent, NULL, NULL, NULL
    );

    // About content
    CreateWindowW(
        L"STATIC", L"About This Application", 
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 100, 800, 30, 
        g_hwndScreens[SCREEN_ABOUT], NULL, NULL, NULL
    );

    CreateWindowW(
        L"STATIC", 
        L"This is a multi-screen application demonstrating\r\n"
        L"Windows API GUI development with C.\r\n\r\n"
        L"Features:\r\n"
        L"- Serial Communication\r\n"
        L"- Real-time Data Plotting\r\n"
        L"- Configuration Options\r\n"
        L"- User-friendly Interface", 
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        100, 150, 600, 200, 
        g_hwndScreens[SCREEN_ABOUT], NULL, NULL, NULL
    );

    CreateWindowW(
        L"STATIC", L"Version 1.0", 
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 350, 800, 30, 
        g_hwndScreens[SCREEN_ABOUT], NULL, NULL, NULL
    );

    CreateWindowW(
        L"BUTTON", L"View License", 
        WS_VISIBLE | WS_CHILD,
        350, 400, 100, 30, 
        g_hwndScreens[SCREEN_ABOUT], NULL, NULL, NULL
    );
}

// Create the navigation controls
void CreateNavigationControls(HWND hwndParent) {
    const WCHAR* buttonLabels[NUM_SCREENS] = {
        L"Input", L"Plot", L"Settings", L"About"
    };
    
    for (int i = 0; i < NUM_SCREENS; i++) {
        g_hwndNavButtons[i] = CreateWindowW(
            L"BUTTON", buttonLabels[i], 
            WS_VISIBLE | WS_CHILD,
            i * 120 + 10, 10, 110, 30, 
            hwndParent, (HMENU)(UINT_PTR)(1000 + i), NULL, NULL
        );
    }
}

// Switch to a specific screen
void SwitchToScreen(int screenId) {
    if (screenId < 0 || screenId >= NUM_SCREENS) 
        return;
    
    // Hide all screens
    for (int i = 0; i < NUM_SCREENS; i++) {
        ShowWindow(g_hwndScreens[i], SW_HIDE);
    }
    
    // Show the selected screen
    ShowWindow(g_hwndScreens[screenId], SW_SHOW);
    g_currentScreen = screenId;
    
    // Get the parent window
    HWND hwndParent = GetParent(g_hwndScreens[screenId]);
    
    // Set the window title based on the screen
    WCHAR title[100];
    swprintf(title, 100, L"Multi-Screen Application - %s", 
        screenId == SCREEN_INPUT ? L"Input" : 
        screenId == SCREEN_PLOT ? L"Plot" : 
        screenId == SCREEN_SETTINGS ? L"Settings" : L"About");
    SetWindowTextW(hwndParent, title);
    
    // Trigger a resize to ensure correct layout
    RECT rect;
    GetClientRect(hwndParent, &rect);
    PostMessageW(hwndParent, WM_SIZE, 0, MAKELPARAM(rect.right, rect.bottom));
}

// Resize the current screen
void ResizeScreen(int screenId, int width, int height) {
    // Position the screen beneath the navigation buttons
    MoveWindow(g_hwndScreens[screenId], 0, 50, width, height, TRUE);
    
    if (screenId == SCREEN_INPUT) {
        // Resize input screen controls
        MoveWindow(hSystemState, 50, 20, width - 100, 50, TRUE);
        // Resize other input controls as needed
        MoveWindow(hTable, 50, 230, width - 100, height - 280, TRUE);
    }
    else if (screenId == SCREEN_PLOT) {
        // Resize plot canvas
        MoveWindow(g_hwndPlotCanvas, 50, 50, width - 100, height - 150, TRUE);
        
        // Reposition data input controls
        int controlY = height - 80;
        CreateWindowW(L"STATIC", L"Add new data point:", WS_VISIBLE | WS_CHILD,
            50, controlY, 150, 25, g_hwndScreens[SCREEN_PLOT], NULL, NULL, NULL);
        MoveWindow(g_hwndDataValueEdit, 200, controlY, 100, 25, TRUE);
        MoveWindow(g_hwndAddDataButton, 320, controlY, 100, 25, TRUE);
        MoveWindow(g_hwndClearDataButton, 430, controlY, 100, 25, TRUE);
    }
}

// Add a new data point to the plot
void AddDataPoint(double value) {
    // Calculate time (just use sequential numbering)
    double time = g_dataCount > 0 ? g_plotData[g_dataCount-1].time + 1.0 : 0.0;
    
    // If we've reached the maximum, shift everything left
    if (g_dataCount >= MAX_DATA_POINTS) {
        for (int i = 0; i < MAX_DATA_POINTS-1; i++) {
            g_plotData[i] = g_plotData[i+1];
        }
        g_dataCount = MAX_DATA_POINTS-1;
    }
    
    // Add the new point
    g_plotData[g_dataCount].time = time;
    g_plotData[g_dataCount].value = value;
    g_dataCount++;
}

// Draw the plot
void DrawPlot(HDC hdc, RECT rect) {
    // Clear the background
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    
    // If no data, just show a message
    if (g_dataCount == 0) {
        SetTextAlign(hdc, TA_CENTER | TA_BASELINE);
        TextOutW(hdc, rect.right/2, rect.bottom/2, L"No data to display", 18);
        return;
    }
    
    // Calculate plot area
    int plotWidth = rect.right - 2 * PLOT_MARGIN_X;
    int plotHeight = rect.bottom - 2 * PLOT_MARGIN_Y;
    int plotLeft = PLOT_MARGIN_X;
    int plotBottom = rect.bottom - PLOT_MARGIN_Y;
    
    // Find min/max values
    double minTime = g_plotData[0].time;
    double maxTime = g_plotData[0].time;
    double minValue = g_plotData[0].value;
    double maxValue = g_plotData[0].value;
    
    for (int i = 0; i < g_dataCount; i++) {
        if (g_plotData[i].time < minTime) minTime = g_plotData[i].time;
        if (g_plotData[i].time > maxTime) maxTime = g_plotData[i].time;
        if (g_plotData[i].value < minValue) minValue = g_plotData[i].value;
        if (g_plotData[i].value > maxValue) maxValue = g_plotData[i].value;
    }
    
    // Add some padding to the value range
    double valuePadding = (maxValue - minValue) * 0.1;
    if (valuePadding < 0.1) valuePadding = 0.1;
    minValue -= valuePadding;
    maxValue += valuePadding;
    
    // Draw axes
    HPEN axisPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN oldPen = SelectObject(hdc, axisPen);
    
    // X-axis
    MoveToEx(hdc, plotLeft, plotBottom, NULL);
    LineTo(hdc, plotLeft + plotWidth, plotBottom);
    
    // Y-axis
    MoveToEx(hdc, plotLeft, plotBottom, NULL);
    LineTo(hdc, plotLeft, plotBottom - plotHeight);
    
    // Draw grid lines and labels
    HPEN gridPen = CreatePen(PS_DOT, 1, RGB(200, 200, 200));
    SelectObject(hdc, gridPen);
    
    // X grid (time)
    SetTextAlign(hdc, TA_CENTER | TA_TOP);
    int numXGridLines = 5;
    for (int i = 0; i <= numXGridLines; i++) {
        int x = plotLeft + (i * plotWidth / numXGridLines);
        double time = minTime + (i * (maxTime - minTime) / numXGridLines);
        
        // Grid line
        MoveToEx(hdc, x, plotBottom, NULL);
        LineTo(hdc, x, plotBottom - plotHeight);
        
        // Label
        WCHAR label[32];
        swprintf(label, 32, L"%.1f", time);
        TextOutW(hdc, x, plotBottom + 5, label, wcslen(label));
    }
    
    // Y grid (value)
    SetTextAlign(hdc, TA_RIGHT | TA_BASELINE);
    int numYGridLines = 4;
    for (int i = 0; i <= numYGridLines; i++) {
        int y = plotBottom - (i * plotHeight / numYGridLines);
        double value = minValue + (i * (maxValue - minValue) / numYGridLines);
        
        // Grid line
        MoveToEx(hdc, plotLeft, y, NULL);
        LineTo(hdc, plotLeft + plotWidth, y);
        
        // Label
        WCHAR label[32];
        swprintf(label, 32, L"%.1f", value);
        TextOutW(hdc, plotLeft - 5, y, label, wcslen(label));
    }
    
    // Draw data points and connecting lines
    HPEN dataPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 200));
    SelectObject(hdc, dataPen);
    
    // Scale factors
    double timeScale = plotWidth / (maxTime - minTime);
    double valueScale = plotHeight / (maxValue - minValue);
    
    // Plot the points
    for (int i = 0; i < g_dataCount; i++) {
        int x = plotLeft + (int)((g_plotData[i].time - minTime) * timeScale);
        int y = plotBottom - (int)((g_plotData[i].value - minValue) * valueScale);
        
        // Draw point
        Ellipse(hdc, x-3, y-3, x+3, y+3);
        
        // Draw line to next point
        if (i < g_dataCount - 1) {
            int nextX = plotLeft + (int)((g_plotData[i+1].time - minTime) * timeScale);
            int nextY = plotBottom - (int)((g_plotData[i+1].value - minValue) * valueScale);
            MoveToEx(hdc, x, y, NULL);
            LineTo(hdc, nextX, nextY);
        }
    }
    
    // Add title and labels
    SetTextAlign(hdc, TA_CENTER | TA_TOP);
    TextOutW(hdc, plotLeft + plotWidth/2, 10, L"Time vs Value Plot", 17);
    
    SetTextAlign(hdc, TA_CENTER | TA_BASELINE);
    TextOutW(hdc, plotLeft + plotWidth/2, rect.bottom - 10, L"Time", 4);
    
    // Draw Y-axis label (rotated text is complicated in GDI)
    SetTextAlign(hdc, TA_CENTER | TA_BASELINE);
    TextOutW(hdc, 15, plotBottom - plotHeight/2, L"Value", 5);
    
    // Clean up
    SelectObject(hdc, oldPen);
    DeleteObject(axisPen);
    DeleteObject(gridPen);
    DeleteObject(dataPen);
}
