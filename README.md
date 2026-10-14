# Convolución 2D: versión secuencial

Esta es la versión de referencia del trabajo que compara código paralelo generado con IA y código hecho a mano. Todas las demás versiones deben dar exactamente el mismo resultado que esta.

## Cómo compilar y ejecutar

```
g++ -O2 -std=c++17 secuencial.cpp -o secuencial
echo "512 3 1" | ./secuencial
```

## Entrada

Tres números enteros separados por espacios, que se leen por la entrada estándar:

1. Tamaño de la imagen (n). La imagen es de n por n píxeles.
2. Tamaño del kernel (k). El kernel es de k por k.
3. Número de hilos. Esta versión lo lee pero lo ignora, para que todas las versiones reciban lo mismo.

## Salida

Dos líneas de texto:

```
Tiempo de procesamiento (ms): <tiempo>
Checksum: <número>
```

El checksum es un número que resume toda la imagen de salida. Si una versión paralela da el mismo checksum que esta versión, su resultado es el mismo. Los valores de referencia están en `checksums_referencia.md`.

## Lo que deben copiar igual todas las versiones

- **Imagen:** el estado inicial es 12345. Para cada píxel, en orden por filas, primero se actualiza el estado con `estado = (estado * 75 + 74) % 65537` y el valor del píxel es `estado % 256`.
- **Kernel:** todos sus valores son 1.
- **Bordes:** los vecinos que quedan fuera de la imagen valen 0.
- **Números:** todo se calcula con enteros. Al final la suma de cada píxel se divide entre k por k con división entera.
- **Checksum:** empieza en `h = 7`. Por cada píxel de la salida, en orden por filas, se calcula `h = (h * 131 + píxel) % 1000003`.
- **Compilación:** las mismas opciones en las cuatro versiones.

## Tiempo

Se mide solo la llamada a la convolución. No incluye la generación de la imagen ni el cálculo del checksum.

Esta versión usa `clock()`, que funciona bien con un solo hilo. Con varios hilos, `clock()` suma el tiempo de procesador de todos los hilos y no mide el tiempo real. Por eso las versiones paralelas necesitan otro método, y el grupo debe acordar uno común para las cuatro versiones.
