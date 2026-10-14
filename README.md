# Convolución 2D: versiones paralelas

Este directorio contiene dos versiones paralelas del benchmark de convolución 2D con OpenMP:

- `paralelo_ia.cpp`: versión paralela directa.
- `paralelo_ia_opt.cpp`: versión paralela con optimizaciones de acceso a memoria y separación de la zona interior.

Ambas versiones deben dar exactamente el mismo resultado para los mismos valores de entrada.

## Cómo compilar y ejecutar

### Versión paralela directa

```
g++ -O2 -std=c++17 -fopenmp paralelo_ia.cpp -o paralelo_ia
echo "512 3 4" | ./paralelo_ia
```

### Versión paralela optimizada

```
g++ -O2 -std=c++17 -fopenmp paralelo_ia_opt.cpp -o paralelo_ia_opt
echo "512 3 4" | ./paralelo_ia_opt
```

## Entrada

Tres números enteros separados por espacios, que se leen por la entrada estándar:

1. Tamaño de la imagen (n). La imagen es de n por n píxeles.
2. Tamaño del kernel (k). El kernel es de k por k.
3. Número de hilos. Las versiones paralelas usan exactamente este número de hilos mediante OpenMP.

## Salida

Dos líneas de texto:

```
Tiempo de procesamiento (ms): <tiempo>
Checksum: <número>
```

El checksum es un número que resume toda la imagen de salida. Si ambas versiones dan el mismo checksum para una entrada, sus resultados son iguales. Los valores de referencia están en `checksums_referencia.md`.

## Lo que deben copiar igual todas las versiones

- **Imagen:** el estado inicial es 12345. Para cada píxel, en orden por filas, primero se actualiza el estado con `estado = (estado * 75 + 74) % 65537` y el valor del píxel es `estado % 256`.
- **Kernel:** todos sus valores son 1.
- **Bordes:** los vecinos que quedan fuera de la imagen valen 0.
- **Números:** todo se calcula con enteros. Al final la suma de cada píxel se divide entre k por k con división entera.
- **Checksum:** empieza en `h = 7`. Por cada píxel de la salida, en orden por filas, se calcula `h = (h * 131 + píxel) % 1000003`.
- **Compilación:** ambas versiones se compilan con `-O2 -std=c++17 -fopenmp`.

## Tiempo

Se mide solo la llamada a la convolución. No incluye la generación de la imagen ni el cálculo del checksum.

Las dos versiones paralelas usan `omp_get_wtime()`, que mide el tiempo real transcurrido. La medición incluye solo la llamada a la convolución: no incluye la generación de la imagen ni el cálculo del checksum.

## Diferencias entre las versiones

`paralelo_ia.cpp` paraleliza las filas de la imagen con `#pragma omp parallel for`.

`paralelo_ia_opt.cpp` mantiene el mismo algoritmo y la misma salida, pero además:

- usa `schedule(static)` porque las filas tienen un costo similar;
- separa la zona interior de los bordes para evitar comprobaciones de límites en la mayoría de los píxeles;
- utiliza punteros locales y punteros de fila para reducir el cálculo repetido de índices.