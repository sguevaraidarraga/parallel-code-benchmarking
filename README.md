# Versión paralela con OpenMP — CL11

Adaptación de *Parallel BMP Image Convolution*, atribuido a Danilo Todorović.
Procedencia: [Parallel-Manual](https://github.com/juancadg03/Parallel-Manual).

## Cómo ejecutar las pruebas

Necesitas **Python 3.8 o posterior** y **GCC con OpenMP** (en Windows, MSYS2 UCRT64).
Abre una terminal en la carpeta del repositorio, en la rama `Parallel-Manual`.

En Windows (PowerShell):

```powershell
py -3 run_tests.py check  # Verificar el cálculo; debe terminar con PASS
py -3 run_tests.py quick  # Medición corta para comprobar que todo funciona
py -3 run_tests.py full   # Ejecutar todas las configuraciones del ejercicio
```

En Linux/WSL, sustituye `py -3` por `python3`.
La herramienta compila el programa y genera las imágenes automáticamente.
Si no encuentra GCC, añade `--compiler "C:/ruta/bin/g++.exe"` al comando.

El modo `full` ejecuta **80 configuraciones, con 30 mediciones cada una**:

- Imágenes: 512×512, 1024×1024, 2048×2048 y 4096×4096.
- Kernels: 3×3, 5×5, 7×7 y 11×11.
- Hilos: 1, 2, 4, 8 y 16.

Los resultados aparecen en pantalla y se guardan en
`benchmark_results/<ejecución>/results.txt`, junto con las imágenes procesadas.
**No se generan archivos CSV.** El modo completo puede tardar y ocupar más de 1 GB.

## Procedimiento y evaluación

**Protocolo:**

- Realizar **30 ejecuciones por configuración**.
- Medir exclusivamente el **tiempo de procesamiento**, incluida la gestión de hilos.
  Excluir lectura y escritura de imágenes, reserva de memoria y cálculo de estadísticas.
- Mantener las mismas condiciones de hardware y software para todas las variantes.

**Métricas que debe reportar la persona que realiza las pruebas:**

| Métrica | Cálculo o unidad |
| --- | --- |
| Tiempo de ejecución | Milisegundos (ms); registrar las 30 muestras y su promedio. |
| Speedup | Sp = T₁ / Tp |
| Eficiencia | Ep = Sp / p |
| Desviación estándar | Desviación estándar muestral de las 30 mediciones, en ms. |

**T₁** es el tiempo promedio de la versión secuencial de referencia;
**Tp**, el tiempo promedio de la versión paralela con **p** hilos.
Los promedios deben corresponder a la misma imagen y al mismo kernel.

El programa muestra los tiempos, el promedio y la desviación estándar.
**El speedup y la eficiencia se calculan después**, con los tiempos de la
referencia secuencial. Ejecutar esta versión con un hilo no sustituye esa referencia.

## Antes de comparar versiones

Usar las mismas imágenes, kernels y reglas de cálculo. Esta versión procesa
BMP RGB de 24 bits, gira el kernel 180°, recorta los bordes sin relleno y
redondea los valores al intervalo [0,255]. No es directamente equivalente a
una versión en escala de grises con relleno de ceros.

Para usar las imágenes acordadas por el equipo:

```powershell
py -3 run_tests.py full --input-dir "C:/datos/imagenes"
```

La carpeta debe contener `input512x512.bmp`, `input1024x1024.bmp`,
`input2048x2048.bmp` e `input4096x4096.bmp`, con esas dimensiones y sin compresión.

Se verificaron los modos `check` y `quick` en Windows. El experimento completo
queda a cargo de la persona que realizará las mediciones.
