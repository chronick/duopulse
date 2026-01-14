/**
 * Shared constants for pattern evaluation
 */

export const METRIC_KEYS = [
  'syncopation',
  'density',
  'velocityRange',
  'voiceSeparation',
  'regularity',
];

export const METRIC_LABELS = ['Sync', 'Dens', 'VelRng', 'VoiceSep', 'Reg'];

export const VOICES = [
  { name: 'V1', key: 'v1', velKey: 'v1Vel', className: 'v1' },
  { name: 'V2', key: 'v2', velKey: 'v2Vel', className: 'v2' },
  { name: 'AUX', key: 'aux', velKey: 'auxVel', className: 'aux' },
];

export const VOICE_KEYS = ['v1', 'v2', 'aux'];

export const ZONES = [
  { key: 'stable', label: 'STABLE', range: 'SHAPE 0-30%', color: '#44aa44' },
  { key: 'syncopated', label: 'SYNCOPATED', range: 'SHAPE 30-70%', color: '#aaaa44' },
  { key: 'wild', label: 'WILD', range: 'SHAPE 70-100%', color: '#aa4444' },
];

export const ZONE_COLORS = {
  stable: '#44aa44',
  syncopated: '#aaaa44',
  wild: '#aa4444',
};

export const STATUS_COLORS = {
  success: '#44ff44',
  warning: '#ffaa44',
  error: '#ff4444',
};

export const SWEEP_PARAMS = ['shape', 'energy', 'axisX', 'axisY', 'drift', 'accent'];

export const DEFAULT_BPM = 120;
export const MIN_BPM = 60;
export const MAX_BPM = 200;
