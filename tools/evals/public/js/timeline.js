/**
 * Metrics Timeline Chart
 * Visualizes Pentagon metric progress over time using baseline tag history
 */

/**
 * Render metrics timeline chart as SVG
 * @param {Object} history - Metrics history data with timeline array
 * @param {number} width - Chart width
 * @param {number} height - Chart height
 * @returns {string} SVG HTML string
 */
export function renderTimelineChart(history, width = 800, height = 300) {
  if (!history || !history.timeline || history.timeline.length < 2) {
    return `
      <div class="no-timeline-data">
        <p>Not enough historical data to display timeline</p>
        <p class="hint">Run evaluations and tag baselines to track progress over time</p>
      </div>
    `;
  }

  const timeline = history.timeline;
  const padding = { top: 20, right: 60, bottom: 50, left: 60 };
  const chartWidth = width - padding.left - padding.right;
  const chartHeight = height - padding.top - padding.bottom;

  // Generate unique ID for this chart instance
  const chartId = `timeline-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

  // Extract alignment scores (primary metric)
  const alignmentScores = timeline.map(d => d.overallAlignment * 100); // Convert to percentage

  // Scale functions
  const minScore = Math.min(...alignmentScores);
  const maxScore = Math.max(...alignmentScores);
  const scoreRange = maxScore - minScore || 10; // Avoid division by zero
  const yMin = Math.max(0, minScore - scoreRange * 0.2);
  const yMax = Math.min(100, maxScore + scoreRange * 0.2);

  const xScale = (i) => padding.left + (i / (timeline.length - 1)) * chartWidth;
  const yScale = (score) => padding.top + chartHeight - ((score - yMin) / (yMax - yMin)) * chartHeight;

  // Generate path data for line chart
  const pathData = timeline.map((d, i) => {
    const x = xScale(i);
    const y = yScale(d.overallAlignment * 100);
    return i === 0 ? `M ${x},${y}` : `L ${x},${y}`;
  }).join(' ');

  // Generate grid lines (horizontal)
  const gridLines = [];
  const numGridLines = 5;
  for (let i = 0; i <= numGridLines; i++) {
    const score = yMin + (yMax - yMin) * (i / numGridLines);
    const y = yScale(score);
    gridLines.push(`
      <line x1="${padding.left}" y1="${y}" x2="${padding.left + chartWidth}" y2="${y}"
            stroke="#2a2a2a" stroke-width="1" stroke-dasharray="2,2" />
      <text x="${padding.left - 10}" y="${y + 4}" fill="#666" font-size="11" text-anchor="end">
        ${score.toFixed(0)}%
      </text>
    `);
  }

  // Generate data points with data attributes for tooltip
  const dataPoints = timeline.map((d, i) => {
    const x = xScale(i);
    const y = yScale(d.overallAlignment * 100);
    const color = d.alignmentStatus === 'GOOD' ? '#44ff44'
      : d.alignmentStatus === 'FAIR' ? '#ffaa44' : '#ff4444';

    return `
      <circle cx="${x}" cy="${y}" r="5" fill="${color}" stroke="#1a1a1a" stroke-width="2"
              class="timeline-point"
              data-index="${i}"
              data-tag="${d.tag || ''}"
              data-hash="${d.hash || ''}"
              data-fullhash="${d.fullHash || ''}"
              data-date="${d.date || ''}"
              data-score="${(d.overallAlignment * 100).toFixed(1)}"
              data-status="${d.alignmentStatus || ''}"
              style="cursor: pointer;" />
    `;
  }).join('');

  // Generate x-axis date labels (show ~5 evenly spaced dates)
  const numDateLabels = Math.min(5, timeline.length);
  const dateLabels = [];
  for (let i = 0; i < numDateLabels; i++) {
    const idx = Math.floor(i * (timeline.length - 1) / (numDateLabels - 1));
    const d = timeline[idx];
    const x = xScale(idx);
    const y = height - padding.bottom + 20;
    const date = new Date(d.date);
    const label = `${date.getMonth() + 1}/${date.getDate()}`;

    dateLabels.push(`
      <text x="${x}" y="${y}" fill="#888" font-size="11" text-anchor="middle">
        ${label}
      </text>
    `);
  }

  // Chart title and axis labels
  const svg = `
    <svg width="${width}" height="${height}" class="timeline-chart" id="${chartId}">
      <!-- Grid lines -->
      ${gridLines.join('')}

      <!-- Main line -->
      <path d="${pathData}" fill="none" stroke="#00d4ff" stroke-width="2" class="timeline-path" />

      <!-- Data points -->
      ${dataPoints}

      <!-- X-axis -->
      <line x1="${padding.left}" y1="${padding.top + chartHeight}"
            x2="${padding.left + chartWidth}" y2="${padding.top + chartHeight}"
            stroke="#444" stroke-width="2" />

      <!-- Y-axis -->
      <line x1="${padding.left}" y1="${padding.top}"
            x2="${padding.left}" y2="${padding.top + chartHeight}"
            stroke="#444" stroke-width="2" />

      <!-- X-axis date labels -->
      ${dateLabels.join('')}

      <!-- Y-axis title -->
      <text x="${padding.left - 45}" y="${padding.top + chartHeight / 2}"
            fill="#aaa" font-size="12" text-anchor="middle"
            transform="rotate(-90 ${padding.left - 45} ${padding.top + chartHeight / 2})">
        Overall Alignment Score (%)
      </text>

      <!-- Chart title -->
      <text x="${width / 2}" y="15" fill="#fff" font-size="14" text-anchor="middle" font-weight="600">
        Pentagon Metric Progress Over Time
      </text>
    </svg>
  `;

  // Add summary stats
  const firstScore = alignmentScores[0];
  const lastScore = alignmentScores[alignmentScores.length - 1];
  const improvement = lastScore - firstScore;
  const improvementColor = improvement >= 0 ? '#44ff44' : '#ff4444';

  const summary = `
    <div class="timeline-summary">
      <div class="timeline-stat">
        <span class="stat-label">Starting Score:</span>
        <span class="stat-value">${firstScore.toFixed(1)}%</span>
      </div>
      <div class="timeline-stat">
        <span class="stat-label">Current Score:</span>
        <span class="stat-value">${lastScore.toFixed(1)}%</span>
      </div>
      <div class="timeline-stat">
        <span class="stat-label">Change:</span>
        <span class="stat-value" style="color: ${improvementColor}">
          ${improvement >= 0 ? '+' : ''}${improvement.toFixed(1)}%
        </span>
      </div>
      <div class="timeline-stat">
        <span class="stat-label">Data Points:</span>
        <span class="stat-value">${timeline.length}</span>
      </div>
    </div>
  `;

  // Tooltip HTML
  const tooltip = `
    <div class="timeline-tooltip" id="tooltip-${chartId}" style="
      position: absolute;
      display: none;
      background: rgba(0, 0, 0, 0.95);
      border: 1px solid #444;
      border-radius: 6px;
      padding: 12px 16px;
      font-size: 12px;
      pointer-events: none;
      z-index: 100;
      min-width: 180px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.5);
    "></div>
  `;

  // Interactive script for tooltip
  const script = `
    <script type="module">
      (function() {
        const chart = document.getElementById('${chartId}');
        const tooltip = document.getElementById('tooltip-${chartId}');
        if (!chart || !tooltip) return;

        chart.querySelectorAll('.timeline-point').forEach(point => {
          point.addEventListener('mouseenter', (e) => {
            const tag = point.dataset.tag;
            const hash = point.dataset.hash;
            const fullHash = point.dataset.fullhash;
            const date = point.dataset.date;
            const score = point.dataset.score;
            const status = point.dataset.status;

            const statusColor = status === 'GOOD' ? '#44ff44' : status === 'FAIR' ? '#ffaa44' : '#ff4444';
            const formattedDate = new Date(date).toLocaleDateString('en-US', {
              year: 'numeric', month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit'
            });

            const githubUrl = fullHash ? \`https://github.com/chronick/duopulse/commit/\${fullHash}\` : '';

            tooltip.innerHTML = \`
              <div style="margin-bottom: 8px;">
                <div style="color: #fff; font-weight: 600; font-size: 13px;">\${tag || 'Commit'}</div>
                <div style="color: #888; font-size: 11px;">\${formattedDate}</div>
              </div>
              <div style="margin-bottom: 8px;">
                <span style="color: #aaa;">Score:</span>
                <span style="color: \${statusColor}; font-weight: 600;"> \${score}%</span>
                <span style="color: \${statusColor}; font-size: 10px;"> [\${status}]</span>
              </div>
              \${hash ? \`
                <div style="margin-top: 8px; padding-top: 8px; border-top: 1px solid #333;">
                  <a href="\${githubUrl}" target="_blank" rel="noopener"
                     style="color: #64b5f6; text-decoration: none; font-family: monospace; font-size: 12px;"
                     onclick="event.stopPropagation();">
                    \${hash} ↗
                  </a>
                </div>
              \` : ''}
            \`;
            tooltip.style.display = 'block';
            tooltip.style.pointerEvents = 'auto';
          });

          point.addEventListener('mousemove', (e) => {
            const container = chart.closest('.timeline-container');
            const rect = container.getBoundingClientRect();
            const x = e.clientX - rect.left + 15;
            const y = e.clientY - rect.top - 10;
            tooltip.style.left = x + 'px';
            tooltip.style.top = y + 'px';
          });

          point.addEventListener('mouseleave', (e) => {
            // Delay hiding to allow clicking the link
            setTimeout(() => {
              if (!tooltip.matches(':hover')) {
                tooltip.style.display = 'none';
                tooltip.style.pointerEvents = 'none';
              }
            }, 100);
          });
        });

        // Hide tooltip when mouse leaves it
        tooltip.addEventListener('mouseleave', () => {
          tooltip.style.display = 'none';
          tooltip.style.pointerEvents = 'none';
        });
      })();
    <\/script>
  `;

  return `<div class="timeline-container" style="position: relative;">${svg}${tooltip}${summary}${script}</div>`;
}

/**
 * Styles for timeline chart (inject into page)
 */
export const timelineStyles = `
.timeline-container {
  background: var(--bg-secondary);
  border-radius: 8px;
  padding: 20px;
  margin: 20px 0;
  border: 1px solid var(--grid-line);
}

.timeline-chart {
  display: block;
  margin: 0 auto;
}

.timeline-path {
  filter: drop-shadow(0 0 4px rgba(0, 212, 255, 0.5));
}

.timeline-point {
  cursor: pointer;
  transition: r 0.2s ease;
}

.timeline-point:hover {
  r: 6;
}

.timeline-summary {
  display: flex;
  justify-content: space-around;
  margin-top: 20px;
  padding-top: 20px;
  border-top: 1px solid var(--grid-line);
}

.timeline-stat {
  text-align: center;
}

.timeline-stat .stat-label {
  display: block;
  font-size: 11px;
  color: var(--text-dim);
  margin-bottom: 4px;
}

.timeline-stat .stat-value {
  display: block;
  font-size: 18px;
  font-weight: 600;
  color: var(--text-primary);
}

.no-timeline-data {
  text-align: center;
  padding: 40px;
  color: var(--text-secondary);
}

.no-timeline-data .hint {
  font-size: 12px;
  color: var(--text-dim);
  margin-top: 8px;
}
`;
