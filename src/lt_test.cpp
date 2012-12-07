#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lt_common.h"
#include "lt_decoder.h"
#include "lt_encoder.h"
#include "logger.h"

int main () {
    glob_log.set_level(5);
    FILE *f = fopen("test/med", "rb");
    fseek(f, 0, SEEK_END);
    long pos = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *bytes = (unsigned char *)malloc(pos);
    fread(bytes, pos, 1, f);
    fclose(f);

    int seed = 42;
    int total_blocks = 25;
    int num_blocks;
    int *selected_blocks;

    printf("Basic block select test\n\n");

    lt_selector *ltsA = new lt_selector(seed, total_blocks);
    printf("Generating block lists in order, usng ltsA\n");
    for (int i = 1; i < 10; i++) {
        num_blocks = ltsA->select_blocks(i, &selected_blocks);
    }

    printf("\n================================\n\n");

    lt_selector *ltsB = new lt_selector(seed, total_blocks);
    printf("Generating block lists in reverse order, usng ltsB\n");
    for (int i = 9; i > 0; i--) {
        num_blocks = ltsB->select_blocks(i, &selected_blocks);
    }

    /*
        printf("Chunk %d: %d blocks: ", i, num_blocks);
        for (int b = 0; b < num_blocks; b++) {
            printf("%d ", selected_blocks[b]);
        }
        */

    printf("\n================================\n");
    printf("Generate/reverse test\n");
    printf("================================\n\n");

    lt_encoder *enc = new lt_encoder();
    enc->init(bytes, pos, 256, 10);
    lt_descriptor *msg_desc = enc->get_desc();
    printf("Initted lt encoder!\n");
    printf("total_chunks %zu, num_peers %zu, chunk_len %zu, seed %d\n",
            msg_desc->total_chunks, msg_desc->num_peers,
            msg_desc->chunk_len, msg_desc->seed);

    printf("Generating initial block lists in the encoder\n");

    unsigned char *dest;
    unsigned int chunk_id;
    for (int i = 0; i < 11; i++) {
        enc->generate_chunk(&dest, &chunk_id);
        if (dest == NULL) {
            enc->next_stream();
            i--;
        }
    }

    printf("\n================================\n\n");

    printf("Building decoder, reversing rng sequence\n");
    lt_decoder *dec = new lt_decoder();
    dec->add_chunk((unsigned char *)msg_desc, sizeof(lt_descriptor), 0);

    lt_decoder::Chunk chunk;
    for (int i = 1; i < 11; i++) {
        chunk.id = i;
        dec->build_block_list(&chunk);

        printf("Chunk %d: %lu blocks ", i, chunk.block_list.size());
        for (unsigned int b = 0; b < chunk.block_list.size(); b++) {
            printf("%d ", chunk.block_list[b]);
        }
        printf("\n");
    }
}
