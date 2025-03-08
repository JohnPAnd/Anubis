// #include <AquireData.hpp>
// #include <ControlSystem.hpp>
// #include <HardwareSetup.hpp>
// #include <Logging.hpp>


//New Libraries
#include "IMU.hpp"
#include "Manifold.hpp"
#include "PressureTransducer.hpp"

//Printing flags (set these to 1 to print the respective information to terminal)
#define errorFlag 0

//How much data aquasition loops
#define LOOPS 1000


//Base Information used in the software (physical constants and wiring)
#define relayPin1 3
#define relayPin2 5
#define relayPin3 7
#define relayPin4 8
#define relayPin5 10
#define relayPin6 12
#define relayPin7 11
#define relayPin8 13
#define relayPin9 15
#define relayPin10 16
#define IMUaddress "/dev/ttyACM0"
#define IMUbaudrate B115200


int main() {
    int error = 0;

    uint8_t address = 0;
    uint8_t channel_mask = {CHAN0};
    uint32_t samples_per_channel = 10;
    int32_t read_request_size = 10;
    double scan_rate = 1000.0;
    double avgOutput;
    float quaternion[4];

    PressureTrans tankPT(address, channel_mask, samples_per_channel, read_request_size, scan_rate);
    if((error = tankPT.openPressTrans()) && errorFlag){
        printf("Error in setup of Pressure Transducer");
    }


    IMU yostIMU(IMUaddress, (speed_t) IMUbaudrate);
    

    if(yostIMU.getSerialPort() && errorFlag){
        printf("Error %i from open: %s\n", errno, strerror(errno));
    }

    if((error = yostIMU.intializePort()) && errorFlag){
        if(error == 1){
            printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        }else{
            printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        }
    }

    if((error = yostIMU.checkContinuity()) && errorFlag){
        if(error == 1){
            printf("Error reading: %s", strerror(errno));
        }else{
            printf("Too much bytes read");
        }
    }

    int thrusterPin[] = {relayPin1,relayPin2,relayPin3,relayPin4,relayPin5,
        relayPin6,relayPin7,relayPin8,relayPin9,relayPin10};

    wiringPiSetupPhys();

    Manifold thrusters(thrusterPin);

    //correction binary representation:
		//empty empty x(on/off) x(direction) y(on/off) y(direction) z(on/off) z(direction)
		//1 = on; 0 = off
		//1 = positive direction; 0 = negative direction
    char correction = 0b00111000;
    thrusters.change_signal(correction);

    printf("\nQuaternion Orientation:\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tPressure Transducer:\n");
    for(int i = 0; i < LOOPS; i++){
        if((error = yostIMU.getQuaternion(quaternion)) && errorFlag){
            if(error == 1){
                printf("Error reading: %s", strerror(errno));
            }else{
                printf("Too much bytes read");
            }
        }
        
        if((error = tankPT.readPressTrans(&avgOutput)) && errorFlag){
            printf("Error in reading Pressure Transducer");
        }

        printf("%32.28f  %32.28f  %32.28f  %32.28f\t\t %10.5f V\n", quaternion[0], quaternion[1], quaternion[2], quaternion[3], avgOutput);
    }

    if((error = tankPT.closePressTrans()) && errorFlag){
        printf("Error in closing Pressure Transducer");
    }

    

    



    // //Sets up IMU communication
    // int serial_port;
    // serial_port = openUSB();
    // if(testUSB(serial_port) == 1){
    //     //__USB worked
    //     IMUConfig(serial_port); //Calls a function from HardwareSetup to set properties of IMU
    // }//whAT DO WE DO IF testUSB == 0;

    // //Sets up DAQ communication
    // OpenCom();
    // ComConfig();

    // //Set up Pressure Transducer
    // PresTranConfig();

    
    // //Start IMU data Streaming (Doesn't Read Data)
    // startStreamIMU(serial_port);

    // //Reads the instant of data
    // double pastOrientation[4]; //This array should be filled but idk what to put here. Might run a setup to collect data for this array only and loop current like in a do-while setup
    // double currentOrientation[4];
    // readIMU(serial_port, currentOrientation);

    // quatToEul(currentOrientation); //convert qaternion orietnation to euler (radian)

    // //Reads the pressure transducer
    // double force = readPressTrans(PTpin);

    // //Determines the speed you are traveling based on curent and past orientation
    // double velocity[3];
    // velocityDifference(pastOrientation, currentOrientation, velocity, 3);

    // //Runs the deadband based off speed, position, and thrust
    // int correction[3]; //Array of commands for firing the thrusters
    // deadband(currentOrientation, velocity, force, correction, 3);

    // //
    // //
    // //Run the thruster coorection here
    // //


    // //
    // //
    // //Run logging
    // //


    // //Shutdown Sequence
    // stopStreamIMU(serial_port);

    // CloseCom();

    
    return 0;
}