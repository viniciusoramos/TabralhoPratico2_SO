#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "tlb.h"
#include "page_table.h"
#include "memory.h"
#include "statistics.h"

int main(void)
{
    FILE *backing = fopen(BACKING_STORE_PATH, "rb");

    if (backing == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir %s\n", BACKING_STORE_PATH);
        fprintf(stderr, "Execute antes: cd data && python3 generate_data.py\n");
        return 1;
    }

    page_table_init();
    tlb_init();
    memory_init(backing);
    statistics_init();

    int logical_address;

    while (scanf("%d", &logical_address) == 1) {
        count_address();

        /*
         * O arquivo de entrada possui inteiros de 32 bits.
         * Apenas os 16 bits menos significativos devem ser considerados,
         * portanto mascaramos o endereço com 0xFFFF.
         *
         * Estrutura do endereço lógico (16 bits):
         *   bits 15..8 -> número da página  (PAGE_SIZE = 256 => 8 bits)
         *   bits  7..0 -> deslocamento      (offset dentro da página)
         */
        unsigned int masked = ((unsigned int) logical_address) & 0xFFFF;
        int page = (masked >> 8) & 0xFF;
        int offset = masked & 0xFF;

        int frame = tlb_lookup(page);

        if (frame != -1) {
            count_tlb_hit();
        } else {
            frame = page_table_lookup(page);

            if (frame == -1) {
                count_page_fault();
                frame = handle_page_fault(page);
            }

            tlb_insert(page, frame);
        }

        /*
         * Atualização do LRU aproximado a cada acesso à memória:
         * marca a página acessada como referenciada e, em seguida,
         * envelhece (aging) os contadores de todas as páginas válidas.
         */
        page_table_set_reference(page);
        page_table_update_aging();
        
        /*
         * O endereço físico é formado pela concatenação do número do
         * quadro com o deslocamento:
         *   physical_address = frame * FRAME_SIZE + offset
         */
        int physical_address = frame * FRAME_SIZE + offset;
        signed char value = read_memory(frame, offset);

        printf("Logical address: %d Physical address: %d Value: %d\n",
               logical_address,
               physical_address,
               value);
    }

    print_statistics();

    fclose(backing);

    return 0;
}
