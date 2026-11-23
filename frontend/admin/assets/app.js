/* ==========================================================================
   SDU Forum Admin · 通用脚本
   - 布局注入（侧边栏 / 顶栏 / 面包屑）
   - 通用组件：Toast / Modal / Drawer / 表格 / 分页 / 空状态
   - 工具函数：$ / $$ / el / fmtDate / fmtNum / fmtRelative 等
   ========================================================================== */
(function (window, document) {
  'use strict';

  /* ========================================================================
     1. 图标（Lucide 风格，stroke-width 1.75）
     ======================================================================== */
  const ICON_PATHS = {
    dashboard: '<rect width="7" height="9" x="3" y="3" rx="1"/><rect width="7" height="5" x="14" y="3" rx="1"/><rect width="7" height="9" x="14" y="12" rx="1"/><rect width="7" height="5" x="3" y="16" rx="1"/>',
    users: '<path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><path d="M22 21v-2a4 4 0 0 0-3-3.87"/><path d="M16 3.13a4 4 0 0 1 0 7.75"/>',
    ban: '<circle cx="12" cy="12" r="10"/><path d="m4.9 4.9 14.2 14.2"/>',
    shield: '<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/>',
    'shield-check': '<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/><path d="m9 12 2 2 4-4"/>',
    'file-text': '<path d="M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z"/><path d="M14 2v4a2 2 0 0 0 2 2h4"/><path d="M10 9H8"/><path d="M16 13H8"/><path d="M16 17H8"/>',
    comment: '<path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>',
    tags: '<path d="m12.586 2.586 7 7a2 2 0 0 1 0 2.828l-6.172 6.172a2 2 0 0 1-2.828 0l-7-7A2 2 0 0 1 3 10.172V4a2 2 0 0 1 2-2h6.172a2 2 0 0 1 1.414.586Z"/><circle cx="7.5" cy="7.5" r="1"/>',
    filter: '<polygon points="22 3 2 3 10 12.46 10 19 14 21 14 12.46 22 3"/>',
    flag: '<path d="M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1z"/><line x1="4" x2="4" y1="22" y2="15"/>',
    lightbulb: '<path d="M15 14c.2-1 .7-1.7 1.5-2.5 1-.9 1.5-2.2 1.5-3.5A6 6 0 0 0 6 8c0 1 .2 2.2 1.5 3.5.7.7 1.3 1.5 1.5 2.5"/><path d="M9 18h6"/><path d="M10 22h4"/>',
    megaphone: '<path d="m3 11 18-5v12L3 14v-3z"/><path d="M11.6 16.8a3 3 0 1 1-5.8-1.6"/>',
    chart: '<path d="M3 3v18h18"/><path d="m19 9-5 5-4-4-3 3"/>',
    history: '<path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"/><path d="M3 3v5h5"/><path d="M12 7v5l4 2"/>',
    settings: '<path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"/><circle cx="12" cy="12" r="3"/>',
    search: '<circle cx="11" cy="11" r="8"/><path d="m21 21-4.3-4.3"/>',
    bell: '<path d="M6 8a6 6 0 0 1 12 0c0 7 3 9 3 9H3s3-2 3-9"/><path d="M10.3 21a1.94 1.94 0 0 0 3.4 0"/>',
    moon: '<path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/>',
    sun: '<circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/>',
    logout: '<path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"/><polyline points="16 17 21 12 16 7"/><line x1="21" x2="9" y1="12" y2="12"/>',
    'panel-left': '<rect width="18" height="18" x="3" y="3" rx="2"/><path d="M9 3v18"/>',
    x: '<path d="M18 6 6 18"/><path d="m6 6 12 12"/>',
    plus: '<path d="M5 12h14"/><path d="M12 5v14"/>',
    download: '<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="7 10 12 15 17 10"/><line x1="12" x2="12" y1="15" y2="3"/>',
    refresh: '<path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/><path d="M21 3v5h-5"/><path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/><path d="M8 16H3v5"/>',
    eye: '<path d="M2 12s3-7 10-7 10 7 10 7-3 7-10 7-10-7-10-7Z"/><circle cx="12" cy="12" r="3"/>',
    'eye-off': '<path d="M9.88 9.88a3 3 0 1 0 4.24 4.24"/><path d="M10.73 5.08A10.43 10.43 0 0 1 12 5c7 0 10 7 10 7a13.16 13.16 0 0 1-1.67 2.68"/><path d="M6.61 6.61A13.526 13.526 0 0 0 2 12s3 7 10 7a9.74 9.74 0 0 0 5.39-1.61"/><line x1="2" x2="22" y1="2" y2="22"/>',
    edit: '<path d="M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7"/><path d="M18.5 2.5a2.12 2.12 0 0 1 3 3L12 15l-4 1 1-4Z"/>',
    trash: '<path d="M3 6h18"/><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6"/><path d="M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/>',
    'alert-triangle': '<path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3Z"/><line x1="12" x2="12" y1="9" y2="13"/><line x1="12" x2="12.01" y1="17" y2="17"/>',
    check: '<path d="M20 6 9 17l-5-5"/>',
    'check-circle': '<circle cx="12" cy="12" r="10"/><path d="m8 12 3 3 5-5"/>',
    'x-circle': '<circle cx="12" cy="12" r="10"/><path d="m15 9-6 6"/><path d="m9 9 6 6"/>',
    info: '<circle cx="12" cy="12" r="10"/><path d="M12 16v-4"/><path d="M12 8h.01"/>',
    clock: '<circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/>',
    calendar: '<rect width="18" height="18" x="3" y="4" rx="2"/><line x1="16" x2="16" y1="2" y2="6"/><line x1="8" x2="8" y1="2" y2="6"/><line x1="3" x2="21" y1="10" y2="10"/>',
    mail: '<rect width="20" height="16" x="2" y="4" rx="2"/><path d="m22 7-8.97 5.7a1.94 1.94 0 0 1-2.06 0L2 7"/>',
    'user-x': '<path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><line x1="17" x2="22" y1="8" y2="13"/><line x1="22" x2="17" y1="8" y2="13"/>',
    'user-check': '<path d="M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"/><circle cx="9" cy="7" r="4"/><polyline points="16 11 18 13 22 9"/>',
    inbox: '<polyline points="22 12 16 12 14 15 10 15 8 12 2 12"/><path d="M5.45 5.11 2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z"/>',
    lock: '<rect width="18" height="11" x="3" y="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/>',
    key: '<circle cx="7.5" cy="15.5" r="5.5"/><path d="m21 2-9.6 9.6"/><path d="m15.5 7.5 3 3L22 7l-3-3"/>',
    'trending-up': '<polyline points="22 7 13.5 15.5 8.5 10.5 2 17"/><polyline points="16 7 22 7 22 13"/>',
    'chevron-left': '<path d="m15 18-6-6 6-6"/>',
    'chevron-right': '<path d="m9 18 6-6-6-6"/>',
    'chevron-down': '<path d="m6 9 6 6 6-6"/>',
    'more-horizontal': '<circle cx="12" cy="12" r="1"/><circle cx="19" cy="12" r="1"/><circle cx="5" cy="12" r="1"/>',
    sparkles: '<path d="m12 3-1.9 5.8a2 2 0 0 1-1.3 1.3L3 12l5.8 1.9a2 2 0 0 1 1.3 1.3L12 21l1.9-5.8a2 2 0 0 1 1.3-1.3L21 12l-5.8-1.9a2 2 0 0 1-1.3-1.3Z"/>',
    image: '<rect width="18" height="18" x="3" y="3" rx="2" ry="2"/><circle cx="9" cy="9" r="2"/><path d="m21 15-3.086-3.086a2 2 0 0 0-2.828 0L6 21"/>',
    list: '<line x1="8" x2="21" y1="6" y2="6"/><line x1="8" x2="21" y1="12" y2="12"/><line x1="8" x2="21" y1="18" y2="18"/><line x1="3" x2="3.01" y1="6" y2="6"/><line x1="3" x2="3.01" y1="12" y2="12"/><line x1="3" x2="3.01" y1="18" y2="18"/>',
    grid: '<rect width="7" height="7" x="3" y="3" rx="1"/><rect width="7" height="7" x="14" y="3" rx="1"/><rect width="7" height="7" x="14" y="14" rx="1"/><rect width="7" height="7" x="3" y="14" rx="1"/>',
    heart: '<path d="M19 14c1.49-1.46 3-3.21 3-5.5A5.5 5.5 0 0 0 16.5 3c-1.76 0-3 .5-4.5 2-1.5-1.5-2.74-2-4.5-2A5.5 5.5 0 0 0 2 8.5c0 2.3 1.5 4.05 3 5.5l7 7Z"/>',
    upload: '<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><polyline points="17 8 12 3 7 8"/><line x1="12" x2="12" y1="3" y2="15"/>',
    'volume-x': '<path d="M11 4.7a.7.7 0 0 0-1.2-.5L6.4 7.6A1.4 1.4 0 0 1 5.4 8H3a1 1 0 0 0-1 1v6a1 1 0 0 0 1 1h2.4a1.4 1.4 0 0 1 1 .4l3.4 3.4a.7.7 0 0 0 1.2-.5Z"/><line x1="22" x2="16" y1="9" y2="15"/><line x1="16" x2="22" y1="9" y2="15"/>'
  };

  function icon(name, size) {
    const path = ICON_PATHS[name] || ICON_PATHS.info;
    const s = size || 18;
    const cls = s === 16 ? 'icon icon-16' : s === 20 ? 'icon icon-20' : s === 24 ? 'icon icon-24' : 'icon';
    return '<svg class="' + cls + '" viewBox="0 0 24 24" width="' + s + '" height="' + s + '" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' + path + '</svg>';
  }

  /* ========================================================================
     2. 工具函数
     ======================================================================== */
  function $(selector, root) {
    return (root || document).querySelector(selector);
  }

  function $$(selector, root) {
    return Array.prototype.slice.call((root || document).querySelectorAll(selector));
  }

  function el(tag, attrs, children) {
    const node = document.createElement(tag);
    if (attrs) {
      Object.keys(attrs).forEach(function (key) {
        const value = attrs[key];
        if (value == null) return;
        if (key === 'class') node.className = value;
        else if (key === 'html') node.innerHTML = value;
        else if (key === 'text') node.textContent = value;
        else if (key === 'style' && typeof value === 'object') Object.assign(node.style, value);
        else if (key.indexOf('on') === 0 && typeof value === 'function') node.addEventListener(key.slice(2).toLowerCase(), value);
        else node.setAttribute(key, value);
      });
    }
    if (children != null) {
      (Array.isArray(children) ? children : [children]).forEach(function (child) {
        if (child == null) return;
        node.appendChild(typeof child === 'string' ? document.createTextNode(child) : child);
      });
    }
    return node;
  }

  const HTML_ESCAPE = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' };

  function escapeHtml(value) {
    return String(value == null ? '' : value).replace(/[&<>"']/g, function (ch) {
      return HTML_ESCAPE[ch];
    });
  }

  function fmtNum(value) {
    const n = Number(value);
    if (!isFinite(n)) return '0';
    return n.toLocaleString('zh-CN');
  }

  function pad2(n) {
    return n < 10 ? '0' + n : String(n);
  }

  function fmtDate(input, pattern) {
    const d = input instanceof Date ? input : new Date(input);
    if (isNaN(d.getTime())) return '-';
    const p = pattern || 'YYYY-MM-DD';
    const map = {
      YYYY: d.getFullYear(),
      MM: pad2(d.getMonth() + 1),
      DD: pad2(d.getDate()),
      HH: pad2(d.getHours()),
      mm: pad2(d.getMinutes()),
      ss: pad2(d.getSeconds())
    };
    return p.replace(/YYYY|MM|DD|HH|mm|ss/g, function (key) { return map[key]; });
  }

  function fmtDateTime(input) {
    return fmtDate(input, 'YYYY-MM-DD HH:mm');
  }

  function fmtRelative(input) {
    const d = input instanceof Date ? input : new Date(input);
    if (isNaN(d.getTime())) return '-';
    const diff = Date.now() - d.getTime();
    if (diff < 0) return fmtDate(d, 'MM-DD HH:mm');
    const sec = Math.floor(diff / 1000);
    if (sec < 60) return '刚刚';
    const min = Math.floor(sec / 60);
    if (min < 60) return min + ' 分钟前';
    const hour = Math.floor(min / 60);
    if (hour < 24) return hour + ' 小时前';
    const day = Math.floor(hour / 24);
    if (day < 7) return day + ' 天前';
    if (day < 365) return fmtDate(d, 'MM-DD');
    return fmtDate(d, 'YYYY-MM-DD');
  }

  function debounce(fn, wait) {
    let timer = null;
    return function () {
      const args = arguments;
      const context = this;
      clearTimeout(timer);
      timer = setTimeout(function () { fn.apply(context, args); }, wait || 300);
    };
  }

  function hashCode(str) {
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      hash = (hash * 31 + str.charCodeAt(i)) | 0;
    }
    return hash;
  }

  function readStore(key) {
    try { return window.localStorage.getItem(key); } catch (error) { return null; }
  }

  function writeStore(key, value) {
    try { window.localStorage.setItem(key, value); } catch (error) { /* 忽略隐私模式等存储异常 */ }
  }

  const AVATAR_TONES = [
    { bg: '#EFF4FC', fg: '#1E5BB8' },
    { bg: '#ECFDF5', fg: '#047857' },
    { bg: '#FFF7ED', fg: '#C2410C' },
    { bg: '#FEF2F2', fg: '#B91C1C' },
    { bg: '#F5F3FF', fg: '#6D28D9' }
  ];

  function avatarHTML(name, id, size) {
    const px = size || 32;
    const text = String(name || '?').trim();
    const tone = AVATAR_TONES[Math.abs(hashCode(String(id == null ? text : id))) % AVATAR_TONES.length];
    const fontSize = Math.max(11, Math.round(px * 0.4));
    return '<span class="avatar" style="--av-bg:' + tone.bg + ';--av-color:' + tone.fg +
      ';width:' + px + 'px;height:' + px + 'px;font-size:' + fontSize + 'px">' +
      escapeHtml(text.slice(0, 1)) + '</span>';
  }

  function pill(text, tone, withDot) {
    return '<span class="pill pill-' + (tone || 'neutral') + '">' +
      (withDot ? '<span class="dot"></span>' : '') + escapeHtml(text) + '</span>';
  }

  function emptyHTML(title, desc, iconName) {
    return '<div class="empty">' +
      '<div class="empty-icon">' + icon(iconName || 'inbox', 20) + '</div>' +
      '<div class="empty-title">' + escapeHtml(title || '暂无数据') + '</div>' +
      (desc ? '<div class="empty-desc">' + escapeHtml(desc) + '</div>' : '') +
      '</div>';
  }

  /* ========================================================================
     3. Toast
     ======================================================================== */
  let toastRoot = null;

  function ensureToastRoot() {
    if (!toastRoot) {
      toastRoot = el('div', { class: 'toast-root', id: 'toast-root' });
      document.body.appendChild(toastRoot);
    }
    return toastRoot;
  }

  const TOAST_ICONS = {
    success: 'check-circle',
    warning: 'alert-triangle',
    error: 'x-circle',
    info: 'info'
  };

  function toast(message, type, duration) {
    const kind = type || 'success';
    const item = el('div', {
      class: 'toast' + (kind === 'success' ? '' : ' ' + kind),
      html: icon(TOAST_ICONS[kind] || 'info', 16) + '<span>' + escapeHtml(message) + '</span>'
    });
    ensureToastRoot().appendChild(item);
    setTimeout(function () {
      item.classList.add('leaving');
      setTimeout(function () { item.remove(); }, 220);
    }, duration || 3000);
  }

  /* ========================================================================
     4. Modal
     ======================================================================== */
  function modal(opts) {
    const options = opts || {};
    const width = options.width || 480;
    const overlay = el('div', { class: 'overlay overlay-center' });
    const dialog = el('div', { class: 'modal', style: { width: width + 'px' } });

    const head = el('div', {
      class: 'modal-head',
      html: '<div><div class="modal-title">' + escapeHtml(options.title || '') + '</div>' +
        (options.desc ? '<div class="modal-desc">' + escapeHtml(options.desc) + '</div>' : '') + '</div>'
    });
    const closeBtn = el('button', { class: 'icon-btn', type: 'button', 'aria-label': '关闭', html: icon('x', 18) });
    head.appendChild(closeBtn);

    const body = el('div', { class: 'modal-body' });
    if (typeof options.body === 'string') body.innerHTML = options.body;
    else if (options.body) body.appendChild(options.body);

    const foot = el('div', { class: 'modal-foot' });
    let confirmBtn = null;

    if (options.footer) {
      if (typeof options.footer === 'string') foot.innerHTML = options.footer;
      else foot.appendChild(options.footer);
    } else {
      const cancelBtn = el('button', { class: 'btn btn-secondary', type: 'button', text: options.cancelText || '取消' });
      confirmBtn = el('button', {
        class: 'btn ' + (options.danger ? 'btn-danger-solid' : 'btn-primary'),
        type: 'button',
        text: options.confirmText || '确定'
      });
      foot.appendChild(cancelBtn);
      foot.appendChild(confirmBtn);
      cancelBtn.addEventListener('click', function () {
        close();
        if (options.onCancel) options.onCancel();
      });
    }

    dialog.appendChild(head);
    dialog.appendChild(body);
    if (!options.hideFooter) dialog.appendChild(foot);
    overlay.appendChild(dialog);
    document.body.appendChild(overlay);

    let closed = false;

    function close() {
      if (closed) return;
      closed = true;
      overlay.classList.remove('open');
      setTimeout(function () { overlay.remove(); }, 200);
      document.removeEventListener('keydown', onKeydown);
      if (options.onClose) options.onClose();
    }

    function onKeydown(event) {
      if (event.key === 'Escape') close();
    }

    closeBtn.addEventListener('click', close);
    document.addEventListener('keydown', onKeydown);

    if (options.closeOnOverlay !== false) {
      overlay.addEventListener('mousedown', function (event) {
        if (event.target === overlay) close();
      });
    }

    if (confirmBtn) {
      let busy = false;
      confirmBtn.addEventListener('click', function () {
        if (busy) return;
        if (!options.onConfirm) { close(); return; }
        busy = true;
        confirmBtn.classList.add('loading');
        Promise.resolve()
          .then(function () { return options.onConfirm(close); })
          .then(function (result) {
            if (result !== false) close();
          })
          .catch(function () {
            toast('操作失败，请稍后重试', 'error');
          })
          .finally(function () {
            busy = false;
            confirmBtn.classList.remove('loading');
          });
      });
    }

    requestAnimationFrame(function () { overlay.classList.add('open'); });
    return { close: close, overlay: overlay, dialog: dialog, body: body };
  }

  /* ========================================================================
     5. Drawer
     ======================================================================== */
  function drawer(opts) {
    const options = opts || {};
    const overlay = el('div', { class: 'overlay overlay-drawer' });
    const panel = el('div', { class: 'drawer', style: { width: (options.width || 480) + 'px' } });

    const head = el('div', {
      class: 'drawer-head',
      html: '<div><div class="drawer-title">' + escapeHtml(options.title || '') + '</div>' +
        (options.subtitle ? '<div class="drawer-sub">' + escapeHtml(options.subtitle) + '</div>' : '') + '</div>'
    });
    const closeBtn = el('button', { class: 'icon-btn', type: 'button', 'aria-label': '关闭', html: icon('x', 18) });
    head.appendChild(closeBtn);

    const body = el('div', { class: 'drawer-body' });
    if (typeof options.body === 'string') body.innerHTML = options.body;
    else if (options.body) body.appendChild(options.body);

    panel.appendChild(head);
    panel.appendChild(body);

    if (options.footer) {
      const foot = el('div', { class: 'drawer-foot' });
      if (typeof options.footer === 'string') foot.innerHTML = options.footer;
      else foot.appendChild(options.footer);
      panel.appendChild(foot);
    }

    overlay.appendChild(panel);
    document.body.appendChild(overlay);

    let closed = false;

    function close() {
      if (closed) return;
      closed = true;
      overlay.classList.remove('open');
      setTimeout(function () { overlay.remove(); }, 240);
      document.removeEventListener('keydown', onKeydown);
      if (options.onClose) options.onClose();
    }

    function onKeydown(event) {
      if (event.key === 'Escape') close();
    }

    closeBtn.addEventListener('click', close);
    document.addEventListener('keydown', onKeydown);
    if (options.closeOnOverlay !== false) {
      overlay.addEventListener('mousedown', function (event) {
        if (event.target === overlay) close();
      });
    }

    requestAnimationFrame(function () { overlay.classList.add('open'); });
    return { close: close, overlay: overlay, body: body, panel: panel };
  }

  /* ========================================================================
     6. 表格渲染
     ======================================================================== */
  function renderTable(container, columns, rows, options) {
    const opts = options || {};
    const mount = typeof container === 'string' ? $(container) : container;
    if (!mount) return;

    const selectable = !!opts.selectable;
    const selected = opts.selected || new Set();
    const rowKey = opts.rowKey || 'id';
    const cols = columns || [];
    const list = rows || [];
    const colSpan = cols.length + (selectable ? 1 : 0);

    const allChecked = list.length > 0 && list.every(function (row) {
      return selected.has(String(row[rowKey]));
    });
    const someChecked = !allChecked && list.some(function (row) {
      return selected.has(String(row[rowKey]));
    });

    let html = '<div class="table-wrap"><table class="data-table"><thead><tr>';
    if (selectable) {
      html += '<th class="col-check"><input type="checkbox" class="checkbox js-check-all" aria-label="全选"' +
        (allChecked ? ' checked' : '') + '></th>';
    }
    cols.forEach(function (col) {
      const style = col.width ? ' style="width:' + col.width + '"' : '';
      let thClass = '';
      if (col.align === 'right') thClass += 'col-actions ';
      if (col.className) thClass += col.className;
      const cls = thClass.trim() ? ' class="' + thClass.trim() + '"' : '';
      html += '<th' + style + cls + '>' + escapeHtml(col.title || '') + '</th>';
    });
    html += '</tr></thead><tbody>';

    if (!list.length) {
      html += '<tr><td colspan="' + colSpan + '" class="table-empty">' +
        emptyHTML(opts.emptyText || '暂无数据', opts.emptyDesc || '没有找到符合条件的内容', opts.emptyIcon) +
        '</td></tr>';
    } else {
      list.forEach(function (row, index) {
        const key = String(row[rowKey]);
        html += '<tr data-key="' + escapeHtml(key) + '"' + (selected.has(key) ? ' class="is-selected"' : '') + '>';
        if (selectable) {
          html += '<td class="col-check"><input type="checkbox" class="checkbox js-check-row"' +
            (selected.has(key) ? ' checked' : '') + ' aria-label="选择该行"></td>';
        }
        cols.forEach(function (col) {
          let content;
          if (typeof col.cell === 'function') content = col.cell(row, index);
          else content = escapeHtml(row[col.key] == null ? '-' : row[col.key]);
          if (content == null) content = '-';
          const cls = col.align === 'right'
            ? ' class="col-actions' + (col.className ? ' ' + col.className : '') + '"'
            : (col.className ? ' class="' + col.className + '"' : '');
          html += '<td' + cls + '>' + content + '</td>';
        });
        html += '</tr>';
      });
    }

    html += '</tbody></table></div>';
    mount.innerHTML = html;

    if (selectable) {
      const checkAll = $('.js-check-all', mount);
      if (checkAll) {
        checkAll.indeterminate = someChecked;
        checkAll.addEventListener('change', function () {
          list.forEach(function (row) {
            const key = String(row[rowKey]);
            if (checkAll.checked) selected.add(key);
            else selected.delete(key);
          });
          renderTable(mount, cols, list, Object.assign({}, opts, { selected: selected }));
          if (opts.onSelectionChange) opts.onSelectionChange(selected);
        });
      }
      $$('.js-check-row', mount).forEach(function (checkbox) {
        checkbox.addEventListener('change', function () {
          const tr = checkbox.closest('tr');
          const key = tr ? tr.getAttribute('data-key') : null;
          if (!key) return;
          if (checkbox.checked) selected.add(key);
          else selected.delete(key);
          if (tr) tr.classList.toggle('is-selected', checkbox.checked);
          renderTable(mount, cols, list, Object.assign({}, opts, { selected: selected }));
          if (opts.onSelectionChange) opts.onSelectionChange(selected);
        });
      });
    }

    if (typeof opts.onRowClick === 'function') {
      $$('tbody tr[data-key]', mount).forEach(function (tr) {
        tr.addEventListener('click', function (event) {
          if (event.target.closest('input, button, a, label, select')) return;
          const key = tr.getAttribute('data-key');
          const row = list.find(function (item) { return String(item[rowKey]) === key; });
          if (row) opts.onRowClick(row);
        });
      });
    }
  }

  /* ========================================================================
     7. 分页
     ======================================================================== */
  function paginationPages(current, total) {
    const pages = [];
    if (total <= 7) {
      for (let i = 1; i <= total; i++) pages.push(i);
      return pages;
    }
    pages.push(1);
    const start = Math.max(2, current - 1);
    const end = Math.min(total - 1, current + 1);
    if (start > 2) pages.push('...');
    for (let i = start; i <= end; i++) pages.push(i);
    if (end < total - 1) pages.push('...');
    pages.push(total);
    return pages;
  }

  function renderPagination(container, options) {
    const opts = options || {};
    const mount = typeof container === 'string' ? $(container) : container;
    if (!mount) return;

    const total = opts.total || 0;
    const pageSize = opts.pageSize || 20;
    const pages = Math.max(1, Math.ceil(total / pageSize));
    const current = Math.min(Math.max(1, opts.page || 1), pages);
    const sizes = opts.pageSizes || [10, 20, 50];
    if (sizes.indexOf(pageSize) === -1) sizes.push(pageSize);
    sizes.sort(function (a, b) { return a - b; });

    let html = '<div class="pagination"><div class="pagination-info">共 <b>' + fmtNum(total) +
      '</b> 条 · 每页 <select class="select select-sm js-page-size" style="width:auto">';
    sizes.forEach(function (size) {
      html += '<option value="' + size + '"' + (size === pageSize ? ' selected' : '') + '>' + size + '</option>';
    });
    html += '</select> 条</div><div class="pagination-actions">';
    html += '<button type="button" class="page-btn js-prev" ' + (current <= 1 ? 'disabled' : '') + ' aria-label="上一页">' + icon('chevron-left', 16) + '</button>';
    paginationPages(current, pages).forEach(function (item) {
      if (item === '...') {
        html += '<span class="page-ellipsis">…</span>';
      } else {
        html += '<button type="button" class="page-btn js-page' + (item === current ? ' active' : '') +
          '" data-page="' + item + '">' + item + '</button>';
      }
    });
    html += '<button type="button" class="page-btn js-next" ' + (current >= pages ? 'disabled' : '') + ' aria-label="下一页">' + icon('chevron-right', 16) + '</button>';
    html += '</div></div>';
    mount.innerHTML = html;

    const emit = function (nextPage, nextSize) {
      if (opts.onChange) opts.onChange(nextPage, nextSize);
    };

    const prev = $('.js-prev', mount);
    const next = $('.js-next', mount);
    if (prev) prev.addEventListener('click', function () { if (current > 1) emit(current - 1, pageSize); });
    if (next) next.addEventListener('click', function () { if (current < pages) emit(current + 1, pageSize); });

    $$('.js-page', mount).forEach(function (btn) {
      btn.addEventListener('click', function () {
        const target = Number(btn.getAttribute('data-page'));
        if (target !== current) emit(target, pageSize);
      });
    });

    const sizeSelect = $('.js-page-size', mount);
    if (sizeSelect) {
      sizeSelect.addEventListener('change', function () {
        emit(1, Number(sizeSelect.value));
      });
    }
  }

  /* ========================================================================
     8. 侧边栏菜单
     ======================================================================== */
  const MENU = [
    {
      title: '概览',
      items: [
        { key: 'dashboard', label: '运营看板', icon: 'dashboard', href: 'index.html' }
      ]
    },
    {
      title: '用户管理',
      items: [
        { key: 'users', label: '用户列表', icon: 'users', href: 'users.html' },
        { key: 'bans', label: '封禁管理', icon: 'ban', href: 'bans.html' },
        { key: 'mutes', label: '禁言管理', icon: 'volume-x', href: 'mutes.html' },
        { key: 'admins', label: '管理员与角色', icon: 'shield', href: '#' },
        { key: 'verifications', label: '实名认证', icon: 'user-check', href: '#' }
      ]
    },
    {
      title: '内容管理',
      items: [
        {
          key: 'review', label: '审核队列', icon: 'inbox', href: 'review.html',
          badge: function () {
            const list = window.POSTS || [];
            return list.filter(function (item) { return item.status === 'pending'; }).length;
          }
        },
        { key: 'posts', label: '帖子管理', icon: 'file-text', href: 'posts.html' },
        { key: 'comments', label: '评论管理', icon: 'comment', href: 'comments.html' },
        { key: 'tags', label: '标签管理', icon: 'tags', href: 'tags.html' },
        { key: 'sensitive', label: '敏感词库', icon: 'filter', href: 'sensitive-words.html' }
      ]
    },
    {
      title: '社区治理',
      items: [
        {
          key: 'reports', label: '举报工单', icon: 'flag', href: 'reports.html',
          badge: function () {
            const list = window.REPORTS || [];
            return list.filter(function (item) { return item.status === 'pending'; }).length;
          }
        },
        {
          key: 'feedbacks', label: '用户反馈', icon: 'lightbulb', href: 'feedback.html',
          badge: function () {
            const list = window.FEEDBACKS || [];
            return list.filter(function (item) { return item.status === 'pending'; }).length;
          }
        }
      ]
    },
    {
      title: '运营中心',
      items: [
        { key: 'announcements', label: '公告管理', icon: 'megaphone', href: 'announcements.html' },
        { key: 'notifications', label: '消息推送', icon: 'bell', href: 'notifications.html' },
        { key: 'recommendations', label: '推荐位管理', icon: 'grid', href: 'recommendations.html' },
        { key: 'points', label: '积分与等级', icon: 'sparkles', href: 'points.html' },
        { key: 'analytics', label: '数据统计', icon: 'chart', href: 'analytics.html' }
      ]
    },
    {
      title: '系统',
      items: [
        { key: 'logs', label: '操作日志', icon: 'history', href: '#' },
        { key: 'settings', label: '系统设置', icon: 'settings', href: '#' }
      ]
    }
  ];

  function findMenu(key) {
    for (let i = 0; i < MENU.length; i++) {
      const group = MENU[i];
      for (let j = 0; j < group.items.length; j++) {
        if (group.items[j].key === key) {
          return { group: group.title, item: group.items[j] };
        }
      }
    }
    return { group: '概览', item: { label: '运营看板' } };
  }

  function buildSidebar(activeKey) {
    let html = '<div class="sidebar-brand">' +
      '<span class="brand-mark">' +
      '<svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' +
      '<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/>' +
      '<path d="M8.5 9.5h7"/><path d="M8.5 12.5h4.5"/>' +
      '</svg></span>' +
      '<div class="brand-text"><div class="brand-name">SDU Forum</div><div class="brand-sub">Admin Console</div></div>' +
      '</div><nav class="nav">';

    MENU.forEach(function (group) {
      html += '<div class="nav-group"><div class="nav-group-title">' + escapeHtml(group.title) + '</div>';
      group.items.forEach(function (item) {
        const pending = !item.href || item.href === '#';
        let badge = typeof item.badge === 'function' ? item.badge() : item.badge;
        if (badge == null || badge === 0) badge = '';
        html += '<a class="nav-item' + (item.key === activeKey ? ' active' : '') + '" href="' + (item.href || '#') +
          '" data-key="' + item.key + '" data-label="' + escapeHtml(item.label) + '"' +
          (pending ? ' data-pending="1"' : '') + ' title="' + escapeHtml(item.label) + '">' +
          icon(item.icon, 18) +
          '<span class="nav-label">' + escapeHtml(item.label) + '</span>' +
          (badge ? '<span class="nav-badge">' + (badge > 99 ? '99+' : badge) + '</span>' : '') +
          '</a>';
      });
      html += '</div>';
    });

    html += '</nav><div class="sidebar-footer">' +
      avatarHTML('超', 'admin', 32) +
      '<div class="user-meta"><div class="user-name">超级管理员</div><div class="user-role">平台管理员</div></div>' +
      '<button type="button" class="icon-btn logout-btn" data-action="logout" title="退出登录" aria-label="退出登录">' + icon('logout', 18) + '</button>' +
      '</div>';

    return html;
  }

  function buildTopbar(activeKey) {
    const found = findMenu(activeKey);
    const notify = window.REPORTS || [];
    const pendingReports = notify.filter(function (item) { return item.status === 'pending'; }).length;
    const pendingFeedbacks = (window.FEEDBACKS || []).filter(function (item) { return item.status === 'pending'; }).length;

    return '<button type="button" class="icon-btn" id="sidebar-toggle" title="折叠 / 展开侧边栏" aria-label="折叠或展开侧边栏">' + icon('panel-left', 18) + '</button>' +
      '<div class="breadcrumb"><span>' + escapeHtml(found.group) + '</span><span class="sep">/</span><span class="current">' + escapeHtml(found.item.label) + '</span></div>' +
      '<div class="topbar-search">' + icon('search', 16) +
      '<input type="text" id="global-search" placeholder="搜索学号、昵称、帖子内容…" aria-label="全局搜索">' +
      '<kbd>⌘K</kbd></div>' +
      '<div class="topbar-actions">' +
      '<div class="dropdown" id="notify-dropdown">' +
      '<button type="button" class="icon-btn bell-btn" title="通知" aria-label="通知">' + icon('bell', 18) + '<span class="dot"></span></button>' +
      '<div class="menu-panel" style="width:280px">' +
      '<div class="menu-label">通知中心</div>' +
      '<div class="menu-item" data-notify="reports">' + icon('flag', 16) + '有 ' + pendingReports + ' 条举报待处理</div>' +
      '<div class="menu-item" data-notify="feedbacks">' + icon('lightbulb', 16) + '有 ' + pendingFeedbacks + ' 条反馈待回复</div>' +
      '<div class="menu-item" data-notify="system">' + icon('check-circle', 16) + '系统运行正常，暂无异常</div>' +
      '</div></div>' +
      '<button type="button" class="icon-btn" id="theme-toggle" title="切换主题" aria-label="切换主题">' + icon('moon', 18) + '</button>' +
      '<div class="dropdown" id="profile-dropdown">' +
      '<button type="button" class="avatar-btn" id="profile-trigger" title="超级管理员" aria-label="账户菜单">' + avatarHTML('超', 'admin', 32) + '</button>' +
      '<div class="menu-panel">' +
      '<div class="menu-label">超级管理员 · admin</div>' +
      '<div class="menu-item" data-action="profile">' + icon('settings', 16) + '个人设置</div>' +
      '<div class="menu-divider"></div>' +
      '<div class="menu-item danger" data-action="logout">' + icon('logout', 16) + '退出登录</div>' +
      '</div></div></div>';
  }

  function renderLayout(activeKey) {
    if (document.body.classList.contains('has-layout')) return;
    const main = document.querySelector('main[data-page]');
    if (!main) return;

    const shell = el('div', { class: 'app-shell' });
    const sidebar = el('aside', { class: 'sidebar', id: 'app-sidebar', html: buildSidebar(activeKey) });
    const appBody = el('div', { class: 'app-body' });
    const topbar = el('header', { class: 'topbar', html: buildTopbar(activeKey) });

    main.classList.add('content');
    main.removeAttribute('hidden');

    document.body.insertBefore(shell, main);
    shell.appendChild(sidebar);
    shell.appendChild(appBody);
    appBody.appendChild(topbar);
    appBody.appendChild(main);

    document.body.classList.add('has-layout');
    document.body.setAttribute('data-page', activeKey);

    /* ---- 侧边栏折叠 ---- */
    const collapseKey = 'sdu-admin-sidebar-collapsed';
    if (readStore(collapseKey) === '1' || window.innerWidth <= 1024) {
      document.body.classList.add('is-collapsed');
    }

    const toggleBtn = document.getElementById('sidebar-toggle');
    if (toggleBtn) {
      toggleBtn.addEventListener('click', function () {
        document.body.classList.toggle('is-collapsed');
        writeStore(collapseKey, document.body.classList.contains('is-collapsed') ? '1' : '0');
      });
    }

    window.addEventListener('resize', debounce(function () {
      if (window.innerWidth <= 1024) document.body.classList.add('is-collapsed');
    }, 150));

    /* ---- 导航 ---- */
    $$('.nav-item', sidebar).forEach(function (link) {
      link.addEventListener('click', function (event) {
        if (link.getAttribute('data-pending') === '1') {
          event.preventDefault();
          toast('「' + link.getAttribute('data-label') + '」页面正在建设中', 'info');
        }
      });
    });

    /* ---- 顶栏下拉 ---- */
    const notifyDropdown = document.getElementById('notify-dropdown');
    const profileDropdown = document.getElementById('profile-dropdown');

    function closeMenus(except) {
      $$('.menu-panel.open').forEach(function (panel) {
        if (!except || !except.contains(panel)) panel.classList.remove('open');
      });
    }

    if (notifyDropdown) {
      const trigger = $('.icon-btn', notifyDropdown);
      trigger.addEventListener('click', function (event) {
        event.stopPropagation();
        const panel = $('.menu-panel', notifyDropdown);
        const willOpen = !panel.classList.contains('open');
        closeMenus(notifyDropdown);
        panel.classList.toggle('open', willOpen);
      });
      $$('.menu-item', notifyDropdown).forEach(function (item) {
        item.addEventListener('click', function () {
          const kind = item.getAttribute('data-notify');
          closeMenus();
          if (kind === 'reports') toast('举报处理页面待建设，当前待处理 ' + pendingReports + ' 条', 'info');
          else if (kind === 'feedbacks') toast('用户反馈页面待建设，当前待回复 ' + pendingFeedbacks + ' 条', 'info');
          else toast('系统运行正常', 'success');
        });
      });
    }

    if (profileDropdown) {
      const trigger = document.getElementById('profile-trigger');
      trigger.addEventListener('click', function (event) {
        event.stopPropagation();
        const panel = $('.menu-panel', profileDropdown);
        const willOpen = !panel.classList.contains('open');
        closeMenus(profileDropdown);
        panel.classList.toggle('open', willOpen);
      });
      $$('.menu-item', profileDropdown).forEach(function (item) {
        item.addEventListener('click', function () {
          const action = item.getAttribute('data-action');
          closeMenus();
          if (action === 'profile') toast('个人设置页面正在建设中', 'info');
          if (action === 'logout') confirmLogout();
        });
      });
    }

    document.addEventListener('click', function () { closeMenus(); });

    /* ---- 侧边栏退出 ---- */
    const sidebarLogout = $('[data-action="logout"]', sidebar);
    if (sidebarLogout) sidebarLogout.addEventListener('click', confirmLogout);

    function confirmLogout() {
      modal({
        title: '退出登录',
        desc: '退出后需要重新验证管理员身份，确定要退出吗？',
        confirmText: '退出登录',
        danger: true,
        onConfirm: function () {
          window.location.href = 'login.html';
        }
      });
    }

    /* ---- 主题切换 ---- */
    const themeToggle = document.getElementById('theme-toggle');
    const savedTheme = readStore('sdu-admin-theme');
    if (savedTheme === 'dark') document.documentElement.setAttribute('data-theme', 'dark');

    function syncThemeIcon() {
      if (!themeToggle) return;
      const isDark = document.documentElement.getAttribute('data-theme') === 'dark';
      themeToggle.innerHTML = icon(isDark ? 'sun' : 'moon', 18);
    }
    syncThemeIcon();
    if (themeToggle) {
      themeToggle.addEventListener('click', function () {
        const isDark = document.documentElement.getAttribute('data-theme') === 'dark';
        if (isDark) document.documentElement.removeAttribute('data-theme');
        else document.documentElement.setAttribute('data-theme', 'dark');
        writeStore('sdu-admin-theme', isDark ? 'light' : 'dark');
        syncThemeIcon();
      });
    }

    /* ---- 全局搜索 ---- */
    const searchInput = document.getElementById('global-search');
    if (searchInput) {
      searchInput.addEventListener('keydown', function (event) {
        if (event.key !== 'Enter') return;
        const keyword = searchInput.value.trim();
        if (!keyword) return;
        if (document.body.getAttribute('data-page') === 'users') {
          window.dispatchEvent(new CustomEvent('admin:global-search', { detail: keyword }));
        } else {
          window.location.href = 'users.html?keyword=' + encodeURIComponent(keyword);
        }
      });
    }

    document.addEventListener('keydown', function (event) {
      if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === 'k') {
        event.preventDefault();
        if (searchInput) searchInput.focus();
      }
    });
  }

  /* ========================================================================
     9. 初始化
     ======================================================================== */
  function init() {
    const main = document.querySelector('main[data-page]');
    if (main) renderLayout(main.getAttribute('data-page'));
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

  /* ========================================================================
     10. 对外暴露（命名空间 + 少量工具函数）
     ======================================================================== */
  window.App = {
    icon: icon,
    $: $,
    $$: $$,
    el: el,
    escapeHtml: escapeHtml,
    fmtNum: fmtNum,
    fmtDate: fmtDate,
    fmtDateTime: fmtDateTime,
    fmtRelative: fmtRelative,
    debounce: debounce,
    avatarHTML: avatarHTML,
    pill: pill,
    emptyHTML: emptyHTML,
    toast: toast,
    modal: modal,
    drawer: drawer,
    renderTable: renderTable,
    renderPagination: renderPagination,
    renderLayout: renderLayout,
    menu: MENU
  };

  window.$ = $;
  window.$$ = $$;
  window.el = el;
  window.fmtNum = fmtNum;
  window.fmtDate = fmtDate;
  window.fmtRelative = fmtRelative;
})(window, document);
