#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# 集成测试数据库初始化：在当前库中重建 it_ 前缀表。
# 安全约束：schema.sql 中的表名全部以 it_ 开头，不会触碰业务表。
# ---------------------------------------------------------------------------
set -euo pipefail

# 避免继承 Trae 自带的旧版 libstdc++（会导致 mysql 客户端 GLIBCXX 报错）
unset LD_LIBRARY_PATH

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCHEMA="${SCRIPT_DIR}/../sql/schema.sql"

DB_HOST="${SDU_IT_DB_HOST:-127.0.0.1}"
DB_PORT="${SDU_IT_DB_PORT:-3306}"
DB_USER="${SDU_IT_DB_USER:-wgc}"
DB_PASSWORD="${SDU_IT_DB_PASSWORD:-123456}"
DB_NAME="${SDU_IT_DB_NAME:-testdb}"

if [[ ! -f "${SCHEMA}" ]]; then
    echo "[setup_db] schema not found: ${SCHEMA}" >&2
    exit 1
fi

if ! command -v mysql >/dev/null 2>&1; then
    echo "[setup_db] mysql client not found in PATH" >&2
    exit 1
fi

export MYSQL_PWD="${DB_PASSWORD}"

mysql --protocol=TCP -h "${DB_HOST}" -P "${DB_PORT}" -u "${DB_USER}" "${DB_NAME}" \
    --default-character-set=utf8mb4 < "${SCHEMA}"

echo "[setup_db] it_* tables recreated in ${DB_NAME}@${DB_HOST}:${DB_PORT}"
