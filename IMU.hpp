//For USB + IMU Setup
#include <stdio.h>
#include <stdint.h>
#include <ostream>
#include <iostream>
#include <cstring>   	

// Linux headers for Usb Setup
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()


//Declares a function that turns the binary of an intergar to float
float bintofloat(unsigned int x) {
    float *f = (float *)&x;
    return *f;
}


class IMU{
    public:
        const char* address;
        speed_t baudrate; //A class used in terminos.h (Define as B+baudrate ex.B115200)
        int serialport;

    IMU(const char* address, speed_t baudrate){
        this->address    = address;
        this->baudrate   = baudrate;
    }

    int getSerialPort(){
        serialport = open(address, O_RDWR);
        
        if (serialport < 0) {
            printf("Error %i from open: %s\n", errno, strerror(errno));
            return 1;    
        }

        return 0;
    }

    int closeSerialPort(){
        if( close(serialport) == -1){
            if (errno == EBADF){
                printf("Serial port is closed\n");
            } else {
                printf("Error %i closing port: %s\n", errno, strerror(errno));
                return 1;
            }

            return 0;
        }
    }

    int intializePort(){
        struct termios tty;
        if(tcgetattr(serialport, &tty) != 0) {
            printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
            return 1; //can't get attribute
        }

        tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
        tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
        tty.c_cflag &= ~CSIZE; // Clear all the size bits, then use one of the statements below
        tty.c_cflag |= CS8; // 8 bits per byte (most common)
        tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
        tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS hardware flow control
        tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO; // Disable echo
        tty.c_lflag &= ~ECHOE; // Disable erasure
        tty.c_lflag &= ~ECHONL; // Disable new-line echo
        tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
        tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
        tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
        tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
        tty.c_cc[VMIN] = 0;

	    cfsetspeed(&tty, baudrate); //IN & OUT Baudrate. Our IMU defualt is 115200

        if (tcsetattr(serialport, TCSANOW, &tty) != 0) {
            printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
            return 2; //can't set attribute
        }

        return 0;
    }

    int checkContinuity(){
        unsigned char msg1[] = {247, 223, 223}; //Get firmware version
        write(serialport, msg1, sizeof(msg1));  //Sends command to IMU

        unsigned char read_buf [13];            //We expect 16 bytes of data back
        memset(&read_buf, '\0', sizeof(read_buf));

        int n = read(serialport, &read_buf, sizeof(read_buf));
        if (n < 0) {
            printf("Error reading: %s", strerror(errno));
            return 1;
        }
        if (n > 13) {
            printf("Too much bytes read");
            return 2;
        }

        return 0;
    }

    int getQuaternion(float* quaternion){
        unsigned char msg1[] = {247, 0, 0};     //Get orientation in quanterion (x,y,z,w)
        write(serialport, msg1, sizeof(msg1));  //Sends command to IMU

        unsigned char read_buf [17];            //We expect 16 bytes of data back
        memset(&read_buf, '\0', sizeof(read_buf));

        int n = read(serialport, &read_buf, sizeof(read_buf));
        if (n < 0) {
            printf("Error reading: %s", strerror(errno));
            return 1;
        }
        if (n > 16) {
            printf("Too much bytes read");
            return 2;
        }

        unsigned int first = 0x00000000;
        first += (read_buf[0] << 24) + (read_buf[1] << 16) + (read_buf[2] << 8) + (read_buf[3] << 0);
        float quat1 = bintofloat(first);

        unsigned int second = 0x00000000;
        second += (read_buf[4] << 24) + (read_buf[5] << 16) + (read_buf[6] << 8) + (read_buf[7] << 0);
        float quat2 = bintofloat(second);

        unsigned int third = 0x00000000;
        third += (read_buf[8] << 24) + (read_buf[9] << 16) + (read_buf[10] << 8) + (read_buf[11] << 0);
        float quat3 = bintofloat(third);

        unsigned int fourth = 0x00000000;
        fourth += (read_buf[12] << 24) + (read_buf[13] << 16) + (read_buf[14] << 8) + (read_buf[15] << 0);
        float quat4 = bintofloat(fourth);

        //printf("Orientation:\n %32.28f\n %32.28f\n %32.28f\n %32.28f\n", quat1, quat2, quat3, quat4);

        quaternion[0] = quat1;
        quaternion[1] = quat2;
        quaternion[2] = quat3;
        quaternion[3] = quat4;

        return 0;
    }

};