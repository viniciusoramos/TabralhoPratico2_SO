#include "tlb.h"
#include "config.h"

static tlb_entry_t tlb[TLB_SIZE];

/*
 * Índice da próxima posição a ser substituída.
 * Essa variável implementa FIFO no TLB.
 */
static int fifo_next = 0;

void tlb_init(void)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page = -1;
        tlb[i].frame = -1;
        tlb[i].valid = 0;
    }

    fifo_next = 0;
}

int tlb_lookup(int page)
{
    /*
     * Procura a página em todas as entradas válidas do TLB.
     * Em caso de acerto (TLB hit), retorna o número do quadro.
     * Caso contrário, retorna -1 (TLB miss).
     */
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page == page) {
            return tlb[i].frame;
        }
    }

    return -1;
}

void tlb_insert(int page, int frame)
{
    /*
     * Política de inserção:
     * 1. Se a página já estiver no TLB, apenas atualiza o quadro
     *    (mantém a posição original, preservando a ordem FIFO).
     * 2. Se existir uma entrada inválida, reutiliza-a.
     * 3. Se o TLB estiver cheio, substitui a entrada mais antiga
     *    apontada por fifo_next (política FIFO).
     */

    /* 1. Página já presente: atualiza apenas o quadro. */
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page == page) {
            tlb[i].frame = frame;
            return;
        }
    }

    /* 2. Procura a primeira entrada inválida disponível. */
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            tlb[i].page = page;
            tlb[i].frame = frame;
            tlb[i].valid = 1;
            return;
        }
    }

    /* 3. TLB cheio: substituição FIFO. */
    tlb[fifo_next].page = page;
    tlb[fifo_next].frame = frame;
    tlb[fifo_next].valid = 1;
    fifo_next = (fifo_next + 1) % TLB_SIZE;
}

void tlb_remove(int page)
{
    /*
     * Invalida a entrada associada à página informada (se existir).
     * Usada quando uma página é removida da memória física para que
     * o TLB não retorne um mapeamento obsoleto.
     */
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page == page) {
            tlb[i].page = -1;
            tlb[i].frame = -1;
            tlb[i].valid = 0;
            return;
        }
    }
}

void tlb_clear(void)
{
    tlb_init();
}
