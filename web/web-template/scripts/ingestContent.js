// ============================================================================
// CONTENT INGEST SCRIPT
// Pulls product-specific markdown content into the template as JSON
// Run: node scripts/ingestContent.js
// ============================================================================

const fs = require('fs');
const path = require('path');

// Paths
const templateRoot = path.join(__dirname, '..');
const repoRoot = path.join(templateRoot, '..', '..');
const contentRoot = path.join(templateRoot, '..', 'web-content');

const readmePath = path.join(repoRoot, 'README.md');
const changelogPath = path.join(repoRoot, 'CHANGELOG.md');
const screenshotsSrc = path.join(contentRoot, 'assets', 'screenshots');
const screenshotsDest = path.join(templateRoot, 'public', 'screenshots');
const contentJsonPath = path.join(templateRoot, 'src', 'config', 'content.json');
const changelogJsonPath = path.join(templateRoot, 'src', 'config', 'changelog.json');

// Utilities
function ensureDir(dir) {
  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true });
  }
}

// ============================================================================
// README SECTION EXTRACTION
// Parses README.md and extracts content under specified top-level headers
// ============================================================================

// Extract content under a specific top-level header (# Header)
// Returns the raw content between this header and the next top-level header
function extractSection(md, sectionName) {
  const lines = md.split('\n');
  let capturing = false;
  let sectionLines = [];

  for (const line of lines) {
    // Check if this is a top-level header (# Header)
    const headerMatch = line.match(/^# (.+)/);

    if (headerMatch) {
      // If we were capturing, stop now (we hit the next section)
      if (capturing) {
        break;
      }
      // Check if this is the section we're looking for (case-insensitive)
      if (headerMatch[1].trim().toLowerCase() === sectionName.toLowerCase()) {
        capturing = true;
      }
      continue;
    }

    // Capture content if we're in the target section
    if (capturing) {
      sectionLines.push(line);
    }
  }

  return sectionLines.join('\n').trim();
}

// Parse overview section -> { title, description }
function parseOverview(md) {
  const overviewContent = extractSection(md, 'Overview');
  const lines = overviewContent.split('\n').map(line => line.trim());
  const description = lines
    .filter(line => line && !line.startsWith('#'))
    .join(' ')
    .trim();
  return { title: 'Overview', description };
}

// Parse features section -> [{ title, description }]
// Expected format per line: "- Title: Description"
function parseFeatures(md) {
  const featuresContent = extractSection(md, 'Features');
  const lines = featuresContent.split('\n');
  const features = [];

  for (const rawLine of lines) {
    const line = rawLine.trim();
    if (!line.startsWith('-') && !line.startsWith('*')) continue;
    const text = line.replace(/^[-*]\s*/, '');
    const [titlePart, ...rest] = text.split(':');
    const title = (titlePart || '').trim();
    const description = rest.join(':').trim();
    if (title || description) {
      features.push({ title, description });
    }
  }

  return features;
}

// Parse changelog markdown -> { generatedAt, versions }
function parseChangelog(md) {
  const versions = [];
  const lines = md.split('\n');

  let currentVersion = null;
  let currentSection = null;

  for (const line of lines) {
    const versionMatch = line.match(/^## \[(\d+\.\d+\.\d+)\](?: - (\d{4}-\d{2}-\d{2}))?/);
    if (versionMatch) {
      if (currentVersion) versions.push(currentVersion);
      currentVersion = {
        version: versionMatch[1],
        date: versionMatch[2] || null,
        sections: {}
      };
      currentSection = null;
      continue;
    }

    const sectionMatch = line.match(/^### (.+)/);
    if (sectionMatch && currentVersion) {
      currentSection = sectionMatch[1].toLowerCase();
      currentVersion.sections[currentSection] = [];
      continue;
    }

    const itemMatch = line.match(/^- (.+)/);
    if (itemMatch && currentVersion && currentSection) {
      currentVersion.sections[currentSection].push(itemMatch[1]);
    }
  }

  if (currentVersion) versions.push(currentVersion);

  return {
    generatedAt: new Date().toISOString(),
    versions
  };
}

// Copy screenshots from content to template public dir
function copyScreenshots() {
  if (!fs.existsSync(screenshotsSrc)) return;
  ensureDir(screenshotsDest);
  const files = fs.readdirSync(screenshotsSrc);
  for (const file of files) {
    if (file.startsWith('.')) continue; // skip .gitkeep or hidden
    const src = path.join(screenshotsSrc, file);
    const dest = path.join(screenshotsDest, file);
    const stat = fs.statSync(src);
    if (stat.isFile()) {
      fs.copyFileSync(src, dest);
    }
  }
}

// Main
try {
  // Read README.md and extract Overview/Features sections
  const readmeMd = fs.existsSync(readmePath) ? fs.readFileSync(readmePath, 'utf8') : '';
  const overview = parseOverview(readmeMd);
  const features = parseFeatures(readmeMd);

  // Changelog
  const changelogMd = fs.existsSync(changelogPath) ? fs.readFileSync(changelogPath, 'utf8') : '';
  const changelog = changelogMd ? parseChangelog(changelogMd) : { generatedAt: new Date().toISOString(), versions: [] };

  // Write content JSON
  ensureDir(path.dirname(contentJsonPath));
  const content = {
    generatedAt: new Date().toISOString(),
    overview,
    features
  };
  fs.writeFileSync(contentJsonPath, JSON.stringify(content, null, 2));

  // Write changelog JSON
  ensureDir(path.dirname(changelogJsonPath));
  fs.writeFileSync(changelogJsonPath, JSON.stringify(changelog, null, 2));

  // Copy screenshots
  copyScreenshots();

  console.log('✓ Content ingested from README.md');
  console.log(`  - Overview (# Overview section) -> ${contentJsonPath}`);
  console.log(`  - Features (# Features section) -> ${contentJsonPath}`);
  console.log(`  - Changelog -> ${changelogJsonPath}`);
  console.log(`  - Screenshots -> ${screenshotsDest}`);
} catch (error) {
  console.error('Error ingesting content:', error.message);
  process.exit(1);
}

