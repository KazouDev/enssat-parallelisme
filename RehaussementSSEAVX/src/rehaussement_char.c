#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>

#define MAX_VALEUR 255
#define BILLION 1000000000.0

typedef unsigned char Pixel;

typedef struct {
    int width;
    int height;
    Pixel *data;
} Image;

Image lire_image(const char *nom) {
    FILE *f = fopen(nom, "r");
    if (!f) { perror(nom); exit(1); }

    char mot[256];
    if (fscanf(f, "%255s", mot) != 1 || strcmp(mot, "P2") != 0) {
        fprintf(stderr, "Format P2 attendu\n"); exit(1);
    }

    int c = fgetc(f);
    while (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '#') {
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
        }
        c = fgetc(f);
    }
    if (c != EOF) ungetc(c, f);

    int maxval;
    Image img;
    if (fscanf(f, "%d %d %d", &img.width, &img.height, &maxval) != 3 ||
        img.width <= 0 || img.height <= 0 || maxval < 1 || maxval > 255) {
        fprintf(stderr, "Dimensions ou maxval invalides (PGM 8 bits attendu)\n"); exit(1);
    }

    size_t n = (size_t)img.width * img.height;
    img.data = malloc(n * sizeof(Pixel));
    if (!img.data) { perror("malloc"); exit(1); }

    for (size_t i = 0; i < n; i++) {
        int valeur;
        if (fscanf(f, "%d", &valeur) != 1 || valeur < 0 || valeur > maxval) {
            fprintf(stderr, "Pixel invalide\n"); exit(1);
        }
        img.data[i] = (Pixel)valeur;
    }
    fclose(f);
    return img;
}

void ecrire_image(const char *nom, Image img) {
    FILE *f = fopen(nom, "w");
    if (!f) { perror(nom); exit(1); }
    fprintf(f, "P2\n%d %d\n255\n", img.width, img.height);
    size_t n = (size_t)img.width * img.height;
    for (size_t i = 0; i < n; i++) {
        fprintf(f, "%d%c", (int)img.data[i], (i + 1) % 15 == 0 ? '\n' : ' ');
    }
    fprintf(f, "\n");
    fclose(f);
}

void etalement_lineaire(Image in, Image out) {
    size_t n = (size_t)in.width * in.height;
    __m256i vmin = _mm256_set1_epi8((char)255);
    __m256i vmax = _mm256_setzero_si256();
    int min = 255, max = 0;
    size_t i = 0;

    for (; i + 32 <= n; i += 32) {
        __m256i pixels = _mm256_loadu_si256((const __m256i *)&in.data[i]);
        vmin = _mm256_min_epu8(vmin, pixels);
        vmax = _mm256_max_epu8(vmax, pixels);
    }
    unsigned char petits[32], grands[32];
    _mm256_storeu_si256((__m256i *)petits, vmin);
    _mm256_storeu_si256((__m256i *)grands, vmax);
    for (int j = 0; j < 32; j++) {
        if (petits[j] < min) min = petits[j];
        if (grands[j] > max) max = grands[j];
    }
    for (; i < n; i++) {
        if (in.data[i] < min) min = in.data[i];
        if (in.data[i] > max) max = in.data[i];
    }

    if (max == min) {
        memset(out.data, 0, n * sizeof(Pixel));
        return;
    }

    float facteur = 255.0f / (max - min);
    __m256 vmin2 = _mm256_set1_ps((float)min);
    __m256 vfacteur = _mm256_set1_ps(facteur);
    i = 0;
    for (; i + 32 <= n; i += 32) {
        __m256i blocs[4];
        for (int j = 0; j < 4; j++) {
            __m128i pixels = _mm_loadl_epi64((const __m128i *)&in.data[i + j * 8]);
            __m256 valeurs = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(pixels));
            valeurs = _mm256_mul_ps(_mm256_sub_ps(valeurs, vmin2), vfacteur);
            blocs[j] = _mm256_cvttps_epi32(valeurs);
        }
        __m256i a = _mm256_packs_epi32(blocs[0], blocs[1]);
        __m256i b = _mm256_packs_epi32(blocs[2], blocs[3]);
        a = _mm256_permute4x64_epi64(a, 0xD8);
        b = _mm256_permute4x64_epi64(b, 0xD8);
        __m256i resultat = _mm256_packus_epi16(a, b);
        resultat = _mm256_permute4x64_epi64(resultat, 0xD8);
        _mm256_storeu_si256((__m256i *)&out.data[i], resultat);
    }
    for (; i < n; i++) out.data[i] = (unsigned char)((in.data[i] - min) * facteur);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage : %s image.pgm\n", argv[0]);
        return 1;
    }
    Image input = lire_image(argv[1]);
    Image output = {input.width, input.height, malloc((size_t)input.width * input.height * sizeof(Pixel))};
    if (!output.data) { perror("malloc"); return 1; }

    printf("Traitement de l'image (%dx%d)...\n", input.width, input.height);
    struct timespec start, stop;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
    etalement_lineaire(input, output);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);

    double sec = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / BILLION;
    printf("Temps de calcul %2.9f s\n", sec);
    printf("Temps de calcul %2.6f ms\n", sec * 1000.0);
    printf("Temps de calcul %2.3f us\n", sec * 1000000.0);

    char nom[1024];
    snprintf(nom, sizeof(nom), "%s_char_new.pgm", argv[1]);
    ecrire_image(nom, output);
    printf("Écriture du fichier résultat : %s\n", nom);
    free(input.data);
    free(output.data);
    return 0;
}
