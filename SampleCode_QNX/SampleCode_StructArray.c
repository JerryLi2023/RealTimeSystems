/*
 * SampleCode_StructArray.c
 * Source: Lab1_Task6A.c (user-supplied lab example).
 * Purpose: Pass an array of structures into functions (ordinary C).
 * Adaptation: Kept card/GiveValue/PrintScan; condensed printing and checked array bounds.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <errno.h>

typedef struct card {
    char suit[10];
    int value;
} card;

/* val must point to at least 13 cards, as in the original lab. */
void GiveValue(card *val, const char *suits)
{
    int i;
    for (i = 0; i < 13; ++i) {
        val[i].value = i + 1;
        snprintf(val[i].suit, sizeof(val[i].suit), "%s", suits);
    }
}

/* read is a 1-based card number, not a zero-based array index. */
int PrintScan(const card *val, int read)
{
    static const char *names[] = {"Ace", "Two", "Three", "Four", "Five",
        "Six", "Seven", "Eight", "Nine", "Ten", "Jack", "Queen", "King"};
    if (read < 1 || read > 52 || val[read-1].value < 1 || val[read-1].value > 13) {
        errno = EINVAL;
        return -1;
    }
    printf("suit: %-8s value: %s\n", val[read-1].suit, names[val[read-1].value-1]);
    return 0;
}
#ifdef SAMPLECODE_DEMO
int main(void)
{
    card deck[52];
    int read, i;
    GiveValue(&deck[0], "Hearts");
    GiveValue(&deck[13], "Diamonds");
    GiveValue(&deck[26], "Clubs");
    GiveValue(&deck[39], "Spades");
    printf("Print every nth card (1..52): ");
    if (scanf("%d", &read) != 1 || read < 1 || read > 52) return 1;
    for (i = read; i <= 52; i += read) PrintScan(deck, i);
    return 0;
}
#endif
