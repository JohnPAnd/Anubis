//For Relay Control
#include <wiringPi.h>
#include <stdio.h> //for printf 

//Sets up the relay board structure and commands
class Relays{
	protected:
	int pin;
	public:
	Relays(int pin){
		this->pin = pin;
		pinMode(pin, OUTPUT);
		digitalWrite(pin, LOW);
	}
	void off(){//this is flipped and so is relay3. change when hardware is proper.
		digitalWrite(pin, HIGH);
	}
	void on(){//this is flipped and so is relay3. change when hardware is proper.
		digitalWrite(pin, LOW);
	}
	// void toggle(bool change){
	// 	if(change){
	// 		on();
	// 	}
	// 	else{
	// 		off();
	// 	}
	// }
};

class Manifold{
	public:
	Relays* relay1;
	Relays* relay2;
	Relays* relay3;
	Relays* relay4;
	Relays* relay5;
	Relays* relay6;
	Relays* relay7;
	Relays* relay8;
	Relays* relay9;
	Relays* relay10;	
	
	Manifold(const int pin_array[10]){
		//Pin array must be x,x,x,x,y,y,y,y,z,z
		relay1 = new Relays(pin_array[0]);
		relay2 = new Relays(pin_array[1]);
		relay3 = new Relays(pin_array[2]);
		relay4 = new Relays(pin_array[3]);
		relay5 = new Relays(pin_array[4]);
		relay6 = new Relays(pin_array[5]);
		relay7 = new Relays(pin_array[6]);
		relay8 = new Relays(pin_array[7]);
		relay9 = new Relays(pin_array[8]);
		relay10 = new Relays(pin_array[9]);
		all_off();
	}

	void all_on() {
		relay1->on();
		relay2->on();
		relay3->off();
		relay4->on();
		relay5->on();
		relay6->on();
		relay7->on();
		relay8->on();
		relay9->on();
		relay10->on();
	}

	void all_off(){
		relay1->off();
		relay2->off();
		relay3->on();
		relay4->off();
		relay5->off();
		relay6->off();
		relay7->off();
		relay8->off();
		relay9->off();
		relay10->off();
	}

	void change_signal(char thrust){
		//thrust binary representation:
		//empty empty x(on/off) x(direction) y(on/off) y(direction) z(on/off) z(direction)
		//1 = on; 0 = off
		//1 = positive direction; 0 = negative direction

		if( (thrust & 0b00100000) > 0){		//If x thruster is on
			if( (thrust & 0b00010000) > 0){	//positive x thrust
				relay1->on();
				relay2->on();
				relay3->off();
				relay4->off();
                printf("x up\n");
			}else{							//negative x thrust
				relay1->off();
				relay2->off();
				relay3->on();
				relay4->on();
                printf("x down\n");
			}
		}else{
			relay1->off();
			relay2->off();
			relay3->off();
			relay4->off();
            printf("x off\n");
		}

		if( (thrust & 0b00001000) > 0){		//If y thruster is on
			if( (thrust & 0b00000100) > 0){	//positive y thrust
				relay5->on();
				relay6->on();
				relay7->off();
				relay8->off();
                printf("y up\n");
			}else{							//negative y thrust
				relay5->off();
				relay6->off();
				relay7->on();
				relay8->on();
                printf("y down\n");
			}
		}else{
			relay5->off();
			relay6->off();
			relay7->off();
			relay8->off();
            printf("y off\n");
		}

		if( (thrust & 0b0000010) > 0){		//If z thruster is on
			if( (thrust & 0b00000001) > 0){	//positive z thrust
				relay9->on();
				relay10->off();
                printf("z up\n");
			}else{							//negative z thrust
				relay9->off();
				relay10->on();
                printf("z down\n");
			}
		}else{
			relay9->off();
			relay10->off();
            printf("z off\n");
		}

	}
};


