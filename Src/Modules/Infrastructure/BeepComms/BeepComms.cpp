/**
 * @file BeepComms.cpp
 * This file implements a module that provides the sending part of the audio communication system.
 */

#include "BeepComms.h"
#include "Platform/SystemCall.h"
#include "Platform/Thread.h"
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <algorithm>

#define SAMPLE_RATE 44100
#define BUFFER_SIZE SAMPLE_RATE
#define VOLUME_MULTIPLIER 16000

MAKE_MODULE(BeepComms, infrastructure);


#ifdef TARGET_ROBOT

#ifdef TARGET_ROBOT
  snd_pcm_t* pcm_handle;
#endif

typedef short sample_t;

BeepComms::BeepComms()
{
  init_pcm();
  startWorkers();
}

BeepComms::~BeepComms()
{
    stopWorkers();
    snd_pcm_close(pcm_handle);
}

void BeepComms::update(BeepCommData& audioData)
{
    if (theEnhancedKeyStates.isPressedFor(KeyStates::headFront, 100u))
    {
        if (buttonToggle)   
        {
            buttonToggle = false; 
            requestMultipleFrequencies(1000, 0.5, {500, 600});
        } 
    } else {
        buttonToggle = true;
    }
}

//play 5 sine waves simultaneously
void BeepComms::requestMultipleFrequencies(float duration, float volume, std::vector<float> frequencies){
   BeepRequest request;
   request.duration = duration;
   request.volume = volume;
   request.frequencies = frequencies;
   std::lock_guard lock(mtx); // aquire lock to write to request queue
   requestQueue.push_back(request); // qrite request to queue
   workerSignal.notify_one(); // notify worker of new request
};

void BeepComms::startWorkers()
{
    workerThread = std::thread(&BeepComms::handleBeepRequests, this);
}

void BeepComms::init_pcm()
{
    //open the audio device
    snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    snd_pcm_hw_params_any(pcm_handle, hw_params);
    snd_pcm_hw_params_set_buffer_size(pcm_handle, hw_params, 1152);
    snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S32_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 1);
    snd_pcm_hw_params_set_rate(pcm_handle, hw_params, SAMPLE_RATE, 0);
    snd_pcm_hw_params_set_periods(pcm_handle, hw_params, 10, 0);
    snd_pcm_hw_params_set_period_time(pcm_handle, hw_params, 100000, 0); // 0.1 seconds

    //snd_pcm_hw_params(pcm_handle, hw_params);
    snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, SAMPLE_RATE, 1, 500000);

}

void BeepComms::stopWorkers()
{
    shutdownWorkers = true;
    workerSignal.notify_all();
    workerThread.join();
}

void BeepComms::handleBeepRequests()
{
    std::unique_lock lock(mtx); // aquire mutex lock
    while (true)
    {
        workerSignal.wait(lock); // release lock and wait for signal from main thread
        if (shutdownWorkers) break; // exit loop. lock auto releases when function exits
        BeepRequest request = requestQueue.front();
        requestQueue.pop_front();
        lock.unlock(); // signal signature moved to this thread, release for other workers
        
        //generate superimposed sine waves as signals
        sample_t buf[BUFFER_SIZE] = {0}; // signal buffer
        int signalSize = (int) (SAMPLE_RATE * request.duration / 1000); // total size of signal in samples
        
        do {
            int toGenerate = std::min(signalSize, BUFFER_SIZE);
            for (int i = 0; i < toGenerate; i++) {
                buf[i] = 0;
                for (float& frequency : request.frequencies) {
                    buf[i] += (sample_t) (VOLUME_MULTIPLIER * (request.volume * sin(frequency * M_PI * 2 * i / SAMPLE_RATE)));
                }
            }
            signalSize -= toGenerate;
            
            snd_pcm_sframes_t n = snd_pcm_writei(pcm_handle, &buf, toGenerate); // write data to audio buffer
            if (n < 0) {
                printf("Audio Stream Lost (%s) Recovering...\n", snd_strerror(int(n)));
                snd_pcm_recover(pcm_handle, int(n), 1);
                n = snd_pcm_writei(pcm_handle, &buf, toGenerate); // write data to audio buffer
            }
        } while (signalSize > 0);
        

        lock.lock();
    }
}

#else // !defined TARGET_ROBOT

BeepComms::BeepComms() {}
BeepComms::~BeepComms() {}
void BeepComms::update(BeepCommData&) {}

#endif
