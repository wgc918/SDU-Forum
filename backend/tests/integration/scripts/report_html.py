#!/usr/bin/env python3
"""将 CTest 生成的 JUnit XML 转换为可视化 HTML 报告。"""

import sys
import xml.etree.ElementTree as ET
from datetime import datetime
from html import escape


def parse_junit(path):
    tree = ET.parse(path)
    root = tree.getroot()

    suites = root.findall("testsuite") if root.tag == "testsuites" else [root]
    rows = []
    totals = {"tests": 0, "failures": 0, "errors": 0, "skipped": 0, "time": 0.0}

    for suite in suites:
        for case in suite.findall("testcase"):
            name = case.get("name", "")
            classname = case.get("classname", "")
            time_s = float(case.get("time", "0") or 0)
            status = "passed"
            detail = ""
            for child in case:
                tag = child.tag.lower()
                if tag == "failure":
                    status = "failed"
                    detail = (child.get("message") or child.text or "").strip()
                    totals["failures"] += 1
                elif tag == "error":
                    status = "error"
                    detail = (child.get("message") or child.text or "").strip()
                    totals["errors"] += 1
                elif tag == "skipped":
                    status = "skipped"
                    detail = (child.get("message") or "").strip()
                    totals["skipped"] += 1
            totals["tests"] += 1
            totals["time"] += time_s
            rows.append({
                "name": name,
                "classname": classname,
                "status": status,
                "time": time_s,
                "detail": detail,
            })

    return rows, totals


def render(rows, totals, source):
    generated = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    total = totals["tests"] or 1
    failed = totals["failures"] + totals["errors"]
    pass_rate = 100.0 * (total - failed - totals["skipped"]) / total

    color = "#16a34a" if failed == 0 else "#dc2626"
    body_rows = []
    for row in rows:
        status = row["status"]
        badge = {
            "passed": "background:#dcfce7;color:#166534",
            "failed": "background:#fee2e2;color:#991b1b",
            "error": "background:#fee2e2;color:#991b1b",
            "skipped": "background:#fef9c3;color:#854d0e",
        }.get(status, "")
        body_rows.append(
            "<tr>"
            f"<td>{escape(row['classname'])}</td>"
            f"<td>{escape(row['name'])}</td>"
            f"<td><span style='padding:2px 8px;border-radius:10px;font-size:12px;{badge}'>"
            f"{escape(status)}</span></td>"
            f"<td style='text-align:right'>{row['time']:.3f}s</td>"
            f"<td><pre>{escape(row['detail'][:800])}</pre></td>"
            "</tr>"
        )

    return f"""<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<title>SDU-Forum 集成测试报告</title>
<style>
  body {{ font-family: -apple-system, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
         margin: 32px; color: #111827; background: #f9fafb; }}
  h1 {{ font-size: 22px; margin: 0 0 4px; }}
  .meta {{ color: #6b7280; font-size: 13px; margin-bottom: 20px; }}
  .cards {{ display: flex; gap: 12px; margin-bottom: 24px; flex-wrap: wrap; }}
  .card {{ background: #fff; border: 1px solid #e5e7eb; border-radius: 10px;
           padding: 14px 20px; min-width: 120px; }}
  .card .label {{ color: #6b7280; font-size: 12px; }}
  .card .value {{ font-size: 24px; font-weight: 600; margin-top: 4px; }}
  table {{ width: 100%; border-collapse: collapse; background: #fff;
           border: 1px solid #e5e7eb; border-radius: 10px; overflow: hidden; }}
  th, td {{ padding: 10px 12px; border-bottom: 1px solid #f3f4f6;
            font-size: 13px; vertical-align: top; text-align: left; }}
  th {{ background: #f3f4f6; font-weight: 600; }}
  pre {{ margin: 0; white-space: pre-wrap; word-break: break-word; color: #7f1d1d; }}
</style>
</head>
<body>
  <h1>SDU-Forum 后端集成测试报告</h1>
  <div class="meta">来源: {escape(source)} · 生成时间: {generated}</div>
  <div class="cards">
    <div class="card"><div class="label">用例总数</div><div class="value">{totals['tests']}</div></div>
    <div class="card"><div class="label">失败/错误</div>
      <div class="value" style="color:{color}">{failed}</div></div>
    <div class="card"><div class="label">跳过</div>
      <div class="value" style="color:#ca8a04">{totals['skipped']}</div></div>
    <div class="card"><div class="label">通过率</div><div class="value">{pass_rate:.1f}%</div></div>
    <div class="card"><div class="label">总耗时</div>
      <div class="value">{totals['time']:.1f}s</div></div>
  </div>
  <table>
    <thead><tr><th>测试套件</th><th>用例</th><th>状态</th><th>耗时</th><th>详情</th></tr></thead>
    <tbody>
      {''.join(body_rows) if body_rows else '<tr><td colspan="5">无用例</td></tr>'}
    </tbody>
  </table>
</body>
</html>
"""


def main():
    if len(sys.argv) < 2:
        print("usage: report_html.py <junit.xml> [output.html]", file=sys.stderr)
        return 2

    source = sys.argv[1]
    output = sys.argv[2] if len(sys.argv) > 2 else "integration-report.html"
    rows, totals = parse_junit(source)
    html = render(rows, totals, source)
    with open(output, "w", encoding="utf-8") as handle:
        handle.write(html)
    print(f"[report_html] {totals['tests']} tests, {totals['failures']} failures -> {output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
