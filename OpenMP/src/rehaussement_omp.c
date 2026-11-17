#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define InitClock    double start, stop
#define ClockStart   start = omp_get_wtime()
#define ClockEnd     stop = omp_get_wtime()
#define ClockMesure_Sec "Temps de calcul %2.9f s\n", (stop - start)
#define ClockMesure_m_Sec "Temps de calcul %2.6f ms\n", (stop - start) * 1000.0
#define ClockMesure_u_Sec "Temps de calcul %2.3f us\n", (stop - start) * 1000000.0

#define MAX_CHAINE 256
#define MAX_VALEUR 255
#define NBPOINTSPARLIGNES 15

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define CHECK_ALLOC(ptr) if (!(ptr)) { fprintf(stderr, "Erreur d'allocation mémoire\n"); exit(EXIT_FAILURE); }

typedef struct {
    int width;
    int height;
    int maxval;
    int **data;
} ImagePGM;

int **alloc_image(int height, int width) {
    int **img = malloc(height * sizeof(int *));
    CHECK_ALLOC(img);
    for (int i = 0; i < height; i++) {
        img[i] = calloc(width, sizeof(int));
        CHECK_ALLOC(img[i]);
    }
    return img;
}

void free_image(ImagePGM img) {
    for (int i = 0; i < img.height; i++)
        free(img.data[i]);
    free(img.data);
}

ImagePGM read_pgm(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Erreur ouverture fichier source");
        exit(EXIT_FAILURE);
    }

    ImagePGM img;
    char line[MAX_CHAINE];

    if (!fgets(line, sizeof(line), f) || strncmp(line, "P2", 2) != 0) {
        fprintf(stderr, "Erreur : format non supporté (P2 attendu)\n");
        exit(EXIT_FAILURE);
    }

    do {
        if (!fgets(line, sizeof(line), f)) {
            fprintf(stderr, "Erreur lecture dimensions\n");
            exit(EXIT_FAILURE);
        }
    } while (line[0] == '#');

    sscanf(line, "%d %d", &img.width, &img.height);
    fscanf(f, "%d", &img.maxval);

    img.data = alloc_image(img.height, img.width);

    for (int i = 0; i < img.height; i++)
        for (int j = 0; j < img.width; j++)
            fscanf(f, "%d", &img.data[i][j]);

    fclose(f);
    return img;
}

void write_pgm(const char *filename, ImagePGM img) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Erreur ouverture fichier destination");
        exit(EXIT_FAILURE);
    }

    fprintf(f, "P2\n");
    fprintf(f, "# Image étalée automatiquement\n");
    fprintf(f, "%d %d\n%d\n", img.width, img.height, img.maxval);

    int n = 0;
    for (int i = 0; i < img.height; i++) {
        for (int j = 0; j < img.width; j++) {
            fprintf(f, "%3d ", img.data[i][j]);
            n++;
            if (n == NBPOINTSPARLIGNES) {
                n = 0;
                fprintf(f, "\n");
            }
        }
    }
    fprintf(f, "\n");
    fclose(f);
}

void etalement_omp(ImagePGM in, ImagePGM out) {
    int min = MAX_VALEUR, max = 0;

    #pragma omp parallel for reduction(min:min) reduction(max:max)
    for (int i = 0; i < in.height; i++)
        for (int j = 0; j < in.width; j++) {
            min = MIN(min, in.data[i][j]);
            max = MAX(max, in.data[i][j]);
        }

    if (max == min) {
        for (int i = 0; i < in.height; i++)
            memcpy(out.data[i], in.data[i], in.width * sizeof(int));
        return;
    }

    float facteur = (float)(MAX_VALEUR) / (float)(max - min);

    #pragma omp parallel for
    for (int i = 0; i < in.height; i++)
        for (int j = 0; j < in.width; j++)
            out.data[i][j] = (int)((in.data[i][j] - min) * facteur);
}

int main(int argc, char **argv) {

    InitClock;

    if (argc != 2) {
        fprintf(stderr, "Usage : %s image.pgm\n", argv[0]);
        return EXIT_FAILURE;
    }

    char srcFile[MAX_CHAINE];
    char dstFile[MAX_CHAINE + 16];

    snprintf(srcFile, sizeof(srcFile), "%s", argv[1]);
    snprintf(dstFile, sizeof(dstFile), "%s_omp.pgm", argv[1]);

    printf("Lecture du fichier : %s\n", srcFile);
    ImagePGM input = read_pgm(srcFile);

    ImagePGM output = {
        .width = input.width,
        .height = input.height,
        .maxval = input.maxval,
        .data = alloc_image(input.height, input.width)
    };

    printf("Traitement de l'image (%dx%d) avec %d threads...\n",
           input.width, input.height, omp_get_max_threads());

    ClockStart;

    etalement_omp(input, output);

    ClockEnd;

    printf(ClockMesure_Sec);
    printf(ClockMesure_m_Sec);
    printf(ClockMesure_u_Sec);

    printf("Écriture du fichier résultat : %s\n", dstFile);
    write_pgm(dstFile, output);

    free_image(input);
    free_image(output);

    printf("Traitement terminé avec succès.\n");
    return EXIT_SUCCESS;
}