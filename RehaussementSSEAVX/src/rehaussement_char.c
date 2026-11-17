#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>

#define InitClock    struct timespec start, stop
#define ClockStart   clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start)
#define ClockEnd     clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop)
#define BILLION      1000000000L
#define ClockMesure_Sec "Temps de calcul %2.9f s\n",(( stop.tv_sec - start.tv_sec )+ (stop.tv_nsec - start.tv_nsec )/(double)BILLION)
#define ClockMesure_m_Sec "Temps de calcul %2.6f ms\n",(( stop.tv_sec - start.tv_sec )*1000.0 + (stop.tv_nsec - start.tv_nsec )/1000000.0)
#define ClockMesure_u_Sec "Temps de calcul %2.3f us\n",(( stop.tv_sec - start.tv_sec )*1000000.0 + (stop.tv_nsec - start.tv_nsec )/1000.0)

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

void etalement_avx_char(const unsigned char *in, unsigned char *out, int n) {
    int i;

    __m256i vmin = _mm256_set1_epi8((char)MAX_VALEUR);
    __m256i vmax = _mm256_set1_epi8(0);

    for (i = 0; i + 32 <= n; i += 32) {
        __m256i v = _mm256_loadu_si256((const __m256i *)&in[i]);
        vmin = _mm256_min_epu8(vmin, v);
        vmax = _mm256_max_epu8(vmax, v);
    }

    unsigned char tmin[32], tmax[32];
    _mm256_storeu_si256((__m256i *)tmin, vmin);
    _mm256_storeu_si256((__m256i *)tmax, vmax);
    int min = MAX_VALEUR, max = 0;
    for (int k = 0; k < 32; k++) {
        min = MIN(min, tmin[k]);
        max = MAX(max, tmax[k]);
    }
    for (; i < n; i++) {
        min = MIN(min, in[i]);
        max = MAX(max, in[i]);
    }

    if (max == min) {
        memcpy(out, in, n);
        return;
    }

    float facteur = (float)MAX_VALEUR / (float)(max - min);
    unsigned short fq = (unsigned short)(facteur * 256.0f + 0.5f);

    __m256i vfact = _mm256_set1_epi16((short)fq);
    __m256i masque_haut = _mm256_set1_epi16((short)0xFF00);
    vmin = _mm256_set1_epi8((char)min);

    for (i = 0; i + 32 <= n; i += 32) {
        __m256i v = _mm256_loadu_si256((const __m256i *)&in[i]);
        v = _mm256_sub_epi8(v, vmin);

        __m256i pairs = _mm256_slli_epi16(v, 8);
        __m256i impairs = _mm256_and_si256(v, masque_haut);

        pairs = _mm256_mulhi_epu16(pairs, vfact);
        impairs = _mm256_mulhi_epu16(impairs, vfact);

        v = _mm256_or_si256(pairs, _mm256_slli_epi16(impairs, 8));
        _mm256_storeu_si256((__m256i *)&out[i], v);
    }
    for (; i < n; i++)
        out[i] = (unsigned char)((((unsigned)(in[i] - min) << 8) * fq) >> 16);
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
    snprintf(dstFile, sizeof(dstFile), "%s_char.pgm", argv[1]);

    printf("Lecture du fichier : %s\n", srcFile);
    ImagePGM input = read_pgm(srcFile);

    ImagePGM output = {
        .width = input.width,
        .height = input.height,
        .maxval = input.maxval,
        .data = alloc_image(input.height, input.width)
    };

    int w = input.width;
    int n = input.width * input.height;

    unsigned char *in = malloc(n * sizeof(unsigned char));
    unsigned char *out = malloc(n * sizeof(unsigned char));
    CHECK_ALLOC(in);
    CHECK_ALLOC(out);

    for (int i = 0; i < input.height; i++)
        for (int j = 0; j < w; j++)
            in[i * w + j] = (unsigned char)input.data[i][j];

    memset(out, 0, n * sizeof(unsigned char));

    printf("Traitement de l'image (%dx%d)...\n", input.width, input.height);

    ClockStart;

    etalement_avx_char(in, out, n);

    ClockEnd;

    printf(ClockMesure_Sec);
    printf(ClockMesure_m_Sec);
    printf(ClockMesure_u_Sec);

    for (int i = 0; i < output.height; i++)
        for (int j = 0; j < w; j++)
            output.data[i][j] = (int)out[i * w + j];

    printf("Écriture du fichier résultat : %s\n", dstFile);
    write_pgm(dstFile, output);

    free(in);
    free(out);
    free_image(input);
    free_image(output);

    printf("Traitement terminé avec succès.\n");
    return EXIT_SUCCESS;
}
