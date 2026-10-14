# Cómo correr las pruebas

Estos scripts ejecutan las cuatro versiones del programa, repiten cada configuración 30 veces, comprueban que los resultados sean correctos y guardan los tiempos en un archivo CSV.

## Qué se necesita

- Python 3. No hace falta instalar nada más.
- Los cuatro programas ya compilados, en la misma carpeta que los scripts, con estos nombres exactos: `secuencial`, `paralelo_ia`, `paralelo_ia_opt` y `paralelo_manual`. En Windows llevan `.exe` y hay que cambiar las rutas al inicio de `ejecutar_pruebas.py`.
- Que todos los programas reciban y entreguen lo mismo que la versión secuencial (ver `README.md`). La entrada es `n k hilos` y la salida son dos líneas, la del tiempo y la del checksum.
- Que todos se compilen con las mismas opciones.
- Que los programas paralelos midan tiempo real y no con `clock()`, como explica `README.md`.

## Pasos

1. Compilar las cuatro versiones.
2. Guardar los datos de la máquina con `python3 informacion_maquina.py`. Crea `informacion_maquina.txt`, que sirve para la tabla de hardware y software del paper.
3. Hacer una prueba rápida. Poner `MODO_PRUEBA = True` al inicio de `ejecutar_pruebas.py` y correr `python3 ejecutar_pruebas.py`. Revisar que ninguna fila de `resultados.csv` diga `no` o `error` en la columna `valido`. Al terminar, borrar `resultados.csv`.
4. Hacer la corrida completa. Poner `MODO_PRUEBA = False` y correr `python3 ejecutar_pruebas.py`. Son 7680 ejecuciones y puede tardar horas. Durante la corrida no se debe usar la máquina para otra cosa ni dejar que se suspenda.
5. Si la corrida se interrumpe, se vuelve a correr el mismo comando y continúa desde donde quedó.
6. Calcular las métricas con `python3 calcular_metricas.py`. Crea `metricas.csv`.

## Importante

- Si se cambia cualquier programa después de empezar la corrida, hay que borrar `resultados.csv` y empezar de nuevo. Con el programa cambiado, los datos anteriores ya no se pueden comparar.
- Una ejecución es válida cuando su checksum coincide con el de `checksums_referencia.md`. Las ejecuciones que no son válidas no se usan en las métricas y el script avisa cuántas hay.

## Archivos que se generan

**`resultados.csv`** tiene una fila por ejecución:

- `version`, `imagen`, `kernel` e `hilos`: la configuración que se ejecutó.
- `repeticion`: número de la repetición, de 1 a 30.
- `tiempo_ms`: tiempo de procesamiento en milisegundos.
- `checksum`: número que imprimió el programa.
- `valido`: `si` si coincide con la referencia, `no` si es distinto y `error` si el programa falló.

**`metricas.csv`** tiene una fila por configuración:

- `repeticiones`: cuántas ejecuciones válidas se usaron.
- `tiempo_promedio_ms`, `desviacion_ms`, `minimo_ms` y `maximo_ms`: resumen de los tiempos. La desviación estándar usa N - 1.
- `speedup`: T1 dividido entre Tp. T1 es el tiempo promedio de la versión secuencial con la misma imagen y el mismo kernel.
- `eficiencia`: el speedup dividido entre el número de hilos.