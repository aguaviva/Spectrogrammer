#include <string.h>
#include <time.h>
#include <stdio.h>
#include "ModalSampleRate.h"

bool ModalSampleRateAndFFT(float *sample_rate, uint32_t *fft_size)
{
    if (ImGui::Button("Audio"))
        ImGui::OpenPopup("Audio settings");

    bool res = false;
    if (ImGui::BeginPopupModal("Audio settings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Spacing();

        {
            const float items_vals[] = { 96000, 48000, 24000, 8000 };
            const char* items_str[] = { "96000", "48000", "24000", "8000" };

            const char* current_item = NULL;
            for (int n = 0; n < IM_ARRAYSIZE(items_str); n++)
            {
                if (*sample_rate == items_vals[n])
                {
                    current_item = items_str[n];
                    break;
                }
            }

            if (ImGui::BeginCombo("Sample rate", current_item)) // The second parameter is the label previewed before opening the combo.
            {
                for (int n = 0; n < IM_ARRAYSIZE(items_str); n++)
                {
                    bool is_selected = (*sample_rate == items_vals[n]); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(items_str[n], is_selected))
                    {
                        *sample_rate = items_vals[n];
                        res = true;
                    }
                        
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Spacing();
        {
            const float items_vals[] = { 8192, 4096, 2048, 1024, 512, 256, 128 };
            const char* items_str[] = { "8192", "4096", "2048", "1024", "512", "256", "128" };

            const char* current_item = NULL;
            for (int n = 0; n < IM_ARRAYSIZE(items_str); n++)
            {
                if (*fft_size == items_vals[n])
                {
                    current_item = items_str[n];
                    break;
                }
            }

            if (ImGui::BeginCombo("FFT size", current_item)) // The second parameter is the label previewed before opening the combo.
            {
                for (int n = 0; n < IM_ARRAYSIZE(items_str); n++)
                {
                    bool is_selected = (*fft_size == items_vals[n]); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(items_str[n], is_selected))
                    {
                        *fft_size = items_vals[n];
                        res = true;
                    }
                        
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Spacing();

        ImGui::Text("Freq resolution: %f", *sample_rate /(float)*fft_size );

        ImGui::Spacing();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();        
    }
    return res;
}