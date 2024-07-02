#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    int width;
    int height;
    unsigned char *data; 
} Image;

bool isValidPercentage(const char *percentage_str) {
    
    if (percentage_str == NULL || strlen(percentage_str) < 2) {
        return false;
    }

    if (percentage_str[strlen(percentage_str) - 1] != '%') {
        return false;
    }

    char *endptr;
    float value = strtof(percentage_str, &endptr);

    if (endptr != NULL && *endptr == '%' && *(endptr + 1) == '\0') {
        if (value < 0.0f || value > 100.0f) {
            return false;
        } 
        if (endptr == percentage_str) {
            return false;
        }
        return true;
    }

    return false;
}

float percentToFloat(const char *percentage) {
    int length = strlen(percentage);
    if (percentage[length - 1] != '%') {
        fprintf(stderr, "The ratio does not end with '%'.\n");
        return -1.0f; 
    }
    char *temp = (char *)malloc((length + 1) * sizeof(char));
    if (temp == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    strncpy(temp, percentage, length - 1);
    float result = atof(temp) / 100.0f;
    free(temp);
    return result;
}

Image bmpToStruct(FILE *bmpFile) {
    Image image;
    unsigned char buffer[54];
    int dataOffset;
    int imageSize;

    fseek(bmpFile, 0, SEEK_SET);

    size_t bytesRead = fread(buffer, 1, 54, bmpFile);
    if (bytesRead != 54) {
        fprintf(stderr, "Error reading BMP file header.\n");
        return (Image){0, 0, NULL}; 
    }

    image.width = (buffer[18] | (buffer[19] << 8) | (buffer[20] << 16) | (buffer[21] << 24));
    image.height = (buffer[22] | (buffer[23] << 8) | (buffer[24] << 16) | (buffer[25] << 24));
    if (image.width <= 0 || image.height <= 0) {
        fprintf(stderr, "Invalid image dimensions in BMP file.\n");
        return (Image){0, 0, NULL}; 
    }

    dataOffset = (buffer[10] | (buffer[11] << 8) | (buffer[12] << 16) | (buffer[13] << 24));

    fseek(bmpFile, dataOffset, SEEK_SET);

    imageSize = image.width * image.height * 3;
    if (imageSize <= 0) {
        fprintf(stderr, "Invalid image size calculation.\n");
        return (Image){0, 0, NULL}; 
    }

    image.data = (unsigned char*)malloc(imageSize);
    if (image.data == NULL) {
        perror("Memory allocation failed");
        return (Image){0, 0, NULL}; 
    }

    bytesRead = fread(image.data, 1, imageSize, bmpFile);
    if (bytesRead != imageSize) {
        fprintf(stderr, "Error reading BMP pixel data.\n");
        free(image.data);
        return (Image){0, 0, NULL}; 
    }
    return image;
}

Image blend(const Image *imgA, const Image *imgB, float ratio) {
    if (imgA->width != imgB->width || imgA->height != imgB->height) {
        fprintf(stderr, "Images must have the same dimensions.\n");
        exit(EXIT_FAILURE);
    }

    Image result;
    result.width = imgA->width;
    result.height = imgA->height;
    result.data = (unsigned char *)malloc(result.width * result.height * 3);
    if (!result.data) {
        perror("Memory allocation failed for blended image");
        exit(EXIT_FAILURE);
    }

    float inverseRatio = 1.0f - ratio;
    for (int i = 0; i < result.width * result.height; i++) {
        for (int channel = 0; channel < 3; channel++) {
            int index = (i * 3) + channel;
            result.data[index] = (unsigned char)(
                (imgA->data[index] * ratio + imgB->data[index] * inverseRatio) > 255.0f
                ? 255
                : ((imgA->data[index] * ratio + imgB->data[index] * inverseRatio) < 0.0f
                    ? 0
                    : (imgA->data[index] * ratio + imgB->data[index] * inverseRatio))
            );
        }
    }

    return result;
}

void structToBMP(const Image *img, const unsigned char *headerStr, const char *filename) {
    if (img == NULL || headerStr == NULL || filename == NULL) {
        fprintf(stderr, "Invalid input parameters.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }
    
    fwrite(headerStr, 1, 54, file);

    unsigned int dataSize = img->width * img->height * 3;

    fwrite(img->data, 1, dataSize, file);

    int padding = (img->width * 3) % 4;
    if (padding > 0) {
        unsigned char padByte = 0;
        for (int i = 0; i < img->height; i++) {
            if (fputc(0, file) == EOF) {
                perror("Error writing padding byte");
                break;
            }
        }
    }

    fclose(file);
}

unsigned char* extractBMPHeader(FILE *bmpFile) {
    unsigned char *header = (unsigned char*)malloc(55);
    if (header == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    size_t bytesRead = fread(header, 1,54, bmpFile);
    if (bytesRead != 54) {
        fprintf(stderr, "Failed to read the BMP header.\n%d",bytesRead);
        free(header);
        exit(EXIT_FAILURE);
    }

    return header;
}

int main(int argc, char *argv[]){
    if (argc != 5){
        printf("Usage: %s <x.bmp> <ratio> <y.bmp> <result.bmp>\n", argv[0]);
        return 1;
    }
    if (!isValidPercentage(argv[2])){
    	printf("The input ratio is invalid.");
    	return 1;;
	}

    FILE *originImage0 = fopen(argv[1], "rb");
    if (!originImage0) {
        perror("Error opening the first BMP file");
        return 1;
    }
	unsigned char *header = extractBMPHeader(originImage0);
    fclose(originImage0);
	
    float ratio = percentToFloat(argv[2]);
    //printf("The ratio is:%f\n",ratio);

    FILE *originImage1 = fopen(argv[1], "rb");
    if (!originImage1) {
        perror("Error opening the first BMP file");
        return 1;
    }
    Image image1 = bmpToStruct(originImage1);
    
    fclose(originImage1);
    

    FILE *originImage2 = fopen(argv[3], "rb");
    if (!originImage2) {
        perror("Error opening the second BMP file");
        free(image1.data); 
        return 1;
    }

    Image image2 = bmpToStruct(originImage2);
    fclose(originImage2);
	
	printf("Image 1 dimensions: %d x %d\n", image1.width, image1.height);
	printf("Image 2 dimensions: %d x %d\n", image2.width, image2.height);

    Image result_image = blend(&image1, &image2, ratio);
    structToBMP(&result_image,header,argv[4]);

    free(image1.data);
    free(image2.data);
    free(result_image.data);

    printf("Blending complete. Result saved as '%s'\n", argv[4]);
    return 0;
}
