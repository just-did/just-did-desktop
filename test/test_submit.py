#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JustDid /sync/submit 接口测试脚本

用法：
  1. 先启动 JustDid.exe（双击运行，HTTP 服务默认端口 18080）
  2. 运行测试：
       python test/test_submit.py                    # 跑全部场景
       python test/test_submit.py --scenario normal # 跑单个场景
       python test/test_submit.py --scenario resume --app-root D:/.../build/src/Release

场景说明：
  normal          正常提交（2030-01-01/02 两个日期）→ 期望 200 code=0
  idempotent      同批ID连续提交两次 → 第二次幂等返回 200 code=0
  badid           批ID含非法字符 → 期望 400 -3
  corrupt         请求体不是 ZIP → 期望 400 -3
  foldermismatch  ZIP内文件夹名与批ID不一致 → 期望 400 -3
  baddate         staging 文件名日期非法 → 期望 400 -3
  emptyfile       一个空文件+一个有效文件 → 期望 200 code=0，空日期被忽略
  oversize        解压后超过 5MB → 期望 400 -2
  resume          模拟中断续跑：预置「覆盖中」记录+快照，提交同批ID → 期望 200 code=0

注意：测试数据会写入 2026-08-01 ~ 2026-08-03 的日报文件。
"""

import argparse
import io
import json
import os
import re
import sqlite3
import sys
import uuid
import zipfile
import http.client

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 18080

# 测试日期（2026-08-01 ~ 03）
TEST_DATE_1 = "20260801"
TEST_DATE_2 = "20260802"
TEST_DATE_3 = "20260803"

SAMPLE_RECORDS_1 = "09:54\n完成需求文档初稿\n\n10:30\n参加项目评审会议\n"
SAMPLE_RECORDS_2 = "14:00\n修复登录页面样式问题\n"


def build_zip(batch_id, files):
    """files: {文件名: 内容}，文件名相对 {batchId}/ 文件夹"""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as zf:
        for name, content in files.items():
            zf.writestr(f"{batch_id}/{name}", content)
    return buf.getvalue()


def submit(batch_id, data, host, port):
    conn = http.client.HTTPConnection(host, port, timeout=60)
    conn.request("POST", "/sync/submit", body=data, headers={
        "Content-Type": "application/zip",
        "X-Batch-ID": batch_id,
    })
    resp = conn.getresponse()
    body = resp.read()
    conn.close()
    try:
        return resp.status, json.loads(body.decode("utf-8"))
    except Exception:
        return resp.status, {"raw": body[:200]}


def check(name, actual_status, actual_json, expect_status, expect_code):
    ok = (actual_status == expect_status
          and actual_json.get("code") == expect_code)
    mark = "[PASS]" if ok else "[FAIL]"
    code = actual_json.get("code")
    msg = actual_json.get("message", "")
    print(f"{mark} {name}")
    print(f"       期望 HTTP {expect_status}/code={expect_code}，"
          f"实际 HTTP {actual_status}/code={code}，message={msg!r}")
    return ok


# --- 场景实现 ---

def scenario_normal(host, port):
    batch_id = uuid.uuid4().hex
    data = build_zip(batch_id, {
        f"staging-{TEST_DATE_1}.txt": SAMPLE_RECORDS_1,
        f"staging-{TEST_DATE_2}.txt": SAMPLE_RECORDS_2,
    })
    s, j = submit(batch_id, data, host, port)
    return check("normal 正常提交", s, j, 200, 0)


def scenario_idempotent(host, port):
    batch_id = uuid.uuid4().hex
    data = build_zip(batch_id, {f"staging-{TEST_DATE_1}.txt": SAMPLE_RECORDS_1})
    s1, j1 = submit(batch_id, data, host, port)
    s2, j2 = submit(batch_id, data, host, port)  # 同批ID重发
    ok = check("idempotent 幂等重发（第二次）", s2, j2, 200, 0)
    if s1 != 200 or j1.get("code") != 0:
        print("[FAIL] idempotent 第一次提交未成功，幂等测试无效")
        return False
    return ok


def scenario_badid(host, port):
    data = build_zip("ok-id", {f"staging-{TEST_DATE_1}.txt": SAMPLE_RECORDS_1})
    s, j = submit("bad/id!", data, host, port)
    return check("badid 批ID含非法字符", s, j, 400, -3)


def scenario_corrupt(host, port):
    s, j = submit(uuid.uuid4().hex, b"this is not a zip", host, port)
    return check("corrupt 请求体非ZIP", s, j, 400, -3)


def scenario_foldermismatch(host, port):
    batch_id = uuid.uuid4().hex
    other_id = uuid.uuid4().hex
    data = build_zip(other_id, {f"staging-{TEST_DATE_1}.txt": SAMPLE_RECORDS_1})
    s, j = submit(batch_id, data, host, port)
    return check("foldermismatch ZIP内文件夹名与批ID不一致", s, j, 400, -3)


def scenario_baddate(host, port):
    batch_id = uuid.uuid4().hex
    data = build_zip(batch_id, {
        "staging-20261399.txt": SAMPLE_RECORDS_1,   # 非法日期
    })
    s, j = submit(batch_id, data, host, port)
    return check("baddate staging文件名日期非法", s, j, 400, -3)


def scenario_emptyfile(host, port):
    batch_id = uuid.uuid4().hex
    data = build_zip(batch_id, {
        f"staging-{TEST_DATE_1}.txt": "",            # 空 → 应被忽略
        f"staging-{TEST_DATE_2}.txt": SAMPLE_RECORDS_2,
    })
    s, j = submit(batch_id, data, host, port)
    ok = check("emptyfile 空文件忽略+有效日期正常处理", s, j, 200, 0)
    if ok:
        idx = j.get("updated_index", [])
        dates = sorted(f"{e['year']}{e['month']:02d}{e['day']:02d}" for e in idx)
        expect_dates = [TEST_DATE_2]
        if dates != expect_dates:
            print(f"[FAIL] emptyfile updated_index 日期期望 {expect_dates}，实际 {dates}")
            return False
        print(f"       updated_index 日期 {dates}（空日期 {TEST_DATE_1} 已被忽略）")
    return ok


def scenario_oversize(host, port):
    batch_id = uuid.uuid4().hex
    big = "x" * (5 * 1024 * 1024 + 64 * 1024)  # 5MB + 64KB
    data = build_zip(batch_id, {f"staging-{TEST_DATE_1}.txt": big})
    s, j = submit(batch_id, data, host, port)
    return check("oversize 解压后超5MB", s, j, 400, -2)


def find_app_root(cli_root):
    """定位 exe 启动目录（config.yml / just-did-data 所在目录）"""
    candidates = []
    if cli_root:
        candidates.append(os.path.abspath(cli_root))
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # 优先 exe 所在目录（双击启动时的工作目录）
    candidates.append(os.path.abspath(os.path.join(script_dir, "..", "build", "src", "Release")))
    candidates.append(os.getcwd())
    for c in candidates:
        if os.path.exists(os.path.join(c, "config.yml")) or os.path.exists(os.path.join(c, "just-did-data")):
            return c
    return candidates[0]


def resolve_data_root(app_root):
    """按应用同款规则解析数据根：config.yml 的 data-root（相对按 app-root 解析、绝对原样）
    → 默认 {app-root}/just-did-data"""
    config_path = os.path.join(app_root, "config.yml")
    try:
        with open(config_path, encoding="utf-8") as f:
            for line in f:
                m = re.match(r"^data-root:\s*(.+?)\s*$", line)
                if m:
                    value = m.group(1).strip().strip("\"'")
                    if value:
                        if os.path.isabs(value):
                            return value
                        return os.path.normpath(os.path.join(app_root, value))
                    break
    except OSError:
        pass
    return os.path.join(app_root, "just-did-data")


def scenario_resume(host, port, app_root):
    """模拟「暂存成功、覆盖前中断」的状态：预置覆盖中记录+快照，再提交同批ID续跑"""
    batch_id = uuid.uuid4().hex
    snapshot_content = "16:00\n断点续跑测试记录\n"

    # 1. 预置快照文件 data/YYYY/MM/{batchId}-{DD}.txt
    y, m, d = TEST_DATE_3[:4], TEST_DATE_3[4:6], TEST_DATE_3[6:8]
    data_root = resolve_data_root(app_root)
    snap_dir = os.path.join(data_root, "data", y, m)
    os.makedirs(snap_dir, exist_ok=True)
    snap_path = os.path.join(snap_dir, f"{batch_id}-{d}.txt")
    with open(snap_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(snapshot_content)
    print(f"       已预置快照: {snap_path}")

    # 2. 预置批次记录 status=覆盖中
    db_path = os.path.join(data_root, "just_do.db")
    conn = sqlite3.connect(db_path, timeout=5)
    try:
        conn.execute(
            "INSERT OR REPLACE INTO pc_batch_records (batch_id, dates, status) VALUES (?, ?, ?)",
            (batch_id, TEST_DATE_3, "覆盖中"))
        conn.commit()
    finally:
        conn.close()
    print(f"       已预置批次记录: {db_path} (batch_id={batch_id}, status=覆盖中)")

    # 3. 提交同批ID（数据体可携带，覆盖中分支会忽略）
    data = build_zip(batch_id, {f"staging-{TEST_DATE_3}.txt": "内容不重要，覆盖中分支忽略\n"})
    s, j = submit(batch_id, data, host, port)
    ok = check("resume 覆盖中续跑", s, j, 200, 0)

    # 4. 校验：正式日报文件应为快照内容，快照应已被 rename 消耗
    target_path = os.path.join(data_root, "data", y, m, f"{d}.txt")
    if os.path.exists(target_path):
        with open(target_path, encoding="utf-8") as f:
            content = f.read()
        if content != snapshot_content:
            print(f"[FAIL] resume 正式文件内容与快照不一致: {content!r}")
            ok = False
        else:
            print(f"       正式文件内容与快照一致，快照已消耗: {not os.path.exists(snap_path)}")
    else:
        print(f"[FAIL] resume 正式文件不存在: {target_path}")
        ok = False
    return ok


SCENARIOS = {
    "normal": scenario_normal,
    "idempotent": scenario_idempotent,
    "badid": scenario_badid,
    "corrupt": scenario_corrupt,
    "foldermismatch": scenario_foldermismatch,
    "baddate": scenario_baddate,
    "emptyfile": scenario_emptyfile,
    "oversize": scenario_oversize,
    "resume": scenario_resume,
}


def main():
    parser = argparse.ArgumentParser(description="JustDid /sync/submit 接口测试")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--scenario", choices=["all"] + list(SCENARIOS), default="all")
    parser.add_argument("--app-root", default=None,
                        help="exe 启动目录（config.yml / just-did-data 所在位置），resume 场景需要")
    args = parser.parse_args()

    app_root = find_app_root(args.app_root)
    print(f"== /sync/submit 接口测试  host={args.host}:{args.port}  app-root={app_root} ==\n")

    names = list(SCENARIOS) if args.scenario == "all" else [args.scenario]
    results = []
    for name in names:
        fn = SCENARIOS[name]
        try:
            results.append((name, fn(args.host, args.port) if name != "resume"
                            else fn(args.host, args.port, app_root)))
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
