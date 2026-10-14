#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <omp.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <limits>

using namespace std;

#pragma pack(push, 1)
struct BMPFileHeader {
    char signature[2];
    uint32_t fileSize;
    uint32_t reserved;
    uint32_t dataOffset;
};

struct BMPInfoHeader {
    uint32_t headerSize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t importantColors;
};
#pragma pack(pop)

void readBMP(const char* filename, vector<uint8_t>& B, vector<uint8_t>& G, vector<uint8_t>& R, int& width, int& height) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    // Check fread return values
    if (fread(&fileHeader, sizeof(BMPFileHeader), 1, file) != 1) {
        perror("Error reading BMP header");
        fclose(file);
        exit(1);
    }
    if (fread(&infoHeader, sizeof(BMPInfoHeader), 1, file) != 1) {
        perror("Error reading BMP info header");
        fclose(file);
        exit(1);
    }

    if (fileHeader.signature[0] != 'B' || fileHeader.signature[1] != 'M' ||
        infoHeader.headerSize < sizeof(BMPInfoHeader) || infoHeader.planes != 1 ||
        infoHeader.bitsPerPixel != 24 || infoHeader.compression != 0 ||
        fileHeader.dataOffset < sizeof(BMPFileHeader) + uint64_t(infoHeader.headerSize)) {
        fprintf(stderr, "Only 24-bit uncompressed BMP files are supported.\n");
        fclose(file);
        exit(1);
    }

    // Limit dimensions before calculating strides or allocating memory.
    if (infoHeader.width <= 0 || infoHeader.width > 4096 ||
        infoHeader.height == 0 || infoHeader.height < -4096 || infoHeader.height > 4096) {
        fclose(file);
        throw runtime_error("BMP dimensions must be between 1 and 4096.");
    }
    width = infoHeader.width;
    height = abs(infoHeader.height);
    bool topDown = infoHeader.height < 0;

    if (fseek(file, fileHeader.dataOffset, SEEK_SET) != 0) {
        perror("Error seeking to pixel data");
        fclose(file);
        exit(1);
    }

    int rowSize = (width * 3 + 3) & ~3;
    vector<uint8_t> row(rowSize);

    B.resize(width * height);
    G.resize(width * height);
    R.resize(width * height);

    for (int i = 0; i < height; ++i) {
        if (fread(row.data(), 1, rowSize, file) != static_cast<size_t>(rowSize)) {
            perror("Error reading pixel data");
            fclose(file);
            exit(1);
        }
        int y = topDown ? i : height - 1 - i;
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            B[idx] = row[x * 3];
            G[idx] = row[x * 3 + 1];
            R[idx] = row[x * 3 + 2];
        }
    }

    fclose(file);
}

void writeBMP(const char* filename, const vector<uint8_t>& B, const vector<uint8_t>& G, const vector<uint8_t>& R, int width, int height) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error creating file");
        exit(1);
    }

    BMPFileHeader fileHeader{};
    fileHeader.signature[0] = 'B';
    fileHeader.signature[1] = 'M';
    BMPInfoHeader infoHeader = {
        sizeof(BMPInfoHeader), width, height, 1, 24, 0, 0, 0, 0, 0, 0
    };

    int rowSize = (width * 3 + 3) & ~3;
    infoHeader.imageSize = rowSize * height;
    fileHeader.fileSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + infoHeader.imageSize;
    fileHeader.dataOffset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    if (fwrite(&fileHeader, sizeof(BMPFileHeader), 1, file) != 1 ||
        fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, file) != 1) {
        fclose(file);
        throw runtime_error("Could not write BMP headers.");
    }

    vector<uint8_t> row(rowSize);
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            row[x * 3] = B[idx];
            row[x * 3 + 1] = G[idx];
            row[x * 3 + 2] = R[idx];
        }
        if (fwrite(row.data(), 1, rowSize, file) != static_cast<size_t>(rowSize)) {
            fclose(file);
            throw runtime_error("Could not write BMP pixels.");
        }
    }

    if (fclose(file) != 0) throw runtime_error("Could not finish writing BMP.");
}

// Each thread owns complete output rows. The input and kernel are read-only.
// Output storage is allocated by the caller, outside the processing timer.
int convolveParallel(const vector<uint8_t>& B, const vector<uint8_t>& G,
                     const vector<uint8_t>& R, int width, int height,
                     const vector<float>& kernel, int kernelSize, int threads,
                     vector<uint8_t>& outB, vector<uint8_t>& outG,
                     vector<uint8_t>& outR) {
    const int outWidth = width - kernelSize + 1;
    const int outHeight = height - kernelSize + 1;
    int actualThreads = 0;

    #pragma omp parallel num_threads(threads) default(none) \
        shared(B, G, R, kernel, outB, outG, outR, actualThreads) \
        firstprivate(width, outWidth, outHeight, kernelSize)
    {
        #pragma omp single
        actualThreads = omp_get_num_threads();

        #pragma omp for schedule(static)
        for (int y = 0; y < outHeight; ++y) {
            for (int x = 0; x < outWidth; ++x) {
                float sumB = 0.0f, sumG = 0.0f, sumR = 0.0f;
                for (int ky = 0; ky < kernelSize; ++ky) {
                    const int inputRow = (y + ky) * width + x;
                    for (int kx = 0; kx < kernelSize; ++kx) {
                        // Rotate the kernel 180 degrees: mathematical convolution.
                        const float weight = kernel[(kernelSize - 1 - ky) * kernelSize
                                                    + kernelSize - 1 - kx];
                        const int index = inputRow + kx;
                        sumB += B[index] * weight;
                        sumG += G[index] * weight;
                        sumR += R[index] * weight;
                    }
                }
                const int outputIndex = y * outWidth + x;
                outB[outputIndex] = static_cast<uint8_t>(max(0.0f, min(sumB, 255.0f)) + 0.5f);
                outG[outputIndex] = static_cast<uint8_t>(max(0.0f, min(sumG, 255.0f)) + 0.5f);
                outR[outputIndex] = static_cast<uint8_t>(max(0.0f, min(sumR, 255.0f)) + 0.5f);
            }
        }
    }
    return actualThreads;
}

int parseInteger(const string& value) {
    size_t consumed = 0;
    const int result = stoi(value, &consumed);
    if (consumed != value.size()) throw invalid_argument("Expected an integer: " + value);
    return result;
}

bool requiredImageSize(int value) {
    return value == 512 || value == 1024 || value == 2048 || value == 4096;
}

int main(int argc, char* argv[]) {
    try {
        bool benchmark = argc > 1 && string(argv[argc - 1]) == "--benchmark";
        const int argumentCount = argc - (benchmark ? 1 : 0);
        if (argumentCount < 5) {
            cerr << "Usage: " << argv[0]
                 << " input.bmp output.bmp threads kernel_size [coefficients...] [--benchmark]\n"
                 << "Threads: 1, 2, 4, 8, 16. Kernel size: 3, 5, 7, 11.\n"
                 << "Without coefficients, a normalized box blur is used.\n";
            return 1;
        }
        const int threads = parseInteger(argv[3]);
        const int kernelSize = parseInteger(argv[4]);
        if (threads != 1 && threads != 2 && threads != 4 && threads != 8 && threads != 16)
            throw invalid_argument("Threads must be 1, 2, 4, 8 or 16.");
        if (kernelSize != 3 && kernelSize != 5 && kernelSize != 7 && kernelSize != 11)
            throw invalid_argument("Kernel size must be 3, 5, 7 or 11.");
        const int coefficients = kernelSize * kernelSize;
        if (argumentCount != 5 && argumentCount != 5 + coefficients)
            throw invalid_argument("Supply all kernel coefficients or none.");

        vector<float> kernel(coefficients, 1.0f / coefficients);
        double absoluteWeightSum = 0.0;
        for (int i = 0; i < coefficients; ++i) {
            if (argumentCount != 5) {
                const string value = argv[5 + i];
                size_t consumed = 0;
                kernel[i] = stof(value, &consumed);
                if (consumed != value.size() || !isfinite(kernel[i]))
                    throw invalid_argument("Kernel coefficients must be finite numbers.");
            }
            absoluteWeightSum += abs(static_cast<double>(kernel[i]));
        }
        if (absoluteWeightSum > numeric_limits<float>::max() / 512.0)
            throw invalid_argument("Kernel coefficients are too large for float accumulation.");

        int width, height;
        vector<uint8_t> B, G, R;
        readBMP(argv[1], B, G, R, width, height);
        if (width < kernelSize || height < kernelSize)
            throw invalid_argument("The kernel is larger than the image.");
        if (benchmark && (width != height || !requiredImageSize(width)))
            throw invalid_argument("Benchmark input must be 512, 1024, 2048 or 4096 pixels per side.");

        const int outWidth = width - kernelSize + 1;
        const int outHeight = height - kernelSize + 1;
        vector<uint8_t> outB(outWidth * outHeight), outG(outB.size()), outR(outB.size());
        omp_set_dynamic(0);
        const int runs = benchmark ? 30 : 1;
        vector<double> timings(runs);
        for (int run = 0; run < runs; ++run) {
            const auto start = chrono::steady_clock::now();
            const int actualThreads = convolveParallel(B, G, R, width, height, kernel,
                                                       kernelSize, threads, outB, outG, outR);
            const auto end = chrono::steady_clock::now();
            if (actualThreads != threads)
                throw runtime_error("OpenMP could not provide the requested number of threads.");
            timings[run] = chrono::duration<double, milli>(end - start).count();
        }
        // Disk I/O, allocation and statistics are outside the measured interval.
        writeBMP(argv[2], outB, outG, outR, outWidth, outHeight);
        cout << fixed << setprecision(6);
        if (benchmark) {
            const double mean = accumulate(timings.begin(), timings.end(), 0.0) / runs;
            double squaredDifferences = 0.0;
            for (double time : timings) squaredDifferences += (time - mean) * (time - mean);
            const double sampleStddev = sqrt(squaredDifferences / (runs - 1));
            cout << "Image: " << width << 'x' << height
                 << "; kernel: " << kernelSize << 'x' << kernelSize
                 << "; threads: " << threads << '\n';
            for (int run = 0; run < runs; ++run)
                cout << "Run " << run + 1 << ": " << timings[run] << " ms\n";
            cout << "Average time: " << mean << " ms\n"
                 << "Sample standard deviation: " << sampleStddev << " ms\n";
        } else {
            cout << "Processed " << width << 'x' << height << " -> " << outWidth << 'x'
                 << outHeight << " with " << threads << " threads and " << kernelSize << 'x'
                 << kernelSize << " kernel. Time: " << timings.front() << " ms\n";
        }
        return 0;
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
