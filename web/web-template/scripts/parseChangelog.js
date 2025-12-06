// ============================================================================
// CHANGELOG PARSER
// Converts CHANGELOG.md to JSON for the React app
// Run: node scripts/parseChangelog.js
// ============================================================================

const fs = require('fs');
const path = require('path');

/**
 * Parse a CHANGELOG.md file into structured JSON
 * @param {string} content - Raw markdown content
 * @returns {Array} Array of version objects
 */
function parseChangelog(content) {
  const versions = [];
  const lines = content.split('\n');
  
  let currentVersion = null;
  let currentSection = null;
  
  for (const line of lines) {
    // Match version header: ## [1.0.0] - 2025-12-05
    const versionMatch = line.match(/^## \[(\d+\.\d+\.\d+)\](?: - (\d{4}-\d{2}-\d{2}))?/);
    if (versionMatch) {
      if (currentVersion) {
        versions.push(currentVersion);
      }
      currentVersion = {
        version: versionMatch[1],
        date: versionMatch[2] || null,
        sections: {}
      };
      currentSection = null;
      continue;
    }
    
    // Match section header: ### Added, ### Fixed, ### Changed, etc.
    const sectionMatch = line.match(/^### (.+)/);
    if (sectionMatch && currentVersion) {
      currentSection = sectionMatch[1].toLowerCase();
      currentVersion.sections[currentSection] = [];
      continue;
    }
    
    // Match list item: - Some change
    const itemMatch = line.match(/^- (.+)/);
    if (itemMatch && currentVersion && currentSection) {
      currentVersion.sections[currentSection].push(itemMatch[1]);
    }
  }
  
  // Don't forget the last version
  if (currentVersion) {
    versions.push(currentVersion);
  }
  
  return versions;
}

// Main execution
// Source changelog comes from the repository root (single canonical changelog)
const changelogPath = path.join(__dirname, '..', '..', 'CHANGELOG.md');
const outputPath = path.join(__dirname, '..', 'src', 'config', 'changelog.json');

try {
  const content = fs.readFileSync(changelogPath, 'utf8');
  const versions = parseChangelog(content);
  
  const output = {
    generatedAt: new Date().toISOString(),
    versions: versions
  };
  
  fs.writeFileSync(outputPath, JSON.stringify(output, null, 2));
  console.log(`✓ Parsed ${versions.length} version(s) from CHANGELOG.md`);
  console.log(`✓ Output written to ${outputPath}`);
} catch (error) {
  console.error('Error parsing changelog:', error.message);
  process.exit(1);
}

