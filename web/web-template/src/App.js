// ============================================================================
// MAIN APPLICATION COMPONENT
// ============================================================================

import React, { useEffect } from 'react';
import Nav from './components/Nav';
import Header from './components/Header';
import Downloads from './components/Downloads';
import Features from './components/Features';
import Changelog from './components/Changelog';
import config from './config';
import './App.css';

/**
 * App Component
 * Root component that orchestrates the application layout
 */
function App() {
  // Set document title to plugin name
  useEffect(() => {
    document.title = config.pluginName;
  }, []);

  return (
    <div className="app">
      <Nav />
      <main className="app__main">
        <div className="app__content">
          <Header />
          <Downloads />
          <Features />
          <Changelog />
        </div>
      </main>
    </div>
  );
}

export default App;
