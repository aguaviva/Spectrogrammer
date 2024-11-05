#include "ScaleUI.h"
#include <stdio.h>


void draw_log_scale(ImRect frame_bb, ScaleBufferBase *pScaleBufferX)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    for (int e=1;e<5;e++) // number of zeroes
    {
        for (int i=0;i<10;i++)
        {
            float freq = (i + 1) * pow(10.0f, e);
            float t = pScaleBufferX->FreqToX(freq);
            float x = lerp(t, frame_bb.Min.x, frame_bb.Max.x);

            if (x < frame_bb.Min.x)
                continue;
            if (x > frame_bb.Max.x)
                break;

            if (i==0)
            {
                char str[255];
                sprintf(str, "%i", (int)freq); 
                float textWidth = ImGui::CalcTextSize(str).x;
                // append HZ to last
                if (e==4)
                {
                    strcat(str, " Hz");
                }
                window->DrawList->AddText(
                    ImVec2(x - textWidth/2, frame_bb.Min.y + 400), 
                    ImGui::GetColorU32(ImGuiCol_Text), 
                    str
                );
            }

            window->DrawList->AddLine(
                ImVec2(x, frame_bb.Min.y),
                ImVec2(x, frame_bb.Max.y),
                (i==0) ? ImGui::GetColorU32(ImGuiCol_Text) : ImGui::GetColorU32(ImGuiCol_TextDisabled));
        }

    }
}

void draw_lin_scale(ImRect frame_bb, ScaleBufferBase *pScaleBufferX)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    for (int i=0;i<3;i++)
    {
        for (int m=0;m<10;m++)
        {
            float freq = (m * 1000) + (i * 10000);
            float t = pScaleBufferX->FreqToX(freq);
            float x = lerp(t, frame_bb.Min.x, frame_bb.Max.x);

            if (x < frame_bb.Min.x)
                continue;
            if (x > frame_bb.Max.x)
                break;

            if (m == 0)
            {
                char str[255];
                sprintf(str, "%i", (int)freq); 
                float textWidth = ImGui::CalcTextSize(str).x;
                window->DrawList->AddText(
                    ImVec2(x - textWidth/2, frame_bb.Min.y + 400), 
                    ImGui::GetColorU32(ImGuiCol_Text), 
                    str
                );
            }

            window->DrawList->AddLine(
                ImVec2(x, frame_bb.Min.y),
                ImVec2(x, frame_bb.Max.y),
                (m==0) ? ImGui::GetColorU32(ImGuiCol_Text) : ImGui::GetColorU32(ImGuiCol_TextDisabled));
        }
    }
}


void draw_scale_y(ImRect frame_bb, float min, float max)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, true);
    const float ref = 32768;

    for (int dec=0;dec>-130;dec-=25)
    {
        float t = unlerp( min, max, dec);
        float y = lerp(t, frame_bb.Max.y, frame_bb.Min.y);

        if (y < frame_bb.Min.y)
            continue;
        if (y > frame_bb.Max.y)
            break;

        char str[255];
        sprintf(str, "%i", (int)dec); 
        window->DrawList->AddText(
            ImVec2(frame_bb.Min.x, y), 
            ImGui::GetColorU32(ImGuiCol_TextDisabled), 
            str
        );

        window->DrawList->AddLine(
            ImVec2(frame_bb.Min.x, y),
            ImVec2(frame_bb.Max.x, y),
            ImGui::GetColorU32(ImGuiCol_TextDisabled));
    }

    ImGui::PopClipRect();
}
