// [Codex 2026-10-30] Passthrough template: add product controls and sound assertions when a plugin is created.
export default {
  approvedCommit: '5382fbe',
  wonderlabCommit: '5248a15',
  emsdkVersion: '5.0.3',
  processorHeader: 'PluginProcessor.h',
  processorClass: 'audio_plugin::AudioPluginAudioProcessor',
  adapter: 'passthrough',
  viewports: [
    { name: 'phone', width: 390, height: 844, touch: true },
    { name: 'tablet', width: 820, height: 1180, touch: true },
    { name: 'desktop', width: 1440, height: 900, touch: false }
  ],
  pixels: { threshold: 0.12, maxDiffRatio: 0.001 },
  // Analytics is outside this local test's scope. Every local request is checked, without exclusions.
  analyticsHosts: ['www.googletagmanager.com', 'www.google-analytics.com', 'region1.google-analytics.com'],
  scope: null,
  preset: null,
  menu: null,
  aboutBuild: null,
  bands: [],
  expectedInventory: { sliders: 0, combos: 0, buttons: 0, parameters: 0 }
};
