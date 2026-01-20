/**
 * Iterations Page JavaScript
 * Loads iteration data, renders timeline chart, and displays iteration cards
 */

import { renderTimelineChart } from './timeline.js';

let iterationsData = null;
let metricsHistory = null;
let filteredIterations = [];

/**
 * Initialize the iterations page
 */
async function init() {
  try {
    // Load data
    await Promise.all([
      loadIterations(),
      loadMetricsHistory()
    ]);

    // Render components
    renderSummaryStats();
    renderTimeline();
    renderIterationCards();

    // Setup event listeners
    setupEventListeners();

    // Handle URL hash (deep linking to specific iteration)
    handleURLHash();
  } catch (error) {
    console.error('Error initializing iterations page:', error);
  }
}

/**
 * Load iterations data from JSON
 */
async function loadIterations() {
  try {
    const response = await fetch('../data/iterations.json');
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }
    iterationsData = await response.json();
    filteredIterations = iterationsData.iterations || [];
    console.log('Loaded', filteredIterations.length, 'iterations');
  } catch (error) {
    console.warn('Could not load iterations.json:', error.message);
    iterationsData = { iterations: [], summary: { total: 0, successful: 0, failed: 0, successRate: 0 } };
    filteredIterations = [];
  }
}

/**
 * Load metrics history for timeline chart
 */
async function loadMetricsHistory() {
  try {
    const response = await fetch('../data/metrics-history.json');
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }
    metricsHistory = await response.json();
    console.log('Loaded metrics history:', metricsHistory.timeline?.length || 0, 'data points');
  } catch (error) {
    console.warn('Could not load metrics-history.json:', error.message);
    metricsHistory = null;
  }
}

/**
 * Render summary statistics
 */
function renderSummaryStats() {
  const summary = iterationsData.summary;

  document.getElementById('stat-total').textContent = summary.total;
  document.getElementById('stat-success').textContent = summary.successful;
  document.getElementById('stat-failed').textContent = summary.failed;
  document.getElementById('stat-rate').textContent = `${(summary.successRate * 100).toFixed(1)}%`;
}

/**
 * Render timeline chart
 */
function renderTimeline() {
  const container = document.getElementById('timeline-chart');

  if (!metricsHistory || !metricsHistory.timeline || metricsHistory.timeline.length < 2) {
    container.innerHTML = `
      <div class="no-timeline-data">
        <p>Not enough historical data to display timeline</p>
        <p class="hint">Metrics history is generated from baseline tags in git</p>
      </div>
    `;
    return;
  }

  // Render using existing timeline component
  const chartHTML = renderTimelineChart(metricsHistory, 1000, 300);
  container.innerHTML = chartHTML;

  // Add click handlers to navigate to iterations
  // (Would need to correlate timeline points with iteration IDs based on commit hash)
}

/**
 * Render iteration cards
 */
function renderIterationCards() {
  const container = document.getElementById('iterations-list');

  if (filteredIterations.length === 0) {
    container.innerHTML = `
      <div class="no-iterations">
        <p>No iterations found.</p>
        <p class="hint">Run <code>/iterate "goal"</code> to start hill-climbing.</p>
      </div>
    `;
    return;
  }

  const cards = filteredIterations.map(createIterationCard).join('');
  container.innerHTML = cards;

  // Add event listeners to cards
  document.querySelectorAll('.card-header').forEach(header => {
    header.addEventListener('click', toggleCardDetails);
  });

  document.querySelectorAll('.toggle-details').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      toggleCardDetails.call(btn.closest('.card-header'), e);
    });
  });
}

/**
 * Create HTML for a single iteration card
 * @param {Object} iteration - Iteration data
 * @returns {string} HTML string
 */
function createIterationCard(iteration) {
  const statusClass = iteration.status || 'unknown';
  const statusLabel = iteration.status || 'Unknown';

  // Format dates
  const startDate = iteration.startedAt ? new Date(iteration.startedAt).toLocaleString() : 'N/A';
  const endDate = iteration.completedAt ? new Date(iteration.completedAt).toLocaleString() : 'In progress';

  // Lever info
  const leverInfo = iteration.lever ? `
    <div class="card-lever">
      ${iteration.lever.primary}: ${iteration.lever.oldValue !== null ? iteration.lever.oldValue.toFixed(2) : '?'}
      → ${iteration.lever.newValue !== null ? iteration.lever.newValue.toFixed(2) : '?'}
      ${iteration.lever.delta ? `(${iteration.lever.delta})` : ''}
    </div>
  ` : '';

  // Metrics info
  const metricsInfo = iteration.metrics ? `
    <div class="card-metrics">
      ${iteration.metrics.target ? `
        <div class="metric-item">
          <div class="metric-label">Target Metric</div>
          <div class="metric-value ${iteration.metrics.target.delta?.startsWith('+') ? 'positive' : (iteration.metrics.target.delta?.startsWith('-') ? 'negative' : '')}">
            ${iteration.metrics.target.delta}
          </div>
        </div>
      ` : ''}
      ${iteration.metrics.maxRegression ? `
        <div class="metric-item">
          <div class="metric-label">Max Regression</div>
          <div class="metric-value ${iteration.metrics.maxRegression.startsWith('-') ? 'negative' : 'positive'}">
            ${iteration.metrics.maxRegression}
          </div>
        </div>
      ` : ''}
    </div>
  ` : '';

  // Links
  const links = [];
  if (iteration.pr && iteration.pr !== 'null') {
    links.push(`<a href="https://github.com/chronick/duopulse/pull/${iteration.pr.replace('#', '')}" class="card-link" target="_blank">View PR ${iteration.pr}</a>`);
  }
  if (iteration.commit && iteration.commit !== 'null') {
    links.push(`<a href="https://github.com/chronick/duopulse/commit/${iteration.commit}" class="card-link" target="_blank"><code>${iteration.commit.substring(0, 7)}</code></a>`);
  }
  if (iteration.branch && iteration.branch !== 'null') {
    links.push(`<span class="card-link"><code>${iteration.branch}</code></span>`);
  }

  return `
    <div class="iteration-card ${statusClass}" id="${iteration.id}">
      <div class="card-header">
        <div class="card-title-row">
          <div class="card-id">#${iteration.id}</div>
          <div class="card-status ${statusClass}">${statusLabel}</div>
        </div>
        <h3 class="card-goal">${escapeHTML(iteration.goal)}</h3>
        ${leverInfo}
        ${metricsInfo}
        ${links.length > 0 ? `<div class="card-links">${links.join('')}</div>` : ''}
      </div>

      <div class="card-details" id="details-${iteration.id}">
        ${createCardDetails(iteration)}
      </div>

      <button class="toggle-details">
        <span class="toggle-text">Show Details</span>
        <span class="toggle-icon">▼</span>
      </button>
    </div>
  `;
}

/**
 * Create detailed section content for card
 * @param {Object} iteration - Iteration data
 * @returns {string} HTML string
 */
function createCardDetails(iteration) {
  const sections = [];

  if (iteration.sections?.goal) {
    sections.push(`
      <div class="card-details-section">
        <h4>Goal</h4>
        <pre>${escapeHTML(iteration.sections.goal)}</pre>
      </div>
    `);
  }

  if (iteration.sections?.decision) {
    sections.push(`
      <div class="card-details-section">
        <h4>Decision</h4>
        <pre>${escapeHTML(iteration.sections.decision)}</pre>
      </div>
    `);
  }

  if (iteration.sections?.notes) {
    sections.push(`
      <div class="card-details-section">
        <h4>Notes</h4>
        <pre>${escapeHTML(iteration.sections.notes)}</pre>
      </div>
    `);
  }

  if (sections.length === 0) {
    return '<p>No additional details available</p>';
  }

  return sections.join('');
}

/**
 * Toggle card details expansion
 * @param {Event} e - Click event
 */
function toggleCardDetails(e) {
  const card = e.currentTarget.closest('.iteration-card');
  const details = card.querySelector('.card-details');
  const toggleBtn = card.querySelector('.toggle-details');
  const toggleText = toggleBtn.querySelector('.toggle-text');
  const toggleIcon = toggleBtn.querySelector('.toggle-icon');

  if (details.classList.contains('expanded')) {
    details.classList.remove('expanded');
    toggleText.textContent = 'Show Details';
    toggleIcon.classList.remove('expanded');
  } else {
    details.classList.add('expanded');
    toggleText.textContent = 'Hide Details';
    toggleIcon.classList.add('expanded');
  }
}

/**
 * Setup event listeners for filters and search
 */
function setupEventListeners() {
  // Search
  document.getElementById('search-iterations').addEventListener('input', (e) => {
    applyFilters();
  });

  // Status filter
  document.getElementById('filter-status').addEventListener('change', (e) => {
    applyFilters();
  });

  // Sort
  document.getElementById('sort-iterations').addEventListener('change', (e) => {
    applySort();
  });
}

/**
 * Apply search and status filters
 */
function applyFilters() {
  const searchTerm = document.getElementById('search-iterations').value.toLowerCase();
  const statusFilter = document.getElementById('filter-status').value;

  filteredIterations = iterationsData.iterations.filter(iteration => {
    // Status filter
    if (statusFilter !== 'all' && iteration.status !== statusFilter) {
      return false;
    }

    // Search filter
    if (searchTerm) {
      const searchableText = [
        iteration.id,
        iteration.goal,
        iteration.lever?.primary,
        iteration.sections?.goal,
        iteration.sections?.decision,
        iteration.sections?.notes
      ].filter(Boolean).join(' ').toLowerCase();

      if (!searchableText.includes(searchTerm)) {
        return false;
      }
    }

    return true;
  });

  applySort();
}

/**
 * Apply sorting to filtered iterations
 */
function applySort() {
  const sortBy = document.getElementById('sort-iterations').value;

  switch (sortBy) {
    case 'oldest':
      filteredIterations.sort((a, b) => a.id.localeCompare(b.id));
      break;
    case 'impact':
      filteredIterations.sort((a, b) => {
        const deltaA = parseFloat(a.metrics?.target?.delta?.replace('%', '') || '0');
        const deltaB = parseFloat(b.metrics?.target?.delta?.replace('%', '') || '0');
        return deltaB - deltaA; // Highest first
      });
      break;
    case 'newest':
    default:
      filteredIterations.sort((a, b) => b.id.localeCompare(a.id));
      break;
  }

  renderIterationCards();
}

/**
 * Handle URL hash for deep linking to specific iteration
 */
function handleURLHash() {
  if (window.location.hash) {
    const iterationId = window.location.hash.substring(1);
    const card = document.getElementById(iterationId);
    if (card) {
      setTimeout(() => {
        card.scrollIntoView({ behavior: 'smooth', block: 'center' });
        card.classList.add('highlight');
        setTimeout(() => card.classList.remove('highlight'), 2000);
      }, 100);
    }
  }
}

/**
 * Escape HTML to prevent XSS
 * @param {string} text - Text to escape
 * @returns {string} Escaped text
 */
function escapeHTML(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

// Initialize on DOM load
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
