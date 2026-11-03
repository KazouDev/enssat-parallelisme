#!/bin/bash
export LC_ALL=C
# Usage : tools/systeme_info.sh [fichier_sortie]
#   Appelé automatiquement par benchmark.py -> <output>/<name>/system_info.txt
OUTPUT="${1:-system_info_$(date +%F).txt}"
mkdir -p "$(dirname "$OUTPUT")"

{
    echo "=================================="
    echo "   CONFIGURATION DE LA MACHINE"
    echo "=================================="
    echo "Date : $(date)"
    echo "Machine : $(hostname)"
    echo ""

    echo "========== SYSTEME =========="
    uname -a
    echo ""

    if [ "$(uname)" = "Darwin" ]; then
        echo "========== PROCESSEUR =========="
        echo "Architecture : $(uname -m)"
        echo "Model name : $(sysctl -n machdep.cpu.brand_string)"
        echo "CPU(s) : $(sysctl -n hw.ncpu)"
        echo "Coeurs performance : $(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null)"
        echo "Coeurs efficacite : $(sysctl -n hw.perflevel1.physicalcpu 2>/dev/null)"
        echo "L1d cache (P) : $(( $(sysctl -n hw.perflevel0.l1dcachesize 2>/dev/null || echo 0) / 1024 )) KiB"
        echo "L2 cache (P) : $(( $(sysctl -n hw.perflevel0.l2cachesize 2>/dev/null || echo 0) / 1024 / 1024 )) MiB"
        echo ""

        echo "========== MEMOIRE RAM =========="
        echo "Total : $(( $(sysctl -n hw.memsize) / 1024 / 1024 / 1024 )) Gi"
        echo ""

        echo "========== CHARGE CPU =========="
        uptime
        echo ""

    else
        echo "========== PROCESSEUR =========="
        lscpu | grep -E 'Architecture:|Model name:|^CPU\(s\):|Thread\(s\) per core:|Core\(s\) per socket:|Socket\(s\):|CPU max MHz:|CPU min MHz:|L1d cache:|L1i cache:|L2 cache:|L3 cache:'
        echo ""

        echo "========== MEMOIRE RAM =========="
        free -h
        echo ""

        echo "========== FREQUENCES CPU =========="
        grep -m 8 "cpu MHz" /proc/cpuinfo
        echo ""

        echo "========== CHARGE CPU =========="
        uptime
        echo ""

    fi

    echo "========== COMPILATEUR =========="
    gcc --version | head -n 1
    g++ --version | head -n 1
    echo ""

    if [ "$(uname)" != "Darwin" ]; then
        echo "========== OPTIONS CPU =========="
        lscpu | grep -E 'Flags:|Vulnerability' | head -n 1
        echo ""

    fi

} > "$OUTPUT"

cat "$OUTPUT"

echo ""
echo "Informations enregistrées dans $OUTPUT"
