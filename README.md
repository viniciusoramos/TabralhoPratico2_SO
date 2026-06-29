# Trabalho Prático 2 — Gerenciador de Memória Virtual

Simulador de gerência de memória virtual em C: traduz endereços lógicos para físicos
usando TLB (FIFO), tabela de páginas, paginação por demanda a partir do
`BACKING_STORE.bin` e substituição de páginas por LRU aproximado (*Aging*).

**Disciplina:** Sistemas Operacionais — PUC Minas

**Integrantes:**

- Vinícius Oliveira Ramos
- Thiago Costa Soares

## Estrutura

```text
vm_manager/
├── Makefile
├── README.md
├── include/
│   ├── config.h
│   ├── tlb.h
│   ├── page_table.h
│   ├── memory.h
│   └── statistics.h
├── src/
│   ├── main.c
│   ├── tlb.c
│   ├── page_table.c
│   ├── memory.c
│   └── statistics.c
├── data/
│   └── generate_data.py
└── report/
```

## Gerar arquivos comuns de entrada

Entre no diretório `data` e execute:

```bash
python3 generate_data.py
```

Isso criará:

- `BACKING_STORE.bin`
- `addresses_random.txt`
- `addresses_location.txt`

## Compilação

Na raiz do projeto:

```bash
make
```

## Execução

```bash
./vm < data/addresses_random.txt
```

ou

```bash
./vm < data/addresses_location.txt
```

## Status da implementação

Todas as funções marcadas originalmente com `TODO` foram implementadas:

- Tradução de endereços (extração de página/offset e cálculo do endereço físico);
- Tabela de páginas (consulta, atualização e invalidação);
- TLB com política FIFO (busca, inserção e remoção);
- Tratamento de *page fault* com paginação por demanda a partir do `BACKING_STORE.bin`;
- Substituição de páginas com LRU aproximado (*Aging* de 8 bits);
- Estatísticas (taxa de *page faults* e taxa de acerto da TLB).

O relatório técnico completo está em [`report/RELATORIO.md`](report/RELATORIO.md)
(versão em PDF: [`report/RELATORIO.pdf`](report/RELATORIO.pdf)).

## Resultados

| Métrica | `addresses_random.txt` | `addresses_location.txt` |
|---|---:|---:|
| Endereços traduzidos | 10.000 | 10.000 |
| *Page Faults* | 4.958 | 1.164 |
| *Page Fault Rate* | 0,496 | 0,116 |
| *TLB Hits* | 654 | 7.925 |
| *TLB Hit Rate* | 0,065 | 0,792 |

A localidade de referência reduz a taxa de faltas de página e eleva fortemente a taxa de
acerto da TLB, conforme esperado pela teoria.

## Compilação sem `make`/`gcc`

O projeto compila com qualquer compilador C11. Caso `make` ou `gcc` não estejam
disponíveis, é possível usar, por exemplo, o `zig cc`:

```bash
zig cc -Wall -Wextra -O2 -std=c11 -Iinclude src/*.c -o vm
```

## Validação

A correção foi verificada por dois métodos independentes:

1. **Invariante determinístico** — como `BACKING_STORE.bin` armazena `p % 256` em cada
   posição `p`, toda tradução correta satisfaz `Value == (signed char)(endereço & 0xFF)`.
   Verificado para os 10.000 endereços de cada arquivo de teste.
2. **Reimplementação de referência** — a saída do programa coincide linha a linha com um
   simulador independente escrito a partir da especificação.
