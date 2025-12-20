#include "AuxOutput.h"

namespace daisysp_idm_grids
{

void AuxOutput::Init(float sampleRate)
{
    sampleRate_ = sampleRate;
    mode_       = AuxMode::HAT;
}

void AuxOutput::SetMode(AuxMode mode)
{
    mode_ = mode;
}

void AuxOutput::ComputeAuxOutput(AuxOutputState& auxState,
                                 const SequencerState& seqState,
                                 bool inFillZone,
                                 bool auxFires,
                                 bool isEvent)
{
    // Update mode in state
    auxState.mode = mode_;

    switch (mode_)
    {
        case AuxMode::HAT:
            // Third trigger voice: fire on aux hit mask
            if (auxFires)
            {
                FireTrigger(auxState);
            }
            break;

        case AuxMode::FILL_GATE:
            // Gate high during fill zones
            SetFillGate(auxState, inFillZone);
            break;

        case AuxMode::PHRASE_CV:
        {
            // Ramp over phrase (0-5V), reset at boundary
            float progress = seqState.GetPhraseProgress(patternLength_, phraseLength_);
            UpdatePhraseRamp(auxState, progress);
            break;
        }

        case AuxMode::EVENT:
            // Trigger on "interesting" moments
            if (isEvent)
            {
                FireTrigger(auxState);
            }
            break;

        default:
            break;
    }
}

float AuxOutput::ComputeAuxModeOutput(AuxMode mode,
                                      const AuxOutputState& auxState,
                                      const SequencerState& seqState,
                                      int patternLength,
                                      int phraseLength,
                                      bool inFillZone,
                                      bool auxFires,
                                      bool isEvent) const
{
    switch (mode)
    {
        case AuxMode::HAT:
        case AuxMode::EVENT:
            // Trigger output: 5V when high, 0V when low
            return auxState.trigger.high ? kMaxVoltage : 0.0f;

        case AuxMode::FILL_GATE:
            // Gate output: 5V during fill, 0V otherwise
            return inFillZone ? kMaxVoltage : 0.0f;

        case AuxMode::PHRASE_CV:
        {
            // Ramp output: 0-5V over phrase
            float progress = seqState.GetPhraseProgress(patternLength, phraseLength);
            return progress * kMaxVoltage;
        }

        default:
            return 0.0f;
    }
}

float AuxOutput::ProcessAuxOutput(const AuxOutputState& auxState) const
{
    float voltage = auxState.GetVoltage();
    return GateScaler::VoltageToCodecSample(voltage);
}

void AuxOutput::FireTrigger(AuxOutputState& auxState)
{
    if (auxState.mode == AuxMode::HAT || auxState.mode == AuxMode::EVENT)
    {
        auxState.trigger.Fire();
    }
}

void AuxOutput::SetFillGate(AuxOutputState& auxState, bool gateHigh)
{
    auxState.gateHigh = gateHigh;
}

void AuxOutput::UpdatePhraseRamp(AuxOutputState& auxState, float progress)
{
    // Clamp progress to valid range
    if (progress < 0.0f)
    {
        progress = 0.0f;
    }
    if (progress > 1.0f)
    {
        progress = 1.0f;
    }

    auxState.phraseRamp = progress;
}

} // namespace daisysp_idm_grids
