#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""SDU-Forum 管理员账号插入脚本。

独立运行，不修改也不依赖后端源码，仅使用 Python 3 标准库和系统 mysql 客户端。
密码哈希与 backend/infrastructrue/security/password_hasher.cpp 完全一致：
    PBKDF2-HMAC-SHA512，派生密钥 32 字节，随机盐 16 字节，默认迭代 150000 次。
对应表结构（admin_account）：
    admin_id CHAR(8)、password_hash CHAR(64)、salt CHAR(32)、iterations INT UNSIGNED

示例：
    # 交互输入密码
    python3 scripts/create_admin_account.py --admin-id admin001

    # 直接指定密码（1-8 个字符，与登录接口限制一致）
    python3 scripts/create_admin_account.py -i admin001 -p 12345678

    # 自定义数据库连接（默认读取 config/config-dev.ini 的开发配置）
    python3 scripts/create_admin_account.py -i admin001 -p 12345678 \
        --host 127.0.0.1 --port 3306 --db-user root --db-password xxx -d testdb

    # 账号已存在时重置密码
    python3 scripts/create_admin_account.py -i admin001 -p 87654321 --update

    # 只打印 SQL，不连接数据库
    python3 scripts/create_admin_account.py -i admin001 -p 12345678 --dry-run
"""

import argparse
import getpass
import hashlib
import os
import re
import secrets
import shutil
import subprocess
import sys

SALT_BYTES = 16
DERIVED_KEY_BYTES = 32
DEFAULT_ITERATIONS = 150000
MAX_ADMIN_ID_LENGTH = 8
MAX_PASSWORD_LENGTH = 8


def hash_password(password: str, iterations: int):
    """生成与后端一致的 PBKDF2-HMAC-SHA512 哈希，返回 (hash_hex, salt_hex)。"""
    salt = secrets.token_bytes(SALT_BYTES)
    derived = hashlib.pbkdf2_hmac(
        "sha512",
        password.encode("utf-8"),
        salt,
        iterations,
        dklen=DERIVED_KEY_BYTES,
    )
    return derived.hex(), salt.hex()


def sql_string(value: str) -> str:
    """转义 MySQL 字符串字面量。"""
    escaped = value.replace("\\", "\\\\").replace("'", "''")
    return "'" + escaped + "'"


def quote_identifier(identifier: str) -> str:
    """校验并引用库名/表名，支持 db.table 形式。"""
    if not re.fullmatch(r"[A-Za-z0-9_$]+(\.[A-Za-z0-9_$]+)?", identifier):
        raise ValueError("非法的数据库或表名: %s" % identifier)
    return ".".join("`%s`" % part for part in identifier.split("."))


def run_mysql(args, sql: str) -> str:
    """通过系统 mysql 客户端执行 SQL，返回标准输出。"""
    mysql = shutil.which("mysql")
    if mysql is None:
        raise RuntimeError("未找到 mysql 客户端，请先安装 MySQL/MariaDB 客户端")

    env = os.environ.copy()
    # 避免外部注入的 LD_LIBRARY_PATH 让系统 mysql 加载到不兼容的 libstdc++。
    env.pop("LD_LIBRARY_PATH", None)
    if args.db_password:
        env["MYSQL_PWD"] = args.db_password

    cmd = [
        mysql,
        "-h", args.host,
        "-P", str(args.port),
        "-u", args.db_user,
        "--default-character-set=utf8mb4",
        "--batch",
        "--skip-column-names",
        "-e", sql,
    ]
    result = subprocess.run(cmd, env=env, capture_output=True, text=True)
    if result.returncode != 0:
        message = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError("mysql 执行失败: %s" % message)
    return result.stdout.strip()


def parse_args():
    parser = argparse.ArgumentParser(
        description="向 SDU-Forum 的 admin_account 表插入管理员账号",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("-i", "--admin-id", required=True,
                        help="管理员登录账号，最长 8 个字符")
    parser.add_argument("-p", "--password",
                        help="管理员密码，最长 8 个字符；省略时交互输入")
    parser.add_argument("--iterations", type=int, default=DEFAULT_ITERATIONS,
                        help="PBKDF2 迭代次数")
    parser.add_argument("--host", default=os.getenv("SDU_DB_HOST", "127.0.0.1"),
                        help="数据库地址")
    parser.add_argument("--port", type=int,
                        default=int(os.getenv("SDU_DB_PORT", "3306")),
                        help="数据库端口")
    parser.add_argument("-u", "--db-user",
                        default=os.getenv("SDU_DB_USER", "wgc"),
                        help="数据库用户名")
    parser.add_argument("--db-password",
                        default=os.getenv("SDU_DB_PASSWORD", "123456"),
                        help="数据库密码，也可用环境变量 SDU_DB_PASSWORD")
    parser.add_argument("-d", "--database",
                        default=os.getenv("SDU_DB_NAME", "testdb"),
                        help="数据库名")
    parser.add_argument("-t", "--table", default="admin_account",
                        help="管理员表名，可带库名前缀")
    parser.add_argument("--update", action="store_true",
                        help="账号已存在时更新其密码")
    parser.add_argument("--dry-run", action="store_true",
                        help="只打印将要执行的 SQL，不连接数据库")
    return parser.parse_args()


def validate(args):
    args.admin_id = args.admin_id.strip()
    if not args.admin_id:
        raise ValueError("admin_id 不能为空")
    if len(args.admin_id.encode("utf-8")) > MAX_ADMIN_ID_LENGTH:
        raise ValueError("admin_id 过长，最多 %d 个字符" % MAX_ADMIN_ID_LENGTH)

    if args.password is None:
        args.password = getpass.getpass("请输入管理员密码（最多 %d 个字符）: "
                                        % MAX_PASSWORD_LENGTH)
        confirm = getpass.getpass("请再次输入密码: ")
        if args.password != confirm:
            raise ValueError("两次输入的密码不一致")
    if not args.password:
        raise ValueError("密码不能为空")
    if len(args.password.encode("utf-8")) > MAX_PASSWORD_LENGTH:
        raise ValueError("密码过长，最多 %d 个字符" % MAX_PASSWORD_LENGTH)

    if args.iterations <= 0:
        raise ValueError("iterations 必须为正整数")


def main() -> int:
    args = parse_args()
    try:
        validate(args)
        target = "%s.%s" % (quote_identifier(args.database), quote_identifier(args.table))
        password_hash, salt = hash_password(args.password, args.iterations)

        if args.dry_run:
            print("-- 账号: %s，迭代次数: %d" % (args.admin_id, args.iterations))
            print("INSERT INTO %s (`admin_id`,`password_hash`,`salt`,`iterations`) "
                  "VALUES (%s,%s,%s,%d);"
                  % (target, sql_string(args.admin_id), sql_string(password_hash),
                     sql_string(salt), args.iterations))
            return 0

        existing = run_mysql(
            args,
            "SELECT id FROM %s WHERE admin_id=%s LIMIT 1;"
            % (target, sql_string(args.admin_id)),
        )

        if existing and not args.update:
            raise ValueError("账号 %s 已存在（id=%s），如需重置密码请加 --update"
                             % (args.admin_id, existing.splitlines()[0]))

        if existing:
            sql = ("UPDATE %s SET password_hash=%s, salt=%s, iterations=%d "
                   "WHERE admin_id=%s;"
                   % (target, sql_string(password_hash), sql_string(salt),
                      args.iterations, sql_string(args.admin_id)))
            action = "已更新"
        else:
            sql = ("INSERT INTO %s (`admin_id`,`password_hash`,`salt`,`iterations`) "
                   "VALUES (%s,%s,%s,%d);"
                   % (target, sql_string(args.admin_id), sql_string(password_hash),
                      sql_string(salt), args.iterations))
            action = "已插入"

        run_mysql(args, sql)

        row = run_mysql(
            args,
            "SELECT id, admin_id, iterations FROM %s WHERE admin_id=%s LIMIT 1;"
            % (target, sql_string(args.admin_id)),
        )
        print("[OK] %s管理员账号 %s" % (action, args.admin_id))
        if row:
            print("     记录: id=%s" % ", ".join(row.split("\t")))
        return 0
    except (ValueError, RuntimeError) as exc:
        print("[ERROR] %s" % exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
