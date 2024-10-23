#include "Processor.h"
#include "fft.h"
#include "pass_through.h"
#include "ChunkerProcessor.h"
#include "ScaleBufferX.h"
#include "ScaleBufferY.h"
#include "BufferAverage.h"
#include "Spectrogrammer.h"
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

Processor *pProcessor = NULL;
ScaleBufferBase *pScaleBufferX = nullptr;
ScaleBufferY  scaleBufferY;
BufferAverage bufferAverage;
ChunkerProcessor chunker;

BufferIODouble audioSamples;
BufferIODouble spectrumSamples;
BufferIODouble spectrumSamplesHold;

sample_buf * buffers;
AudioQueue* pFreeQueue = nullptr;
AudioQueue* pRecQueue = nullptr;

#ifdef ANDROID
static int32_t handleInputEvent(struct android_app* app, AInputEvent* inputEvent)
{   
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
}
#endif

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
    
    pProcessor = new myFFT();
    //pProcessor = new PassThrough();
    pProcessor->init(fft_size, sample_rate);

    min_freq = pProcessor->bin2Freq(1);
    max_freq = pProcessor->bin2Freq(pProcessor->getBinCount()-1);

    bufferAverage.setAverageCount(averaging);
}

void Spectrogrammer_Shutdown()
{
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
                    ImVec2(x - textWidth/2, frame_bb.Min.y + 150), 
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

BufferIODouble *Spectrogrammer_ProcessAudio(bool play, Processor *pProcessor, ScaleBufferBase *pScaleBufferX)
{
#ifdef ANDROID
    AudioQueue* pFreeQueue = nullptr;
    AudioQueue* pRecQueue = nullptr;
    float sampleRate;
    GetBufferQueues(&sampleRate, &pFreeQueue, &pRecQueue);
#else
    sample_buf *buf = nullptr;
    while (pFreeQueue->front(&buf))
    {
        pFreeQueue->pop();
        pRecQueue->push(buf);
    }
#endif
    
    // pass available buffers to processor
    if (pRecQueue!=NULL )
    {
        sample_buf *buf = nullptr;
        while (pRecQueue->front(&buf))
        {
            pRecQueue->pop();

            //generate_debug_signal(buf);

            if (chunker.pushAudioChunk(buf) == false)
            {
                droppedBuffers++;
            }
        }    
    }    

    //if we have enough data queued process the fft
    BufferIODouble *pBins = NULL;
    while (chunker.Process(pProcessor, decay, fraction_overlap))
    {
        if (play)
        {
            pBins = pProcessor->getBufferIO();            
            pBins = bufferAverage.Do(pBins);
            if (pBins!=NULL)
            {           
                scaleBufferY.apply(pBins, logY?1.0f:20.0f, logY);
                pBins = &scaleBufferY;

                pScaleBufferX->Build(pBins);
                BufferIODouble *pScaledXBins = pScaleBufferX->GetBuffer();
                
                Draw_update(
                    pScaledXBins->GetData()+1, 
                    pScaledXBins->GetSize()-1
                );
                processedChunks++;
            }
        }
    }

    // return processed buffers
    sample_buf *buf = nullptr;
    while (chunker.getFreeBufferFrontAndPop(&buf))
        pFreeQueue->push(buf);

    return pBins;
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
    bool hold_pressed = ImGui::Checkbox("Hold", &hold_spectrum);
    ImGui::SameLine();
    bool bScaleXChanged = ImGui::Checkbox("Log x", &logX);
    ImGui::SameLine();
    ImGui::Checkbox("Log y", &logY);

    if (bScaleXChanged || pScaleBufferX==NULL)
    {
        delete pScaleBufferX;

        if (logX)
            pScaleBufferX = new ScaleBufferXLog();
        else
            pScaleBufferX = new ScaleBufferXLin();
    }

    //ImGui::SliderFloat("overlap", &fraction_overlap, 0.0f, 0.99f);
    ImGui::SliderFloat("decay", &decay, 0.0f, 0.99f);
    if (ImGui::SliderInt("averaging", &averaging, 1,500))
        bufferAverage.setAverageCount(averaging);

    // Draw FFT UI
    ImRect frame_fft_bb;
    bool draw_frame_fft_bb;
    {
        int frame_height = 150;
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

    BufferIODouble *pBins = Spectrogrammer_ProcessAudio(play, pProcessor, pScaleBufferX);

    if (draw_frame_fft_bb)
    {
        if (pBins!=NULL && play)
        {
            float *pDataIn = pBins->GetData();
            spectrumSamples.Resize(2 * (pProcessor->getBinCount()-1));
            float *pDataOut = spectrumSamples.GetData();   
            int i=0;             
            for (int bin=1;bin<pBins->GetSize();bin++) //skip bin 0 is DC
            {
                pDataOut[2 * i + 0] = pScaleBufferX->FreqToX(pProcessor->bin2Freq(bin)); 
                pDataOut[2 * i + 1] = pDataIn[bin];
                i++;
            }
        }

        ImU32 col = IM_COL32(200, 200, 200, 200);
        draw_lines(frame_fft_bb, spectrumSamples.GetData(), spectrumSamples.GetSize()/2, col, 0, 1);

        if (hold_spectrum)
        {
            if (hold_pressed)
            {
                spectrumSamplesHold.copy(&spectrumSamples);
            }

            ImU32 col = IM_COL32(200, 0, 0, 200);
            draw_lines(frame_fft_bb, spectrumSamplesHold.GetData(), spectrumSamplesHold.GetSize()/2, col, 0, 1);
        }
        
        if (logX)
            draw_log_scale(frame_fft_bb);   
        else
            draw_lin_scale(frame_fft_bb);   

    }

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
