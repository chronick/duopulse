/**
 * DuoPulse Pattern Evaluation - Web Audio Engine
 * Tunable percussive synthesizers for pattern playback
 */

export class AudioEngine {
  constructor() {
    this.ctx = null;
    this.masterGain = null;
    this.compressor = null;
    this.currentKit = 'analog';
    this.kitParams = this.getDefaultKitParams();
  }

  async init() {
    if (this.ctx) {
      // Already initialized, just ensure resumed
      if (this.ctx.state === 'suspended') {
        await this.ctx.resume();
      }
      return;
    }

    // Create AudioContext only when called (should be from user gesture)
    this.ctx = new AudioContext();

    // Master compressor for punch
    this.compressor = this.ctx.createDynamicsCompressor();
    this.compressor.threshold.value = -12;
    this.compressor.knee.value = 6;
    this.compressor.ratio.value = 4;
    this.compressor.attack.value = 0.003;
    this.compressor.release.value = 0.1;

    // Master gain
    this.masterGain = this.ctx.createGain();
    this.masterGain.gain.value = 0.8;

    this.compressor.connect(this.masterGain);
    this.masterGain.connect(this.ctx.destination);

    if (this.ctx.state === 'suspended') {
      await this.ctx.resume();
    }
  }

  getDefaultKitParams() {
    return {
      analog: {
        name: 'Analog 808',
        v1: { // Kick
          freq: 55,
          pitchDecay: 0.15,
          pitchRange: 2.5,
          decay: 0.4,
          click: 0.3,
          drive: 1.2,
        },
        v2: { // Snare
          freq: 180,
          noiseDecay: 0.12,
          toneDecay: 0.08,
          noiseMix: 0.6,
          ringFreq: 220,
        },
        aux: { // Hat
          freq: 8000,
          decay: 0.06,
          resonance: 4,
          brightness: 0.7,
        },
      },
      digital: {
        name: 'Digital Sharp',
        v1: {
          freq: 48,
          pitchDecay: 0.08,
          pitchRange: 3,
          decay: 0.25,
          click: 0.5,
          drive: 1.5,
        },
        v2: {
          freq: 220,
          noiseDecay: 0.08,
          toneDecay: 0.04,
          noiseMix: 0.7,
          ringFreq: 380,
        },
        aux: {
          freq: 10000,
          decay: 0.04,
          resonance: 6,
          brightness: 0.9,
        },
      },
      industrial: {
        name: 'Industrial',
        v1: {
          freq: 42,
          pitchDecay: 0.2,
          pitchRange: 4,
          decay: 0.5,
          click: 0.6,
          drive: 2.0,
        },
        v2: {
          freq: 160,
          noiseDecay: 0.15,
          toneDecay: 0.1,
          noiseMix: 0.8,
          ringFreq: 280,
        },
        aux: {
          freq: 6000,
          decay: 0.08,
          resonance: 8,
          brightness: 0.5,
        },
      },
      minimal: {
        name: 'Minimal Techno',
        v1: {
          freq: 50,
          pitchDecay: 0.12,
          pitchRange: 2,
          decay: 0.3,
          click: 0.2,
          drive: 1.0,
        },
        v2: {
          freq: 200,
          noiseDecay: 0.1,
          toneDecay: 0.06,
          noiseMix: 0.5,
          ringFreq: 180,
        },
        aux: {
          freq: 9000,
          decay: 0.05,
          resonance: 3,
          brightness: 0.6,
        },
      },
    };
  }

  setKit(kitName) {
    if (this.kitParams[kitName]) {
      this.currentKit = kitName;
    }
  }

  getKit() {
    return this.kitParams[this.currentKit];
  }

  getKitNames() {
    return Object.entries(this.kitParams).map(([id, kit]) => ({
      id,
      name: kit.name,
    }));
  }

  // V1 - Kick Drum
  triggerKick(velocity = 1.0, time = null) {
    if (!this.ctx) return;
    const t = time ?? this.ctx.currentTime;
    const kit = this.getKit().v1;

    // Main oscillator
    const osc = this.ctx.createOscillator();
    osc.type = 'sine';

    // Pitch envelope (frequency sweep down)
    const startFreq = kit.freq * kit.pitchRange;
    osc.frequency.setValueAtTime(startFreq, t);
    osc.frequency.exponentialRampToValueAtTime(kit.freq, t + kit.pitchDecay);

    // Amplitude envelope
    const gain = this.ctx.createGain();
    const amp = 0.8 * velocity * kit.drive;
    gain.gain.setValueAtTime(amp, t);
    gain.gain.exponentialRampToValueAtTime(0.001, t + kit.decay);

    // Click transient
    if (kit.click > 0) {
      const clickOsc = this.ctx.createOscillator();
      clickOsc.type = 'square';
      clickOsc.frequency.setValueAtTime(1200, t);

      const clickGain = this.ctx.createGain();
      clickGain.gain.setValueAtTime(kit.click * velocity * 0.3, t);
      clickGain.gain.exponentialRampToValueAtTime(0.001, t + 0.01);

      const clickFilter = this.ctx.createBiquadFilter();
      clickFilter.type = 'highpass';
      clickFilter.frequency.value = 800;

      clickOsc.connect(clickFilter);
      clickFilter.connect(clickGain);
      clickGain.connect(this.compressor);

      clickOsc.start(t);
      clickOsc.stop(t + 0.02);
    }

    // Waveshaper for distortion
    const waveshaper = this.ctx.createWaveShaper();
    waveshaper.curve = this.makeDistortionCurve(kit.drive * 200);

    osc.connect(waveshaper);
    waveshaper.connect(gain);
    gain.connect(this.compressor);

    osc.start(t);
    osc.stop(t + kit.decay + 0.1);
  }

  // V2 - Snare/Clap
  triggerSnare(velocity = 1.0, time = null) {
    if (!this.ctx) return;
    const t = time ?? this.ctx.currentTime;
    const kit = this.getKit().v2;

    // Noise burst
    const noiseBuffer = this.createNoiseBuffer(0.3);
    const noise = this.ctx.createBufferSource();
    noise.buffer = noiseBuffer;

    const noiseFilter = this.ctx.createBiquadFilter();
    noiseFilter.type = 'bandpass';
    noiseFilter.frequency.value = 3500;
    noiseFilter.Q.value = 1.5;

    const noiseGain = this.ctx.createGain();
    const noiseAmp = velocity * kit.noiseMix * 0.6;
    noiseGain.gain.setValueAtTime(noiseAmp, t);
    noiseGain.gain.exponentialRampToValueAtTime(0.001, t + kit.noiseDecay);

    noise.connect(noiseFilter);
    noiseFilter.connect(noiseGain);
    noiseGain.connect(this.compressor);

    // Pitched body
    const osc = this.ctx.createOscillator();
    osc.type = 'triangle';
    osc.frequency.setValueAtTime(kit.freq, t);
    osc.frequency.exponentialRampToValueAtTime(kit.freq * 0.5, t + kit.toneDecay);

    const oscGain = this.ctx.createGain();
    const oscAmp = velocity * (1 - kit.noiseMix) * 0.5;
    oscGain.gain.setValueAtTime(oscAmp, t);
    oscGain.gain.exponentialRampToValueAtTime(0.001, t + kit.toneDecay);

    osc.connect(oscGain);
    oscGain.connect(this.compressor);

    // Ring modulator for metallic character
    const ringOsc = this.ctx.createOscillator();
    ringOsc.type = 'sine';
    ringOsc.frequency.value = kit.ringFreq;

    const ringGain = this.ctx.createGain();
    ringGain.gain.setValueAtTime(velocity * 0.15, t);
    ringGain.gain.exponentialRampToValueAtTime(0.001, t + kit.toneDecay * 0.8);

    ringOsc.connect(ringGain);
    ringGain.connect(this.compressor);

    noise.start(t);
    osc.start(t);
    ringOsc.start(t);

    noise.stop(t + kit.noiseDecay + 0.1);
    osc.stop(t + kit.toneDecay + 0.1);
    ringOsc.stop(t + kit.toneDecay + 0.1);
  }

  // AUX - Hi-hat / Percussion
  triggerHat(velocity = 1.0, time = null) {
    if (!this.ctx) return;
    const t = time ?? this.ctx.currentTime;
    const kit = this.getKit().aux;

    // Multi-oscillator metallic source
    const oscs = [];
    const freqs = [kit.freq * 0.8, kit.freq, kit.freq * 1.2, kit.freq * 1.5];

    const mixGain = this.ctx.createGain();
    const amp = velocity * 0.25 * kit.brightness;
    mixGain.gain.setValueAtTime(amp, t);
    mixGain.gain.exponentialRampToValueAtTime(0.001, t + kit.decay);

    // Bandpass filter for character
    const filter = this.ctx.createBiquadFilter();
    filter.type = 'bandpass';
    filter.frequency.value = kit.freq;
    filter.Q.value = kit.resonance;

    // Highpass to remove low end
    const hpf = this.ctx.createBiquadFilter();
    hpf.type = 'highpass';
    hpf.frequency.value = 6000;

    for (const freq of freqs) {
      const osc = this.ctx.createOscillator();
      osc.type = 'square';
      osc.frequency.value = freq;
      osc.connect(filter);
      oscs.push(osc);
    }

    filter.connect(hpf);
    hpf.connect(mixGain);
    mixGain.connect(this.compressor);

    oscs.forEach(osc => {
      osc.start(t);
      osc.stop(t + kit.decay + 0.05);
    });
  }

  // Trigger by voice name
  trigger(voice, velocity = 1.0, time = null) {
    switch (voice) {
      case 'v1': this.triggerKick(velocity, time); break;
      case 'v2': this.triggerSnare(velocity, time); break;
      case 'aux': this.triggerHat(velocity, time); break;
    }
  }

  // Utility: Create noise buffer
  createNoiseBuffer(duration) {
    const length = this.ctx.sampleRate * duration;
    const buffer = this.ctx.createBuffer(1, length, this.ctx.sampleRate);
    const data = buffer.getChannelData(0);

    for (let i = 0; i < length; i++) {
      data[i] = Math.random() * 2 - 1;
    }

    return buffer;
  }

  // Utility: Create distortion curve
  makeDistortionCurve(amount) {
    const samples = 44100;
    const curve = new Float32Array(samples);
    const deg = Math.PI / 180;

    for (let i = 0; i < samples; i++) {
      const x = (i * 2) / samples - 1;
      curve[i] = ((3 + amount) * x * 20 * deg) / (Math.PI + amount * Math.abs(x));
    }

    return curve;
  }

  // Update kit parameter
  setKitParam(voice, param, value) {
    if (this.kitParams[this.currentKit]?.[voice]) {
      this.kitParams[this.currentKit][voice][param] = value;
    }
  }

  // Get current time for scheduling
  get now() {
    return this.ctx?.currentTime ?? 0;
  }

  // Suspend/resume
  async suspend() {
    await this.ctx?.suspend();
  }

  async resume() {
    await this.ctx?.resume();
  }
}
