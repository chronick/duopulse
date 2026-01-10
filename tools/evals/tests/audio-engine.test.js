/**
 * Tests for audio engine
 */

import { jest, describe, test, expect, beforeEach } from '@jest/globals';
import { AudioEngine } from '../public/audio-engine.js';

// Mock Web Audio API
class MockOscillator {
  constructor() {
    this.type = 'sine';
    this.frequency = { value: 440, setValueAtTime: jest.fn(), exponentialRampToValueAtTime: jest.fn() };
    this.connect = jest.fn();
    this.start = jest.fn();
    this.stop = jest.fn();
  }
}

class MockGainNode {
  constructor() {
    this.gain = { value: 1, setValueAtTime: jest.fn(), exponentialRampToValueAtTime: jest.fn() };
    this.connect = jest.fn();
  }
}

class MockBiquadFilter {
  constructor() {
    this.type = 'lowpass';
    this.frequency = { value: 1000 };
    this.Q = { value: 1 };
    this.connect = jest.fn();
  }
}

class MockDynamicsCompressor {
  constructor() {
    this.threshold = { value: 0 };
    this.knee = { value: 0 };
    this.ratio = { value: 1 };
    this.attack = { value: 0 };
    this.release = { value: 0 };
    this.connect = jest.fn();
  }
}

class MockAudioBuffer {
  constructor(numChannels, length, sampleRate) {
    this.numberOfChannels = numChannels;
    this.length = length;
    this.sampleRate = sampleRate;
    this._data = new Float32Array(length);
  }
  getChannelData() {
    return this._data;
  }
}

class MockBufferSource {
  constructor() {
    this.buffer = null;
    this.connect = jest.fn();
    this.start = jest.fn();
    this.stop = jest.fn();
  }
}

class MockWaveShaper {
  constructor() {
    this.curve = null;
    this.connect = jest.fn();
  }
}

class MockAudioContext {
  constructor() {
    this.currentTime = 0;
    this.sampleRate = 44100;
    this.state = 'running';
    this.destination = { connect: jest.fn() };
  }
  createOscillator() { return new MockOscillator(); }
  createGain() { return new MockGainNode(); }
  createBiquadFilter() { return new MockBiquadFilter(); }
  createDynamicsCompressor() { return new MockDynamicsCompressor(); }
  createBuffer(numChannels, length, sampleRate) {
    return new MockAudioBuffer(numChannels, length, sampleRate);
  }
  createBufferSource() { return new MockBufferSource(); }
  createWaveShaper() { return new MockWaveShaper(); }
  resume() { return Promise.resolve(); }
  suspend() { return Promise.resolve(); }
}

// Install mock
global.AudioContext = MockAudioContext;

describe('AudioEngine', () => {
  let engine;

  beforeEach(() => {
    engine = new AudioEngine();
  });

  describe('initialization', () => {
    test('starts with no audio context', () => {
      expect(engine.ctx).toBeNull();
    });

    test('creates audio context on init', async () => {
      await engine.init();
      expect(engine.ctx).toBeInstanceOf(MockAudioContext);
    });

    test('creates master gain on init', async () => {
      await engine.init();
      expect(engine.masterGain).toBeDefined();
    });

    test('creates compressor on init', async () => {
      await engine.init();
      expect(engine.compressor).toBeDefined();
    });

    test('init is idempotent', async () => {
      await engine.init();
      const ctx1 = engine.ctx;
      await engine.init();
      expect(engine.ctx).toBe(ctx1);
    });
  });

  describe('kit management', () => {
    test('has default kit set to analog', () => {
      expect(engine.currentKit).toBe('analog');
    });

    test('can switch kits', () => {
      engine.setKit('digital');
      expect(engine.currentKit).toBe('digital');
    });

    test('ignores invalid kit names', () => {
      engine.setKit('invalid');
      expect(engine.currentKit).toBe('analog');
    });

    test('getKitNames returns all kits', () => {
      const names = engine.getKitNames();
      expect(names).toHaveLength(4);
      expect(names.map(k => k.id)).toContain('analog');
      expect(names.map(k => k.id)).toContain('digital');
      expect(names.map(k => k.id)).toContain('industrial');
      expect(names.map(k => k.id)).toContain('minimal');
    });

    test('getKit returns current kit params', () => {
      const kit = engine.getKit();
      expect(kit).toHaveProperty('v1');
      expect(kit).toHaveProperty('v2');
      expect(kit).toHaveProperty('aux');
    });
  });

  describe('kit parameters', () => {
    test('v1 (kick) has expected params', () => {
      const kit = engine.getKit();
      expect(kit.v1).toHaveProperty('freq');
      expect(kit.v1).toHaveProperty('pitchDecay');
      expect(kit.v1).toHaveProperty('decay');
    });

    test('v2 (snare) has expected params', () => {
      const kit = engine.getKit();
      expect(kit.v2).toHaveProperty('freq');
      expect(kit.v2).toHaveProperty('noiseDecay');
      expect(kit.v2).toHaveProperty('noiseMix');
    });

    test('aux (hat) has expected params', () => {
      const kit = engine.getKit();
      expect(kit.aux).toHaveProperty('freq');
      expect(kit.aux).toHaveProperty('decay');
      expect(kit.aux).toHaveProperty('resonance');
    });

    test('can update kit params', () => {
      engine.setKitParam('v1', 'freq', 60);
      expect(engine.getKit().v1.freq).toBe(60);
    });
  });

  describe('triggering sounds', () => {
    beforeEach(async () => {
      await engine.init();
    });

    test('triggerKick does not throw', () => {
      expect(() => engine.triggerKick()).not.toThrow();
    });

    test('triggerSnare does not throw', () => {
      expect(() => engine.triggerSnare()).not.toThrow();
    });

    test('triggerHat does not throw', () => {
      expect(() => engine.triggerHat()).not.toThrow();
    });

    test('trigger dispatches to correct voice', () => {
      const kickSpy = jest.spyOn(engine, 'triggerKick');
      const snareSpy = jest.spyOn(engine, 'triggerSnare');
      const hatSpy = jest.spyOn(engine, 'triggerHat');

      engine.trigger('v1', 0.8);
      expect(kickSpy).toHaveBeenCalledWith(0.8, null);

      engine.trigger('v2', 0.6);
      expect(snareSpy).toHaveBeenCalledWith(0.6, null);

      engine.trigger('aux', 0.5);
      expect(hatSpy).toHaveBeenCalledWith(0.5, null);
    });

    test('trigger handles custom time', () => {
      const kickSpy = jest.spyOn(engine, 'triggerKick');
      engine.trigger('v1', 0.8, 1.5);
      expect(kickSpy).toHaveBeenCalledWith(0.8, 1.5);
    });
  });

  describe('utility methods', () => {
    beforeEach(async () => {
      await engine.init();
    });

    test('createNoiseBuffer creates buffer', () => {
      const buffer = engine.createNoiseBuffer(0.1);
      expect(buffer).toBeInstanceOf(MockAudioBuffer);
    });

    test('makeDistortionCurve creates curve', () => {
      const curve = engine.makeDistortionCurve(200);
      expect(curve).toBeInstanceOf(Float32Array);
      expect(curve.length).toBe(44100);
    });

    test('now returns current time', () => {
      expect(engine.now).toBe(0);
    });
  });
});
