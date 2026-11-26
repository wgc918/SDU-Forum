#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# 一键运行集成测试
#
# 用法:
#   tests/integration/scripts/run_integration.sh [--repeat N] [--no-build] [其他 ctest 参数]
#
# 行为:
#   1. 清理 LD_LIBRARY_PATH（Trae 自带旧版 libstdc++ 会导致二进制无法启动）
#   2. 可选增量构建（默认构建）
#   3. 重建 it_* 测试表
#   4. 执行 label=integration 的全部用例，输出 JUnit XML
#   5. 生成可视化 HTML 报告
# ---------------------------------------------------------------------------
set -euo pipefail

unset LD_LIBRARY_PATH

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_DIR="${SDU_BUILD_DIR:-${BACKEND_DIR}/build}"
REPORT_DIR="${BUILD_DIR}/reports"
JUNIT="${REPORT_DIR}/integration-junit.xml"

DO_BUILD=1
REPEAT_ARGS=()
CTEST_EXTRA=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-build) DO_BUILD=0; shift ;;
        --repeat)
            REPEAT_ARGS=(--repeat "until-pass:$2")
            shift 2
            ;;
        *)
            CTEST_EXTRA+=("$1")
            shift
            ;;
    esac
done

export SDU_FORUM_CONFIG="${BACKEND_DIR}/tests/integration/config/config-int.ini"
mkdir -p "${REPORT_DIR}"

if [[ "${DO_BUILD}" -eq 1 ]]; then
    echo "[run_integration] building integration tests..."
    cmake -S "${BACKEND_DIR}" -B "${BUILD_DIR}" -DSDU_BUILD_INTEGRATION_TESTS=ON >/dev/null
    cmake --build "${BUILD_DIR}" -j"$(nproc)"
fi

echo "[run_integration] recreating it_* tables..."
bash "${SCRIPT_DIR}/setup_db.sh"

echo "[run_integration] running ctest (label=integration)..."
STATUS=0
ctest --test-dir "${BUILD_DIR}" -L integration --output-on-failure \
    --output-junit "${JUNIT}" "${REPEAT_ARGS[@]}" "${CTEST_EXTRA[@]}" || STATUS=$?

if [[ -f "${JUNIT}" ]]; then
    python3 "${SCRIPT_DIR}/report_html.py" "${JUNIT}" "${REPORT_DIR}/integration-report.html" || true
    echo "[run_integration] report: ${REPORT_DIR}/integration-report.html"
fi

exit "${STATUS}"
