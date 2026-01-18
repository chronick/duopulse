#!/usr/bin/env node
/**
 * generate-iteration-id.js
 *
 * Generates a unique iteration ID based on date and sequence number.
 * Format: YYYY-MM-DD-NNN (e.g., 2026-01-18-001)
 *
 * Usage:
 *   node generate-iteration-id.js
 *
 * Output:
 *   Prints the next available iteration ID to stdout
 */

const fs = require('fs');
const path = require('path');

const ITERATIONS_DIR = path.join(__dirname, '../../docs/design/iterations');

function getToday() {
  const now = new Date();
  const year = now.getFullYear();
  const month = String(now.getMonth() + 1).padStart(2, '0');
  const day = String(now.getDate()).padStart(2, '0');
  return `${year}-${month}-${day}`;
}

function getExistingIterations(datePrefix) {
  if (!fs.existsSync(ITERATIONS_DIR)) {
    return [];
  }

  const files = fs.readdirSync(ITERATIONS_DIR);
  return files
    .filter(f => f.startsWith(datePrefix) && f.endsWith('.md'))
    .map(f => {
      const match = f.match(/(\d{4}-\d{2}-\d{2})-(\d{3})\.md$/);
      if (match) {
        return parseInt(match[2], 10);
      }
      return 0;
    })
    .filter(n => n > 0);
}

function main() {
  const today = getToday();
  const existing = getExistingIterations(today);

  let nextSeq = 1;
  if (existing.length > 0) {
    nextSeq = Math.max(...existing) + 1;
  }

  const iterationId = `${today}-${String(nextSeq).padStart(3, '0')}`;
  console.log(iterationId);
}

main();
