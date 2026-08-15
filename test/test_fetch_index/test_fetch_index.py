#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JustDid /sync/fetch-index 接口测试脚本（add-fetch-index-api 变更验证）

用法：
  1. 先启动 JustDid.exe（双击运行，HTTP 服务默认端口 18080）
  2. 运行测试：
       python test/test_fetch_index/test_fetch_index.py                     # 跑全部场景
       python test/test_fetch_index/test_fetch_index.py --scenario single   # 跑单个场景
       python test/test_fetch_index/test_fetch_index.py --app-root D:/.../build/src/Release

场景说明：
  single    单日期请求 → 200，index 条目字段与 pc_daliy_report_index 行一致（path 带 data/ 前缀）
  multi     多日期请求（混合有/无条目日期）→ 仅返回有条目的日期，按日期升序
  missing   全部日期无索引条目 → 404 + {"code":1,"message":"无文件"}
  badparam  参数错误 → 400 code=-2/-3
  dbonly    预置 file_size=0 的索引行（不建磁盘文件，跑完删除）→ 仍返回该条目，验证 DB 为准

注意：dbonly 场景会临时在 just_do.db 的 pc_daliy_report_index 表中插入并删除一行。
"""

import argparse
import json
import os
import sqlite3
import sys
import http.client

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 18080

MISSING_DATE_1 = "19700101"   # 无索引条目
MISSING_DATE_2 = "19700102"   # 无索引条目
DBONLY_DATE = "20200102"      # dbonly 场景用（预置索引行，不建磁盘文件）


def fetch_index_req(dates, host, port):
    conn = http.client.HTTPConnection(host, port, timeout=60)
    # dates=None 时发送空对象 {}（模拟未提供日期参数）
    body = {} if dates is None else {"dates": dates}
    conn.request("POST", "/sync/fetch-index",
                 body=json.dumps(body),
                 headers={"Content-Type": "application/json"})
    resp = conn.getresponse()
    raw = resp.read()
    ctype = resp.getheader("Content-Type", "")
    conn.close()
    try:
        j = json.loads(raw.decode("utf-8"))
    except Exception:
        j = {"raw": raw[:200]}
    return resp.status, ctype, j


def db_query(app_root, sql, params=()):
    conn = sqlite3.connect(os.path.join(app_root, "just_do.db"), timeout=5)
    try:
        return conn.execute(sql, params).fetchall()
    finally:
        conn.close()


def db_execute(app_root, sql, params=()):
    conn = sqlite3.connect(os.path.join(app_root, "just_do.db"), timeout=5)
    try:
        conn.execute(sql, params)
        conn.commit()
    finally:
        conn.close()


def existing_entries(app_root):
    """索引表中已有条目：[(year, month, day, path, file_size), ...] 按日期升序"""
    return db_query(app_root,
                    "SELECT year, month, day, path, file_size "
                    "FROM pc_daliy_report_index ORDER BY year, month, day")


def date_str(y, m, d):
    return f"{y:04d}{m:02d}{d:02d}"


def entry_json(row):
    y, m, d, path, size = row
    return {"year": y, "month": m, "day": d, "path": path, "file_size": size}


def scenario_single(host, port, app_root):
    rows = existing_entries(app_root)
    if not rows:
        print("[FAIL] single 索引表无数据，无法测试（请先在 app 中产生日报数据）")
        return False
    y, m, d, path, size = rows[0]
    ds = date_str(y, m, d)

    s, ctype, j = fetch_index_req([ds], host, port)
    expect = entry_json(rows[0])
    ok = (s == 200 and "application/json" in ctype and j.get("code") == 0
          and j.get("message") == "成功"
          and j.get("index") == [expect]
          and path.startswith("data/"))
    print(f"{'[PASS]' if ok else '[FAIL]'} single 单日期请求 → 200 + 条目与 DB 行一致（path 带 data/ 前缀）")
    print(f"       请求 {[ds]}，期望条目 {expect}，实际 HTTP {s} body={j}")
    return ok


def scenario_multi(host, port, app_root):
    rows = existing_entries(app_root)
    if len(rows) < 2:
        print("[FAIL] multi 索引表不足 2 条数据，无法测试升序与过滤")
        return False
    rows3 = rows[:3]
    exist_dates = [date_str(y, m, d) for y, m, d, _, _ in rows3]
    dates = [MISSING_DATE_1] + exist_dates[1:] + [MISSING_DATE_2] + exist_dates[:1]
    # 期望：仅有条目的日期、按日期升序（与 SQL 排序一致）
    expect = [entry_json(r) for r in rows3]

    s, ctype, j = fetch_index_req(dates, host, port)
    ok = (s == 200 and "application/json" in ctype and j.get("code") == 0
          and j.get("index") == expect)
    print(f"{'[PASS]' if ok else '[FAIL]'} multi 混合日期请求 → 仅返回有条目的日期、升序")
    print(f"       请求 {dates}")
    if not ok:
        print(f"       期望 index={expect}")
        print(f"       实际 HTTP {s} body={j}")
    return ok


def scenario_missing(host, port, app_root):
    s, ctype, j = fetch_index_req([MISSING_DATE_1, MISSING_DATE_2], host, port)
    ok = (s == 404 and j.get("code") == 1 and j.get("message") == "无文件")
    print(f"{'[PASS]' if ok else '[FAIL]'} missing 全部日期无索引条目 → 404 + {{\"code\":1,\"message\":\"无文件\"}}")
    print(f"       HTTP {s}, body={j}")
    return ok


def scenario_badparam(host, port, app_root):
    ok_all = True

    # 无日期参数 → 400 code=-3
    s, _, j = fetch_index_req(None, host, port)  # 空对象 {}
    ok = (s == 400 and j.get("code") == -3)
    ok_all &= ok
    print(f"{'[PASS]' if ok else '[FAIL]'} badparam 无日期参数 → 400 code=-3  (HTTP {s}, {j})")

    # 超过 32 个文件 → 400 code=-2
    dates = [f"2026{i:02d}{d:02d}" for i in range(1, 3) for d in range(1, 17)]  # 32 个
    dates.append("20260301")  # 第 33 个
    s, _, j = fetch_index_req(dates, host, port)
    ok = (s == 400 and j.get("code") == -2)
    ok_all &= ok
    print(f"{'[PASS]' if ok else '[FAIL]'} badparam 33 个日期超上限 → 400 code=-2  (HTTP {s}, {j})")
    return ok_all


def scenario_dbonly(host, port, app_root):
    # 1. 预置索引行：file_size=0，不建磁盘文件
    db_execute(app_root,
               "INSERT OR REPLACE INTO pc_daliy_report_index "
               "(year, month, day, path, file_size, version) VALUES (?, ?, ?, ?, 0, 1)",
               (2020, 1, 2, "data/2020/01/02.txt"))
    print(f"       已预置索引行: {DBONLY_DATE} (file_size=0, 无磁盘文件)")
    try:
        s, ctype, j = fetch_index_req([DBONLY_DATE], host, port)
        expect = [{"year": 2020, "month": 1, "day": 2,
                   "path": "data/2020/01/02.txt", "file_size": 0}]
        ok = (s == 200 and "application/json" in ctype and j.get("code") == 0
              and j.get("index") == expect)
        print(f"{'[PASS]' if ok else '[FAIL]'} dbonly 仅有索引行（file_size=0）→ 照常返回，DB 为准")
        if not ok:
            print(f"       期望 index={expect}，实际 HTTP {s} body={j}")
        return ok
    finally:
        # 2. 清理：删除预置索引行
        db_execute(app_root,
                   "DELETE FROM pc_daliy_report_index "
                   "WHERE year=? AND month=? AND day=?",
                   (2020, 1, 2))
        print(f"       已清理索引行: {DBONLY_DATE}")


SCENARIOS = {
    "single": scenario_single,
    "multi": scenario_multi,
    "missing": scenario_missing,
    "badparam": scenario_badparam,
    "dbonly": scenario_dbonly,
}


def find_app_root(cli_root):
    """定位 just_do.db 所在目录（exe 启动目录）"""
    candidates = []
    if cli_root:
        candidates.append(os.path.abspath(cli_root))
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates.append(os.path.abspath(
        os.path.join(script_dir, "..", "..", "build", "src", "Release")))
    candidates.append(os.getcwd())
    for c in candidates:
        if os.path.exists(os.path.join(c, "just_do.db")):
            return c
    return candidates[0]


def main():
    # Windows 控制台默认 GBK，强制 UTF-8 输出避免中文乱码
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass

    parser = argparse.ArgumentParser(description="JustDid /sync/fetch-index 接口测试")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--scenario", choices=["all"] + list(SCENARIOS), default="all")
    parser.add_argument("--app-root", default=None,
                        help="exe 启动目录（just_do.db 所在位置），DB 比对与 dbonly 场景需要")
    args = parser.parse_args()

    app_root = find_app_root(args.app_root)
    print(f"== /sync/fetch-index 接口测试  host={args.host}:{args.port}  app-root={app_root} ==\n")

    names = list(SCENARIOS) if args.scenario == "all" else [args.scenario]
    results = []
    for name in names:
        fn = SCENARIOS[name]
        try:
            results.append((name, fn(args.host, args.port, app_root)))
        except ConnectionRefusedError:
            print(f"[FAIL] {name} 连接被拒绝——请确认 JustDid.exe 已启动且 HTTP 服务已开启")
            results.append((name, False))
        except Exception as e:
            print(f"[FAIL] {name} 异常: {e!r}")
            results.append((name, False))
        print()

    failed = [n for n, ok in results if not ok]
    print(f"== 结果: {len(results) - len(failed)}/{len(results)} 通过 ==")
    if failed:
        print(f"失败场景: {', '.join(failed)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
