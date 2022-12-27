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
#define RESOLUTION 1024

struct Theme
{
    Color background = MCGREEN;
    Color foreground = WHITE;
};

void DrawDottedLine(Vector2 startPos, Vector2 endPos, float thick, float divisions, Color color);

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
    int i, lastMax = 0, dataValue = 0;
    std::list<int> graphData;
    std::chrono::milliseconds timerMs;
    Font font1 = LoadFont("src/fonts/JetBrainsMono/JetBrainsMono-Bold.ttf");
    float fontSize = font1.baseSize;
    double tiempo = 0;
    double tiempoAnterior = 0;
    double tiempoAcumulado = 0;
    int caida = 0;
    int cantPicos = 0;

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

    float V_SCALE = 0.4f;
    float H_SCALE = 4.0f;
    float OFFSET = 0.0f;
    float LINE_THICKNESS = 2.0f;
    int SUBDIVISIONS = 10;
    float THRESHOLD = 0.0f;

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
        if(IsKeyDown(KEY_P))
        {
            timerMs = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
            );
            graphData.pop_back();
            int test_data = (int)(sin((double)timerMs.count()/100)*512);
            graphData.push_front(test_data);
        }else{
            if(PORT_STATE==1) serial.readString(data,'\n',10,200);
            else data[0] = '\n';
            dataValue = atoi(data);
            graphData.pop_back();
            graphData.push_front(dataValue);

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
        if(IsKeyPressed(KEY_R))
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
            if (CURRENT_THEME < THEMES_COUNT-1) CURRENT_THEME++;
            else CURRENT_THEME = 0;
            
            COLOR_BACKGROUND = themes[CURRENT_THEME].background;
            COLOR_FOREGROUND = themes[CURRENT_THEME].foreground;
        }
        //----------------------------------------------------------------------------------
        // Dibuja
        //----------------------------------------------------------------------------------

        BeginDrawing();

            ClearBackground(COLOR_BACKGROUND);

            caida = 0;
            cantPicos = 0;
            Color colorPico = COLOR_FOREGROUND;
            lastMax = 0;
            i = 0;
            bool pico = false;
            /*auto tiempo = std::chrono::steady_clock::now();
            auto tiempoAnterior = tiempo;*/
            tiempo = 0;
            tiempoAnterior = 0;
            tiempoAcumulado = 0;
            for (std::list<int>::iterator it = graphData.begin(); it != std::prev(graphData.end()); it++)
            {
                int posx1 = screenWidth/2+EXCURSION_MAX/2-i*H_SCALE;
                int posx2 = screenWidth/2+EXCURSION_MAX/2-(i+1)*H_SCALE;
                int posy1 = screenHeight/2-(*it)*V_SCALE+OFFSET;
                int posy2 = screenHeight/2-(*std::next(it))*V_SCALE+OFFSET;

                if(posy2 >= posy1)
                {
                    colorPico = COLOR_FOREGROUND;
                    if(posy2 > lastMax)
                    {
                        caida = 0;
                        lastMax = posy2;                        
                    }
                    pico = true;
                }
                else if((posy2 < lastMax) && pico)
                {
                    caida++;
                    //colorPico = RED;      //Pinta los valores detectados de rojo
                    if(caida > 5)
                    {
                        cantPicos++;
                        caida = 0;
                        colorPico = COLOR_FOREGROUND;
                        pico = false;

                        /*tiempo = std::chrono::steady_clock::now();
                        std::chrono::duration<double> elapsed_seconds = tiempo-tiempoAnterior;
                        tiempoAcumulado = (double)elapsed_seconds.count()*100;
                        tiempoAnterior = tiempo;*/
                        tiempo = i;
                        tiempoAcumulado += (tiempo-tiempoAnterior)*0.01*60; //en segundos
                        //tiempoAcumulado = tiempoAcumulado*0.01*60;
                        tiempoAnterior = tiempo;
                    }
                }

                if((posx1 < screenWidth-VISOR_MARGIN_H) && (posx2 > VISOR_MARGIN_H))
                    DrawLineEx((Vector2){posx1,posy1},(Vector2){posx2,posy2},LINE_THICKNESS,colorPico);
                i++;
            }

            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,0,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,COLOR_BACKGROUND);
            DrawRectangle(screenWidth/2-EXCURSION_MAX/2,screenHeight-(screenHeight-EXCURSION_MAX)/2,EXCURSION_MAX,(screenHeight-EXCURSION_MAX)/2,COLOR_BACKGROUND);
            DrawRectangleLinesEx(visorRectangle,3.0f,COLOR_FOREGROUND);
            for(i = -SUBDIVISIONS/2+1; i < SUBDIVISIONS/2; i++)
            {
                xLineStart.x = xOriginStart.x+EXCURSION_MAX/SUBDIVISIONS*i;
                xLineEnd.x = xOriginEnd.x+EXCURSION_MAX/SUBDIVISIONS*i;
                DrawLineEx(xLineStart,xLineEnd,0.25f,COLOR_FOREGROUND);
                yLineStart.y = yOriginStart.y+EXCURSION_MAX/SUBDIVISIONS*i;
                yLineEnd.y = yOriginEnd.y+EXCURSION_MAX/SUBDIVISIONS*i;
                DrawLineEx(yLineStart,yLineEnd,0.25f,COLOR_FOREGROUND);
            }

            float valueFontSize = fontSize/1.5;
            for(i = -(SUBDIVISIONS/2); i <= (SUBDIVISIONS/2); i++)
            {
                float num = ((i*(EXCURSION_MAX/SUBDIVISIONS)/V_SCALE)*5/RESOLUTION)+OFFSET/V_SCALE*5/RESOLUTION;
                DrawTextEx(font1,std::string(std::to_string(num)).c_str(),(Vector2){screenWidth-VISOR_MARGIN_H+valueFontSize/2,screenHeight/2-i*(EXCURSION_MAX/SUBDIVISIONS)-valueFontSize/2},valueFontSize,1,COLOR_FOREGROUND);
                {
                    yLineStart.y = yOriginStart.y+OFFSET;
                    yLineEnd.y = yOriginEnd.y+OFFSET;
                    DrawDottedLine(yLineStart, yLineEnd, 2.0f, 30, COLOR_FOREGROUND);
                }
            }

            yLineStart.y = yOriginStart.y-THRESHOLD*V_SCALE+OFFSET;
            yLineEnd.y = yOriginEnd.y-THRESHOLD*V_SCALE+OFFSET;
            DrawDottedLine(yLineStart, yLineEnd, 0.5f, 50, COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("T").c_str(),(Vector2){VISOR_MARGIN_H-valueFontSize,screenHeight/2-THRESHOLD*V_SCALE+OFFSET-valueFontSize/2},valueFontSize,1,COLOR_FOREGROUND);

            DrawTextEx(font1,std::string("L: LINE THICKNESS").c_str(),(Vector2){20,90+fontSize*0},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("V: VERTICAL SCALING").c_str(),(Vector2){20,90+fontSize*1},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("H: HORIZONTAL SCALING").c_str(),(Vector2){20,90+fontSize*2},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("S: SUBDIVISIONS AMOUNT").c_str(),(Vector2){20,90+fontSize*3},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("O: VERTICAL OFFSET").c_str(),(Vector2){20,90+fontSize*4},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("T: VERTICAL THRESHOLD").c_str(),(Vector2){20,90+fontSize*5},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("R: RESET PARAMETERS").c_str(),(Vector2){20,90+fontSize*6},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("C: CHANGE THEME").c_str(),(Vector2){20,90+fontSize*7},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("SHIFT: REVERSE COMMAND").c_str(),(Vector2){20,90+fontSize*8},fontSize,1,COLOR_FOREGROUND);
            DrawTextEx(font1,std::string("CTRL: FINE TUNING").c_str(),(Vector2){20,90+fontSize*9},fontSize,1,COLOR_FOREGROUND);
            //DrawTextEx(font1,std::to_string(cantPicos).c_str(),(Vector2){20,90+fontSize*7},fontSize,1,COLOR_FOREGROUND);
            int bpm = cantPicos*(60/(EXCURSION_MAX*0.02));
            //int bpm = tiempoAcumulado/cantPicos;
            //DrawTextEx(font1,std::string("PULSO: "+std::to_string((int)(bpm))+" [BPM]").c_str(),(Vector2){20,90+fontSize*20},fontSize,1,COLOR_FOREGROUND);

            if(PORT_STATE!=1) DrawTextEx(font1,std::string("SERIAL PORT DISCONNECTED. PRESS \"R\" TO RECONNECT").c_str(),(Vector2){20,20},fontSize,1,COLOR_FOREGROUND);

        EndDrawing();

        //----------------------------------------------------------------------------------
    }
    
    UnloadFont(font1);
    serial.closeDevice();

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