#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"
//#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"
//#include "android_native_app_glue.h"
//#include <android/asset_manager.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <stdlib.h>
#include <math.h>
#include "waterfall.h"
#include "colormaps.h"

// some helper functions

void calc_scale(float seconds_per_row, float desired_distance, float *notch_distance, float *scale)
{
    *notch_distance = desired_distance;
    *scale = 1;

    float min_err = 10000;
    float scale_list[] = {1.0f/20.0f, 1.0f/10.0f, 1.0f/2.0f, 1.0f, 3.0f, 4.0f, 10.0f, 30.0f, 60.0f, 120.0f, 300.0f, 600.0f, 1200.0f, 3600.0f};
    for (int i=0;i<14;i++)
    {        
        float nd = scale_list[i]/seconds_per_row;
        float err = fabs(nd - desired_distance);
        if (err<min_err)
            min_err = err;
        if (err>min_err)
            break;
        *notch_distance = nd;
        *scale = scale_list[i];
    }
}

float get_time_magnitude(float total_time)
{
    // find time magnitude
    int time_magnitude = -1;
    if (total_time>60*60)
        time_magnitude=3;
    else if (total_time>60)
        time_magnitude=2;
    else if (total_time>20)
        time_magnitude=1;
    else 
        time_magnitude=0;

    return time_magnitude;
}


static int texture_width = -1;
static int texture_height = -1;
static GLuint image_texture = 0xffffffff;
static int offset = 0;

void Init_waterfall(int16_t width, int16_t height)
{
    texture_width = width;
    texture_height = height;

    // Create a OpenGL texture identifier
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    uint16_t* image_data = (uint16_t*)malloc(texture_width * texture_height * 2);
    if (image_data!=NULL)
    {
        
        for (int y=0;y<texture_height;y++)
        {
            for (int x=0;x<texture_width;x++)
            {
                image_data[x+y*texture_width] = 0;
            }
        }

        // Upload pixels into texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, image_data);   
        glFlush();
        free(image_data);
    }
}

void update_texture_row(uint16_t *image_data)
{
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexSubImage2D(GL_TEXTURE_2D,
        0,
        0,
        offset,
        texture_width,
        1, //height
        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, image_data);    
    glBindTexture(GL_TEXTURE_2D, 0);

    offset--;
    if (offset<0)
        offset = texture_height-1;
}

void Draw_update(float *pData, uint32_t size)
{
    if (image_texture==0xffffffff)
        return;

    uint16_t image_data[4*1024];
    for(int i=0;i<texture_width;i++)
    {
        image_data[i] = GetColorMap(pData[i] *255);
    }

    update_texture_row(image_data);
}

void Draw_waterfall(ImRect frame_bb)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    if (image_texture==0xffffffff)
        return;

    float foffset = offset/(float)(texture_height-1);

    window->DrawList->AddImage((ImTextureID)image_texture, frame_bb.Min, ImVec2(frame_bb.Max.x, frame_bb.Max.y - offset), ImVec2(0,foffset), ImVec2(1,1), IM_COL32_WHITE);
    window->DrawList->AddImage((ImTextureID)image_texture, ImVec2(frame_bb.Min.x, frame_bb.Max.y - offset), frame_bb.Max, ImVec2(0,0), ImVec2(1,foffset), IM_COL32_WHITE);
}

void Draw_hover_data(ImRect frame_bb, bool hovered)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImU32 col = IM_COL32(200, 200, 200, 200);

    // Tooltip on hover
    if (hovered && frame_bb.Contains(g.IO.MousePos))
    {
        window->DrawList->AddLine(
            ImVec2(g.IO.MousePos.x, frame_bb.Min.y),
            ImVec2(g.IO.MousePos.x, frame_bb.Max.y),
            col);

        char str[255];
        sprintf(str, "%f Hz", g.IO.MousePos.x - frame_bb.Min.x); 
        window->DrawList->AddText(ImVec2(g.IO.MousePos.x, frame_bb.Min.y), col, str);
    }
}

void Draw_vertical_scale(ImRect frame_bb, float seconds_per_row)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    //window.pos 


    ImU32 col = IM_COL32(200, 200, 200, 200);

    // calculate scale
    float scale;
    float notch_distance;
    float desired_distance = 25;
    calc_scale(seconds_per_row, desired_distance, &notch_distance, &scale);
    
    // find time magnitude
    float total_time = seconds_per_row * texture_height;
    int time_magnitude = get_time_magnitude(total_time);

    // draw time scale
    for(int i=0;;i++)
    {
        float y = floor((float)i * notch_distance);
        if (y>=texture_height)
            break;

        bool long_notch = (i % 5)==0;

        window->DrawList->AddLine(
            ImVec2(frame_bb.Min.x, frame_bb.Min.y + y),
            ImVec2(frame_bb.Min.x + (long_notch?50:25), frame_bb.Min.y + y),
            col);
        
        if (long_notch)
        {
            char str[255];
            int t = floor(i*scale);
            switch(time_magnitude)
            {
                case 0: sprintf(str, "%0.2fs", i*scale); break;
                case 1: sprintf(str, "%02is", t % 60); break;
                case 2: sprintf(str, "%im%02is", t/60, t % 60); break;
                case 3: sprintf(str, "%ih%02im", t/(60*60), (t%(60*60)) / 60); break;
            }

            window->DrawList->AddText(ImVec2(frame_bb.Min.x + 50, frame_bb.Min.y + y), col, str);
        }

    }    
}

void Shutdown_waterfall()
{
    glFlush();
    if (image_texture!=0xffffffff)
    {
        glDeleteTextures(1, &image_texture);
        image_texture = 0xffffffff;
        texture_width = -1;
        texture_height = -1;
        offset = 0;
    }
}
