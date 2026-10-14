"""Verify the manual implementation against the team's checksum table, without CSV."""
import argparse
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description='Verify CL11 checksums; not the 30-run benchmark.')
    parser.add_argument('mode', choices=('check', 'quick'), nargs='?', default='check')
    parser.add_argument('--compiler', default='g++')
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if not compiler and Path(args.compiler).is_file():
        compiler = str(Path(args.compiler).resolve())
    if not compiler and os.name == 'nt' and args.compiler == 'g++':
        candidate = Path('C:/msys64/ucrt64/bin/g++.exe')
        if candidate.is_file():
            compiler = str(candidate)
    if not compiler:
        raise RuntimeError('Install GCC with OpenMP or use --compiler PATH.')
    env = os.environ.copy()
    env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
    binary = ROOT / ('paralelo_manual.exe' if os.name == 'nt' else 'paralelo_manual')
    subprocess.run([compiler, '-O2', '-std=c++17', '-fopenmp', str(ROOT / 'convolve.cpp'),
                    '-o', str(binary)], env=env, check=True)
    table = (ROOT / 'checksums_referencia.md').read_text(encoding='utf-8')
    pattern = r'\|\s*(\d+) x \1\s*\|\s*(\d+) x \2\s*\|\s*(\d+)\s*\|'
    references = {(int(n), int(k)): int(c) for n, k, c in re.findall(pattern, table)}
    expected_keys = {(n, k) for n in (512, 1024, 2048, 4096) for k in (3, 5, 7, 11)}
    if set(references) != expected_keys:
        raise RuntimeError('The reference table must contain all 16 configurations.')
    cases = 0
    for (n, k), expected in references.items():
        if args.mode == 'quick' and (n, k) != (512, 3):
            continue
        for threads in (1, 2, 4, 8, 16):
            result = subprocess.run([str(binary)], input=f'{n} {k} {threads}\n',
                                    capture_output=True, text=True, env=env, check=True)
            lines = result.stdout.splitlines()
            if len(lines) != 2 or not lines[0].startswith('Tiempo de procesamiento (ms): ') or not lines[1].startswith('Checksum: '):
                raise RuntimeError('Unexpected output: ' + result.stdout)
            duration = float(lines[0].split(':', 1)[1])
            actual = int(lines[1].split(':', 1)[1])
            if not math.isfinite(duration) or duration < 0 or actual != expected:
                raise RuntimeError(f'Mismatch n={n}, k={k}, threads={threads}: {actual} != {expected}')
            cases += 1
            print(f'OK {n}x{n}, kernel {k}, threads {threads}: checksum {actual}', flush=True)
    print(f'PASS: {cases} checks. Executable: {binary.name}')
    print('These are correctness checks, not final performance measurements.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print('ERROR: ' + str(error), file=sys.stderr)
        sys.exit(1)
