#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JustDid /sync/fetch 接口测试脚本（fetch-always-zip 变更验证）

用法：
  1. 先启动 JustDid.exe（双击运行，HTTP 服务默认端口 18080）
  2. 运行测试：
       python test/test_fetch/test_fetch.py                    # 跑全部场景
       python test/test_fetch/test_fetch.py --scenario single # 跑单个场景
       python test/test_fetch/test_fetch.py --scenario emptyfile --app-root D:/.../build/src/Release

场景说明：
  single     单文件请求 → 200 + ZIP（DEFLATE，1 个条目 YYYY/MM/DD.txt，无 data/ 前缀）
  multi      多日期请求（部分日期无文件）→ ZIP 仅含存在的日期，内容与本地文件一致
  missing    全部日期无文件 → 404 + {"code":1,"message":"无文件"}
  emptyfile  空文件日期（脚本临时创建 data/2020/01/01.txt 后清理）→ 仍收录为空内容条目
  badparam   参数错误 → 400 code=-2/-3

注意：emptyfile 场景会临时在 app-root/data/ 下创建并删除测试文件。
"""

import argparse
import io
import json
import os
import re
import sys
import zipfile
import http.client

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 18080

# 当前 app 数据中存在的日期（依赖运行环境实际数据，按需调整）
EXISTING_DATE_1 = "20260801"
EXISTING_DATE_2 = "20260802"
MISSING_DATE = "20260725"    # 无文件
MISSING_DATE_2 = "20260814"  # 无文件
NO_DATA_DATE = "20190101"    # 全部无文件场景用
EMPTY_DATE = "20200101"      # emptyfile 场景用（临时创建）

ENTRY_RE = re.compile(r"^\d{4}/\d{2}/\d{2}\.txt$")


def fetch_req(dates, host, port):
    conn = http.client.HTTPConnection(host, port, timeout=60)
    # dates=None 时发送空对象 {}（模拟未提供日期参数）
    body = {} if dates is None else {"dates": dates}
    conn.request("POST", "/sync/fetch",
                 body=json.dumps(body),
                 headers={"Content-Type": "application/json"})
    resp = conn.getresponse()
    body = resp.read()
    ctype = resp.getheader("Content-Type", "")
    conn.close()
    return resp.status, ctype, body


def verify_zip_body(body, data_root, expect_entries):
    """校验 ZIP：DEFLATE、条目无 data/ 前缀、条目内容与本地文件一致。
    返回 (ok, 错误信息)"""
    try:
        zf = zipfile.ZipFile(io.BytesIO(body))
    except Exception as e:
        return False, f"响应体不是有效 ZIP: {e!r}"

    with zf:
        names = zf.namelist()
        if names != expect_entries:
            return False, f"条目期望 {expect_entries}，实际 {names}"

        for name in names:
            info = zf.getinfo(name)
            if info.compress_type != zipfile.ZIP_DEFLATED:
                return False, f"条目 {name} 压缩方式 {info.compress_type}，期望 DEFLATED(8)"
            if not ENTRY_RE.match(name) or name.startswith("data/"):
                return False, f"条目 {name} 路径不符（应为 YYYY/MM/DD.txt 且无 data/ 前缀）"

            local_path = os.path.join(data_root, "data", *name.split("/"))
            if not os.path.exists(local_path):
                return False, f"本地文件不存在: {local_path}"
            with open(local_path, "rb") as f:
                local_content = f.read()
            entry_content = zf.read(name)
            # 本地文件为 Windows 写入（\r\n），ZIP 条目为规范格式（\n），按换行归一化比对
            local_norm = local_content.replace(b"\r\n", b"\n")
            if entry_content != local_norm:
                return False, (f"条目 {name} 内容与本地文件不一致（换行归一化后）：\n"
                               f"  zip:  {entry_content[:200]!r}\n"
                               f"  file: {local_norm[:200]!r}")
    return True, ""


def print_zip_info(body, indent="       "):
    zf = zipfile.ZipFile(io.BytesIO(body))
    with zf:
        for info in zf.infolist():
            method = "DEFLATE" if info.compress_type == zipfile.ZIP_DEFLATED else info.compress_type
            print(f"{indent}{info.filename}  ({info.file_size}B, {method})")


def scenario_single(host, port, app_root):
    s, ctype, body = fetch_req([EXISTING_DATE_1], host, port)
    if s != 200 or "application/zip" not in ctype:
        print(f"[FAIL] single HTTP {s}, Content-Type {ctype!r}, 期望 200 + application/zip")
        return False
    ok, err = verify_zip_body(body, resolve_data_root(app_root),
                              [f"{EXISTING_DATE_1[:4]}/{EXISTING_DATE_1[4:6]}/{EXISTING_DATE_1[6:]}.txt"])
    print(f"{'[PASS]' if ok else '[FAIL]'} single 单文件请求 → 200 + ZIP（1 条目，DEFLATE，无前缀）")
    print_zip_info(body)
    if not ok:
        print(f"       {err}")
    return ok


def scenario_multi(host, port, app_root):
    dates = [EXISTING_DATE_1, MISSING_DATE, EXISTING_DATE_2, MISSING_DATE_2]
    expect = [
        f"{EXISTING_DATE_1[:4]}/{EXISTING_DATE_1[4:6]}/{EXISTING_DATE_1[6:]}.txt",
        f"{EXISTING_DATE_2[:4]}/{EXISTING_DATE_2[4:6]}/{EXISTING_DATE_2[6:]}.txt",
    ]
    s, ctype, body = fetch_req(dates, host, port)
    if s != 200 or "application/zip" not in ctype:
        print(f"[FAIL] multi HTTP {s}, Content-Type {ctype!r}, 期望 200 + application/zip")
        return False
    ok, err = verify_zip_body(body, resolve_data_root(app_root), expect)
    print(f"{'[PASS]' if ok else '[FAIL]'} multi 多日期请求 → ZIP 仅含存在的日期")
    print(f"       请求 {dates}")
    print_zip_info(body)
    if not ok:
        print(f"       {err}")
    return ok


def scenario_missing(host, port, app_root):
    s, ctype, body = fetch_req([NO_DATA_DATE], host, port)
    try:
        j = json.loads(body.decode("utf-8"))
    except Exception:
        j = {"raw": body[:200]}
    ok = (s == 404 and j.get("code") == 1 and j.get("message") == "无文件")
    print(f"{'[PASS]' if ok else '[FAIL]'} missing 全部日期无文件 → 404 + {{\"code\":1,\"message\":\"无文件\"}}")
    print(f"       HTTP {s}, body={j}")
    return ok


def scenario_emptyfile(host, port, app_root):
    # 1. 临时创建空日报文件 data/2020/01/01.txt
    data_root = resolve_data_root(app_root)
    empty_dir = os.path.join(data_root, "data", "2020", "01")
    os.makedirs(empty_dir, exist_ok=True)
    empty_path = os.path.join(empty_dir, "01.txt")
    with open(empty_path, "wb"):
        pass
    print(f"       已临时创建空文件: {empty_path}")
    try:
        s, ctype, body = fetch_req([EMPTY_DATE], host, port)
        if s != 200 or "application/zip" not in ctype:
            print(f"[FAIL] emptyfile HTTP {s}, Content-Type {ctype!r}, 期望 200 + application/zip")
            return False
        try:
            zf = zipfile.ZipFile(io.BytesIO(body))
            with zf:
                names = zf.namelist()
                expect = ["2020/01/01.txt"]
                content_ok = names == expect and zf.read(expect[0]) == b""
            ok = content_ok
            err = "" if ok else f"条目期望 {expect}（空内容），实际 {names}"
        except Exception as e:
            ok, err = False, f"响应体不是有效 ZIP: {e!r}"
        print(f"{'[PASS]' if ok else '[FAIL]'} emptyfile 空文件日期仍收录为空内容条目")
        if ok:
            print_zip_info(body)
        else:
            print(f"       {err}")
        return ok
    finally:
        # 2. 清理：删除测试文件与空目录
        if os.path.exists(empty_path):
            os.remove(empty_path)
        for d in (os.path.join(data_root, "data", "2020", "01"),
                  os.path.join(data_root, "data", "2020")):
            try:
                os.rmdir(d)
            except OSError:
                pass
        print(f"       已清理: {empty_path}")


def scenario_badparam(host, port, app_root):
    ok_all = True

    # 无日期参数 → 400 code=-3
    s, _, body = fetch_req(None, host, port)  # 空对象 {}
    j = json.loads(body.decode("utf-8"))
    ok = (s == 400 and j.get("code") == -3)
    ok_all &= ok
    print(f"{'[PASS]' if ok else '[FAIL]'} badparam 无日期参数 → 400 code=-3  (HTTP {s}, {j})")

    # 超过 32 个文件 → 400 code=-2
    dates = [f"2026{i:02d}{d:02d}" for i in range(1, 3) for d in range(1, 17)]  # 32 个
    dates.append("20260301")  # 第 33 个
    s, _, body = fetch_req(dates, host, port)
    j = json.loads(body.decode("utf-8"))
    ok = (s == 400 and j.get("code") == -2)
    ok_all &= ok
    print(f"{'[PASS]' if ok else '[FAIL]'} badparam 33 个日期超上限 → 400 code=-2  (HTTP {s}, {j})")
    return ok_all


SCENARIOS = {
    "single": scenario_single,
    "multi": scenario_multi,
    "missing": scenario_missing,
    "emptyfile": scenario_emptyfile,
    "badparam": scenario_badparam,
}


def find_app_root(cli_root):
    """定位 exe 启动目录（config.yml / just-did-data 所在目录）"""
    candidates = []
    if cli_root:
        candidates.append(os.path.abspath(cli_root))
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidates.append(os.path.abspath(
        os.path.join(script_dir, "..", "..", "build", "src", "Release")))
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


def main():
    # Windows 控制台默认 GBK，强制 UTF-8 输出避免中文乱码
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass

    parser = argparse.ArgumentParser(description="JustDid /sync/fetch 接口测试")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--scenario", choices=["all"] + list(SCENARIOS), default="all")
    parser.add_argument("--app-root", default=None,
                        help="exe 启动目录（config.yml / just-did-data 所在位置），内容比对与 emptyfile 场景需要")
    args = parser.parse_args()

    app_root = find_app_root(args.app_root)
    print(f"== /sync/fetch 接口测试  host={args.host}:{args.port}  app-root={app_root} ==\n")

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
