#!/bin/bash
# Compare xsl::asio HTTP server with a standalone asio HTTP server.
# Both serve GET /hello -> 200 "Hello, World!" and are driven by the same
# load generator (test/benches/http/loadgen.cpp), so the comparison is fair.
#
# Usage: test/benches/http/bench.sh [duration] [threads] [conns-per-thread]

set -euo pipefail
cd "$(dirname "$0")/../../.."  # repo root

DURATION="${1:-10}"
THREADS="${2:-4}"
CONNS="${3:-4}"

PORT_XSL=18080
PORT_ASIO=18081
RESULT_DIR="$(mktemp -d)"
trap 'rm -rf "$RESULT_DIR"; kill $(jobs -p) 2>/dev/null || true' EXIT

echo "== building (release, logs compiled out) =="
xmake f -m release -c --log_level=none
xmake build -g benchmarks/http

wait_port() {
    local port="$1"
    for _ in $(seq 1 100); do
        if curl -sf "http://127.0.0.1:${port}/hello" -o /dev/null; then
            return 0
        fi
        sleep 0.1
    done
    echo "server on port ${port} did not come up" >&2
    return 1
}

run_one() {
    local name="$1" target="$2" port="$3"
    echo
    echo "== ${name} (port ${port}) =="
    xmake run --workdir=build "${target}" -p "${port}" &
    local server_pid=$!
    wait_port "${port}"
    # warmup
    xmake run --workdir=build bench_http_loadgen -p "${port}" \
        -t "${THREADS}" -c "${CONNS}" -d 2
    # measured run
    xmake run --workdir=build bench_http_loadgen -p "${port}" \
        -t "${THREADS}" -c "${CONNS}" -d "${DURATION}" \
        -o "${RESULT_DIR}/${name}.json" | tee "${RESULT_DIR}/${name}.txt"
    kill "${server_pid}" 2>/dev/null || true
    wait "${server_pid}" 2>/dev/null || true
    sleep 0.5  # let the listening socket drain
}

run_one xsl bench_http_xsl "${PORT_XSL}"
run_one asio bench_http_asio "${PORT_ASIO}"

echo
echo "== summary (rps, ${THREADS} threads x ${CONNS} conns, ${DURATION}s) =="
python3 - "$RESULT_DIR" "$DURATION" <<'EOF'
import json, sys
d, duration = sys.argv[1], sys.argv[2]
results = {}
for name in ("xsl", "asio"):
    try:
        results[name] = json.load(open(f"{d}/{name}.json"))
    except Exception as exc:  # noqa: BLE001
        print(f"{name}: no result ({exc})")
if "xsl" in results and "asio" in results:
    r_xsl, r_asio = results["xsl"], results["asio"]
    for key, label in (("rps", "rps"),):
        print(f"{label:8}: xsl={r_xsl[key]:>12.1f}  asio={r_asio[key]:>12.1f}")
    for key in ("p50", "p90", "p99", "p999", "max"):
        print(f"lat {key:5}: xsl={r_xsl['latency_us'][key]:>8}us  "
              f"asio={r_asio['latency_us'][key]:>8}us")
    speedup = r_xsl["rps"] / r_asio["rps"] if r_asio["rps"] else float("inf")
    print(f"ratio   : xsl/asio = {speedup:.2f}x  (errors: xsl={r_xsl['errors']}, asio={r_asio['errors']})")
EOF
