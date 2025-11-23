Short version: electrically you can use both as CV, but they’re not the same animal and they “feel” different in a rack.

⸻

What the dedicated CV out actually is

On Patch.Init (Patch SM under the hood) the CV outputs are a separate 12-bit DAC stage, spec’d as:
	•	Range: 0–5 V unipolar for CV_OUT pins  ￼
	•	Purpose: slow control signals (envelopes, LFOs, 1V/oct, gates, etc.)
	•	Front panel: Patch.Init only breaks out CV_OUT_1 as a jack; CV_OUT_2 is used to drive the LED intensity.  ￼

So: the dedicated CV out is a proper, calibrated 0–5 V control output. If you send 0.0 → ~0 V, 1.0 → ~5 V and you’re in the “normal Eurorack CV” world.

⸻

What the audio outs are

The “audio” jacks are driven by the audio codec:
	•	Range: roughly ±9 V (about 18 Vpp) on Patch SM / Patch.Init  ￼
	•	Coupling: DC-coupled outputs (no DC-blocking caps), so they can hold a static voltage and behave like CV outs.  ￼
	•	Bipolar: naturally centered around 0 V, so -1.0 → about +9 V and +1.0 → about -9 V on current hardware (there’s that inversion quirk).  ￼
	•	Resolution / rate: 24-bit audio codec running at audio rate (e.g. 48/96 kHz), so in some sense “overkill” for normal CV.

So from the rack’s perspective they’re high-range bipolar DC outputs that happen to be called audio.

⸻

Practical differences you’ll actually care about

1. Voltage range & expectations
	•	CV out: 0–5 V
	•	Audio out: ±something around 8–9 V

Most Eurorack CV inputs expect something like 0–5 or ±5 V. Feeding them ±9 V is usually safe but:
	•	You can easily slam things into the extreme of their range.
	•	For delicate stuff (1V/oct, precise modulation depth) you’ll want to scale/attenuate in firmware or with an attenuator in the rack.

2. Polarity
	•	CV out: unipolar 0–5 V → great for envelopes, gates, “just works”.
	•	Audio out: bipolar, and on Patch.Init they’re inverted (positive float gives negative voltage and vice-versa).  ￼

In code you can compensate with a -x if you care.

3. Intended use / calibration
	•	The CV out is the one Electrosmith expect you to use for proper CV. It’s spec’d 0–5 V and is what you’d pick for 1V/oct, sequencer lanes, clean modulation, etc.  ￼
	•	The audio outs are designed for line-ish audio levels, not “nice round” 0–5 V ranges. They’ll work as CV but you’re off the beaten path: scaling, offset, and inversion are your problem.

4. Firmware ergonomics
	•	CV outs: accessed via the Patch SM CV API (e.g. WriteCvOut() / cv_out[n] depending on layer). You usually update them at control/audio block rate.
	•	Audio outs: the usual out[chan][i] in the audio callback.

Functionally you can generate the same shapes either way; it’s just a question of scale/offset.

⸻

So when should you use which?

If you’re designing firmware for Patch.Init:
	•	Use the dedicated CV out for:
	•	Pitch CV (1V/oct to an oscillator).
	•	Gates/triggers (0 or 5 V).
	•	“Respectable” modulation you might want to share as open-source without surprising people (e.g. generic 0–5 V envelopes/LFOs).
	•	Use the audio outs as CV when:
	•	You want extra CV lanes beyond the single front-panel CV jack.
	•	You’re modulating things that tolerate/benefit from bipolar / big range signals (filters with attenuverters, wavefolders, FX parameters).
	•	You’re OK handling scaling and inversion in code or with attenuators.

If you want, next step I can sketch a layout like:
	•	CV OUT jack = clean 0–5 V lane (pitch/env)
	•	OUT L = bipolar “macro mod” CV
	•	OUT R = another macro/LFO, both with suggested scaling so they land around ±5 V instead of ±9 V.