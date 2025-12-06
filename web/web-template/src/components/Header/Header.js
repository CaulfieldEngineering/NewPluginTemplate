// ============================================================================
// HEADER COMPONENT
// Displays the plugin name, company branding, and screenshot with lightbox
// ============================================================================

import React, { useState, useEffect, useRef } from 'react';
import config from '../../config';
import content from '../../config/content.json';
import styles from './Header.module.css';

/**
 * Header Component
 * Displays the plugin name, company attribution, and plugin screenshot
 * Click on screenshot to open full-size lightbox
 */
function Header() {
  const { pluginName, companyName, version } = config;
  const [imageError, setImageError] = useState(false);
  const [lightboxOpen, setLightboxOpen] = useState(false);
  
  // Product overview copy sits directly under the title for quicker context
  const description = content?.overview?.description || '';
  
  // Screenshot paths
  const screenshotPath = `${process.env.PUBLIC_URL}/screenshots/screenshot_${version}.png`;
  const placeholderPath = `${process.env.PUBLIC_URL}/screenshots/placeholder.png`;
  
  // Track which image we're currently trying to load
  const currentAttemptRef = useRef('screenshot');
  const [currentImageSrc, setCurrentImageSrc] = useState(screenshotPath);
  const errorHandledRef = useRef(false);
  
  // Reset image state when version changes
  useEffect(() => {
    currentAttemptRef.current = 'screenshot';
    setCurrentImageSrc(screenshotPath);
    setImageError(false);
    errorHandledRef.current = false;
  }, [version, screenshotPath]);
  
  // Close lightbox on Escape key
  useEffect(() => {
    const handleEscape = (e) => {
      if (e.key === 'Escape') setLightboxOpen(false);
    };
    if (lightboxOpen) {
      document.addEventListener('keydown', handleEscape);
      document.body.style.overflow = 'hidden';
    }
    return () => {
      document.removeEventListener('keydown', handleEscape);
      document.body.style.overflow = '';
    };
  }, [lightboxOpen]);
  
  return (
    <header id="overview" className={styles.container}>
      <h1 className={styles.title}>{pluginName}</h1>
      <p className={styles.subtitle}>by {companyName}</p>
      <p className={styles.description}>{description}</p>
      
      <div className={styles.screenshotWrapper}>
        {imageError ? (
          <div className={styles.placeholder}>
            <span className={styles.placeholderText}>Screenshot v{version}</span>
          </div>
        ) : (
          <div className={styles.imageContainer}>
            <img 
              key={currentImageSrc}
              src={currentImageSrc}
              alt={`${pluginName} v${version} user interface - click to enlarge`}
              className={styles.screenshot}
              onError={() => {
                // Prevent infinite loops - only handle error once per image source
                if (errorHandledRef.current) return;
                errorHandledRef.current = true;
                
                if (currentAttemptRef.current === 'screenshot') {
                  // Version screenshot failed, try placeholder
                  currentAttemptRef.current = 'placeholder';
                  setCurrentImageSrc(placeholderPath);
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
              onClick={() => setLightboxOpen(true)}
              title="Click to view full size"
            />
          </div>
        )}
      </div>
      
      {/* Lightbox modal */}
      {lightboxOpen && (
        <div 
          className={styles.lightbox} 
          onClick={() => setLightboxOpen(false)}
        >
          <span className={styles.lightboxClose}>&times;</span>
          <img 
            src={currentImageSrc}
            alt={`${pluginName} v${version} user interface`}
            className={styles.lightboxImage}
            onClick={(e) => e.stopPropagation()}
          />
        </div>
      )}
    </header>
  );
}

export default Header;
