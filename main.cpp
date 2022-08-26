#include <raylib.h>
#include <raymath.h>
#include "include/serialib.h"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <list>
#include <iostream>
#include <chrono>
#include <math.h>

#if defined (_WIN32) || defined(_WIN64)
    //for serial ports above "COM9", we must use this extended syntax of "\\.\COMx".
    //also works for COM0 to COM9.
    //https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea?redirectedfrom=MSDN#communications-resources
    #define SERIAL_PORT "\\\\.\\COM1"
#endif
#if defined (__linux__) || defined(__APPLE__)
    #define SERIAL_PORT "/dev/ttyUSB0"
    //#define SERIAL_PORT "/dev/ttyACM0"
#endif

#define MCGREEN CLITERAL(Color){150,182,171,255}   // Verde Colin McRae

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1366;
    const int screenHeight = 768;

    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Serial Plotter");

    serialib serial;
    char data[50];
    int i;
    std::list<int> graphData;
    std::chrono::milliseconds timerMs;

    int EXCURSION_MAX = screenHeight/1.3f;
    int VISOR_MARGIN_V = (screenHeight-EXCURSION_MAX)/2;
    int VISOR_MARGIN_H = (screenWidth-EXCURSION_MAX)/2;
    Rectangle visorRectangle = {VISOR_MARGIN_H,VISOR_MARGIN_V,EXCURSION_MAX,EXCURSION_MAX};
    Vector2 xOriginStart = {screenWidth/2,VISOR_MARGIN_V};
    Vector2 xOriginEnd = {screenWidth/2,screenHeight-VISOR_MARGIN_V};
    Vector2 yOriginStart = {VISOR_MARGIN_H,screenHeight/2};
    Vector2 yOriginEnd = {screenWidth-VISOR_MARGIN_H,screenHeight/2};
    Vector2 xLineStart = xOriginStart;
    Vector2 xLineEnd = xOriginEnd;
    Vector2 yLineStart = yOriginStart;
    Vector2 yLineEnd = yOriginEnd;

    float V_SCALE = 1.0f;
    float H_SCALE = 1.0f;
    float LINE_THICKNESS = 2.0f;
    int SUBDIVISIONS = 10;

    for(i = 0; i < EXCURSION_MAX; i++)  graphData.push_back(0);

    // Connection to serial port
    char PORT_STATE = serial.openDevice(SERIAL_PORT, 9600);
    // If connection fails, return the error code otherwise, display a success message
    if (PORT_STATE!=1) std::cout << "Could not connect to " << SERIAL_PORT << std::endl;
    else std::cout << "Successful connection to " << SERIAL_PORT << std::endl;

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------
    while (!WindowShouldClose())
    {
        if(IsKeyDown(KEY_T))
        {
            timerMs = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
            );
            graphData.pop_back();
            graphData.push_front((int)(sin((double)timerMs.count()/100)*512));
        }else{
            if(PORT_STATE==1) serial.readString(data,'\n',10,200);
            else data[0] = '\n';
            graphData.pop_back();
            graphData.push_front(atoi(data));
            serial.flushReceiver();
        }

        if(IsKeyPressed(KEY_R))
        {
            PORT_STATE = serial.openDevice(SERIAL_PORT, 9600);
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
        //----------------------------------------------------------------------------------
        // Dibuja
        //----------------------------------------------------------------------------------

        BeginDrawing();

            ClearBackground(MCGREEN);
            i = 0;
            for (std::list<int>::iterator it = graphData.begin(); it != std::prev(graphData.end()); it++)
            {
                //int posx1 = screenWidth/2+(EXCURSION_MAX/2-i)*H_SCALE;
                //int posx2 = screenWidth/2+(EXCURSION_MAX/2-i-1)*H_SCALE;
                int posx1 = screenWidth/2+EXCURSION_MAX/2-i*H_SCALE;
                int posx2 = screenWidth/2+EXCURSION_MAX/2-(i+1)*H_SCALE;
                int posy1 = screenHeight/2-(*it)*V_SCALE;
                int posy2 = screenHeight/2-(*std::next(it))*V_SCALE;
                if((posx1 < screenWidth-VISOR_MARGIN_H) && (posx2 > VISOR_MARGIN_H))
                    DrawLineEx((Vector2){posx1,posy1},(Vector2){posx2,posy2},LINE_THICKNESS,WHITE);
                i++;
            }
            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,0,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,MCGREEN);
            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,screenHeight-(screenHeight-EXCURSION_MAX)/2,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,MCGREEN);
            DrawRectangleLinesEx(visorRectangle,3.0f,WHITE);
            for(i = -SUBDIVISIONS/2+1; i < SUBDIVISIONS/2; i++)
            {
                xLineStart.x = xOriginStart.x+EXCURSION_MAX/SUBDIVISIONS*i;
                xLineEnd.x = xOriginEnd.x+EXCURSION_MAX/SUBDIVISIONS*i;
                DrawLineEx(xLineStart,xLineEnd,0.25f,WHITE);
                yLineStart.y = yOriginStart.y+EXCURSION_MAX/SUBDIVISIONS*i;
                yLineEnd.y = yOriginEnd.y+EXCURSION_MAX/SUBDIVISIONS*i;
                DrawLineEx(yLineStart,yLineEnd,0.25f,WHITE);
            }
            
            DrawText(std::string("L: LINE THICKNESS").c_str(),20,90+20*0,20,WHITE);
            DrawText(std::string("V: VERTICAL SCALING").c_str(),20,90+20*1,20,WHITE);
            DrawText(std::string("H: HORIZONTAL SCALING").c_str(),20,90+20*2,20,WHITE);
            DrawText(std::string("S: SUBDIVISIONS AMOUNT").c_str(),20,90+20*3,20,WHITE);

            float valueFontSize = 10.0f;
            for(i = -(SUBDIVISIONS/2); i <= (SUBDIVISIONS/2); i++)
            {
                float num = ((i*(EXCURSION_MAX/SUBDIVISIONS)/V_SCALE)*5/1024);
                DrawText(std::string(std::to_string(num)).c_str(),screenWidth-VISOR_MARGIN_H+valueFontSize/2,screenHeight/2-i*(EXCURSION_MAX/SUBDIVISIONS)-valueFontSize/2,valueFontSize,WHITE);
            }

            if(PORT_STATE!=1) DrawText(std::string("SERIAL PORT DISCONNECTED. PRESS \"R\" TO RECONNECT").c_str(),20,20,20,WHITE);

        EndDrawing();

        //----------------------------------------------------------------------------------
    }


    CloseWindow();
    //--------------------------------------------------------------------------------------

    // Close the serial device
    serial.closeDevice();

    return 0;
}