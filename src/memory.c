#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "config.h"
#include "page_table.h"
#include "tlb.h"

static signed char physical_memory[NUM_FRAMES][FRAME_SIZE];

/*
 * Indica qual página está carregada em cada quadro.
 * Valor -1 indica quadro livre.
 */
static int frame_to_page[NUM_FRAMES];

static FILE *backing = NULL;

void memory_init(FILE *backing_store)
{
    backing = backing_store;

    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_to_page[i] = -1;

        for (int j = 0; j < FRAME_SIZE; j++) {
            physical_memory[i][j] = 0;
        }
    }
}

static int find_free_frame(void)
{
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frame_to_page[i] == -1) {
            return i;
        }
    }

    return -1;
}

int handle_page_fault(int page)
{
    /*
     * Tratamento de uma falta de página com paginação por demanda:
     * 1. Procura um quadro livre;
     * 2. Se não houver, seleciona uma página vítima (LRU aproximado),
     *    invalida sua entrada na tabela de páginas e a remove do TLB,
     *    liberando o quadro ocupado por ela;
     * 3. Lê a página solicitada do BACKING_STORE.bin para o quadro;
     * 4. Atualiza frame_to_page e a tabela de páginas;
     * 5. Retorna o número do quadro.
     */

    if (backing == NULL) {
        fprintf(stderr, "Erro interno: BACKING_STORE nao inicializado.\n");
        exit(1);
    }

    int frame = find_free_frame();

    if (frame == -1) {
        int victim_page = select_victim_page();

        /* Quadro atualmente ocupado pela página vítima. */
        frame = page_table_get_frame(victim_page);

        /*
         * Salvaguarda defensiva: em operação normal sempre existe uma
         * vítima válida quando a memória está cheia. Caso contrário, há
         * uma inconsistência interna e abortamos em vez de corromper
         * frame_to_page com um índice inválido.
         */
        if (victim_page == -1 || frame < 0 || frame >= NUM_FRAMES) {
            fprintf(stderr, "Erro interno: nenhuma pagina vitima valida.\n");
            exit(1);
        }

        /* Invalida a página vítima na tabela de páginas e no TLB. */
        page_table_invalidate(victim_page);
        tlb_remove(victim_page);

        frame_to_page[frame] = -1;
    }

    /*
     * Lê a página (PAGE_SIZE bytes) do BACKING_STORE.bin tratando-o
     * como arquivo de acesso aleatório.
     */
    if (fseek(backing, (long) page * PAGE_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Erro: fseek falhou para a pagina %d\n", page);
        exit(1);
    }

    size_t read = fread(physical_memory[frame], sizeof(signed char),
                        PAGE_SIZE, backing);

    if (read != (size_t) PAGE_SIZE) {
        fprintf(stderr, "Erro: leitura incompleta da pagina %d\n", page);
        exit(1);
    }

    frame_to_page[frame] = page;
    page_table_update(page, frame);

    return frame;
}

int select_victim_page(void)
{
    /*
     * Seleciona, entre as páginas residentes (válidas), aquela com o
     * menor valor do contador de envelhecimento — a aproximação do
     * algoritmo LRU. Em caso de empate, escolhe a de menor número de
     * página (critério consistente).
     */
    int victim_page = -1;
    int min_counter = 256; /* maior que qualquer contador de 8 bits */

    for (int page = 0; page < PAGE_TABLE_SIZE; page++) {
        if (!page_table_is_valid(page)) {
            continue;
        }

        unsigned char counter = page_table_get_aging_counter(page);

        if (counter < min_counter) {
            min_counter = counter;
            victim_page = page;
        }
    }

    return victim_page;
}

signed char read_memory(int frame, int offset)
{
    /*
     * Retorna o byte (com sinal) armazenado no endereço físico,
     * ou seja, physical_memory[frame][offset].
     */
    if (frame < 0 || frame >= NUM_FRAMES ||
        offset < 0 || offset >= FRAME_SIZE) {
        return 0;
    }

    return physical_memory[frame][offset];
}

int get_page_loaded_in_frame(int frame)
{
    if (frame < 0 || frame >= NUM_FRAMES) {
        return -1;
    }

    return frame_to_page[frame];
}
