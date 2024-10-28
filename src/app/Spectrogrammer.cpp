#include <string.h>
#include "Processor.h"
#include "fft.h"
#include "pass_through.h"
#include "ScaleUI.h"
#include "ScaleBufferX.h"
#include "ScaleBufferY.h"
#include "ChunkerProcessor.h"
#include "BufferAverage.h"
#include "Spectrogrammer.h"
#include "colormaps.h"
#include "ModalSampleRate.h"
#include "ModalHoldPicker.h"
#include <GLES3/gl3.h>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_helpers.h"
#include "waterfall.h"
#ifdef ANDROID
#include <EGL/egl.h>
#include "../android_native_app_glue.h"
#include "audio/audio_main.h"
#include "backends/imgui_impl_android.h"
#else
#include <GLFW/glfw3.h>
#include "backends/imgui_impl_glfw.h"
#endif
uint32_t fft_size = 4*1024;
float sample_rate = 48000.0f / 2.0;
float max_freq = -1.0f; 
float min_freq = -1.0f; 
float bin_width = -1.0f;

// perf counters
int droppedBuffers=0;
int iterationsPerChunk = 0;
int processedChunks = 0;

// GUI state
int averaging = 1;
float fraction_overlap = 2.0f/3.0f; // we are using hanning
float decay = .5f;
float volume = 1;
bool logX=true, logY=true;
bool play = true;
bool hold = false;
float axis_y_min = -120;
float axis_y_max = -20;


enum HOLDING_STATE
{
    HOLDING_STATE_NO_HOLD=0,
    HOLDING_STATE_STARTED=1,
    HOLDING_STATE_READY=2
};

HOLDING_STATE holding_state = HOLDING_STATE_NO_HOLD;

Processor *pProcessor = NULL;
ScaleBufferBase *pScaleBufferX = nullptr;
ChunkerProcessor chunker;

BufferIODouble downsampled;
BufferIODouble scaledPowerY;
BufferIODouble scaledPowerXY;
BufferAverage bufferAverage;
BufferIODouble spectrumSamples;

BufferIODouble heldPower_lines;
BufferIODouble heldPower_bins;
BufferIODouble heldScaledPowerY;
BufferIODouble heldScaledPowerXY;

const char *pWorkingDirectory;

bool bScaleXChanged = false;
uint32_t last_width = -1;


void generate_spectrum_lines_from_bin_data(BufferIODouble *pBins, BufferIODouble *pLines)
{
    float *pDataIn = pBins->GetData();
    pLines->Resize(2 * (pBins->GetSize()-1));
    float *pDataOut = pLines->GetData();   
    int i=0;             
    for (int bin=1;bin<pBins->GetSize();bin++) //skip bin 0 is DC
    {
        pDataOut[2 * i + 0] = pScaleBufferX->FreqToX(pProcessor->bin2Freq(bin)); 
        pDataOut[2 * i + 1] = pDataIn[bin];
        i++;
    }
}

void FFT_Init(float sample_rate, int fft_size)
{
#ifdef ANDROID    
    int audio_buffer_length = 1024;
    Audio_createSLEngine(sample_rate, audio_buffer_length);
    Audio_createAudioRecorder();
    Audio_startPlay();                    

    AudioQueue* pFreeQueue = nullptr;
    AudioQueue* pRecQueue = nullptr;
    Audio_getBufferQueues(&pFreeQueue, &pRecQueue);
#else
    pRecQueue = new AudioQueue(32);
    pFreeQueue = new AudioQueue(32);

    buffers = allocateSampleBufs(20, 2*fft_size);
    for (int i=0;i<20;i++)
    {
        buffers[i].size_ = 2;
        pFreeQueue->push(&buffers[i]);
    }
#endif    

    pProcessor = new myFFT();
    pProcessor->init(fft_size, sample_rate);
    min_freq = pProcessor->bin2Freq(1);
    max_freq = pProcessor->bin2Freq(pProcessor->getBinCount()-1);

    chunker.SetQueues(pRecQueue, pFreeQueue);
    chunker.begin();

    bScaleXChanged = true;
}

void FFT_Shutdown()
{
    chunker.end();
    delete pProcessor;

#ifdef ANDROID    
    Audio_deleteAudioRecorder();
    Audio_deleteSLEngine();
#endif    
}

void Spectrogrammer_Init(void *window)
{
#ifdef ANDROID    
    android_app * pApp = (android_app *)window;
    pWorkingDirectory = pApp->activity->internalDataPath;
#endif

    FFT_Init(sample_rate, fft_size);

    bufferAverage.setAverageCount(averaging);

    last_width = -1;
}

void Spectrogrammer_Shutdown()
{
    FFT_Shutdown();

    Shutdown_waterfall();
}

void Spectrogrammer_MainLoopStep()
{
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f)); // place the next window in the top left corner (0,0)
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize); // make the next window fullscreen
    ImGui::Begin("imgui window", NULL, window_flags); // create a window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    ImGui::Checkbox("Play", &play);

    ImGui::SameLine();

    bool bScaleChanged = false;

    if (ImGui::Button("Hold"))
    {
        ImGui::OpenPopup("Hold Menu");
        
        RefreshFiles(pWorkingDirectory);
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(1000, -1));
    if (ImGui::BeginPopupModal("Hold Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        bool bHoldInProgress = (holding_state == HOLDING_STATE_STARTED);
        if (bHoldInProgress) 
            ImGui::BeginDisabled();

        if (HoldPicker(pWorkingDirectory, holding_state == HOLDING_STATE_READY, &heldPower_bins, &sample_rate, &fft_size))
        {
            FFT_Shutdown();
            FFT_Init(sample_rate, fft_size);
            bScaleChanged = true;
            holding_state = HOLDING_STATE_READY;
        }

        if (ImGui::Button("Holdy"))
        {
            SetColorMap(1);
            holding_state = HOLDING_STATE_STARTED;
        }   

        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            SetColorMap(0);
            holding_state = HOLDING_STATE_NO_HOLD;
        }   

        if (bHoldInProgress) 
            ImGui::EndDisabled();

        ImGui::Separator();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Menu"))
        ImGui::OpenPopup("Settings");

    if (ImGui::BeginPopupModal("Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        bScaleXChanged |= ImGui::Checkbox("Log x", &logX);
        bScaleChanged |= bScaleXChanged;
        ImGui::SameLine();
        bScaleChanged |= ImGui::Checkbox("Log y", &logY);

        bScaleChanged |= ImGui::SliderFloat("y max", &axis_y_max, -120.0f, 0.0f);
        bScaleChanged |= ImGui::SliderFloat("y min", &axis_y_min, -120.0f, 0.0f);

        //ImGui::SliderFloat("overlap", &fraction_overlap, 0.0f, 0.99f);
        ImGui::SliderFloat("decay", &decay, 0.0f, 0.99f);
        if (ImGui::SliderInt("averaging", &averaging, 1,500))
            bufferAverage.setAverageCount(averaging);
        
        ImGui::Spacing();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ModalSampleRateAndFFT(&sample_rate, &fft_size))
    {
        FFT_Shutdown();
        FFT_Init(sample_rate, fft_size);
    }

    // Draw FFT UI
    ImRect frame_fft_bb;
    bool draw_frame_fft_bb;
    {
        int frame_height = 400;
        bool hovered;
        draw_frame_fft_bb = block_add("fft", ImVec2(ImGui::GetContentRegionAvail().x, frame_height), &frame_fft_bb, &hovered);
        if (draw_frame_fft_bb)
        {
            const ImGuiStyle& style = GImGui->Style;
            ImGui::RenderFrame(frame_fft_bb.Min, frame_fft_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);        
            ImGui::InvisibleButton("no", ImVec2(1024, 15));    
        }
    }

    ImRect frame_wfall_bb;
    bool draw_frame_wfall_bb;
    if (true)
    {
        bool hovered;
        draw_frame_wfall_bb = block_add("wfall", ImGui::GetContentRegionAvail(), &frame_wfall_bb, &hovered);
        if (draw_frame_wfall_bb)
        {
            if (last_width != frame_wfall_bb.GetWidth())
            {
                last_width = frame_wfall_bb.GetWidth();
                Shutdown_waterfall();
                Init_waterfall(frame_wfall_bb.GetWidth(), frame_wfall_bb.GetHeight());
                bScaleXChanged = true;
            }
        }
    }

    // now we know the size of the waterfall and the plot
    if (bScaleXChanged || pScaleBufferX==NULL)
    {
        delete pScaleBufferX;

        if (logX)
            pScaleBufferX = new ScaleBufferXLog();
        else
            pScaleBufferX = new ScaleBufferXLin();

        pScaleBufferX->setOutputWidth(frame_wfall_bb.GetWidth(), min_freq, max_freq);
        pScaleBufferX->PreBuild(pProcessor);        

        bScaleXChanged = false;
    }


    //if we have enough audio queued process the fft, update waterfall
    BufferIODouble *pPower = NULL;
    while (chunker.Process(pProcessor, decay, fraction_overlap))
    {
        if (play)
        {
            pPower = pProcessor->getBufferIO();  

            pPower = bufferAverage.Do(pPower);
            if (pPower!=NULL)
            {           
                // values between 0 and 1
                applyScaleY(pPower, axis_y_min, axis_y_max, logY, &scaledPowerY);

                pScaleBufferX->Build(&scaledPowerY, &scaledPowerXY);

                if (holding_state == HOLDING_STATE_READY)
                {
                    scaledPowerXY.sub(&heldScaledPowerXY);
                    scaledPowerXY.add(0.5);
                }

                Draw_update(
                    scaledPowerXY.GetData()+1, 
                    scaledPowerXY.GetSize()-1
                );

                processedChunks++;
            }
        }
    }

    if ((pPower!=NULL) && (holding_state == HOLDING_STATE_STARTED))
    {
        heldPower_bins.copy(pPower); //need original
        heldScaledPowerXY.copy(&scaledPowerXY);                    

        holding_state = HOLDING_STATE_READY;
        bScaleChanged = true;
    } 

    // rescale held data if needed        

    if ((holding_state == HOLDING_STATE_READY) && bScaleChanged)
    {
        // rescale Y axis to held line is correct
        applyScaleY(&heldPower_bins, axis_y_min, axis_y_max, logY, &heldScaledPowerY);            
        generate_spectrum_lines_from_bin_data(&heldScaledPowerY, &heldPower_lines);

        // rescale X axis to waterfall is correct
        pScaleBufferX->Build(&heldScaledPowerY, &heldScaledPowerXY);
    }

    // draw spectrogram

    if (draw_frame_fft_bb)
    {
        if (play)
        {
            generate_spectrum_lines_from_bin_data(&scaledPowerY, &spectrumSamples);
        }

        // draw spectrogram
        ImU32 col = IM_COL32(200, 200, 200, 200);
        draw_lines(frame_fft_bb, spectrumSamples.GetData(), spectrumSamples.GetSize()/2, col, 0, 1);

        // draw baseline
        if (holding_state == HOLDING_STATE_READY)
        {
            ImU32 col = IM_COL32(200, 0, 0, 200);
            draw_lines(frame_fft_bb, heldPower_lines.GetData(), heldPower_lines.GetSize()/2, col, 0, 1);
        }
        
        if (logX)
            draw_log_scale(frame_fft_bb, pScaleBufferX);   
        else
            draw_lin_scale(frame_fft_bb, pScaleBufferX);   
    }

    // Draw waterfall

    if (draw_frame_wfall_bb)
    {
        float seconds_per_row = (((float)fft_size/sample_rate) * (float)averaging) * (1.0 - fraction_overlap);

        Draw_waterfall(frame_wfall_bb);
        Draw_vertical_scale(frame_wfall_bb, seconds_per_row);
        //draw_log_scale(frame_bb);
    }

    ImGui::PopStyleVar(); // window padding

    ImGui::End();
}
