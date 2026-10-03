/*
 * SampleCode_TrafficStateMachine.c
 * Source: Lab4_Task2C.c (user-supplied lab example).
 * Purpose: Advance the original eight-state traffic-light machine.
 * Adaptation: Kept switch/transitions/sleeps; passed input by value instead of racing a global scanf thread.
 * Copy the includes, types and functions you need into your project.
 * Define SAMPLECODE_DEMO to enable the example main(); otherwise no main is built.
 */
#include <stdio.h>
#include <unistd.h>

enum states {State0, State1, State2, State3, State4, State5, State6, State7};

/* Performs ONE step; changes *CurrentState to the next state.
 * input is a snapshot ('e', 'n' or '\0'), not a pointer to shared data.
 * Keeps the lab's blocking sleeps: 2 seconds for green, 1 for other states.
 * Prints the current state's outputs before returning with the NEXT state. */
void SingleStep_TrafficLight_SM(enum states *CurrentState, char input)
{
    switch (*CurrentState) {
        case 0:
            *CurrentState = State1;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 1:
            *CurrentState = State2;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 2:
            printf ("East and West traffic light: Green | North and South traffic light: Red\n");
            if (input == 'n') {
                *CurrentState = State3;
            } else {
                *CurrentState = State2;
            }
            sleep(2);
            break;
        case 3:
            *CurrentState = State4;
            printf ("East and West traffic light: Yellow | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 4:
            *CurrentState = State5;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
        case 5:
            printf ("East and West traffic light: Red | North and South traffic light: Green\n");
            if (input == 'e') {
                *CurrentState = State6;
            } else {
                *CurrentState = State5;
            }
            sleep(2);
            break;
        case 6:
            *CurrentState = State7;
            printf ("East and West traffic light: Red | North and South traffic light: Yellow\n");
            sleep(1);
            break;
        case 7:
            *CurrentState = State0;
            printf ("East and West traffic light: Red | North and South traffic light: Red\n");
            sleep(1);
            break;
    }
}

#ifdef SAMPLECODE_DEMO
int main(void)
{
    enum states CurrentState = State0;
    int i;
    /* Scripted inputs keep the demo small; replace with a sensor snapshot. */
    for (i = 0; i < 12; ++i)
        SingleStep_TrafficLight_SM(&CurrentState, i < 6 ? 'n' : 'e');
    return 0;
}
#endif
