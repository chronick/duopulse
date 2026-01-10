/**
 * DuoPulse Pattern Evaluation - Sequencer Player
 * Handles pattern playback with precise Web Audio timing
 */

import { AudioEngine } from './audio-engine.js';

export class Player {
  constructor() {
    this.audio = new AudioEngine();
    this.pattern = null;
    this.isPlaying = false;
    this.bpm = 120;
    this.currentStep = 0;
    this.lookahead = 25; // ms
    this.scheduleAheadTime = 0.1; // seconds
    this.nextStepTime = 0;
    this.timerID = null;
    this.initialized = false;

    this.mutes = { v1: false, v2: false, aux: false };
    this.onStepChange = null;
    this.onPlayStateChange = null;
  }

  // Lazy init - call this before any audio operation
  async ensureInitialized() {
    if (!this.initialized) {
      await this.audio.init();
      this.initialized = true;
    }
  }

  async init() {
    // No-op on load - lazy init on first user interaction
  }

  setPattern(pattern) {
    this.pattern = pattern;
    this.currentStep = 0;
    if (this.onStepChange) {
      this.onStepChange(this.currentStep);
    }
  }

  getPattern() {
    return this.pattern;
  }

  setBPM(bpm) {
    this.bpm = Math.max(60, Math.min(200, bpm));
  }

  getBPM() {
    return this.bpm;
  }

  setMute(voice, muted) {
    this.mutes[voice] = muted;
  }

  getMute(voice) {
    return this.mutes[voice];
  }

  toggleMute(voice) {
    this.mutes[voice] = !this.mutes[voice];
    return this.mutes[voice];
  }

  get stepDuration() {
    // 16th notes at current BPM
    return 60 / this.bpm / 4;
  }

  async play() {
    if (this.isPlaying || !this.pattern) return;

    await this.ensureInitialized();
    await this.audio.resume();
    this.isPlaying = true;
    this.nextStepTime = this.audio.now;
    this.scheduler();

    if (this.onPlayStateChange) {
      this.onPlayStateChange(true);
    }
  }

  pause() {
    this.isPlaying = false;
    if (this.timerID) {
      clearTimeout(this.timerID);
      this.timerID = null;
    }

    if (this.onPlayStateChange) {
      this.onPlayStateChange(false);
    }
  }

  stop() {
    this.pause();
    this.currentStep = 0;
    if (this.onStepChange) {
      this.onStepChange(this.currentStep);
    }
  }

  async toggle() {
    if (this.isPlaying) {
      this.pause();
    } else {
      await this.play();
    }
  }

  scheduler() {
    while (this.nextStepTime < this.audio.now + this.scheduleAheadTime) {
      this.scheduleStep(this.currentStep, this.nextStepTime);
      this.advanceStep();
    }

    if (this.isPlaying) {
      this.timerID = setTimeout(() => this.scheduler(), this.lookahead);
    }
  }

  scheduleStep(stepIndex, time) {
    if (!this.pattern?.steps?.[stepIndex]) return;

    const step = this.pattern.steps[stepIndex];

    // V1 (Kick)
    if (step.v1 && !this.mutes.v1) {
      this.audio.trigger('v1', step.v1Vel, time);
    }

    // V2 (Snare)
    if (step.v2 && !this.mutes.v2) {
      this.audio.trigger('v2', step.v2Vel, time);
    }

    // AUX (Hat)
    if (step.aux && !this.mutes.aux) {
      this.audio.trigger('aux', step.auxVel, time);
    }
  }

  advanceStep() {
    const length = this.pattern?.params?.length || 16;
    this.currentStep = (this.currentStep + 1) % length;
    this.nextStepTime += this.stepDuration;

    // Fire callback slightly ahead for visual sync
    if (this.onStepChange) {
      const delay = Math.max(0, (this.nextStepTime - this.audio.now) * 1000 - 10);
      setTimeout(() => {
        if (this.isPlaying) {
          this.onStepChange(this.currentStep);
        }
      }, delay);
    }
  }

  // Trigger a single voice manually (for auditioning)
  async triggerVoice(voice, velocity = 1.0) {
    await this.ensureInitialized();
    this.audio.trigger(voice, velocity);
  }

  // Kit management
  setKit(kitName) {
    this.audio.setKit(kitName);
  }

  getKitNames() {
    return this.audio.getKitNames();
  }

  getCurrentKit() {
    return this.audio.currentKit;
  }

  getKitParams(voice) {
    return this.audio.getKit()[voice];
  }

  setKitParam(voice, param, value) {
    this.audio.setKitParam(voice, param, value);
  }
}

// ============================================================================
// Player UI Component
// ============================================================================

export function createPlayerUI(player, container) {
  const html = `
    <div class="player-panel">
      <div class="player-header">
        <h3>Pattern Player</h3>
        <div class="player-kit-select">
          <select id="kit-select">
            ${player.getKitNames().map(k =>
              `<option value="${k.id}">${k.name}</option>`
            ).join('')}
          </select>
        </div>
      </div>

      <div class="player-transport">
        <button class="transport-btn" id="btn-play" title="Play/Pause">
          <svg class="icon-play" viewBox="0 0 24 24"><polygon points="5,3 19,12 5,21"/></svg>
          <svg class="icon-pause" viewBox="0 0 24 24" hidden><rect x="5" y="3" width="4" height="18"/><rect x="15" y="3" width="4" height="18"/></svg>
        </button>
        <button class="transport-btn" id="btn-stop" title="Stop">
          <svg viewBox="0 0 24 24"><rect x="4" y="4" width="16" height="16"/></svg>
        </button>
      </div>

      <div class="player-bpm">
        <label>
          <span class="bpm-label">BPM</span>
          <input type="number" id="bpm-input" value="${player.getBPM()}" min="60" max="200" step="1">
        </label>
        <input type="range" id="bpm-slider" value="${player.getBPM()}" min="60" max="200" step="1">
      </div>

      <div class="player-mutes">
        <button class="mute-btn v1" id="mute-v1" data-voice="v1">
          <span class="mute-label">V1</span>
          <span class="mute-state">ON</span>
        </button>
        <button class="mute-btn v2" id="mute-v2" data-voice="v2">
          <span class="mute-label">V2</span>
          <span class="mute-state">ON</span>
        </button>
        <button class="mute-btn aux" id="mute-aux" data-voice="aux">
          <span class="mute-label">AUX</span>
          <span class="mute-state">ON</span>
        </button>
      </div>

      <div class="player-step-display">
        <div class="step-indicator" id="step-indicator">
          ${Array(16).fill(0).map((_, i) =>
            `<div class="step-dot" data-step="${i}"></div>`
          ).join('')}
        </div>
        <div class="step-counter">
          Step: <span id="step-number">1</span>/16
        </div>
      </div>

      <div class="player-pattern-info" id="pattern-info">
        <div class="pattern-name">No pattern loaded</div>
      </div>

      <div class="player-shortcuts">
        <kbd>Space</kbd> play/pause &nbsp;
        <kbd>Esc</kbd> stop<br>
        <kbd>1</kbd><kbd>2</kbd><kbd>3</kbd> mute V1/V2/AUX<br>
        Right-click mute btn to audition
      </div>
    </div>
  `;

  container.innerHTML = html;

  // Bind events
  const btnPlay = container.querySelector('#btn-play');
  const btnStop = container.querySelector('#btn-stop');
  const bpmInput = container.querySelector('#bpm-input');
  const bpmSlider = container.querySelector('#bpm-slider');
  const kitSelect = container.querySelector('#kit-select');
  const stepIndicator = container.querySelector('#step-indicator');
  const stepNumber = container.querySelector('#step-number');

  btnPlay.addEventListener('click', () => {
    player.toggle();
  });

  btnStop.addEventListener('click', () => {
    player.stop();
  });

  bpmInput.addEventListener('change', (e) => {
    const bpm = parseInt(e.target.value, 10);
    player.setBPM(bpm);
    bpmSlider.value = player.getBPM();
  });

  bpmSlider.addEventListener('input', (e) => {
    const bpm = parseInt(e.target.value, 10);
    player.setBPM(bpm);
    bpmInput.value = player.getBPM();
  });

  kitSelect.addEventListener('change', (e) => {
    player.setKit(e.target.value);
  });

  // Mute buttons
  container.querySelectorAll('.mute-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const voice = btn.dataset.voice;
      const muted = player.toggleMute(voice);
      btn.classList.toggle('muted', muted);
      btn.querySelector('.mute-state').textContent = muted ? 'OFF' : 'ON';
    });

    // Audition on right-click
    btn.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      player.triggerVoice(btn.dataset.voice);
    });
  });

  // Play state callback
  player.onPlayStateChange = (playing) => {
    btnPlay.querySelector('.icon-play').hidden = playing;
    btnPlay.querySelector('.icon-pause').hidden = !playing;
    btnPlay.classList.toggle('active', playing);
  };

  // Step change callback
  player.onStepChange = (step) => {
    stepIndicator.querySelectorAll('.step-dot').forEach((dot, i) => {
      dot.classList.toggle('active', i === step);
    });
    stepNumber.textContent = step + 1;
  };

  // Return update function for pattern info
  return {
    updatePatternInfo(pattern, name = '') {
      const info = container.querySelector('#pattern-info');
      if (!pattern) {
        info.innerHTML = '<div class="pattern-name">No pattern loaded</div>';
        return;
      }

      const params = pattern.params || {};
      info.innerHTML = `
        <div class="pattern-name">${name || 'Pattern'}</div>
        <div class="pattern-params">
          ${params.shape !== undefined ? `<span>SHP: ${(params.shape * 100).toFixed(0)}%</span>` : ''}
          ${params.energy !== undefined ? `<span>NRG: ${(params.energy * 100).toFixed(0)}%</span>` : ''}
        </div>
      `;
    },

    updateStepCount(length) {
      const indicator = container.querySelector('#step-indicator');
      indicator.innerHTML = Array(length).fill(0).map((_, i) =>
        `<div class="step-dot" data-step="${i}"></div>`
      ).join('');

      const counter = container.querySelector('.step-counter');
      counter.innerHTML = `Step: <span id="step-number">1</span>/${length}`;
    }
  };
}
