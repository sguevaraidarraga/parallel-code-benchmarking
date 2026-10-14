"""Portable correctness checks and CL11 measurements; Python standard library only."""
import argparse
import datetime
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
SIZES = (512, 1024, 2048, 4096)
KERNELS = (3, 5, 7, 11)
THREADS = (1, 2, 4, 8, 16)


def write_image(path, size):
    """Deterministic RGB pattern, stored as a bottom-up 24-bit BMP."""
    stride = (size * 3 + 3) & ~3
    with path.open('wb') as out:
        out.write(struct.pack('<2sIHHI', b'BM', 54 + stride * size, 0, 0, 54))
        out.write(struct.pack('<IiiHHIIiiII', 40, size, size, 1, 24, 0,
                              stride * size, 0, 0, 0, 0))
        for y in range(size - 1, -1, -1):
            # The pattern repeats every 256 columns; constructing a period is enough.
            period = bytes(value for x in range(256) for value in
                           ((13*x + 17*y) % 256, (7*x + 29*y) % 256,
                            (31*x + 3*y) % 256))
            row = (period * ((size + 255) // 256))[:size * 3]
            out.write(row + bytes(stride - len(row)))


def read_image(path):
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b'BM':
        raise ValueError('Invalid BMP: ' + str(path))
    offset = struct.unpack_from('<I', data, 10)[0]
    header, width, height, planes, bits, compression = struct.unpack_from('<IiiHHI', data, 14)
    if header < 40 or planes != 1 or bits != 24 or compression != 0 or width <= 0 or height == 0:
        raise ValueError('Expected uncompressed 24-bit BMP: ' + str(path))
    stride = (width * 3 + 3) & ~3
    if offset < 14 + header or len(data) < offset + stride * abs(height):
        raise ValueError('Truncated BMP: ' + str(path))
    rows = [data[offset + y*stride:offset + y*stride + width*3]
            for y in range(abs(height))]
    if height > 0:
        rows.reverse()
    return width, abs(height), b''.join(rows)


def reference(pixels, width, height, kernel, k):
    # Independent scalar oracle. Integer weights keep sums exact in C++ float.
    out = bytearray()
    for y in range(height - k + 1):
        for x in range(width - k + 1):
            for channel in range(3):
                total = 0
                for ky in range(k):
                    for kx in range(k):
                        total += pixels[((y+ky)*width + x+kx)*3 + channel] * kernel[(k-1-ky)*k + k-1-kx]
                out.append(max(0, min(total, 255)))
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description='CL11: checks and measurements without CSV.')
    parser.add_argument('mode', choices=('check', 'quick', 'full'), nargs='?', default='check')
    parser.add_argument('--compiler', default='g++', help='GCC executable or full path')
    parser.add_argument('--input-dir', type=Path, help='Directory containing inputNxN.bmp files')
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler and Path(args.compiler).is_file():
        compiler = str(Path(args.compiler).resolve())
    if not compiler and os.name == 'nt' and args.compiler == 'g++':
        candidate = Path('C:/msys64/ucrt64/bin/g++.exe')
        if candidate.is_file():
            compiler = str(candidate)
    if not compiler:
        raise RuntimeError('GCC with OpenMP was not found. Install it or pass --compiler PATH.')
    env = os.environ.copy()
    env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
    result_root = ROOT / 'benchmark_results'
    result_root.mkdir(exist_ok=True)
    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-')
    folder = Path(tempfile.mkdtemp(prefix=stamp + args.mode + '-', dir=result_root))
    binary = folder / ('convolve.exe' if os.name == 'nt' else 'convolve')
    with (folder / 'results.txt').open('w', encoding='utf-8') as log:
        def report(message):
            print(message, flush=True)
            log.write(message + '\n')
            log.flush()

        def run(command, expect_failure=False):
            result = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, errors='replace')
            if result.stdout:
                report(result.stdout.rstrip())
            if expect_failure:
                if result.returncode == 0:
                    raise RuntimeError('Invalid input was unexpectedly accepted.')
            elif result.returncode != 0:
                raise RuntimeError('Command failed; see ' + str(folder / 'results.txt'))
            return result.stdout

        report('Mode: ' + args.mode + '\nResults: ' + str(folder))
        run([compiler, '--version'])
        run([compiler, '-std=c++11', '-O3', '-fopenmp', '-Wall', '-Wextra',
             ROOT / 'convolve.cpp', '-o', binary])
        if args.mode == 'check':
            source, output = folder / 'input.bmp', folder / 'output.bmp'
            write_image(source, 19)  # Odd width also exercises BMP row padding.
            width, height, pixels = read_image(source)
            cases = 0
            for k in KERNELS:
                identity = [0] * (k*k)
                identity[(k*k)//2] = 1
                asymmetric = [0] * (k*k)
                asymmetric[0], asymmetric[k+1], asymmetric[-1] = 1, 1, -1
                for name, kernel in [('identity', identity), ('asymmetric', asymmetric)]:
                    expected = reference(pixels, width, height, kernel, k)
                    for threads in THREADS:
                        run([binary, source, output, threads, k, *kernel])
                        ow, oh, actual = read_image(output)
                        if (ow, oh) != (width-k+1, height-k+1) or actual != expected:
                            raise RuntimeError('Pixel mismatch: kernel={}, threads={}, case={}'.format(k, threads, name))
                        cases += 1
            for threads, k in [(3, 3), (1, 4)]:
                run([binary, source, output, threads, k], expect_failure=True)
            report('PASS: {} pixel comparisons and 2 invalid-argument checks.'.format(cases))
            return

        sizes = (512,) if args.mode == 'quick' else SIZES
        kernels = (3,) if args.mode == 'quick' else KERNELS
        thread_counts = (1, 2) if args.mode == 'quick' else THREADS
        inputs = args.input_dir.resolve() if args.input_dir else folder / 'inputs'
        if args.input_dir is None:
            inputs.mkdir()
            report('Generating deterministic RGB inputs (outside processing timers).')
            for size in sizes:
                write_image(inputs / ('input{}x{}.bmp'.format(size, size)), size)
        for size in sizes:
            path = inputs / ('input{}x{}.bmp'.format(size, size))
            width, height, _ = read_image(path)
            if (width, height) != (size, size):
                raise ValueError('Wrong input dimensions: ' + str(path))
        images = folder / 'images'
        images.mkdir()
        total = len(sizes) * len(kernels) * len(thread_counts)
        done = 0
        for size in sizes:
            for k in kernels:
                for threads in thread_counts:
                    done += 1
                    report('[{}/{}] {}x{}, kernel {}, threads {}'.format(done, total, size, size, k, threads))
                    source = inputs / ('input{}x{}.bmp'.format(size, size))
                    output = images / ('n{}_k{}_t{}.bmp'.format(size, k, threads))
                    text = run([binary, source, output, threads, k, '--benchmark'])
                    if sum(line.startswith('Run ') for line in text.splitlines()) != 30:
                        raise RuntimeError('Expected exactly 30 measurements.')
        report('DONE: {} configurations, {} measured convolutions.'.format(total, total*30))


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError) as error:
        print('ERROR: ' + str(error), file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print('\nInterrupted. Completed output remains in benchmark_results.', file=sys.stderr)
        sys.exit(130)
