# Convenções de Código — C++

> Complementa [`code-conventions.md`](./code-conventions.md). Só se aplica ao código
> C++ (`src/`).

---

## Documentação (estilo Doxygen)

Três níveis:

**1. Arquivo** — todo `.hpp`/`.cpp` começa com um bloco descritivo:
```cpp
/**
 * Resumo de uma linha do que o arquivo faz.
 *
 * Responsabilidades:
 * - Primeira responsabilidade.
 * - Segunda responsabilidade.
 */
```

**2. Função / método** — obrigatório quando há ≥ 2 parâmetros ou o retorno não é
óbvio:
```cpp
/// Simula um trace sob a política dada e conta as falhas de página.
/// @param trace       Páginas acessadas, em ordem.
/// @param frame_count Número de frames disponíveis.
/// @return Número de falhas de página.
uint64_t simulate(const std::vector<uint32_t>& trace, std::size_t frame_count);
```

**3. Classe** — comentário na classe e nos métodos públicos não-triviais.

---

## Tipos

- Inteiros de **largura fixa**: `uint32_t` para endereço e página, `uint64_t` para
  contadores e posições no trace. Nada de `int`/`long` para esses valores.
- Parâmetro não-trivial passa por `const&`; saída pelo retorno, não por ponteiro.
- `auto` só quando o tipo é óbvio pela linha (`auto it = map.find(p)`).
- Nunca `using namespace std;` em header.

## Naming

| Elemento | Estilo | Exemplo |
|---|---|---|
| Tipo (classe, struct, enum) | `PascalCase` | `LruApprox` |
| Função, variável, membro | `snake_case` | `frame_count` |
| Constante | `kPascalCase` | `kPageSize` |
| Membro privado | `snake_case_` | `history_` |

Nomes seguem o glossário do [`CONTEXT.md`](../../CONTEXT.md).

## Build

- `-std=c++17 -O2 -Wall -Wextra -Wpedantic` — compila **sem warnings**.
- Só STL; nenhuma dependência externa (ver `project-constraints.md`).
