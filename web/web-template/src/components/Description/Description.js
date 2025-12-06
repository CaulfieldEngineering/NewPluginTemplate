// ============================================================================
// DESCRIPTION COMPONENT
// Plugin description and overview
// ============================================================================

import React from 'react';
import content from '../../config/content.json';
import styles from './Description.module.css';

/**
 * Description Component
 * Displays the plugin overview and description
 * Content is pulled from web-content/overview.md via the ingest script
 */
function Description() {
  // Pull description from ingested content
  const description = content?.overview?.description || '';
  
  return (
    <section id="description" className={styles.container}>
      <p className={styles.text}>
        {description}
      </p>
    </section>
  );
}

export default Description;

