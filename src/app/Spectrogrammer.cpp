#include "Processor.h"
#include "fft.h"
#include "pass_through.h"
#include "ChunkerProcessor.h"
#include "ScaleBufferX.h"
#include "ScaleBufferY.h"
#include "BufferAverage.h"
#include "Spectrogrammer.h"
#include "colormaps.h"
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
#include "../android_native_app_glue.h"
#include "audio/audio_main.h"
#include "backends/imgui_impl_android.h"
#else
#include <GLFW/glfw3.h>
#include "backends/imgui_impl_glfw.h"
#endif
int fft_size = 4*1024;
float sample_rate = 48000.0f;
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
bool hold_spectrum = false;
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

BufferIODouble scaledPowerY;
BufferIODouble scaledPowerXY;
BufferAverage bufferAverage;
BufferIODouble spectrumSamples;

BufferIODouble heldPower_lines;
BufferIODouble heldPower_bins;
BufferIODouble heldScaledPowerY;
BufferIODouble heldScaledPowerXY;

sample_buf * buffers;

#ifdef ANDROID
static int32_t handleInputEvent(struct android_app* app, AInputEvent* inputEvent)
{   
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
}
#endif

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

void Spectrogrammer_Init(void *window)
{
#ifdef ANDROID    
    android_app * g_App = (android_app *)window;
    g_App->onInputEvent = handleInputEvent;
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    /*
    // Redirect loading/saving of .ini file to our location.
    // Make sure 'g_IniFilename' persists while we use Dear ImGui.
    ImGuiIO& io = ImGui::GetIO();
    g_IniFilename = std::string(app->activity->internalDataPath) + "/imgui.ini";
    io.IniFilename = g_IniFilename.c_str();;
    */
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
#ifdef ANDROID    
    ImGui_ImplAndroid_Init(g_App->window);
#else
    ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, true);
#endif    
    ImGui_ImplOpenGL3_Init("#version 300 es");

#ifdef ANDROID    
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 22.0f * 2;
    io.Fonts->AddFontDefault(&font_cfg);

    // Arbitrary scale-up
    // FIXME: Put some effort into DPI awareness
    ImGui::GetStyle().ScaleAllSizes(3.0f * 2);

    int audio_buffer_length = 1024;
    Audio_createSLEngine(sample_rate, audio_buffer_length);
    Audio_createAudioRecorder();
    Audio_startPlay();
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

    float sampleRate;
    AudioQueue* pFreeQueue = nullptr;
    AudioQueue* pRecQueue = nullptr;
    GetBufferQueues(&sampleRate, &pFreeQueue, &pRecQueue);

    chunker.SetQueues(pRecQueue, pFreeQueue);
    chunker.begin();

    pProcessor = new myFFT();
    pProcessor->init(fft_size, sample_rate);

    min_freq = pProcessor->bin2Freq(1);
    max_freq = pProcessor->bin2Freq(pProcessor->getBinCount()-1);

    bufferAverage.setAverageCount(averaging);
}

void Spectrogrammer_Shutdown()
{
    chunker.end();

#ifdef ANDROID    
    Audio_deleteAudioRecorder();
    Audio_deleteSLEngine();
#endif    
    Shutdown_waterfall();
}

void draw_log_scale(ImRect frame_bb)
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

void draw_lin_scale(ImRect frame_bb)
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
                    ImVec2(x - textWidth/2, frame_bb.Min.y + 150), 
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

void generate_debug_signal(sample_buf *pBuf)
{
     int t = 0;
    int bufSize = AU_LEN(pBuf->cap_);
    int16_t *buff = (int16_t *)(pBuf->buf_);        
    for (int n = 0; n < bufSize; n++)
    {                    
        //buff[n] = n%5;
        buff[n] = 512.0f + 512.0f*sin(2.0f*M_PI*1000.0f*((float)t/(float)sample_rate));
        t++;
        if (t>sample_rate)
            t = 0;
        //buff[n] = n / (float(bufSize)-1);
    }
}

void Spectrogrammer_MainLoopStep()
{
    ImGuiIO& io = ImGui::GetIO();
    //if (g_EglDisplay == EGL_NO_DISPLAY)
    //    return;

    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
#ifdef ANDROID    
    ImGui_ImplAndroid_NewFrame();
#else
    ImGui_ImplGlfw_NewFrame();
#endif    
    ImGui::NewFrame();

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f)); // place the next window in the top left corner (0,0)
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize); // make the next window fullscreen
    ImGui::Begin("imgui window", NULL, window_flags); // create a window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    ImGui::Checkbox("Play", &play);
    ImGui::SameLine();
    
    if (holding_state == HOLDING_STATE_STARTED) 
        ImGui::BeginDisabled();

    bool bHold_pressed = ImGui::Checkbox("Hold", &hold_spectrum);

    if (holding_state == HOLDING_STATE_STARTED) 
        ImGui::EndDisabled();

    if (bHold_pressed)
    {
        if (hold_spectrum)
            holding_state = HOLDING_STATE_STARTED;
        else
            holding_state = HOLDING_STATE_NO_HOLD;
    }   

    SetColorMap(hold_spectrum?1:0);

    ImGui::SameLine();
    if (ImGui::Button("Menu"))
        ImGui::OpenPopup("Settings");

    bool bScaleXChanged = false;
    bool bScaleChanged = false;

    float sampleRate;
    AudioQueue* pFreeQueue = nullptr;
    AudioQueue* pRecQueue = nullptr;
    GetBufferQueues(&sampleRate, &pFreeQueue, &pRecQueue);
    ImGui::Text("r: %3i, f: %3i", pRecQueue->size(), pFreeQueue->size());   

    bool open = true;
    if (ImGui::BeginPopupModal("Settings", &open, ImGuiWindowFlags_AlwaysAutoResize))
    {
        bScaleXChanged = ImGui::Checkbox("Log x", &logX);
        bScaleChanged |= bScaleXChanged;
        ImGui::SameLine();
        bScaleChanged |= ImGui::Checkbox("Log y", &logY);

        bScaleChanged |= ImGui::SliderFloat("y max", &axis_y_max, -120.0f, 0.0f);
        bScaleChanged |= ImGui::SliderFloat("y min", &axis_y_min, -120.0f, 0.0f);

        //ImGui::SliderFloat("overlap", &fraction_overlap, 0.0f, 0.99f);
        ImGui::SliderFloat("decay", &decay, 0.0f, 0.99f);
        if (ImGui::SliderInt("averaging", &averaging, 1,500))
            bufferAverage.setAverageCount(averaging);

        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (bScaleXChanged || pScaleBufferX==NULL)
    {
        delete pScaleBufferX;

        if (logX)
            pScaleBufferX = new ScaleBufferXLog();
        else
            pScaleBufferX = new ScaleBufferXLin();
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
            static uint32_t last_width = -1;

            if (bScaleXChanged || last_width != frame_wfall_bb.GetWidth())
            {
                pScaleBufferX->setOutputWidth(frame_wfall_bb.GetWidth(), min_freq, max_freq);
                pScaleBufferX->PreBuild(pProcessor);        
            }

            if (last_width != frame_wfall_bb.GetWidth())
            {
                last_width = frame_wfall_bb.GetWidth();
                Shutdown_waterfall();
                Init_waterfall(frame_wfall_bb.GetWidth(), frame_wfall_bb.GetHeight());
            }
        }
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
                //process(pPower);
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


    // now we know the size of the UI

    // rescale held  data if needed        
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
            draw_log_scale(frame_fft_bb);   
        else
            draw_lin_scale(frame_fft_bb);   
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


    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
