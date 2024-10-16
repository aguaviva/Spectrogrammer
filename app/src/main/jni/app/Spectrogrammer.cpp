#include "imgui.h"
#include "imgui_internal.h"
#include "waterfall.h"
#include "Processor.h"
#include "ChunkerProcessor.h"
#include "ScaleBufferX.h"
#include "ScaleBufferY.h"
#include "BufferAverage.h"
#include "audio/audio_main.h"
#include "BufferAverage.h"
#include "imgui_helpers.h"

int fft_size = 4*1024;
float sample_rate = 48000.0f;
float max_freq = -1.0f; 
float min_freq = -1.0f; 
float bin_width = -1.0f;
int screenWidth = 1024;

// perf counters
int droppedBuffers=0;
int iterationsPerChunk = 0;
int processedChunks = 0;

// GUI state
int averaging = 1;
float fraction_overlap = .5f; // 0 to 1
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

void Spectrogrammer_Init()
{
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
    
    pProcessor = new myFFT();
    pProcessor->init(fft_size, sample_rate);

    min_freq = pProcessor->bin2Freq(1);
    max_freq = pProcessor->bin2Freq(fft_size/2.0f);

    bufferAverage.setAverageCount(averaging);

    Init_waterfall(screenWidth, screenWidth+512);
}

void Spectrogrammer_Shutdown()
{
    Audio_deleteAudioRecorder();
    Audio_deleteSLEngine();
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
            float x = pScaleBufferX->FreqToX(freq);

            if (x<0)
                continue;
            if ( x>frame_bb.GetWidth())
                break;

            if (i==0)
            {
                char str[255];
                sprintf(str, "%i", (int)freq); 
                float textWidth   = ImGui::CalcTextSize(str).x;
                // append HZ to last
                if (e==4)
                {
                    strcat(str, " Hz");
                }
                window->DrawList->AddText(
                    ImVec2(frame_bb.Min.x + x - textWidth/2, frame_bb.Min.y + 150), 
                    ImGui::GetColorU32(ImGuiCol_Text), 
                    str
                );
            }

            window->DrawList->AddLine(
                ImVec2(frame_bb.Min.x + x, frame_bb.Min.y),
                ImVec2(frame_bb.Min.x + x, frame_bb.Max.y),
                (i==0) ? ImGui::GetColorU32(ImGuiCol_Text) : ImGui::GetColorU32(ImGuiCol_TextDisabled));
        }

    }
}

void draw_lin_scale(ImRect frame_bb)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    for (int m=0;m<50000;m+=5000)
    {
        for (int i=0;i<5000;i+=1000)
        {
            float freq = m + i;
            float x = pScaleBufferX->FreqToX(freq);

            if (i==0)
            {
                char str[255];
                sprintf(str, "%i", (int)freq); 
                float textWidth   = ImGui::CalcTextSize(str).x;
                window->DrawList->AddText(
                    ImVec2(frame_bb.Min.x + x - textWidth/2, frame_bb.Min.y + 150), 
                    ImGui::GetColorU32(ImGuiCol_Text), 
                    str
                );
            }

            window->DrawList->AddLine(
                ImVec2(frame_bb.Min.x + x, frame_bb.Min.y),
                ImVec2(frame_bb.Min.x + x, frame_bb.Max.y),
                (i==0) ? ImGui::GetColorU32(ImGuiCol_Text) : ImGui::GetColorU32(ImGuiCol_TextDisabled));
        }

    }
}

void Spectrogrammer_MainLoopStep()
{
    ImGuiIO& io = ImGui::GetIO();

    AudioQueue* pFreeQueue = nullptr;
    AudioQueue* pRecQueue = nullptr;

    float sampleRate;
    GetBufferQueues(&sampleRate, &pFreeQueue, &pRecQueue);

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoResize;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f)); // place the next window in the top left corner (0,0)
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize); // make the next window fullscreen
    ImGui::Begin("imgui window", NULL, window_flags); // create a window

    ImGui::PlotLines("Waveform", audioSamples.GetData(), audioSamples.GetSize());

    ImGui::Checkbox("Play", &play);
    ImGui::SameLine();
    bool hold_pressed = ImGui::Checkbox("Hold", &hold_spectrum);
    ImGui::SameLine();
    bool bScaleXChanged = ImGui::Checkbox("Log x", &logX);
    ImGui::SameLine();
    bool bScaleYChanged = ImGui::Checkbox("Log y", &logY);

    if (bScaleXChanged || pScaleBufferX==NULL)
    {
        delete pScaleBufferX;

        if (logX)
            pScaleBufferX = new ScaleBufferXLog();
        else
            pScaleBufferX = new ScaleBufferXLin();

        pScaleBufferX->setOutputWidth(screenWidth, min_freq, max_freq);
        pScaleBufferX->PreBuild(pProcessor);        
    }

    // pass available buffers to processor
    if (pRecQueue!=NULL )
    {
        sample_buf *buf = nullptr;
        while (pRecQueue->front(&buf))
        {
            pRecQueue->pop();

            // last block? copy samples for drawing it
            if (pRecQueue->size()==0 && play)
            {
                int bufSize = AU_LEN(buf->cap_);
                audioSamples.Resize(bufSize);
                int16_t *buff = (int16_t *)(buf->buf_);
                for (int n = 0; n < bufSize; n++)
                {
                    audioSamples.GetData()[n] = (float)buff[n];
                }
            }

            if (chunker.pushAudioChunk(buf) == false)
            {
                droppedBuffers++;
            }
        }    
    }    

    ImGui::SliderFloat("overlap", &fraction_overlap, 0.0f, 0.99f);
    ImGui::SliderFloat("decay", &decay, 0.0f, 0.99f);
    if (ImGui::SliderInt("averaging", &averaging, 1,500))
        bufferAverage.setAverageCount(averaging);

    //if we have enough data queued process the fft
    static BufferIODouble *pBufferIO = NULL;
    while (chunker.Process(pProcessor, decay, fraction_overlap))
    {
        if (play)
        {
            pBufferIO = pProcessor->getBufferIO();
            pBufferIO = bufferAverage.Do(pBufferIO);
            if (pBufferIO!=NULL)
            {
                scaleBufferY.apply(pBufferIO, logY?1.0f:20.0f, logY);
                pBufferIO = &scaleBufferY;
            
                pScaleBufferX->Build(pBufferIO);
                
                // +1 to skip DC
                Draw_update(
                    pScaleBufferX->GetBuffer()->GetData(), 
                    pScaleBufferX->GetBuffer()->GetSize()
                );
                processedChunks++;
            }
        }
    }

    // Draw FFT lines
    {
        uint32_t values_count = 1024;
        int frame_height = 150;

        bool hovered;
        ImRect frame_bb;
        if (block_add("fft", ImVec2(1024, frame_height), &frame_bb, &hovered))
        {
            const ImGuiStyle& style = GImGui->Style;
            ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);        

            if (pBufferIO!=NULL && play)
            {
                float *pDataIn = pBufferIO->GetData();
                spectrumSamples.Resize(2 * pBufferIO->GetSize());
                float *pDataOut = spectrumSamples.GetData();                
                for (int i=0;i<pBufferIO->GetSize();i++)
                {
                    float freq = pProcessor->bin2Freq(i);
                    pDataOut[2*i+0] = pScaleBufferX->FreqToX(freq);
                    pDataOut[2*i+1] = pDataIn[i];
                }
            }

            ImU32 col = IM_COL32(200, 200, 200, 200);
            draw_lines(frame_bb, spectrumSamples.GetData(), spectrumSamples.GetSize()/2, col, 0, 1);

            if (hold_spectrum)
            {
                if (hold_pressed)
                {
                    spectrumSamplesHold.copy(&spectrumSamples);
                }

                ImU32 col = IM_COL32(200, 0, 0, 200);
                draw_lines(frame_bb, spectrumSamplesHold.GetData(), spectrumSamplesHold.GetSize()/2, col, 0, 1);
            }
            
            if (logX)
                draw_log_scale(frame_bb);   
            else
                draw_lin_scale(frame_bb);   

            ImGui::InvisibleButton("no", ImVec2(1024, 15));         
        }
    }

    if (true)
    {
        float seconds_per_row = (((float)fft_size/sample_rate) * (float)averaging) * (1.0 - fraction_overlap);

        bool hovered;
        ImRect frame_bb;
        if (block_add("wfall", ImVec2(screenWidth, screenWidth+512), &frame_bb, &hovered))
        {
            Draw_waterfall(frame_bb);
            Draw_vertical_scale(frame_bb, seconds_per_row);
            //draw_log_scale(frame_bb);
        }
    }

    ImGui::End();

    // return processed buffers
    {
        sample_buf *buf = nullptr;
        while (chunker.getFreeBufferFrontAndPop(&buf)) {
            pFreeQueue->push(buf);
        }
    }
}
