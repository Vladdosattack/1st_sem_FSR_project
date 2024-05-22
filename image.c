#include "lodepng.c"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define PI 3.14159265358979323846


char* load_png_file(const char *filename, int *width, int *height) {
    unsigned char *image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    return (image);
}

typedef struct Node {
    unsigned char r, g, b, a;
    struct Node *up, *down, *left, *right;
    struct Node *parent;
    int rank;
} Node;

void createFilter(double gKernel[][5])
{
    double r, s = 2.00;

    for (int x = -2; x <= 2; x++) {
        for(int y = -2; y <= 2; y++) {
            r = sqrt(x*x + y*y);
            gKernel[x + 2][y + 2] = (exp(-(r*r)/s))/(PI * s);
        }
    }
}

void applyFilter(unsigned char *image, int width, int height) {
    double gKernel[5][5];
    createFilter(gKernel);

    unsigned char *result = malloc(width * height * 4 * sizeof(unsigned char));

    for (int y = 2; y < height - 2; y++) {
        for (int x = 2; x < width - 2; x++) {
            double red = 0.0, green = 0.0, blue = 0.0;
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int index = ((y+dy) * width + (x+dx)) * 4;
                    red += image[index] * gKernel[dy + 2][dx + 2];
                    green += image[index + 1] * gKernel[dy + 2][dx + 2];
                    blue += image[index + 2] * gKernel[dy + 2][dx + 2];
                }
            }
            int resultIndex = (y * width + x) * 4;
            result[resultIndex] = (unsigned char)round(red);
            result[resultIndex + 1] = (unsigned char)round(green);
            result[resultIndex + 2] = (unsigned char)round(blue);
            result[resultIndex + 3] = image[resultIndex + 3];
        }
    }

    for (int i = 0; i < width * height * 4; i++) {
        image[i] = result[i];
    }

    free(result);
}


Node* find(Node* x) {
    if (x->parent != x) {
        x->parent = find(x->parent);
    }
    return x->parent;
}

void union_set(Node* x, Node* y, double epsilon) {
    if (x->r < 40) {
        return;
    }
    Node* px = find(x);
    Node* py = find(y);

    double color_difference = sqrt(pow(x->r - y->r, 2) * 3);
    if (px != py && color_difference < epsilon) {
        if (px->rank > py->rank) {
            py->parent = px;
        } else {
            px->parent = py;
            if (px->rank == py->rank) {
                py->rank++;
            }
        }
    }
}


void find_components(Node* nodes, int width, int height, double epsilon) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Node* node = &nodes[y * width + x];
            if (node->up) {
                union_set(node, node->up, epsilon);
            }
            if (node->down) {
                union_set(node, node->down, epsilon);
            }
            if (node->left) {
                union_set(node, node->left, epsilon);
            }
            if (node->right) {
                union_set(node, node->right, epsilon);
            }
        }
    }
}

void color_components_and_count(Node* nodes, int width, int height) {
    unsigned char* output_image = malloc(width * height * 4 * sizeof(unsigned char));
    int* component_sizes = calloc(width * height, sizeof(int));
    int total_components = 0;

    srand(time(NULL));
    for (int i = 0; i < width * height; i++) {
        Node* p = find(&nodes[i]);
        if (p == &nodes[i]) {
            if (component_sizes[i] < 3) {
                p->r = 0;
                p->g = 0;
                p->b = 0;
            } else {
                p->r = rand() % 256;
                p->g = rand() % 256;
                p->b = rand() % 256;
            }
            total_components++;
        }
        output_image[4 * i + 0] = p->r;
        output_image[4 * i + 1] = p->g;
        output_image[4 * i + 2] = p->b;
        output_image[4 * i + 3] = 255;
        component_sizes[p - nodes]++;
    }

    char *output_filename = "output2.png";
    lodepng_encode32_file(output_filename, output_image, width, height);

    free(output_image);
    free(component_sizes);
}

void applySobelFilter(unsigned char *image, int width, int height) {
    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};


    unsigned char *result = malloc(width * height * 4 * sizeof(unsigned char));

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumX = 0, sumY = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int index = ((y+dy) * width + (x+dx)) * 4;
                    int gray = (image[index] + image[index + 1] + image[index + 2]) / 3;
                    sumX += gx[dy + 1][dx + 1] * gray;
                    sumY += gy[dy + 1][dx + 1] * gray;
                }
            }
            int magnitude = sqrt(sumX * sumX + sumY * sumY);
            if (magnitude > 255) magnitude = 255;
            if (magnitude < 0) magnitude = 0;

            int resultIndex = (y * width + x) * 4;
            result[resultIndex] = (unsigned char)magnitude;
            result[resultIndex + 1] = (unsigned char)magnitude;
            result[resultIndex + 2] = (unsigned char)magnitude;
            result[resultIndex + 3] = image[resultIndex + 3];
        }
    }

    for (int i = 0; i < width * height * 4; i++) {
        image[i] = result[i];
    }


    free(result);
}




int main() {
    int w = 0, h = 0;

    char *filename = "skull.png";

    char *picture = load_png_file(filename, &w, &h);

    applySobelFilter(picture, w, h);
//    applyFilter(picture, w, h);
//    applySobelFilter(picture, w, h);

    Node* nodes = malloc(w * h * sizeof(Node));

    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            Node* node = &nodes[y * w + x];
            unsigned char* pixel = &picture[(y * w + x) * 4];
            node->r = pixel[0];
            node->g = pixel[1];
            node->b = pixel[2];
            node->a = pixel[3];
            node->up = y > 0 ? &nodes[(y - 1) * w + x] : NULL;
            node->down = y < h - 1 ? &nodes[(y + 1) * w + x] : NULL;
            node->left = x > 0 ? &nodes[y * w + (x - 1)] : NULL;
            node->right = x < w - 1 ? &nodes[y * w + (x + 1)] : NULL;
            node->parent = node;
            node->rank = 0;
        }
    }

    double epsilon =48.0;
    find_components(nodes, w, h, epsilon);
    color_components_and_count(nodes, w, h);

    free(nodes);
    free(picture);
    return 0;
}

