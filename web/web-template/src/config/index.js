// ============================================================================
// BUILD CONFIGURATION
// This file is auto-generated during CI/CD builds with actual values.
// Local development uses placeholder values from buildConfig.json.
// ============================================================================

import buildConfig from './buildConfig.json';

/**
 * Build configuration object
 * Contains version info, installer URLs, and build metadata
 */
export const config = {
  ...buildConfig,
  
  // Computed download URLs
  getDownloadUrl: (platform) => {
    const { downloadBaseUrl, installers } = buildConfig;
    const platformDir = platform === 'mac' ? 'mac' : 'win';
    const filename = platform === 'mac' ? installers.mac : installers.windows;
    return `${downloadBaseUrl}/${platformDir}/${filename}`;
  }
};

export default config;

