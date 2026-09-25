	
    


enum states {State0, State1, State2, State3, State4, State5, State6, State7};
enum states CurState;
    
void SingleStep_TrafficLight_SM(void *state_ptr, void *inputs) {

	enum states *CurrentState = (enum states*) state_ptr;

	char *message = (char *)inputs;

	switch (*CurrentState) {
		case 0:
			// do some work;
			*CurrentState = State1;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 1:
			// do some work;
			*CurrentState = State2;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 2:
			// do some work;
			printf ("East and West traffic light: Green | North and South traffic light: Red\n");
			if (*message == 'n') {
				*CurrentState = State3;
			} else {
				*CurrentState = State2;
			}
			sleep(2);
			break;
		case 3:
			// do some work;
			*CurrentState = State4;
			printf ("East and West traffic light: Yellow | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 4:
			// do some work;
			*CurrentState = State5;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
		case 5:
			// do some work;
			printf ("East and West traffic light: Red | North and South traffic light: Green\n");
			if (*message == 'e') {
				*CurrentState = State6;
			} else {
				*CurrentState = State5;
			}
			sleep(2);
			break;
		case 6:
			// do some work;
			*CurrentState = State7;
			printf ("East and West traffic light: Red | North and South traffic light: Yellow\n");
			sleep(1);
			break;
		case 7:
			// do some work;
			*CurrentState = State0;
			printf ("East and West traffic light: Red | North and South traffic light: Red\n");
			sleep(1);
			break;
	}
}