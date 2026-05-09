# Cache-Oblivious BST vs Pointer BST

Integrantes:
Gianpier Segovia Ureta

José Ernesto Guerrero Cueva




**Implementación y comparación de:**

- **vEB-opt**: árbol binario de búsqueda estático con layout de van Emde Boas, búsqueda completamente inlined vía templates C++17.
- **vEB-dyn**: misma estructura, búsqueda recursiva dinámica (versión original, sin optimizar).
- **BST**: árbol de punteros balanceado, construido desde un arreglo ordenado, con pool de memoria contigua.

## Compilar y ejecutar

```bash
make run
# o equivalentemente:
g++ -O3 -march=native -std=c++17 -o bench bench.cpp && ./bench
```

## Idea del layout vEB

Un árbol perfecto de altura `h` se parte recursivamente en la mitad:

```
h = top_h + bot_h       (top_h = h/2, bot_h = h − top_h)
```

En memoria se almacena:

```
[ sub-árbol superior (h=top_h) | sub-árbol inferior 0 | ··· | sub-árbol inferior 2^top_h−1 ]
```

Cada bloque se ordena con el mismo esquema recursivamente (van Emde Boas layout). El resultado clave: **cualquier sub-árbol de altura `k` ocupa exactamente `2^k − 1` posiciones contiguas**. Cuando el bloque de caché tiene tamaño `B`, el sub-árbol de altura `log B` cabe en un bloque, y a partir de ese nivel todos los accesos son hits de caché. Esto da `O(log_B N)` cache misses por búsqueda, frente a `O(log N)` del BST de punteros.

Para `N` no de la forma `2^h − 1` se rellena hasta el siguiente tamaño perfecto con `INT32_MAX` como centinela. Para N = 20 M el relleno es a `2^25 − 1 ≈ 33.5 M`.

## Optimización: templates con `always_inline`

La versión dinámica recursa ~25 veces por búsqueda, con overhead de llamada a función en cada nivel. La versión optimizada usa una función template `veb_find_t<H>` marcada `[[gnu::always_inline]]`:

- El compilador instancia ~10 alturas distintas (25 → {12,13} → {6,7} → {3,4} → {1,2} → {0,1}).
- Toda la búsqueda se inlinea en una sola secuencia lineal de comparaciones y aritmética de punteros — cero overhead de llamadas.
- El caso base (H=1) es branchless: `gap = (key >= *a); return *a == key;`.

Un `switch(h)` en `VEBTree::find_opt` selecciona la instancia correcta en tiempo de compilación.

## Resultados

### Hardware

| Atributo   | Valor                            |
| ---------- | -------------------------------- |
| CPU        | Apple M2                         |
| L1-D cache | 64 KB por núcleo                 |
| L2 cache   | 4 MB por cluster                 |
| RAM        | 8 GB LPDDR5X (memoria unificada) |
| OS         | macOS 26.3.1                     |
| Compilador | Apple Clang 16.0.0               |
| Flags      | `-O3 -march=native -std=c++17`   |

---

### Benchmark N = 1 M (run anterior, sin optimizar)

```
N=951704  Q=1000000  T=7

Structure   avg(ms)   min(ms)   avg(ns/query)
---------   -------   -------   -------------
vEB           124.6     120.4           124.6
BST            79.6      74.4            79.6
```

A esta escala el BST gana: el árbol entero (4 MB) cabe en L2 y el overhead de recursión del vEB domina.

---

### Benchmark N = 20 M (con optimización de templates)

```
Building (N=19031748)...
  vEB: h=25, 134 MB
  BST: 457 MB (pool)

N=19031748  Q=1000000  T=7

Structure      avg(ms)   min(ms)   avg(ns/query)
------------   -------   -------   -------------
vEB-opt           99.7      97.4            99.7
vEB-dyn          327.3     325.0           327.3
BST              225.8     220.7           225.8

Per-run (ms):
  vEB-opt:  104.2   98.9   97.9   99.0   99.2  101.1   97.4
  vEB-dyn:  325.9  325.0  325.1  330.3  326.0  329.9  328.8
  BST:      228.7  220.7  224.7  225.2  223.8  231.4  226.0
```

---

### Análisis

#### Punto de quiebre y ventaja del layout

A N = 20 M **vEB-opt supera al BST por 2.26×** (99.7 ms vs 225.8 ms). El cambio cualitativo respecto a N = 1 M se explica por el tamaño relativo a la caché:

| Estructura                | Tamaño                  | Cabe en... |
| ------------------------- | ----------------------- | ---------- |
| vEB top-subtree (h=12)    | 4 095 × 4 B = **16 KB** | L1-D ✓     |
| vEB bottom-subtree (h=13) | 8 191 × 4 B = **32 KB** | L1-D ✓     |
| vEB total                 | 134 MB                  | DRAM       |
| BST (pool pre-order)      | 457 MB                  | DRAM       |

El sub-árbol de los primeros 12 niveles del vEB (16 KB) **queda residente en L1-D** después de las primeras consultas y es reutilizado en todas las búsquedas siguientes. El BST no tiene ninguna propiedad análoga: sus niveles superiores (pre-order pool) quedan distribuidos en los primeros bytes del pool, pero los nodos de cada nivel 4+ están separados por cientos de MB en el pool pre-order, generando cache misses sistemáticos.

#### El overhead de las llamadas recursivas era el cuello de botella

`vEB-dyn` (327 ms) es **más lento que el BST** (225 ms) incluso a N = 20 M: el overhead de ~25 invocaciones de función por búsqueda enmascara completamente la ventaja del layout. La optimización con templates lo reduce a cero y expone la ventaja real: `vEB-opt` gana por 3.3× sobre `vEB-dyn` y 2.26× sobre `BST`.

El punto de quiebre en el M2 se sitúa entre 1 M y 20 M elementos, donde el efecto de caché domina sobre el overhead del algoritmo.
