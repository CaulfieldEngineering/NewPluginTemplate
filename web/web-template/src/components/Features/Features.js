// ============================================================================
// FEATURES COMPONENT
// Plugin feature list
// ============================================================================

import React from 'react';
import content from '../../config/content.json';
import styles from './Features.module.css';

/**
 * Features Component
 * Displays plugin features in a clean grid
 */
function Features() {
  const features = content?.features || [];
  
  return (
    <section id="features" className={styles.container}>
      <h2 className={styles.title}>Features</h2>
      
      <div className={styles.list}>
        {features.map((feature, index) => (
          <div key={index} className={styles.featureRow}>
            <div className={styles.textBlock}>
              <h3 className={styles.featureTitle}>{feature.title}</h3>
              <p className={styles.featureDescription}>{feature.description}</p>
            </div>
          </div>
        ))}
      </div>
    </section>
  );
}

export default Features;

