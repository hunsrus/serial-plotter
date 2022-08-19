#include "raylib.h"
#include "include/serialib.h"
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <list>

#if defined (_WIN32) || defined(_WIN64)
    //for serial ports above "COM9", we must use this extended syntax of "\\.\COMx".
    //also works for COM0 to COM9.
    //https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea?redirectedfrom=MSDN#communications-resources
    #define SERIAL_PORT "\\\\.\\COM1"
#endif
#if defined (__linux__) || defined(__APPLE__)
    #define SERIAL_PORT "/dev/ttyUSB0"
#endif

#define MCGREEN CLITERAL(Color){150,182,171,255}   // Verde Colin McRae

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    float V_SCALE = 1.0f;
    float H_SCALE = 1.0f;
    float LINE_THICKNESS = 2.0f;
    
    const int screenWidth = 1366;
    const int screenHeight = 768;

    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Serial Plotter");

    serialib serial;
    char data[50];
    int i;
    std::list<int> graphData;

    int EXCURSION_MAX = screenHeight/1.3;
    int VISOR_MARGIN_V = (screenHeight-EXCURSION_MAX)/2;
    int VISOR_MARGIN_H = (screenWidth-EXCURSION_MAX)/2;
    Rectangle visorRectangle = {VISOR_MARGIN_H,VISOR_MARGIN_V,EXCURSION_MAX,EXCURSION_MAX};
    Vector2 xOriginStart = {screenWidth/2,VISOR_MARGIN_V};
    Vector2 xOriginEnd = {screenWidth/2,screenHeight-VISOR_MARGIN_V};
    Vector2 yOriginStart = {VISOR_MARGIN_H,screenHeight/2};
    Vector2 yOriginEnd = {screenWidth-VISOR_MARGIN_H,screenHeight/2};

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
        serial.readString(data,'\n',10,200);
        graphData.pop_back();
        graphData.push_front(atoi(data));
        serial.flushReceiver();

        if(IsKeyPressed(KEY_R))
        {
            PORT_STATE = serial.openDevice(SERIAL_PORT, 9600);
            if (PORT_STATE!=1) std::cout << "Could not connect to " << SERIAL_PORT << std::endl;
            else std::cout << "Successful connection to " << SERIAL_PORT << std::endl;
        }
        if(IsKeyPressed(KEY_V))
        {
            if(IsKeyDown(KEY_LEFT_SHIFT)) V_SCALE -= 0.2f;
            else V_SCALE += 0.2f;
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
                int posx1 = screenWidth/2+(EXCURSION_MAX/2-i)*H_SCALE;
                int posx2 = screenWidth/2+(EXCURSION_MAX/2-i-1)*H_SCALE;
                int posy1 = screenHeight/2-(*it)*V_SCALE;
                int posy2 = screenHeight/2-(*std::next(it))*V_SCALE;
                if((posx1 < screenWidth-VISOR_MARGIN_H) && (posx2 > VISOR_MARGIN_H))
                    DrawLineEx((Vector2){posx1,posy1},(Vector2){posx2,posy2},LINE_THICKNESS,WHITE);
                i++;
            }
            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,0,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,MCGREEN);
            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,screenHeight-(screenHeight-EXCURSION_MAX)/2,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,MCGREEN);
            DrawRectangleLinesEx(visorRectangle,3.0f,WHITE);
            DrawLineEx(xOriginStart,xOriginEnd,0.4f,WHITE);
            DrawLineEx(yOriginStart,yOriginEnd,0.4f,WHITE);
            
            DrawText(std::string("L: LINE THICKNESS").c_str(),20,90+20*0,20,WHITE);
            DrawText(std::string("V: VERTICAL SCALING").c_str(),20,90+20*1,20,WHITE);
            DrawText(std::string("H: HORIZONTAL SCALING").c_str(),20,90+20*2,20,WHITE);

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