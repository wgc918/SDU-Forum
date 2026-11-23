/* ==========================================================================
   SDU Forum Admin · 轻量 SVG 图表
   提供：折线图 / 柱状图 / 环形图 / 迷你 sparkline / 水平进度条
   不依赖任何图表库，全部由原生 SVG 绘制
   ========================================================================== */
(function (window) {
  'use strict';

  const NS = 'http://www.w3.org/2000/svg';

  function resolve(target) {
    return typeof target === 'string' ? document.querySelector(target) : target;
  }

  function esc(value) {
    return String(value == null ? '' : value).replace(/[&<>"']/g, function (ch) {
      return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[ch];
    });
  }

  function compact(value) {
    const n = Number(value) || 0;
    if (Math.abs(n) >= 10000) return (n / 10000).toFixed(n % 10000 === 0 ? 0 : 1) + '万';
    if (Math.abs(n) >= 1000) return (n / 1000).toFixed(n % 1000 === 0 ? 0 : 1) + 'k';
    return String(n);
  }

  function niceScale(maxValue, tickCount) {
    const ticks = tickCount || 4;
    const max = Math.max(Number(maxValue) || 0, 1);
    const rough = max / ticks;
    const power = Math.pow(10, Math.floor(Math.log10(rough)));
    const norm = rough / power;
    let step;
    if (norm <= 1) step = 1;
    else if (norm <= 2) step = 2;
    else if (norm <= 2.5) step = 2.5;
    else if (norm <= 5) step = 5;
    else step = 10;
    step *= power;
    return { max: Math.ceil(max / step) * step, step: step };
  }

  function colorOf(color, fallback) {
    return color || fallback || 'var(--primary)';
  }

  /* ========================================================================
     折线图
     opts: { labels, series: [{ name, color, data, axis:'right' }], height, yTicks, dualAxis }
     ======================================================================== */
  function line(target, opts) {
    const node = resolve(target);
    if (!node) return '';
    const options = opts || {};
    const labels = options.labels || [];
    const series = (options.series || []).filter(function (item) { return item && item.data; });
    const dual = !!options.dualAxis;
    const width = Math.max(node.clientWidth || 720, 320);
    const height = options.height || 260;
    const margin = { top: 16, right: dual ? 54 : 16, bottom: 28, left: 46 };
    const plotW = width - margin.left - margin.right;
    const plotH = height - margin.top - margin.bottom;

    if (!labels.length || !series.length) {
      node.innerHTML = '<div class="empty" style="padding:32px"><div class="empty-title">暂无图表数据</div></div>';
      return '';
    }

    let scale;
    let rightScale = null;
    if (dual) {
      const leftValues = [];
      const rightValues = [];
      series.forEach(function (item) {
        item.data.forEach(function (value) {
          (item.axis === 'right' ? rightValues : leftValues).push(Number(value) || 0);
        });
      });
      scale = niceScale(Math.max(1, Math.max.apply(null, leftValues)), options.yTicks || 4);
      if (rightValues.length) {
        rightScale = niceScale(Math.max(1, Math.max.apply(null, rightValues)), options.yTicks || 4);
      }
    } else {
      const allValues = [];
      series.forEach(function (item) {
        item.data.forEach(function (value) { allValues.push(Number(value) || 0); });
      });
      scale = niceScale(Math.max.apply(null, allValues), options.yTicks || 4);
    }
    const count = labels.length;
    const stepX = count > 1 ? plotW / (count - 1) : 0;
    const xAt = function (index) { return margin.left + index * stepX; };
    const yAt = function (value) { return margin.top + plotH - (value / scale.max) * plotH; };
    const yAtRight = function (value) {
      const max = rightScale ? rightScale.max : scale.max;
      return margin.top + plotH - (value / max) * plotH;
    };

    const clipId = 'line-clip-' + Math.random().toString(36).slice(2, 8);
    let svg = '<svg viewBox="0 0 ' + width + ' ' + height + '" width="100%" role="img" aria-label="折线图">';
    svg += '<defs><clipPath id="' + clipId + '"><rect x="' + margin.left + '" y="' + margin.top + '" width="' + plotW + '" height="' + plotH + '"/></clipPath></defs>';

    /* 横向网格 + Y 轴刻度 */
    const tickCount = options.yTicks || 4;
    for (let i = 0; i <= tickCount; i++) {
      const value = (scale.max / tickCount) * i;
      const y = yAt(value);
      svg += '<line x1="' + margin.left + '" y1="' + y + '" x2="' + (width - margin.right) + '" y2="' + y +
        '" stroke="var(--divider)" stroke-width="1"/>';
      svg += '<text x="' + (margin.left - 8) + '" y="' + (y + 4) + '" text-anchor="end" font-size="11" fill="var(--text-3)">' +
        compact(Math.round(value)) + '</text>';
      if (rightScale) {
        svg += '<text x="' + (width - margin.right + 8) + '" y="' + (y + 4) + '" text-anchor="start" font-size="11" fill="var(--text-3)">' +
          compact(Math.round((rightScale.max / tickCount) * i)) + '</text>';
      }
    }

    /* X 轴标签 */
    const labelStep = Math.max(1, Math.ceil(count / 7));
    labels.forEach(function (label, index) {
      if (index % labelStep !== 0 && index !== count - 1) return;
      const x = xAt(index);
      svg += '<text x="' + x + '" y="' + (height - 8) + '" text-anchor="middle" font-size="11" fill="var(--text-3)">' +
        esc(label) + '</text>';
    });

    series.forEach(function (item) {
      const color = colorOf(item.color);
      const valueToY = (dual && item.axis === 'right') ? yAtRight : yAt;
      const points = item.data.map(function (value, index) {
        return [xAt(index), valueToY(Number(value) || 0)];
      });
      if (!points.length) return;

      const pathD = points.map(function (point, index) {
        return (index === 0 ? 'M' : 'L') + point[0].toFixed(1) + ' ' + point[1].toFixed(1);
      }).join(' ');

      if (item.area !== false) {
        const areaD = pathD + ' L' + points[points.length - 1][0].toFixed(1) + ' ' + (margin.top + plotH) +
          ' L' + points[0][0].toFixed(1) + ' ' + (margin.top + plotH) + ' Z';
        svg += '<path d="' + areaD + '" fill="' + color + '" opacity=".07"/>';
      }

      svg += '<path d="' + pathD + '" fill="none" stroke="' + color + '" stroke-width="2" ' +
        'stroke-linecap="round" stroke-linejoin="round" clip-path="url(#' + clipId + ')"/>';

      points.forEach(function (point, index) {
        svg += '<circle cx="' + point[0].toFixed(1) + '" cy="' + point[1].toFixed(1) + '" r="2.6" ' +
          'fill="var(--bg-card)" stroke="' + color + '" stroke-width="2">' +
          '<title>' + esc(labels[index]) + ' · ' + (item.name ? esc(item.name) + ' ' : '') + (item.data[index] || 0) + '</title></circle>';
      });
    });

    svg += '</svg>';
    node.classList.add('chart');
    node.innerHTML = svg;
    return svg;
  }

  /* ========================================================================
     柱状图（支持 1~2 组数据）
     opts: { labels, series: [{ name, color, data }], height }
     ======================================================================== */
  function bar(target, opts) {
    const node = resolve(target);
    if (!node) return '';
    const options = opts || {};
    const labels = options.labels || [];
    const series = (options.series || []).filter(function (item) { return item && item.data; });
    const width = Math.max(node.clientWidth || 720, 320);
    const height = options.height || 260;
    const margin = { top: 16, right: 16, bottom: 28, left: 44 };
    const plotW = width - margin.left - margin.right;
    const plotH = height - margin.top - margin.bottom;

    if (!labels.length || !series.length) {
      node.innerHTML = '<div class="empty" style="padding:32px"><div class="empty-title">暂无图表数据</div></div>';
      return '';
    }

    const stacked = !!options.stacked;
    const allValues = [];
    if (stacked) {
      labels.forEach(function (label, index) {
        const total = series.reduce(function (sum, item) { return sum + (Number(item.data[index]) || 0); }, 0);
        allValues.push(total);
      });
    } else {
      series.forEach(function (item) {
        item.data.forEach(function (value) { allValues.push(Number(value) || 0); });
      });
    }
    const scale = niceScale(Math.max.apply(null, allValues), 4);
    const groupWidth = plotW / labels.length;
    const barWidth = stacked
      ? Math.min(30, Math.max(10, groupWidth * 0.46))
      : Math.min(20, Math.max(6, (groupWidth * 0.62) / series.length));

    let svg = '<svg viewBox="0 0 ' + width + ' ' + height + '" width="100%" role="img" aria-label="柱状图">';

    for (let i = 0; i <= 4; i++) {
      const value = (scale.max / 4) * i;
      const y = margin.top + plotH - (value / scale.max) * plotH;
      svg += '<line x1="' + margin.left + '" y1="' + y + '" x2="' + (width - margin.right) + '" y2="' + y +
        '" stroke="var(--divider)" stroke-width="1"/>';
      svg += '<text x="' + (margin.left - 8) + '" y="' + (y + 4) + '" text-anchor="end" font-size="11" fill="var(--text-3)">' +
        compact(Math.round(value)) + '</text>';
    }

    const labelStep = Math.max(1, Math.ceil(labels.length / 7));
    labels.forEach(function (label, index) {
      const centerX = margin.left + groupWidth * index + groupWidth / 2;
      if (index % labelStep === 0 || index === labels.length - 1) {
        svg += '<text x="' + centerX + '" y="' + (height - 8) + '" text-anchor="middle" font-size="11" fill="var(--text-3)">' +
          esc(label) + '</text>';
      }
    });

    labels.forEach(function (label, groupIndex) {
      const groupCenter = margin.left + groupWidth * groupIndex + groupWidth / 2;

      if (stacked) {
        let offset = 0;
        series.forEach(function (item) {
          const value = Number(item.data[groupIndex]) || 0;
          if (value <= 0) return;
          const barHeight = (value / scale.max) * plotH;
          const y = margin.top + plotH - offset - barHeight;
          svg += '<rect x="' + (groupCenter - barWidth / 2).toFixed(1) + '" y="' + y.toFixed(1) +
            '" width="' + barWidth + '" height="' + barHeight.toFixed(1) + '" rx="2" fill="' + colorOf(item.color) + '">' +
            '<title>' + esc(label) + ' · ' + (item.name ? esc(item.name) + ' ' : '') + value + '</title></rect>';
          offset += barHeight;
        });
        return;
      }

      const totalWidth = barWidth * series.length + 3 * (series.length - 1);
      const startX = groupCenter - totalWidth / 2;

      series.forEach(function (item, seriesIndex) {
        const value = Number(item.data[groupIndex]) || 0;
        const barHeight = Math.max(1, (value / scale.max) * plotH);
        const x = startX + seriesIndex * (barWidth + 3);
        const y = margin.top + plotH - barHeight;
        svg += '<rect x="' + x.toFixed(1) + '" y="' + y.toFixed(1) + '" width="' + barWidth +
          '" height="' + barHeight.toFixed(1) + '" rx="3" fill="' + colorOf(item.color) + '">' +
          '<title>' + esc(label) + ' · ' + (item.name ? esc(item.name) + ' ' : '') + value + '</title></rect>';
      });
    });

    svg += '</svg>';
    node.classList.add('chart');
    node.innerHTML = svg;
    return svg;
  }

  /* ========================================================================
     环形图
     opts: { segments: [{ label, value, color }], size, thickness, centerValue, centerLabel, legend }
     ======================================================================== */
  function donut(target, opts) {
    const node = resolve(target);
    if (!node) return '';
    const options = opts || {};
    const segments = (options.segments || []).filter(function (item) { return item && item.value > 0; });
    const size = options.size || 168;
    const thickness = options.thickness || 18;
    const radius = (size - thickness) / 2;
    const center = size / 2;
    const circumference = 2 * Math.PI * radius;
    const total = segments.reduce(function (sum, item) { return sum + Number(item.value); }, 0);

    if (!segments.length || total <= 0) {
      node.innerHTML = '<div class="empty" style="padding:32px"><div class="empty-title">暂无图表数据</div></div>';
      return '';
    }

    let svg = '<svg viewBox="0 0 ' + size + ' ' + size + '" class="donut-svg" role="img" aria-label="环形图">';
    svg += '<circle cx="' + center + '" cy="' + center + '" r="' + radius + '" fill="none" stroke="var(--divider)" stroke-width="' + thickness + '"/>';

    let offset = 0;
    segments.forEach(function (item) {
      const length = (Number(item.value) / total) * circumference;
      svg += '<circle cx="' + center + '" cy="' + center + '" r="' + radius + '" fill="none" stroke="' + colorOf(item.color) +
        '" stroke-width="' + thickness + '" stroke-linecap="butt" ' +
        'stroke-dasharray="' + length.toFixed(2) + ' ' + (circumference - length).toFixed(2) + '" ' +
        'stroke-dashoffset="' + (-offset).toFixed(2) + '" transform="rotate(-90 ' + center + ' ' + center + ')">' +
        '<title>' + esc(item.label) + '：' + item.value + '</title></circle>';
      offset += length;
    });

    if (options.centerValue || options.centerLabel) {
      svg += '<text x="' + center + '" y="' + (center - 2) + '" text-anchor="middle" font-size="24" font-weight="700" fill="var(--text-1)">' +
        esc(options.centerValue || '') + '</text>';
      svg += '<text x="' + center + '" y="' + (center + 18) + '" text-anchor="middle" font-size="11" fill="var(--text-3)">' +
        esc(options.centerLabel || '') + '</text>';
    }
    svg += '</svg>';

    let html = '<div class="chart-donut">' + svg;
    if (options.legend !== false) {
      html += '<div class="donut-legend">';
      segments.forEach(function (item) {
        const pct = ((Number(item.value) / total) * 100).toFixed(1);
        html += '<div class="legend-item"><span class="legend-dot" style="background:' + colorOf(item.color) + '"></span>' +
          '<span>' + esc(item.label) + '</span><span class="value">' + pct + '%</span></div>';
      });
      html += '</div>';
    }
    html += '</div>';

    node.innerHTML = html;
    return html;
  }

  /* ========================================================================
     迷你 sparkline
     opts: { data, color, width, height, area }
     ======================================================================== */
  function sparkline(target, opts) {
    const node = resolve(target);
    if (!node) return '';
    const options = opts || {};
    const data = (options.data || []).map(function (value) { return Number(value) || 0; });
    const width = options.width || 96;
    const height = options.height || 28;
    const color = colorOf(options.color);

    if (data.length < 2) {
      node.innerHTML = '';
      return '';
    }

    const min = Math.min.apply(null, data);
    const max = Math.max.apply(null, data);
    const range = max - min || 1;
    const stepX = width / (data.length - 1);
    const points = data.map(function (value, index) {
      return [index * stepX, height - 2 - ((value - min) / range) * (height - 4)];
    });
    const pathD = points.map(function (point, index) {
      return (index === 0 ? 'M' : 'L') + point[0].toFixed(1) + ' ' + point[1].toFixed(1);
    }).join(' ');

    let svg = '<svg width="' + width + '" height="' + height + '" viewBox="0 0 ' + width + ' ' + height +
      '" fill="none" aria-hidden="true">';
    if (options.area !== false) {
      const areaD = pathD + ' L' + width + ' ' + height + ' L0 ' + height + ' Z';
      svg += '<path d="' + areaD + '" fill="' + color + '" opacity=".08"/>';
    }
    svg += '<path d="' + pathD + '" stroke="' + color + '" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>';
    svg += '</svg>';

    node.innerHTML = svg;
    return svg;
  }

  /* ========================================================================
     水平进度条（SVG）
     opts: { label, value, max, color, showValue, suffix }
     ======================================================================== */
  function progress(target, opts) {
    const node = resolve(target);
    if (!node) return '';
    const options = opts || {};
    const max = Number(options.max) || 100;
    const value = Math.max(0, Math.min(Number(options.value) || 0, max));
    const pct = max > 0 ? (value / max) * 100 : 0;
    const color = colorOf(options.color);
    const label = options.label || '';
    const valueText = options.showValue === false ? '' : (options.suffix ? value + options.suffix : Math.round(pct) + '%');

    const svg = '<svg viewBox="0 0 100 8" preserveAspectRatio="none" aria-hidden="true">' +
      '<rect x="0" y="0" width="100" height="8" rx="4" fill="var(--divider)"/>' +
      '<rect x="0" y="0" width="' + pct.toFixed(2) + '" height="8" rx="4" fill="' + color + '"/>' +
      '</svg>';

    node.innerHTML = '<div class="progress-row"><span class="progress-label" title="' + esc(label) + '">' +
      esc(label) + '</span><span class="progress-track">' + svg + '</span>' +
      '<span class="progress-value">' + esc(valueText) + '</span></div>';
    return svg;
  }

  window.Charts = {
    line: line,
    bar: bar,
    donut: donut,
    sparkline: sparkline,
    progress: progress
  };
})(window);
