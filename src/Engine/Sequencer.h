#pragma once

#include "daisysp.h"
#include <cmath>

namespace daisysp_idm_grids
{

class Sequencer
{
public:
    Sequencer() 
        : sampleRate_(48000.0f)
        , currentBpm_(120.0f)
        , lastKnobVal_(-1.0f)
        , lastTapTime_(0)
        , gateState_(false)
        , gateTimer_(0)
        , gateDurationSamples_(0)
    {}

    void Init(float sampleRate)
    {
        sampleRate_ = sampleRate;
        
        // Initialize Metro
        metro_.Init(2.0f, sampleRate);
        SetBpm(currentBpm_);

        // Initialize Beep
        beepOsc_.Init(sampleRate);
        beepOsc_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
        beepOsc_.SetFreq(880.0f);
        beepOsc_.SetAmp(0.5f);

        beepEnv_.Init(sampleRate);
        beepEnv_.SetTime(daisysp::ADSR_SEG_ATTACK, 0.001f);
        beepEnv_.SetTime(daisysp::ADSR_SEG_DECAY, 0.05f);
        beepEnv_.SetTime(daisysp::ADSR_SEG_RELEASE, 0.01f);
        beepEnv_.SetSustainLevel(0.0f);

        // Gate duration (10ms)
        gateDurationSamples_ = static_cast<int>(sampleRate * 0.01f);
    }

    void ProcessControl(float knobVal, bool tapButtonTrigger, uint32_t nowMs)
    {
        // 1. Knob -> Tempo
        if(std::abs(knobVal - lastKnobVal_) > 0.01f)
        {
            float newBpm = kMinTempo + (knobVal * (kMaxTempo - kMinTempo));
            SetBpm(newBpm);
            lastKnobVal_ = knobVal;
        }

        // 2. Tap Tempo
        if(tapButtonTrigger)
        {
            if(lastTapTime_ != 0)
            {
                uint32_t interval = nowMs - lastTapTime_;
                if(interval > 100 && interval < 2000)
                {
                    float newBpm = 60000.0f / static_cast<float>(interval);
                    SetBpm(newBpm);
                    // Soft takeover logic: update lastKnobVal to prevent jump
                    // In a real soft takeover, we'd need more state, but here we just sync 
                    // the "last seen" knob value to current physical knob value is handled by caller?
                    // Actually, if we change BPM via tap, the knob is now out of sync.
                    // The next knob move > threshold will snap it back.
                }
            }
            lastTapTime_ = nowMs;
        }
    }

    float ProcessAudio()
    {
        uint8_t tick = metro_.Process();
        
        if(tick)
        {
            beepEnv_.Retrigger(false);
            gateState_ = true;
            gateTimer_ = gateDurationSamples_;
        }

        if(gateState_)
        {
            gateTimer_--;
            if(gateTimer_ <= 0)
            {
                gateState_ = false;
            }
        }

        // Generate Audio
        float env = beepEnv_.Process(gateState_);
        float osc = beepOsc_.Process();
        
        return osc * env;
    }

    bool IsGateHigh() const { return gateState_; }
    float GetBpm() const { return currentBpm_; }

    void SetBpm(float bpm)
    {
        currentBpm_ = bpm;
        if(currentBpm_ < kMinTempo) currentBpm_ = kMinTempo;
        if(currentBpm_ > kMaxTempo) currentBpm_ = kMaxTempo;
        metro_.SetFreq(currentBpm_ / 60.0f);
    }

private:
    static constexpr float kMinTempo = 30.0f;
    static constexpr float kMaxTempo = 200.0f;

    float sampleRate_;
    float currentBpm_;
    float lastKnobVal_;
    uint32_t lastTapTime_;

    bool gateState_;
    int gateTimer_;
    int gateDurationSamples_;

    daisysp::Metro metro_;
    daisysp::Adsr beepEnv_;
    daisysp::Oscillator beepOsc_;
};

} // namespace daisysp_idm_grids

