# Benchmark de convolución 2D en C++: secuencial vs. paralelo con IA vs. paralelo manual

Este repositorio contiene el código, los scripts, los datos y las gráficas del experimento que mide el impacto de la paralelización (OpenMP) sobre una convolución 2D en C++, y compara código paralelo generado con IA, código generado con IA y ajustado, y código paralelo hecho a mano.

Autores: [completar nombres]. Curso: Programación Paralela, [completar universidad y semestre].

## Versiones comparadas

| Versión | Archivo | Origen | Paralelización | Reloj |
| --- | --- | --- | --- | --- |
| Secuencial | `secuencial.cpp` | Rama secuencial | Ninguna | `clock()` |
| Paralelo IA | `paralelo_ia.cpp` | GitHub Copilot. Prompt en `prompts/` | `#pragma omp parallel for` sobre las filas, con `num_threads(hilos)` y schedule por defecto | `omp_get_wtime()` |
| Paralelo IA + Opt. | `paralelo_ia_opt.cpp` | Copilot + ajustes| Igual, con `schedule(static)`, separación de zona interior y bordes, y punteros `__restrict` y de fila | `omp_get_wtime()` |
| Paralelo manual | `paralelo_manual.cpp` | Rama `Parallel-Manual` (archivo `convolve.cpp`), adaptado de [https://github.com/danitdrvc/Parallel-BMP-Image-Convolution/tree/main] | `omp parallel` con `omp for schedule(static)`, `default(none)` y verificación del número de hilos | `std::chrono::steady_clock` |


Las cuatro versiones generan la misma imagen, usan el mismo kernel (todo unos), tratan los bordes como 0, calculan con enteros y entregan el mismo checksum. Se compilan con la misma línea: `g++ -O2 -std=c++17 -fopenmp`.

## Configuración del experimento

- **Imágenes:** 512×512, 1024×1024, 2048×2048 y 4096×4096.
- **Kernels:** 3×3, 5×5, 7×7 y 11×11.
- **Hilos:** 1, 2, 4, 8 y 16 (el secuencial solo con 1).
- **Repeticiones:** 30 por configuración. Total: 7680 ejecuciones (480 del secuencial y 7200 de las paralelas).
- **Tiempo:** solo la llamada a la convolución. No incluye la generación de la imagen ni el cálculo del checksum.
- **Validación:** cada ejecución se considera válida si su checksum coincide con `checksums_referencia.md`. Las 7680 ejecuciones fueron válidas.
- **Métricas:** tiempo promedio (ms), desviación estándar muestral (N - 1), speedup `Sp = T1 / Tp` y eficiencia `Ep = Sp / p`, donde `T1` es el tiempo promedio de la versión secuencial con la misma imagen y kernel.

## Entorno de ejecución

Todas las pruebas se hicieron en un mismo equipo (datos de `real_results/informacion_maquina.txt`):

- **Procesador:** AMD Ryzen 7 5700X, 8 núcleos físicos y 16 hilos lógicos (2 hilos por núcleo).
- **Memoria:** 15.6 GB visibles dentro de WSL2.
- **Sistema:** contenedor Docker `gcc:13` sobre Windows con WSL2. Linux 6.18.54.1-microsoft-standard-WSL2, glibc 2.36. [Completar versión de Windows y de Docker.]
- **Software:** g++ (GCC) 13.5.0 y Python 3.11.2.
- **Fecha de la corrida:** 2026-10-14.

## Estructura del repositorio

```
secuencial.cpp, paralelo_ia.cpp, paralelo_ia_opt.cpp, paralelo_manual.cpp   Las cuatro versiones
ejecutar_pruebas.py       Corre las 30 repeticiones de cada configuración y valida el checksum
calcular_metricas.py      Calcula promedio, desviación, speedup y eficiencia
informacion_maquina.py    Guarda los datos de hardware y software
graficas.py               Genera las gráficas a partir de metricas.csv
checksums_referencia.md   Checksums esperados para las 16 combinaciones de imagen y kernel
prompts/                  Prompts usados con Copilot y sus respuestas sin editar
real_results/             Resultados de la corrida completa
  resultados.csv          Una fila por ejecución (7680)
  metricas.csv            Una fila por configuración (256)
  informacion_maquina.txt
  graficas/               Figuras en PNG y PDF
```

## Cómo reproducir

Entrada de cada programa por entrada estándar: `n k hilos`. Salida: dos líneas, `Tiempo de procesamiento (ms): <tiempo>` y `Checksum: <número>`.

```
g++ -O2 -std=c++17 -fopenmp secuencial.cpp -o secuencial
g++ -O2 -std=c++17 -fopenmp paralelo_ia.cpp -o paralelo_ia
g++ -O2 -std=c++17 -fopenmp paralelo_ia_opt.cpp -o paralelo_ia_opt
g++ -O2 -std=c++17 -fopenmp paralelo_manual.cpp -o paralelo_manual

echo "512 3 4" | ./paralelo_ia          # prueba individual

python3 informacion_maquina.py          # datos de la máquina
python3 ejecutar_pruebas.py             # corrida completa (MODO_PRUEBA = False)
python3 calcular_metricas.py            # genera metricas.csv
python3 graficas.py                     # requiere pandas y matplotlib
```

Notas:

- `ejecutar_pruebas.py` guarda cada ejecución al terminar. Si se interrumpe, al volver a correrlo continúa donde quedó.
- Si se cambia cualquier programa después de empezar, hay que borrar `resultados.csv` y empezar de nuevo.
- Con `MODO_PRUEBA = True` se hace una prueba rápida de 42 ejecuciones, útil para comprobar el entorno. Sus resultados no deben mezclarse con la corrida completa.
- Para comparar tiempos, las cuatro versiones deben compilarse y ejecutarse en la misma máquina y en las mismas condiciones.

## Cómo interpretar los resultados

- `metricas.csv` tiene: `version`, `imagen`, `kernel`, `hilos`, `repeticiones`, `tiempo_promedio_ms`, `desviacion_ms`, `minimo_ms`, `maximo_ms`, `speedup` y `eficiencia`.
- **El speedup y la eficiencia se miden contra el secuencial.** `paralelo_ia_opt` ya es más rápido que el secuencial con un solo hilo, así que su eficiencia puede superar 1 sin que haya paralelismo superlineal. Para aislar el escalamiento propio de cada versión, `graficas.py` calcula también el *speedup propio* (tiempo con 1 hilo de la misma versión dividido entre el tiempo con `p` hilos).
- La máquina tiene 8 núcleos físicos. Con 16 hilos se usan hilos lógicos (SMT), por lo que la eficiencia a 16 hilos no es comparable con la de 8 o menos.
- Con imágenes pequeñas los tiempos son de pocos milisegundos y la variabilidad relativa es alta, sobre todo con 16 hilos.

## Limitaciones conocidas

- **Relojes distintos.** El secuencial mide con `clock()` y las paralelas con tiempo real. Con 1 hilo ambos deberían coincidir, y los speedups de las paralelas con 1 hilo salen cercanos a 1, pero la medición no está unificada.
- **Entorno virtualizado.** Docker sobre WSL2 añade una capa de virtualización y el ruido de Windows en segundo plano.
- **Una sola máquina y una sola corrida** de 30 repeticiones por configuración.
- **Las versiones IA tienen restricciones de prompt.** El prompt de `paralelo_ia` prohibió explícitamente técnicas avanzadas de optimización, así que no representa el máximo que la IA podría generar.

## Atribución

El código de `paralelo_manual.cpp` parte de [completar fuente, autor y licencia del código original]. Los scripts `ejecutar_pruebas.py`, `calcular_metricas.py` e `informacion_maquina.py` y el código secuencial provienen de la rama `secuencial` de este repositorio.
