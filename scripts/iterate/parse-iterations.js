#!/usr/bin/env node

/**
 * Iteration Log Parser
 *
 * Parses markdown iteration logs from docs/design/iterations/ and generates
 * structured JSON data for the iterations dashboard page.
 *
 * Input: docs/design/iterations/*.md files (following TEMPLATE.md format)
 * Output: tools/evals/public/data/iterations.json
 */

const fs = require('fs');
const path = require('path');

const ITERATIONS_DIR = path.join(__dirname, '../../docs/design/iterations');
const OUTPUT_FILE = path.join(__dirname, '../../tools/evals/public/data/iterations.json');

/**
 * Parse YAML frontmatter from markdown file
 * @param {string} content - File content
 * @returns {Object} Parsed frontmatter
 */
function parseFrontmatter(content) {
  const match = content.match(/^---\n([\s\S]+?)\n---/);
  if (!match) return null;

  const yaml = match[1];
  const data = {};

  yaml.split('\n').forEach(line => {
    const [key, ...valueParts] = line.split(':');
    if (key && valueParts.length > 0) {
      const value = valueParts.join(':').trim();
      // Remove quotes if present
      data[key.trim()] = value.replace(/^["']|["']$/g, '');
    }
  });

  return data;
}

/**
 * Extract sections from markdown content
 * @param {string} content - Markdown content after frontmatter
 * @returns {Object} Sections by heading
 */
function extractSections(content) {
  const sections = {};
  const lines = content.split('\n');
  let currentSection = null;
  let currentContent = [];

  for (const line of lines) {
    if (line.startsWith('## ')) {
      // Save previous section
      if (currentSection) {
        sections[currentSection] = currentContent.join('\n').trim();
      }
      // Start new section
      currentSection = line.replace(/^## /, '').trim();
      currentContent = [];
    } else if (currentSection) {
      currentContent.push(line);
    }
  }

  // Save last section
  if (currentSection) {
    sections[currentSection] = currentContent.join('\n').trim();
  }

  return sections;
}

/**
 * Parse lever information from "Lever Analysis" or "Proposed Changes" section
 * @param {Object} sections - Extracted sections
 * @returns {Object|null} Lever info {primary, oldValue, newValue, delta}
 */
function parseLever(sections) {
  const leverSection = sections['Lever Analysis'] || sections['Proposed Changes'] || '';

  // Try to extract primary lever and values
  const primaryMatch = leverSection.match(/Primary[:\s]+`?([a-zA-Z0-9_]+)`?/i);
  const oldValueMatch = leverSection.match(/Current value[:\s]+([0-9.]+)/i) ||
                        leverSection.match(/Before[:\s\n]+.*?([0-9.]+)f?/i);
  const newValueMatch = leverSection.match(/Proposed value[:\s]+([0-9.]+)/i) ||
                        leverSection.match(/After[:\s\n]+.*?([0-9.]+)f?/i);

  if (!primaryMatch) return null;

  const oldVal = oldValueMatch ? parseFloat(oldValueMatch[1]) : null;
  const newVal = newValueMatch ? parseFloat(newValueMatch[1]) : null;
  const delta = oldVal && newVal ? `${((newVal - oldVal) / oldVal * 100).toFixed(1)}%` : null;

  return {
    primary: primaryMatch[1],
    oldValue: oldVal,
    newValue: newVal,
    delta: delta
  };
}

/**
 * Parse target metric impact from "Result Metrics" or "Evaluation" section
 * @param {Object} sections - Extracted sections
 * @returns {Object|null} Metrics {target, maxRegression}
 */
function parseMetrics(sections) {
  const evalSection = sections['Evaluation'] || sections['Result Metrics'] || '';

  // Try to extract target metric delta
  const targetMatch = evalSection.match(/Target metric[:\s]+([+-]?[0-9.]+)%/i) ||
                      evalSection.match(/Delta[:\s]+([+-]?[0-9.]+)%/i);

  // Try to extract max regression
  const regressionMatch = evalSection.match(/Max regression[:\s]+([+-]?[0-9.]+)%/i) ||
                          evalSection.match(/regressions?\)[:\s]+([+-]?[0-9.]+)%/i);

  if (!targetMatch) return null;

  return {
    target: {
      delta: `${targetMatch[1]}%`
    },
    maxRegression: regressionMatch ? `${regressionMatch[1]}%` : null
  };
}

/**
 * Parse a single iteration file
 * @param {string} filePath - Path to markdown file
 * @returns {Object|null} Parsed iteration data
 */
function parseIterationFile(filePath) {
  try {
    const content = fs.readFileSync(filePath, 'utf8');

    // Parse frontmatter
    const frontmatter = parseFrontmatter(content);
    if (!frontmatter || !frontmatter.iteration_id) {
      console.warn(`Skipping ${path.basename(filePath)}: no valid frontmatter`);
      return null;
    }

    // Extract sections
    const bodyContent = content.replace(/^---[\s\S]+?---\n/, '');
    const sections = extractSections(bodyContent);

    // Parse lever and metrics
    const lever = parseLever(sections);
    const metrics = parseMetrics(sections);

    return {
      id: frontmatter.iteration_id,
      goal: frontmatter.goal || 'No goal specified',
      status: frontmatter.status || 'unknown',
      startedAt: frontmatter.started_at || null,
      completedAt: frontmatter.completed_at || null,
      branch: frontmatter.branch || null,
      commit: frontmatter.commit || null,
      pr: frontmatter.pr || null,
      lever: lever,
      metrics: metrics,
      sections: {
        goal: sections['Goal'] || '',
        decision: sections['Decision'] || '',
        notes: sections['Notes'] || ''
      }
    };
  } catch (error) {
    console.error(`Error parsing ${filePath}:`, error.message);
    return null;
  }
}

/**
 * Main function
 */
function main() {
  console.log('Parsing iteration logs from', ITERATIONS_DIR);

  // Find all markdown files (excluding README and TEMPLATE)
  const files = fs.readdirSync(ITERATIONS_DIR)
    .filter(f => f.endsWith('.md') && !f.match(/^(README|TEMPLATE)/i))
    .map(f => path.join(ITERATIONS_DIR, f));

  if (files.length === 0) {
    console.log('No iteration logs found. Generating empty iterations.json');
    const emptyData = {
      iterations: [],
      summary: {
        total: 0,
        successful: 0,
        failed: 0,
        successRate: 0
      },
      generatedAt: new Date().toISOString()
    };

    // Ensure output directory exists
    const outputDir = path.dirname(OUTPUT_FILE);
    if (!fs.existsSync(outputDir)) {
      fs.mkdirSync(outputDir, { recursive: true });
    }

    fs.writeFileSync(OUTPUT_FILE, JSON.stringify(emptyData, null, 2));
    console.log(`Empty iterations.json written to ${OUTPUT_FILE}`);
    return;
  }

  console.log(`Found ${files.length} iteration log(s)`);

  // Parse all files
  const iterations = files
    .map(parseIterationFile)
    .filter(Boolean) // Remove nulls
    .sort((a, b) => {
      // Sort by ID descending (newest first)
      return b.id.localeCompare(a.id);
    });

  // Calculate summary stats
  const successful = iterations.filter(i => i.status === 'success').length;
  const failed = iterations.filter(i => i.status === 'failed').length;

  const summary = {
    total: iterations.length,
    successful: successful,
    failed: failed,
    successRate: iterations.length > 0 ? successful / iterations.length : 0
  };

  const output = {
    iterations: iterations,
    summary: summary,
    generatedAt: new Date().toISOString()
  };

  // Ensure output directory exists
  const outputDir = path.dirname(OUTPUT_FILE);
  if (!fs.existsSync(outputDir)) {
    fs.mkdirSync(outputDir, { recursive: true });
  }

  // Write output
  fs.writeFileSync(OUTPUT_FILE, JSON.stringify(output, null, 2));
  console.log(`Parsed ${iterations.length} iterations`);
  console.log(`Success rate: ${(summary.successRate * 100).toFixed(1)}%`);
  console.log(`Output written to ${OUTPUT_FILE}`);
}

// Run if called directly
if (require.main === module) {
  main();
}

module.exports = { parseIterationFile, parseFrontmatter, extractSections };
