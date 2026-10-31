#include "soundmgr.h"
#include "sample.h"
#include "prefs.h"
#include "kb.h"
int soundManager::convertVolume(int volumeValue, int volumeType)
{
    int result = 0;
    if (volumeType == VOLUME_TYPE_101) {
        const int& setting = g_config.m_musicVolume;
        if (setting >= 1 && setting <= 10) {
            result = (setting + 1) * volumeValue / 10;
            if (result < 1)
                result = 1;
        }
    } else {
        const int& setting = g_config.m_soundVolume;
        if (setting >= 1 && setting <= 10) {
            result = (setting + 1) * volumeValue / 10;
            if (result < 1)
                result = 1;
        }
    }
    if (result < 0)
        result = 0;
    if (result > 127)
        result = 127;
    return result;
}
void soundManager::waitSample(ds_memsample* sample, int time)
{
    if (time < 0)
        time = 4000;
    unsigned long deadline = GameTime::get() + time;
    while (g_soundManager->getSampleInfo(sample, SAMPLE_INFO_PLAYING)) {
        if (GameTime::isPast(deadline))
            return;
        process1WindowsMessage();
        pollSound();
    }
}
SAMPLE2 loadPlaySample(const char* sampleName)
{
    if (!sampleName)
        return g_nullSample2;
    sample* loaded = ResourceManager::GetSample(sampleName);
    if (!loaded)
        return g_nullSample2;
    loaded->memSample.memCindex = 2;
    SAMPLE2 played;
    played.m_resSample = loaded;
    played.m_playSample = g_soundManager->memorySample(loaded);
    return played;
}
void clearMemSample(SAMPLE2 sample2)
{
    if (!sample2.m_resSample)
        return;
    if (g_shutDownDone)
        return;
    // soundManager::StopSample on the global, written out: guard trio,
    // section, AIL_end_sample, section.
    g_soundManager->stopSample(sample2.m_playSample);
    sample2.m_resSample->dispose();
}
void waitEndSample(SAMPLE2 sample2, int milliWait)
{
    if (milliWait < 0)
        milliWait = 4000;
    unsigned long deadline = GameTime::get() + milliWait;
    while (sample2.m_playSample && g_soundManager->getSampleInfo(
               sample2.m_playSample, soundManager::SAMPLE_INFO_PLAYING)) {
        if (GameTime::isPast(deadline))
            break;
        process1WindowsMessage();
        pollSound();
    }
    clearMemSample(sample2);
}
