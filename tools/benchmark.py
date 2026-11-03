#!/usr/bin/env python3
import argparse
import csv
import shlex
import statistics
import subprocess
import sys
from datetime import datetime
from pathlib import Path

import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description="Benchmark d'un programme (N exécutions)")
parser.add_argument("-i", "--input", required=True, help="commande à exécuter (programme + arguments)")
parser.add_argument("-n", "--runs", type=int, required=True, help="nombre d'exécutions")
parser.add_argument("-o", "--output", default="results", help="dossier des résultats (défaut : results) ; le test va dans <output>/<name>/")
parser.add_argument("-t", "--name", required=True, help="nom du test : nom du sous-dossier et des fichiers")
parser.add_argument("--no-show", action="store_true", help="ne pas ouvrir la fenêtre du graphique")
args = parser.parse_args()

dossier = Path(args.output) / args.name
dossier.mkdir(parents=True, exist_ok=True)
csv_path = dossier / f"{args.name}.csv"
png_path = dossier / f"{args.name}.png"
info_path = dossier / "system_info.txt"
stats_path = dossier / "stats.txt"

script_info = Path(__file__).resolve().parent / "systeme_info.sh"
subprocess.run(["bash", str(script_info), str(info_path)], capture_output=True)

temps = []
commande = shlex.split(args.input)

for i in range(args.runs):
    resultat = subprocess.run(commande, capture_output=True, text=True)

    if resultat.returncode != 0:
        print("Erreur :", resultat.stderr)
        sys.exit(1)

    temps_ms = None
    for ligne in resultat.stdout.splitlines():
        if ligne.startswith("Temps de calcul") and ligne.endswith(" ms"):
            temps_ms = float(ligne.split()[3])
            break

    if temps_ms is None:
        print("Temps de calcul introuvable")
        sys.exit(1)

    temps.append(temps_ms)
    print(f"Exécution {i + 1}/{args.runs} : {temps_ms} ms")

moyenne = statistics.mean(temps)
ecart_type = statistics.stdev(temps) if len(temps) > 1 else 0

stats = (
    f"Test : {args.name}\n"
    f"Commande : {args.input}\n"
    f"Date : {datetime.now():%Y-%m-%d %H:%M:%S}\n"
    f"Exécutions : {len(temps)}\n"
    f"\n"
    f"Moyenne : {moyenne:.6f} ms\n"
    f"Écart-type : {ecart_type:.6f} ms\n"
    f"Médiane : {statistics.median(temps):.6f} ms\n"
    f"Minimum : {min(temps):.6f} ms\n"
    f"Maximum : {max(temps):.6f} ms\n"
)
print("\n" + stats)
stats_path.write_text(stats, encoding="utf-8")

with open(csv_path, "w", newline="") as fichier:
    writer = csv.writer(fichier)
    writer.writerow(["Execution", "Temps (ms)"])
    for i, t in enumerate(temps, start=1):
        writer.writerow([i, t])

executions = range(1, args.runs + 1)

plt.figure(figsize=(10, 5))
plt.scatter(executions, temps, s=12, label="Temps mesuré")
plt.axhline(moyenne, color="red", linestyle="--", label="Moyenne")
plt.axhline(moyenne + ecart_type, color="orange", linestyle=":", label="Moyenne ± écart-type")
plt.axhline(moyenne - ecart_type, color="orange", linestyle=":")
plt.xlabel("Numéro d'exécution")
plt.ylabel("Temps de calcul (ms)")
plt.title(f"{args.name} : {args.input}")
plt.legend()
plt.grid(True)

plt.savefig(png_path)
print(f"\nRésultats dans {dossier}/ : {csv_path.name}, {png_path.name}, {stats_path.name}, {info_path.name}")
if not args.no_show:
    plt.show()
