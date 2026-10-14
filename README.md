# Versión paralela con OpenMP — CL11

Adaptación de *Parallel BMP Image Convolution*, atribuido a Danilo Todorović.
Procedencia: https://github.com/juancadg03/Parallel-Manual.
La implementación usa C++11 y OpenMP para repartir filas de salida entre hilos.

## Requisitos

- Python 3.8 o posterior, sin paquetes adicionales.
- GCC (`g++`) con soporte OpenMP. En Windows se puede usar MSYS2 UCRT64;
  el ejecutor detecta `C:/msys64/ucrt64/bin/g++.exe` si no está en PATH.
- Git para descargar esta rama.

No se necesitan imágenes externas para comenzar: el ejecutor genera BMP de
24 bits reproducibles. La compilación se realiza automáticamente con
`-std=c++11 -O3 -fopenmp -Wall -Wextra`.

## Descargar la rama

```text
git clone --branch Parallel-Manual --single-branch https://github.com/sguevaraidarraga/parallel-code-benchmarking.git
cd parallel-code-benchmarking
```

Si ya tienes esta rama descargada y no hay cambios locales pendientes, basta
con ejecutar `git pull --ff-only` desde su carpeta.

## Windows: tres pasos desde PowerShell

### 1. Comprobar el cálculo

```powershell
py -3 run_tests.py check
```

Compila y compara píxel por píxel contra un cálculo secuencial independiente
usado solo para pruebas. Comprueba los cuatro tamaños de kernel y las cinco
cantidades de hilos, con filtros identidad y asimétricos: 40 comparaciones.
Incluye dos comprobaciones de parámetros inválidos. La salida esperada termina
con `PASS`. Estas verificaciones no son mediciones para el informe y no sustituyen
la equivalencia con la implementación secuencial del equipo.

### 2. Hacer una medición corta

```powershell
py -3 run_tests.py quick
```

Genera una imagen de 512×512 y usa un kernel de promedio 3×3 con 1 y 2 hilos.
Realiza 30 mediciones por configuración, 60 en total. Sirve para comprobar
el flujo de ejecución; no completa el protocolo CL11.

### 3. Ejecutar el protocolo completo

```powershell
py -3 run_tests.py full
```

Recorre imágenes de 512, 1024, 2048 y 4096 píxeles por lado, kernels de
3, 5, 7 y 11, y 1, 2, 4, 8 y 16 hilos: 80 configuraciones, 30 mediciones
cada una, 2400 convoluciones medidas. Puede tardar bastante y guardar más de
1 GB de imágenes. Cierra aplicaciones intensivas y evita ejecutar experimentos
simultáneos. Finaliza con `DONE: 80 configurations, 2400 measured convolutions.`

Si el compilador está en otra ubicación:

```powershell
py -3 run_tests.py check --compiler "C:/ruta/bin/g++.exe"
```

También se incluye el acceso alternativo `run_benchmarks.ps1` con el parámetro
`-Mode check`, `-Mode quick` o `-Mode full`. Los comandos Python anteriores no
requieren cambiar la política de ejecución de PowerShell.

## Linux o WSL

Usa los mismos tres pasos sustituyendo `py -3` por `python3`:

```bash
python3 run_tests.py check
python3 run_tests.py quick
python3 run_tests.py full
```

También se puede ejecutar `bash run_benchmarks.sh full`.

## Resultados sin CSV

Los tiempos individuales, el promedio y la desviación estándar muestral se
muestran en pantalla. Se conserva además una copia de la salida en
`benchmark_results/<fecha-modo-identificador>/results.txt`, para no perderla
al cerrar la terminal. No se generan archivos CSV.
Cada ejecución crea una carpeta nueva y guarda sus imágenes de salida.
La generación de imágenes, la compilación, la escritura del registro y el
cálculo de estadísticas quedan fuera del tiempo medido por el programa.
El cronómetro incluye la gestión de hilos. No se realizan calentamientos:
la primera ejecución forma parte de las 30 muestras.

## Usar las imágenes acordadas por el equipo

```powershell
py -3 run_tests.py full --input-dir "C:/datos/imagenes"
```

La carpeta debe contener BMP RGB de 24 bits sin compresión con estos nombres
y dimensiones exactas: `input512x512.bmp`, `input1024x1024.bmp`,
`input2048x2048.bmp` e `input4096x4096.bmp`.
Se comprueban antes de medir y no se sobrescriben.
Las imágenes de ejemplo antiguas del repositorio no cumplen estos tamaños.

Las imágenes generadas automáticamente usan, en coordenadas x,y desde cero:
B=(13x+17y) mod 256, G=(7x+29y) mod 256 y R=(31x+3y) mod 256.
No son la imagen del generador secuencial del equipo: para comparar variantes
hay que acordar las mismas entradas y reglas de cálculo.

## Reglas del cálculo y alcance

- Procesa los tres canales de color en una sola región paralela.
- Usa el kernel girado 180 grados; sin coeficientes personalizados, aplica
  un promedio uniforme de peso `1/(k*k)`.
- No rellena bordes: una entrada n×n produce (n−k+1)×(n−k+1).
- Acumula en `float`, satura a [0,255] y redondea al convertir a byte.
- La desviación estándar es muestral, con denominador n−1.
- Las pruebas con referencia independiente usan coeficientes enteros pequeños
  para evitar diferencias de redondeo entre lenguajes; no cubren todos los
  posibles kernels decimales ni archivos BMP dañados.
- Speedup y eficiencia no se calculan aquí: requieren los tiempos de una
  referencia secuencial equivalente. Un hilo OpenMP no sustituye esa referencia.

Antes de comparar con otras ramas, deben coincidir imágenes, canales, kernels,
bordes, redondeo, compilación, hardware y política de medición. En particular,
esta versión RGB sin relleno no es directamente equivalente a una versión en
escala de grises con relleno de ceros.

## Procesar una imagen individual

```powershell
g++ -std=c++11 -O3 -fopenmp convolve.cpp -o convolve.exe
.\convolve.exe input500x500.bmp output.bmp 4 5
```

Para 30 mediciones manuales se añade `--benchmark`; ese modo exige uno de los
cuatro tamaños de imagen del ejercicio. Los coeficientes personalizados se
pueden indicar después del tamaño del kernel y antes de `--benchmark`.

## Verificación realizada

Se ejecutaron correctamente los modos `check` y `quick` en Windows con
GCC 15.2.0 (MSYS2 UCRT64). No se ejecutó el protocolo completo ni se verificó
la ejecución en Linux/WSL. Los tiempos de la comprobación corta son de
verificación del flujo y no constituyen los resultados finales del trabajo.
