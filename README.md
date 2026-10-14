# Versión paralela manual — CL11

Implementación OpenMP adaptada al contrato de la rama `secuencial` (revisión
`c94ac0b`). Antecedente del proyecto: *Parallel BMP Image Convolution*, atribuido
a Danilo Todorović; fork de procedencia: https://github.com/juancadg03/Parallel-Manual.
La versión BMP anterior se conserva en el historial Git. Las imágenes BMP de
muestra ya no se utilizan en el experimento.

## Compilar y ejecutar

Se necesita GCC con OpenMP. El código está en `convolve.cpp` y el ejecutable
debe llamarse **paralelo_manual** (en Windows, **paralelo_manual.exe**).

```bash
g++ -O2 -std=c++17 -fopenmp convolve.cpp -o paralelo_manual
echo "512 3 4" | ./paralelo_manual
```

En PowerShell:

```powershell
g++ -O2 -std=c++17 -fopenmp convolve.cpp -o paralelo_manual.exe
"512 3 4" | .\paralelo_manual.exe
```

La entrada es `n k hilos`. Solo se imprimen estas dos líneas:

```text
Tiempo de procesamiento (ms): <tiempo>
Checksum: <número>
```

`n`: 512, 1024, 2048 o 4096; `k`: 3, 5, 7 u 11;
`hilos`: 1, 2, 4, 8 o 16. Una ejecución realiza **una sola convolución**.

## Comprobar los resultados antes de entregar

Con Python 3.8 o posterior, desde la carpeta del repositorio:

```powershell
py -3 run_tests.py quick  # Una combinación con las cinco cantidades de hilos
py -3 run_tests.py check  # Las 16 combinaciones con 1, 2, 4, 8 y 16 hilos
```

En Linux/WSL, cambiar `py -3` por `python3`. El script compila automáticamente
con las mismas opciones anteriores y compara contra `checksums_referencia.md`.
Debe terminar con **PASS: 80 checks** en el modo `check`.
Si GCC no está en PATH, usar `--compiler "C:/ruta/bin/g++.exe"`; también detecta
la instalación de MSYS2 UCRT64 en Windows.
Estas comprobaciones no ejecutan las 30 repeticiones del experimento.

## Reglas compartidas con la versión secuencial

- Misma imagen de un canal: estado inicial 12345, actualización
  `(estado * 75 + 74) % 65537`, píxel `estado % 256`, en orden por filas.
- Kernel lleno de unos; suma y división entre `k*k` con enteros.
- Vecinos fuera de la imagen valen cero. La salida mantiene el tamaño `n*n`.
- Mismo checksum: inicia en 7 y actualiza `(h * 131 + pixel) % 1000003`.
- Cada hilo procesa filas distintas usando OpenMP; no se generan archivos BMP.
- El tiempo real se mide con un reloj monotónico únicamente alrededor de la
  convolución, incluida la gestión de hilos. Generación, reserva de memoria,
  checksum y salida por pantalla quedan fuera del intervalo.

## Pruebas y métricas del equipo

Entregar `paralelo_manual` a quien ejecutará las pruebas. Debe colocarlo junto
a los otros tres ejecutables y los scripts de la rama `secuencial`.
Seguir [README_pruebas.md](https://github.com/sguevaraidarraga/parallel-code-benchmarking/blob/secuencial/README_pruebas.md).
En Windows, usar `.exe` en las rutas de `VERSIONES` dentro de `ejecutar_pruebas.py`.

- **30 ejecuciones por configuración**, realizadas por el script del equipo.
- Medir exclusivamente el **tiempo de procesamiento en ms**.
- Calcular **speedup: Sp = T₁ / Tp**.
- Calcular **eficiencia: Ep = Sp / p**.
- Calcular **desviación estándar muestral** de los tiempos.

T₁ es el promedio secuencial y Tp el promedio paralelo con p hilos, para la
misma imagen y kernel. Compilar todas las versiones con la misma optimización
`-O2` y estándar `-std=c++17`; `-fopenmp` habilita OpenMP donde se utiliza.
Usar la misma máquina y las mismas condiciones de ejecución.

Este programa no genera CSV ni calcula las métricas comparativas: eso queda
a cargo de `ejecutar_pruebas.py` y `calcular_metricas.py` del equipo.
Los resultados de la versión BMP anterior no deben mezclarse con esta versión.
La referencia secuencial aún usa `clock()`; el equipo debe unificar su medición
con tiempo real para la comparación final.

## Verificacion realizada

Las 16 combinaciones coincidieron con los checksums de referencia usando
1, 2, 4, 8 y 16 hilos: 80 comprobaciones correctas en Windows con GCC 15.2.0.
Se verifico tambien el formato de las dos lineas de salida. No se ejecuto
el experimento de 30 repeticiones por configuracion.

