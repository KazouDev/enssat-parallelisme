# Processeurs pour le multimédia — TP d'évaluation de performances

LEVACHER Ethan — IAI3, ENSSAT.

Chaque TP consiste à évaluer les performances d'un programme : exécution répétée
(~ 100 fois), mesures statistiques, courbes, puis comparaison séquentiel / parallélisé.
Chaque partie du compte rendu doit indiquer **la machine** (processeur, RAM, OS)
et **le compilateur** utilisés au moment des tests.

Le compte rendu global est dans [SYNTHESE.md](SYNTHESE.md).

## Arborescence

```
ProcesseurParallele/
├── README.md            ← ce fichier (global)
├── SYNTHESE.md          ← compte rendu global, une section par TP
├── requirements.txt     ← dépendances Python (matplotlib)
├── tools/               ← outils communs à tous les TP
│   ├── benchmark.py     ←   N exécutions → CSV + graphique + system info
│   └── systeme_info.sh  ←   capture de la config machine (Linux / macOS)
└── TP1/                 ← un dossier par TP, même structure
    ├── src/             ←   code source + en-têtes (.h)
    ├── data/            ←   données d'entrée
    ├── output/          ←   sorties du programme
    └── results/         ←   un dossier par test
        └── <nom_du_test>/
            ├── <nom_du_test>.csv
            ├── <nom_du_test>.png
            ├── stats.txt          ← moyenne, écart-type, médiane, min, max
            └── system_info.txt
```

## Installation

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Utilisation (exemple TP1)

```bash
cd TP1
gcc src/rehaussement.c -o a.out
python3 ../tools/benchmark.py -i "./a.out data/image1.pgm" -n 100 -o results -t image1
# → results/image1/image1.csv, image1.png, stats.txt, system_info.txt
```

### Paramètres de `benchmark.py`

| Option | Rôle |
|--------|------|
| `-i`, `--input` | commande à exécuter (programme + arguments) |
| `-n`, `--runs` | nombre d'exécutions |
| `-o`, `--output` | dossier des résultats (défaut : `results`) |
| `-t`, `--name` | nom du test → crée `<output>/<name>/` avec `<name>.csv`, `<name>.png`, `stats.txt` et `system_info.txt` |
| `--no-show` | n'ouvre pas la fenêtre du graphique |

Le programme mesuré doit afficher une ligne de la forme `Temps de calcul X ms`.

## Listes des TP

| TP | Sujet |
|----|-------|
| [TP1](TP1/) | Réhaussement de contraste d'image (PGM) |
