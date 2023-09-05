#include <raylib.h>
#include <raymath.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <list>
#include <iostream>
#include <chrono>
#include <math.h>
#include <thread>
#include <fstream>

#if defined(_WIN32)
// To avoid conflicting windows.h symbols with raylib, some flags are defined
// WARNING: Those flags avoid inclusion of some Win32 headers that could be required
// by user at some point and won't be included...
//-------------------------------------------------------------------------------------

// If defined, the following flags inhibit definition of the indicated items.
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
//#define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

// Type required before windows.h inclusion
typedef struct tagMSG *LPMSG;

#include "include/serialib.h"

// Type required by some unused function...
typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#include <objbase.h>
#include <mmreg.h>
#include <mmsystem.h>

// Some required types defined for MSVC/TinyC compiler
#if defined(_MSC_VER) || defined(__TINYC__)
    #include "propidl.h"
#endif
#endif

#if defined (_WIN32) || defined(_WIN64)
    //for serial ports above "COM9", we must use this extended syntax of "\\.\COMx".
    //also works for COM0 to COM9.
    //https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea?redirectedfrom=MSDN#communications-resources
    #define SERIAL_PORT_PREFIX "\\\\.\\COM"
#endif
#if defined (__linux__) || defined(__APPLE__)
    #include "include/serialib.h"
    #define SERIAL_PORT_PREFIX "/dev/ttyUSB"
    //#define SERIAL_PORT "/dev/ttyACM0"
#endif

#define MCGREEN CLITERAL(Color){150,182,171,255}   // Verde Colin McRae
#define RESOLUTION 1024
#define BAUDS 115200
#define BUFFER_SIZE 100
#define FS 10000 //[Hz]
#define SAMPLES 1000
bool RUNNING = true;
std::string FREC = "0";
char UNIT;

struct Theme
{
    Color background = MCGREEN;
    Color foreground = WHITE;
};

void DrawDottedLine(Vector2 startPos, Vector2 endPos, float thick, float divisions, Color color);
void serialProcess(serialib *serial, std::list<int> *graphData);
void threadTest(std::string str);

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1366;
    const int screenHeight = 768;

    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Serial Plotter");

    int serialPortNumber = 0;
    std::string SERIAL_PORT = std::string(SERIAL_PORT_PREFIX)+std::to_string(serialPortNumber);

    serialib *serial = new serialib();
    char data[BUFFER_SIZE];
    int i, lastMax = 0, dataValue = 0;
    std::list<int> graphData;
    std::chrono::milliseconds timerMs;
    Font font1 = LoadFont("src/fonts/JetBrainsMono/JetBrainsMono-Bold.ttf");
    float fontSize = screenWidth/42;//font1.baseSize;
    double tiempo = 0;
    double tiempoAnterior = 0;
    double tiempoAcumulado = 0;
    int caida = 0;
    int cantPicos = 0;
    std::string command;

    int THEMES_COUNT = 8;
    struct Theme themes[THEMES_COUNT] = {{MCGREEN,WHITE},
    {(Color){46,52,64,255},(Color){229,233,240,255}},
    {(Color){3,57,75,255},(Color){187,231,250,255}},
    {(Color){78,70,67,255},(Color){254,167,0,255}},
    {(Color){82,76,70,255},(Color){234,233,233,255}},
    {(Color){43,43,43,255},(Color){155,155,155,255}},
    {BLACK, WHITE},
    {WHITE,BLACK}};

    int CURRENT_THEME = 0;
    Color COLOR_BACKGROUND = themes[0].background;
    Color COLOR_FOREGROUND = themes[0].foreground;
    int EXCURSION_MAX = screenWidth/2.3f;//screenHeight/1.3f;
    int VISOR_MARGIN_V = (screenHeight-EXCURSION_MAX)/2;
    int VISOR_MARGIN_H = (screenWidth-EXCURSION_MAX)/2;
    int SEPARATION_V = screenHeight/20.0f;
    int SEPARATION_H = screenWidth/15.0f;
    Rectangle visorRectangle = {VISOR_MARGIN_H,VISOR_MARGIN_V,EXCURSION_MAX,EXCURSION_MAX};
    Vector2 xOriginStart = {screenWidth/2,VISOR_MARGIN_V};
    Vector2 xOriginEnd = {screenWidth/2,screenHeight-VISOR_MARGIN_V};
    Vector2 yOriginStart = {VISOR_MARGIN_H,screenHeight/2};
    Vector2 yOriginEnd = {screenWidth-VISOR_MARGIN_H,screenHeight/2};
    Vector2 xLineStart = xOriginStart;
    Vector2 xLineEnd = xOriginEnd;
    Vector2 yLineStart = yOriginStart;
    Vector2 yLineEnd = yOriginEnd;
    int VAL_DISPLAY_ICON_W = VISOR_MARGIN_H-SEPARATION_H*2;//VISOR_MARGIN_H/1.5;
    int VAL_DISPLAY_ICON_H = EXCURSION_MAX/7-SEPARATION_V/1.2f;

    float V_SCALE = 0.2f;
    float H_SCALE = 1.0f;
    float OFFSET = 0.0f;
    float LINE_THICKNESS = 2.0f;
    int SUBDIVISIONS = 10;
    float THRESHOLD = 0.0f;

    int FILTER_TYPE = 0;        // 0: Lowpass - 1: Bandpass - 2: Highpass
    bool FILTER_RESPONSE = 0;   // 0: IIR - 1: FIR
    bool SAMPLING_MODE = 0;     // 0: Finite - 1: Real time

    float valueRMS = 0;
    float valueMean = 0, lastMean = 0;
    float valueMax = 0;
    float valueMin = 0;
    float valueVPP = 0;
    std::string dataRMS, dataMean, dataMax, dataMin, dataFrec, dataVPP;
    int samplesAmount = 0;

    for(i = 0; i < SAMPLES; i++)  graphData.push_back(0);

    // Connection to serial port
    char PORT_STATE = serial->openDevice(SERIAL_PORT.c_str(), BAUDS);
    // If connection fails, return the error code otherwise, display a success message
    if (PORT_STATE!=1) std::cout << "Could not connect to " << SERIAL_PORT << std::endl;
    else
    {
        std::cout << "Successful connection to " << SERIAL_PORT << std::endl;
    }

    //std::thread t1(threadTest, "AEEA");
    std::thread t1(serialProcess, serial, &graphData);

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------
    while (!WindowShouldClose())
    {
        if(IsKeyPressed(KEY_X) && (PORT_STATE!=1))
        {
            if (serialPortNumber < 12) serialPortNumber++;
            else serialPortNumber = 0;

            SERIAL_PORT = std::string(SERIAL_PORT_PREFIX)+std::to_string(serialPortNumber);

            PORT_STATE = serial->openDevice(SERIAL_PORT.c_str(), BAUDS);

            if (PORT_STATE!=1) std::cout << "Could not connect to " << SERIAL_PORT << std::endl;
            else std::cout << "Successful connection to " << SERIAL_PORT << std::endl;
        }
        if(IsKeyPressed(KEY_S))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT))
            {
                if(SUBDIVISIONS > 2) SUBDIVISIONS -= 2;
            }
            else SUBDIVISIONS += 2;
        }
        if(IsKeyPressed(KEY_V))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT))
            {
                if(V_SCALE > 0.1f) V_SCALE -= 0.1f;
            }
            else V_SCALE += 0.1f;
        }
        if(IsKeyPressed(KEY_H))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT)) H_SCALE -= 0.2f;
            else H_SCALE += 0.2f;
        }
        if(IsKeyPressed(KEY_L))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT)) LINE_THICKNESS -= 0.2f;
            else LINE_THICKNESS += 0.2f;
        }
        if(IsKeyPressed(KEY_O))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT)) OFFSET += (EXCURSION_MAX/SUBDIVISIONS)/2.0f;
            else OFFSET -= (EXCURSION_MAX/SUBDIVISIONS)/2.0f;
        }
        if(IsKeyDown(KEY_T))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT))
            {
                //if(-THRESHOLD*V_SCALE < EXCURSION_MAX/2)
                THRESHOLD -= 1.0f;
            }
            else
            {
                //if(THRESHOLD*V_SCALE < (EXCURSION_MAX/2))
                THRESHOLD += 1.0f;
            }
        }
        if(IsKeyPressed(KEY_P))
        {
            V_SCALE = 1.0f;
            H_SCALE = 1.0f;
            OFFSET = 0.0f;
            LINE_THICKNESS = 2.0f;
            SUBDIVISIONS = 10;
            THRESHOLD = 0.0f;
        }
        if(IsKeyPressed(KEY_C))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT))
            {
                if (CURRENT_THEME > 0) CURRENT_THEME--;
                else CURRENT_THEME = THEMES_COUNT-1;
            }else
            {
                if (CURRENT_THEME < THEMES_COUNT-1) CURRENT_THEME++;
                else CURRENT_THEME = 0;
            }
            
            COLOR_BACKGROUND = themes[CURRENT_THEME].background;
            COLOR_FOREGROUND = themes[CURRENT_THEME].foreground;
        }

        if(IsKeyPressed(KEY_F))
        {
            if (FILTER_TYPE < 2) FILTER_TYPE++;
            else FILTER_TYPE = 0;

            command = std::string("[AE-FT"+std::to_string(FILTER_TYPE)+"-EA]");
            if(PORT_STATE==1) serial->writeString(command.c_str());
        }

        if(IsKeyPressed(KEY_R))
        {
            FILTER_RESPONSE = !FILTER_RESPONSE;

            command = std::string("[AE-FR"+std::to_string(FILTER_RESPONSE)+"-EA]");
            if(PORT_STATE==1) serial->writeString(command.c_str());
        }

        if(IsKeyPressed(KEY_M))
        {
            // SS - Send Sample
            if(PORT_STATE==1) serial->writeString("[AE-SS-EA]");
        }

        if(IsKeyPressed(KEY_N))
        {
            SAMPLING_MODE = !SAMPLING_MODE;

            command = std::string("[AE-M"+std::to_string(SAMPLING_MODE)+"-EA]");
            if(PORT_STATE==1) serial->writeString(command.c_str());
        }

        if(IsKeyPressed(KEY_G))
        {
            std::ofstream outputFile("output.txt");

            if (outputFile.is_open())
            {
                for (std::list<int>::iterator it = graphData.begin(); it != std::prev(graphData.end()); it++)
                {
                    outputFile << std::to_string((*it)*5.0f/RESOLUTION)+"\n";
                }
                outputFile.close();
            }
        }
        //----------------------------------------------------------------------------------
        // Dibuja
        //----------------------------------------------------------------------------------

        BeginDrawing();

            ClearBackground(COLOR_BACKGROUND);

            i = 0;
            valueRMS = 0.0f;
            valueMean = 0.0f;
            valueMax = 0.0f;
            valueMin = 1024.0f;
            samplesAmount = 0;
            for (std::list<int>::iterator it = graphData.begin(); it != std::prev(graphData.end()); it++)
            {
                int posx1 = screenWidth/2+EXCURSION_MAX/2-i*H_SCALE;
                int posx2 = screenWidth/2+EXCURSION_MAX/2-(i+1)*H_SCALE;
                int posy1 = screenHeight/2-(*it)*V_SCALE+OFFSET;
                int posy2 = screenHeight/2-(*std::next(it))*V_SCALE+OFFSET;
                
                if((posx1 < screenWidth-VISOR_MARGIN_H) && (posx2 > VISOR_MARGIN_H))
                    DrawLineEx((Vector2){posx1,posy1},(Vector2){posx2,posy2},LINE_THICKNESS,COLOR_FOREGROUND);

                i++;
                
                valueRMS += pow((*it),2.0f);
                valueMean += *it;
                if((*it) > valueMax)
                    valueMax = *it;
                if((*it) < valueMin)
                    valueMin = *it;
                
            }
            lastMean = valueMean;
            samplesAmount = i;

            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,0,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,COLOR_BACKGROUND);
            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,screenHeight-(screenHeight-EXCURSION_MAX)/2,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,COLOR_BACKGROUND);
            //DrawRectangleLinesEx(visorRectangle,3.0f,COLOR_FOREGROUND);
            DrawRectangleRoundedLines(visorRectangle,EXCURSION_MAX/10000.0f,10,3.0f,COLOR_FOREGROUND);
            for(i = -SUBDIVISIONS/2+1; i < SUBDIVISIONS/2; i++)
            {
                xLineStart.x = xOriginStart.x+EXCURSION_MAX/SUBDIVISIONS*i;
                xLineEnd.x = xOriginEnd.x+EXCURSION_MAX/SUBDIVISIONS*i;
                DrawLineEx(xLineStart,xLineEnd,0.25f,COLOR_FOREGROUND);
                yLineStart.y = yOriginStart.y+EXCURSION_MAX/SUBDIVISIONS*i;
                yLineEnd.y = yOriginEnd.y+EXCURSION_MAX/SUBDIVISIONS*i;
                DrawLineEx(yLineStart,yLineEnd,0.25f,COLOR_FOREGROUND);
            }

            // y axis values
            float valueFontSize = fontSize/1.5;
            float yScale;
            std::string yScale_str;
            for(i = -(SUBDIVISIONS/2); i <= (SUBDIVISIONS/2); i++)
            {
                float yScale = ((i*(EXCURSION_MAX/SUBDIVISIONS)/V_SCALE)*5/RESOLUTION)+OFFSET/V_SCALE*5/RESOLUTION;
                yScale_str = std::to_string(yScale);
                yScale_str = yScale_str.substr(0,yScale_str.find(".")+3);
                DrawTextEx(font1,std::string(yScale_str).c_str(),(Vector2){screenWidth-VISOR_MARGIN_H+valueFontSize/2,screenHeight/2-i*(EXCURSION_MAX/SUBDIVISIONS)-valueFontSize/2},valueFontSize,1,COLOR_FOREGROUND);
            }
            // bottom legend
            yScale = ((1*(EXCURSION_MAX/SUBDIVISIONS)/V_SCALE)*5/RESOLUTION)+OFFSET/V_SCALE*5/RESOLUTION;
            yScale_str = std::to_string(yScale);
            yScale_str = yScale_str.substr(0,yScale_str.find(".")+3)+"[v]";
            float xScale;
            std::string xScale_str;
            xScale = (EXCURSION_MAX/SUBDIVISIONS)*(1000.0f/FS)/H_SCALE;
            xScale_str = std::to_string(xScale);
            xScale_str = xScale_str.substr(0,xScale_str.find(".")+3)+"[ms]";
            DrawTextEx(font1,std::string(xScale_str+"/"+yScale_str).c_str(),(Vector2){VISOR_MARGIN_H,screenHeight-VISOR_MARGIN_V+fontSize/2},fontSize,1,COLOR_FOREGROUND);
            // origin dotted line
            yLineStart.y = yOriginStart.y+OFFSET;
            yLineEnd.y = yOriginEnd.y+OFFSET;
            DrawDottedLine(yLineStart, yLineEnd, 2.0f, 30, COLOR_FOREGROUND);
            // trigger dotted line
            yLineStart.y = yOriginStart.y-THRESHOLD*V_SCALE+OFFSET;
            yLineEnd.y = yOriginEnd.y-THRESHOLD*V_SCALE+OFFSET;
            DrawDottedLine(yLineStart, yLineEnd, 0.5f, 50, COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("T").c_str(),(Vector2){VISOR_MARGIN_H-valueFontSize,screenHeight/2-THRESHOLD*V_SCALE+OFFSET-valueFontSize/2},valueFontSize,1,COLOR_FOREGROUND);
            // controls legend
            DrawTextEx(font1,std::string("L: LINE THICKNESS").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*0},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("V: VERTICAL SCALING").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*1},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("H: HORIZONTAL SCALING").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*2},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("S: SUBDIVISIONS AMOUNT").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*3},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("O: VERTICAL OFFSET").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*4},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("T: VERTICAL THRESHOLD").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*5},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("P: RESET PARAMETERS").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*6},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("C: CHANGE THEME").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*7},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("M: GET SAMPLE").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*8},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("N: SAMPLING MODE").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*9},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("G: SAVE TO FILE").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*10},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("SHIFT: REVERSE COMMAND").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*11},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("CTRL: FINE TUNING").c_str(),(Vector2){20,VISOR_MARGIN_V+fontSize*12},fontSize,1,COLOR_FOREGROUND);

            // data computation

            valueRMS = valueRMS*25.0f/pow(RESOLUTION,2.0f); // scaling
            valueMean = valueMean*5.0f/RESOLUTION;
            valueMax = valueMax*5.0f/RESOLUTION;
            valueMin = valueMin*5.0f/RESOLUTION;
            valueVPP = valueMax-valueMin;
            valueRMS = sqrt(valueRMS/samplesAmount);
            valueMean = valueMean/samplesAmount;

            dataRMS = std::to_string(valueRMS);
            dataRMS = dataRMS.substr(0,dataRMS.find(".")+3)+"[V]";
            
            dataMean = std::to_string(valueMean);
            dataMean = dataMean.substr(0,dataMean.find(".")+3)+"[V]";

            dataMax = std::to_string(valueMax);
            dataMax = dataMax.substr(0,dataMax.find(".")+3)+"[V]";
            
            dataMin = std::to_string(valueMin);
            dataMin = dataMin.substr(0,dataMin.find(".")+3)+"[V]";

            dataVPP = std::to_string(valueVPP);
            dataVPP = dataVPP.substr(0,dataVPP.find(".")+3)+"[V]";

            dataFrec = FREC;
            dataFrec = dataFrec.substr(0,dataFrec.find(".")+3)+"[";
            if(UNIT == 'K') dataFrec = dataFrec+"k";
            if(UNIT == 'M') dataFrec = dataFrec+"M";
            dataFrec = dataFrec+"Hz]";

            if(stoi(FREC) > 24)
            {
                SetTargetFPS(stoi(FREC));
                //H_SCALE = stoi(FREC)*0.5f;
            }else{
                SetTargetFPS(60);
            }
            //  value data displays

            Vector2 valueDisplayPos = (Vector2){screenWidth-VISOR_MARGIN_H+SEPARATION_H,VISOR_MARGIN_V};
            
            DrawRectangleRoundedLines((Rectangle){valueDisplayPos.x,valueDisplayPos.y,VAL_DISPLAY_ICON_W,VAL_DISPLAY_ICON_H},VAL_DISPLAY_ICON_W/1000.0f,10,3.0f,COLOR_FOREGROUND);
            DrawTextEx(font1,"RMS",(Vector2){valueDisplayPos.x+valueFontSize/4,valueDisplayPos.y},valueFontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,dataRMS.c_str(),(Vector2){valueDisplayPos.x+VAL_DISPLAY_ICON_W-fontSize*dataRMS.length()*0.5f,valueDisplayPos.y+VAL_DISPLAY_ICON_H-fontSize},fontSize,1,COLOR_FOREGROUND);

            valueDisplayPos.y += VAL_DISPLAY_ICON_H+SEPARATION_V;
            DrawRectangleRoundedLines((Rectangle){valueDisplayPos.x,valueDisplayPos.y,VAL_DISPLAY_ICON_W,VAL_DISPLAY_ICON_H},VAL_DISPLAY_ICON_W/1000.0f,10,3.0f,COLOR_FOREGROUND);
            DrawTextEx(font1,"Media",(Vector2){valueDisplayPos.x+valueFontSize/4,valueDisplayPos.y},valueFontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,dataMean.c_str(),(Vector2){valueDisplayPos.x+VAL_DISPLAY_ICON_W-fontSize*dataMax.length()*0.5f,valueDisplayPos.y+VAL_DISPLAY_ICON_H-fontSize},fontSize,1,COLOR_FOREGROUND);

            valueDisplayPos.y += VAL_DISPLAY_ICON_H+SEPARATION_V;
            DrawRectangleRoundedLines((Rectangle){valueDisplayPos.x,valueDisplayPos.y,VAL_DISPLAY_ICON_W,VAL_DISPLAY_ICON_H},VAL_DISPLAY_ICON_W/1000.0f,10,3.0f,COLOR_FOREGROUND);
            DrawTextEx(font1,"Max",(Vector2){valueDisplayPos.x+valueFontSize/4,valueDisplayPos.y},valueFontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,dataMax.c_str(),(Vector2){valueDisplayPos.x+VAL_DISPLAY_ICON_W-fontSize*dataMin.length()*0.5f,valueDisplayPos.y+VAL_DISPLAY_ICON_H-fontSize},fontSize,1,COLOR_FOREGROUND);

            valueDisplayPos.y += VAL_DISPLAY_ICON_H+SEPARATION_V;
            DrawRectangleRoundedLines((Rectangle){valueDisplayPos.x,valueDisplayPos.y,VAL_DISPLAY_ICON_W,VAL_DISPLAY_ICON_H},VAL_DISPLAY_ICON_W/1000.0f,10,3.0f,COLOR_FOREGROUND);
            DrawTextEx(font1,"Min",(Vector2){valueDisplayPos.x+valueFontSize/4,valueDisplayPos.y},valueFontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,dataMin.c_str(),(Vector2){valueDisplayPos.x+VAL_DISPLAY_ICON_W-fontSize*dataRMS.length()/2,valueDisplayPos.y+VAL_DISPLAY_ICON_H-fontSize},fontSize,1,COLOR_FOREGROUND);

            valueDisplayPos.y += VAL_DISPLAY_ICON_H+SEPARATION_V;
            DrawRectangleRoundedLines((Rectangle){valueDisplayPos.x,valueDisplayPos.y,VAL_DISPLAY_ICON_W,VAL_DISPLAY_ICON_H},VAL_DISPLAY_ICON_W/1000.0f,10,3.0f,COLOR_FOREGROUND);
            DrawTextEx(font1,"VPP",(Vector2){valueDisplayPos.x+valueFontSize/4,valueDisplayPos.y},valueFontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,dataVPP.c_str(),(Vector2){valueDisplayPos.x+VAL_DISPLAY_ICON_W-fontSize*dataVPP.length()/2,valueDisplayPos.y+VAL_DISPLAY_ICON_H-fontSize},fontSize,1,COLOR_FOREGROUND);

            valueDisplayPos.y += VAL_DISPLAY_ICON_H+SEPARATION_V;
            DrawRectangleRoundedLines((Rectangle){valueDisplayPos.x,valueDisplayPos.y,VAL_DISPLAY_ICON_W,VAL_DISPLAY_ICON_H},VAL_DISPLAY_ICON_W/1000.0f,10,3.0f,COLOR_FOREGROUND);
            DrawTextEx(font1,"Frecuencia",(Vector2){valueDisplayPos.x+valueFontSize/4,valueDisplayPos.y},valueFontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,dataFrec.c_str(),(Vector2){valueDisplayPos.x+VAL_DISPLAY_ICON_W-fontSize*dataFrec.length()/2,valueDisplayPos.y+VAL_DISPLAY_ICON_H-fontSize},fontSize,1,COLOR_FOREGROUND);

            valueDisplayPos.y += VAL_DISPLAY_ICON_H+SEPARATION_V;
            DrawRectangleRoundedLines((Rectangle){valueDisplayPos.x,valueDisplayPos.y,VAL_DISPLAY_ICON_W,VAL_DISPLAY_ICON_H},VAL_DISPLAY_ICON_W/1000.0f,10,3.0f,COLOR_FOREGROUND);
            DrawTextEx(font1,"Factor cresta",(Vector2){valueDisplayPos.x+valueFontSize/4,valueDisplayPos.y},valueFontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,dataMean.c_str(),(Vector2){valueDisplayPos.x+VAL_DISPLAY_ICON_W-fontSize*dataRMS.length()/2,valueDisplayPos.y+VAL_DISPLAY_ICON_H-fontSize},fontSize,1,COLOR_FOREGROUND);

            if(PORT_STATE!=1){
                DrawTextEx(font1,std::string("SERIAL PORT DISCONNECTED. PRESS \"X\" TO TRY "+std::string(SERIAL_PORT_PREFIX)+std::to_string(serialPortNumber+1)).c_str(),(Vector2){20,20},fontSize,1,COLOR_FOREGROUND);
            }
            else
            {
                command = "WAVEFORM";
                DrawTextEx(font1,command.c_str(),(Vector2){VISOR_MARGIN_H,VISOR_MARGIN_V-fontSize*1.5f},fontSize,1,COLOR_FOREGROUND);
            }

        EndDrawing();

        //----------------------------------------------------------------------------------
    }
    
    UnloadFont(font1);
    
    if(PORT_STATE==1) serial->writeString("[AE-M0-EA]");
    serial->flushReceiver();
    serial->closeDevice();
    RUNNING = false;
    t1.join();

    CloseWindow();

    return 0;
}

void DrawDottedLine(Vector2 startPos, Vector2 endPos, float thick, float divisions, Color color)
{
    int i;
    float lenght = Vector2Distance(endPos,startPos);
    float divLenght = lenght/divisions;
    Vector2 divNorm = Vector2Normalize(Vector2Subtract(endPos,startPos));
    for(i = 0; i < divisions; i++)
    {
        endPos = Vector2Add(startPos, Vector2Scale(divNorm,divLenght/2));
        DrawLineEx(startPos,endPos,thick,color);
        startPos = Vector2Add(startPos,Vector2Scale(divNorm,divLenght));
    }
}

void threadTest(std::string str)
{
    std::cout << str << std::endl;
}

void serialProcess(serialib *serial, std::list<int> *graphData)
{
    char data[BUFFER_SIZE];
    int dataValue = 0;
    
    while(RUNNING)
    {
        if(serial->available()){
            serial->readString(data,'\n',BUFFER_SIZE,100);
            
            size_t PSTART = std::string(data).find_first_of('[');
            size_t PSTOP = std::string(data).find_first_of(']',PSTART+1);
            if((PSTART != std::string::npos) && (PSTOP != std::string::npos))
            {
                std::string dataSection = std::string(data).substr(PSTART+4,PSTOP-PSTART-7);
                char unitIndicator = dataSection.at(0);
                if((unitIndicator == 'H') || (unitIndicator == 'K') || (unitIndicator == 'M'))
                {
                    dataSection = dataSection.substr(1,std::string::npos);
                    dataValue = atoi(dataSection.c_str());
                    FREC = dataSection;
                    UNIT = unitIndicator;
                    //std::cout << dataSection << "[" << UNIT << "]" << std::endl;
                }else{
                    dataValue = atoi(dataSection.c_str());
                    //dataValue = atoi(data);
                    //dataValue = 0;
                    //if(dataValue > 5.0f) dataValue = 5.0f;
                    graphData->pop_back();
                    graphData->push_front(dataValue);
                }
                
                //serial.flushReceiver();
            }//else dataValue = 0;
        }
    }
}