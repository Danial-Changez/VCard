#include <stdio.h>
#include <stdlib.h>
#include "../include/VCParser.h"
/*
 * A simple driver program to test createCard and writeCard.
 * Usage:
 *   ./testDriver inputFile.vcf outputFile.vcf
 */

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s inputFile.vcf outputFile.vcf\n", argv[0]);
        return 1;
    }

    Card *myCard = NULL;

    // 1. Parse the input file
    VCardErrorCode err = createCard(argv[1], &myCard);
    if (err != OK)
    {
        fprintf(stderr, "createCard returned error: %s\n", errorToString(err));
        return 1;
    }

    // 2. Write the card to the output file
    err = writeCard(argv[2], myCard);
    if (err != OK)
    {
        fprintf(stderr, "writeCard returned error: %s\n", errorToString(err));
        deleteCard(myCard);
        return 1;
    }

    // 3. Optionally, print the card for debugging
    char *debugStr = cardToString(myCard);
    printf("Parsed Card:\n%s\n", debugStr);
    free(debugStr);

    // 4. Cleanup
    deleteCard(myCard);
    printf("Successfully created and wrote card.\n");
    return 0;
}
