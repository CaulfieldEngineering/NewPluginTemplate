// ============================================================================
// CHANGELOG COMPONENT
// Displays version history with release notes and download links
// ============================================================================

import React, { useState, useEffect, useRef } from 'react';
import changelog from '../../config/changelog.json';
import config from '../../config';
import styles from './Changelog.module.css';

/**
 * Apple logo SVG icon
 */
const AppleIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" className={styles.downloadIcon} width="14" height="14" aria-hidden="true">
    <path d="M18.71 19.5c-.83 1.24-1.71 2.45-3.05 2.47-1.34.03-1.77-.79-3.29-.79-1.53 0-2 .77-3.27.82-1.31.05-2.3-1.32-3.14-2.53C4.25 17 2.94 12.45 4.7 9.39c.87-1.52 2.43-2.48 4.12-2.51 1.28-.02 2.5.87 3.29.87.78 0 2.26-1.07 3.81-.91.65.03 2.47.26 3.64 1.98-.09.06-2.17 1.28-2.15 3.81.03 3.02 2.65 4.03 2.68 4.04-.03.07-.42 1.44-1.38 2.83M13 3.5c.73-.83 1.94-1.46 2.94-1.5.13 1.17-.34 2.35-1.04 3.19-.69.85-1.83 1.51-2.95 1.42-.15-1.15.41-2.35 1.05-3.11z"/>
  </svg>
);

/**
 * Windows logo SVG icon
 */
const WindowsIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor" className={styles.downloadIcon} width="14" height="14" aria-hidden="true">
    <path d="M3 12V6.75l6-1.32v6.48L3 12zm17-9v8.75l-10 .15V5.21L20 3zM3 13l6 .09v6.81l-6-1.15V13zm17 .25V22l-10-1.78v-7.75l10 .28z"/>
  </svg>
);

/**
 * Section type labels for display
 */
const SECTION_LABELS = {
  added: 'Added',
  changed: 'Changed',
  deprecated: 'Deprecated',
  removed: 'Removed',
  fixed: 'Fixed',
  security: 'Security'
};

/**
 * Section icons/emojis for visual distinction
 */
const SECTION_ICONS = {
  added: '✦',
  changed: '↻',
  deprecated: '⚠',
  removed: '✕',
  fixed: '✓',
  security: '🔒'
};

/**
 * Generate download URL for a specific version
 */
function getVersionDownloadUrl(version, platform) {
  const { downloadBaseUrl, repositoryName } = config;
  const platformDir = platform === 'mac' ? 'mac' : 'win';
  const ext = platform === 'mac' ? 'pkg' : 'exe';
  const platformLabel = platform === 'mac' ? 'Mac' : 'Win';
  return `${downloadBaseUrl}/${platformDir}/${repositoryName}${platformLabel}Installer_${version}.${ext}`;
}

/**
 * Format date string for display
 */
function formatDate(dateStr) {
  if (!dateStr) return '';
  const date = new Date(dateStr);
  return date.toLocaleDateString('en-US', { 
    year: 'numeric', 
    month: 'long', 
    day: 'numeric' 
  });
}

/**
 * Version Screenshot Component
 * Displays a thumbnail with lightbox support, or placeholder if missing
 */
function VersionScreenshot({ version, onOpenLightbox }) {
  const [imageError, setImageError] = useState(false);
  const screenshotPathRef = useRef(`${process.env.PUBLIC_URL}/screenshots/screenshot_${version}.png`);
  const placeholderPathRef = useRef(`${process.env.PUBLIC_URL}/screenshots/placeholder.png`);
  const [currentImageSrc, setCurrentImageSrc] = useState(screenshotPathRef.current);
  const errorHandledRef = useRef(false);
  
  // Reset image state when version changes
  useEffect(() => {
    screenshotPathRef.current = `${process.env.PUBLIC_URL}/screenshots/screenshot_${version}.png`;
    placeholderPathRef.current = `${process.env.PUBLIC_URL}/screenshots/placeholder.png`;
    setCurrentImageSrc(screenshotPathRef.current);
    setImageError(false);
    errorHandledRef.current = false;
  }, [version]);
  
  return (
    <div className={styles.versionScreenshot}>
      {imageError ? (
        <div className={styles.versionScreenshotPlaceholder}>
          Screenshot v{version}
        </div>
      ) : (
        <img 
          key={currentImageSrc}
          src={currentImageSrc}
          alt={`Version ${version} screenshot - click to enlarge`}
          className={styles.versionScreenshotImg}
          onError={() => {
            // Prevent infinite loops - only handle error once per image source
            if (errorHandledRef.current) return;
            errorHandledRef.current = true;
            
            if (currentImageSrc === screenshotPathRef.current) {
              // Version screenshot failed, try placeholder
              setCurrentImageSrc(placeholderPathRef.current);
              errorHandledRef.current = false; // Allow one more attempt for placeholder
            } else {
              // Placeholder also failed
              setImageError(true);
            }
          }}
          onLoad={() => {
            // Reset error state if image loads successfully
            errorHandledRef.current = false;
            if (imageError) {
              setImageError(false);
            }
          }}
          onClick={() => onOpenLightbox(currentImageSrc, version)}
          title="Click to view full size"
        />
      )}
    </div>
  );
}

/**
 * Single version entry component
 */
function VersionEntry({ version, date, sections, isLatest, onOpenLightbox }) {
  const [expanded, setExpanded] = useState(isLatest);
  
  const sectionKeys = Object.keys(sections);
  const totalChanges = sectionKeys.reduce(
    (sum, key) => sum + sections[key].length, 
    0
  );

  const macUrl = getVersionDownloadUrl(version, 'mac');
  const winUrl = getVersionDownloadUrl(version, 'win');
  
  return (
    <div className={`${styles.version} ${isLatest ? styles.latest : ''}`}>
      <button 
        className={styles.versionHeader}
        onClick={() => setExpanded(!expanded)}
        aria-expanded={expanded}
      >
        <div className={styles.versionInfo}>
          <span className={styles.versionNumber}>v{version}</span>
          {isLatest && <span className={styles.latestBadge}>Latest</span>}
          <span className={styles.versionDate}>{formatDate(date)}</span>
          {/* Download links inline with header */}
          <div className={styles.versionDownloadsInline}>
            <a href={macUrl} className={styles.downloadLink} download onClick={(e) => e.stopPropagation()}>
              <AppleIcon />
              macOS
            </a>
            <span className={styles.downloadSeparator}>|</span>
            <a href={winUrl} className={styles.downloadLink} download onClick={(e) => e.stopPropagation()}>
              <WindowsIcon />
              Windows
            </a>
          </div>
        </div>
        <div className={styles.versionMeta}>
          <span className={styles.changeCount}>{totalChanges} change{totalChanges !== 1 ? 's' : ''}</span>
          <span className={styles.expandIcon}>{expanded ? '−' : '+'}</span>
        </div>
      </button>
      
      {expanded && (
        <div className={styles.versionContent}>
          <div className={styles.versionContentLayout}>
            {/* Screenshot for this version */}
            <div className={styles.versionScreenshotWrapper}>
              <VersionScreenshot version={version} onOpenLightbox={onOpenLightbox} />
            </div>
            
            {/* Content alongside screenshot */}
            <div className={styles.versionContentText}>
              {sectionKeys.map(sectionKey => (
                <div key={sectionKey} className={styles.section}>
                  <h4 className={styles.sectionTitle}>
                    <span className={styles.sectionIcon}>{SECTION_ICONS[sectionKey] || '•'}</span>
                    {SECTION_LABELS[sectionKey] || sectionKey}
                  </h4>
                  <ul className={styles.changeList}>
                    {sections[sectionKey].map((item, index) => (
                      <li key={index} className={styles.changeItem}>{item}</li>
                    ))}
                  </ul>
                </div>
              ))}
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

/**
 * Changelog Component
 * Renders the full version history with lightbox support
 */
function Changelog() {
  const { versions } = changelog;
  const [lightbox, setLightbox] = useState({ open: false, src: '', version: '' });
  
  // Close lightbox on Escape key
  useEffect(() => {
    const handleEscape = (e) => {
      if (e.key === 'Escape') setLightbox({ open: false, src: '', version: '' });
    };
    if (lightbox.open) {
      document.addEventListener('keydown', handleEscape);
      document.body.style.overflow = 'hidden';
    }
    return () => {
      document.removeEventListener('keydown', handleEscape);
      document.body.style.overflow = '';
    };
  }, [lightbox.open]);
  
  const openLightbox = (src, version) => {
    setLightbox({ open: true, src, version });
  };
  
  const closeLightbox = () => {
    setLightbox({ open: false, src: '', version: '' });
  };
  
  if (!versions || versions.length === 0) {
    return null;
  }
  
  return (
    <section id="changelog" className={styles.container}>
      <h2 className={styles.title}>Version History</h2>
      
      <div className={styles.versions}>
        {versions.map((entry, index) => (
          <VersionEntry
            key={entry.version}
            version={entry.version}
            date={entry.date}
            sections={entry.sections}
            isLatest={index === 0}
            onOpenLightbox={openLightbox}
          />
        ))}
      </div>
      
      {/* Lightbox modal */}
      {lightbox.open && (
        <div className={styles.lightbox} onClick={closeLightbox}>
          <span className={styles.lightboxClose}>&times;</span>
          <img 
            src={lightbox.src}
            alt={`Version ${lightbox.version} screenshot`}
            className={styles.lightboxImage}
            onClick={(e) => e.stopPropagation()}
          />
        </div>
      )}
    </section>
  );
}

export default Changelog;

