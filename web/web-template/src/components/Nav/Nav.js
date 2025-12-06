// ============================================================================
// NAVIGATION COMPONENT
// Fixed top navigation bar with smooth scroll links
// ============================================================================

import React, { useState, useEffect } from 'react';
import config from '../../config';
import styles from './Nav.module.css';

/**
 * Navigation items configuration
 */
const NAV_ITEMS = [
  { id: 'overview', label: 'Overview' },
  { id: 'download', label: 'Download' },
  { id: 'features', label: 'Features' },
  { id: 'changelog', label: 'Version History' }
];

/**
 * Nav Component
 * Fixed navigation bar with smooth scrolling to sections
 */
function Nav() {
  const [activeSection, setActiveSection] = useState('');
  const [scrolled, setScrolled] = useState(false);

  // Handle scroll events for active section highlighting and nav background
  useEffect(() => {
    const handleScroll = () => {
      // Add background when scrolled
      setScrolled(window.scrollY > 20);

      // Determine active section
      const sections = NAV_ITEMS.map(item => document.getElementById(item.id));
      const scrollPosition = window.scrollY + 100;

      for (let i = sections.length - 1; i >= 0; i--) {
        const section = sections[i];
        if (section && section.offsetTop <= scrollPosition) {
          setActiveSection(NAV_ITEMS[i].id);
          break;
        }
      }
    };

    window.addEventListener('scroll', handleScroll);
    return () => window.removeEventListener('scroll', handleScroll);
  }, []);

  // Smooth scroll to section
  const scrollToSection = (e, id) => {
    e.preventDefault();
    const element = document.getElementById(id);
    if (element) {
      const navHeight = 60;
      const elementPosition = element.offsetTop - navHeight;
      window.scrollTo({
        top: elementPosition,
        behavior: 'smooth'
      });
    }
  };

  return (
    <nav className={`${styles.nav} ${scrolled ? styles.scrolled : ''}`}>
      <div className={styles.container}>
        <a href="#top" className={styles.logo} onClick={(e) => {
          e.preventDefault();
          window.scrollTo({ top: 0, behavior: 'smooth' });
        }}>
          {config.pluginName}
        </a>
        
        <ul className={styles.links}>
          {/* External link to parent plug-ins directory */}
          <li>
            <a href="/plug-ins/" className={styles.link}>
              Plug-Ins
            </a>
          </li>
          
          {/* Separator */}
          <li className={styles.separator} aria-hidden="true">|</li>
          
          {/* Page section links */}
          {NAV_ITEMS.map(item => (
            <li key={item.id}>
              <a
                href={`#${item.id}`}
                className={`${styles.link} ${activeSection === item.id ? styles.active : ''}`}
                onClick={(e) => scrollToSection(e, item.id)}
              >
                {item.label}
              </a>
            </li>
          ))}
        </ul>
      </div>
    </nav>
  );
}

export default Nav;

