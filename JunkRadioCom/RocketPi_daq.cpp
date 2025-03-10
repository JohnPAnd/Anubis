#include <algorithm>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <wiringPi.h>

#include "daqhats_utils.h"

#include "Functions.hpp"

#define PRINT_DATA false
#define PRINTTHIS false

#define BANGBANG_PULSED true
#define BANGBANG_INDEX 1
#define BANGBANG_PIN 2

#define PULSE_TIME 32000
#define STOP_TIME 20000


struct DAC_Control
{
	// "<?xxxi"
	bool BangBang_Enable = false; // false
	int32_t BangBang_Set = -20; // psi -20
};

struct DAC_Data
{
	// "<Iffffff"
	uint32_t avgCount;
	float data[6];
};


#define MEMKEY_GL 2222
#define MEMSIZE_GL sizeof(DAC_Control)

#define MEMKEY_DAC 1111
#define MEMSIZE_DAC sizeof(DAC_Data)

int main(void)
{
	// DAC Init
	int result = RESULT_SUCCESS;
	uint8_t address = 0;
	char c;
	char display_header[512];
	int i;
	char channel_string[512];
	char options_str[512];
	char mode_string[32];
	char range_string[32];

	float slopes[6] = {
		1203.2, 223.58, 235.6, 1, 1, 1
		//1,1,1,1,1,1
	};
	float offsets[6] = {
		-622.71+13.5, -133.67+22, -131.29+13, 0, 0, 0
		//0,0,0,0,0,0
	};

	wiringPiSetupGpio();
	pinMode(BANGBANG_PIN, OUTPUT);
	digitalWrite(BANGBANG_PIN, LOW);



	// Set the channel mask which is used by the library function
	// mcc128_a_in_scan_start to specify the channels to acquire.
	// The functions below, will parse the channel mask into a
	// character string for display purposes.
	uint8_t channel_mask = {CHAN0 | CHAN1 | CHAN2 | CHAN3 | CHAN4 | CHAN5};
	uint8_t input_mode = A_IN_MODE_SE;
	uint8_t input_range = A_IN_RANGE_BIP_10V;

	uint32_t samples_per_channel = 0;

	int max_channel_array_length = mcc128_info()->NUM_AI_CHANNELS[input_mode];
	int channel_array[max_channel_array_length];
	uint8_t num_channels = convert_chan_mask_to_array(channel_mask, channel_array);

	uint32_t internal_buffer_size_samples = 0;
	uint32_t user_buffer_size = 1000 * num_channels;
	double read_buf[user_buffer_size];
	int total_samples_read = 0;

	int32_t read_request_size = READ_ALL_AVAILABLE;

	// When doing a continuous scan, the timeout value will be ignored in the
	// call to mcc128_a_in_scan_read because we will be requesting that all
	// available samples (up to the default buffer size) be returned.
	double timeout = 5.0;

	double scan_rate = 1000.0;
	double actual_scan_rate = 0.0;
	mcc128_a_in_scan_actual_rate(num_channels, scan_rate, &actual_scan_rate);

	uint32_t options = OPTS_CONTINUOUS;

	uint16_t read_status = 0;
	uint32_t samples_read_per_channel = 0;


	
	// Shared Memory Init
	printf("\nSharedMem Size: %i, %i", MEMSIZE_DAC, MEMSIZE_GL);
	//printf("\nSharedMem Size: %i, %i", sizeof(DAC_Data), sizeof(DAC_Control));

	int shmid_DAC = shmget(MEMKEY_DAC, MEMSIZE_DAC, 0666 | IPC_CREAT);
	DAC_Data* sharedMem_DAC = (DAC_Data*)shmat(shmid_DAC, (void*)0, 0);
	memset(sharedMem_DAC,0,MEMSIZE_DAC);

	int shmid_GL = shmget(MEMKEY_GL, MEMSIZE_GL, 0666 | IPC_CREAT);
	DAC_Control* sharedMem_GL = (DAC_Control*)shmat(shmid_GL, (void*)0, 0);
	memset(sharedMem_GL,0,MEMSIZE_GL);

	{
		DAC_Control set = DAC_Control();
		//set.BangBang_Enable = true;
		//set.BangBang_Set = 200;
		*sharedMem_GL = set;
	}



	// DAC Start
	// Select an MCC128 HAT device to use.
	if (select_hat_device(HAT_ID_MCC_128, &address))
	{
		// Error getting device.
		return -1;
	}

	printf ("\nSelected MCC 128 device at address %d\n", address);

	// Open a connection to the device.
	result = mcc128_open(address);
	STOP_ON_ERROR(result);

	result = mcc128_a_in_mode_write(address, input_mode);
	STOP_ON_ERROR(result);

	result = mcc128_a_in_range_write(address, input_range);
	STOP_ON_ERROR(result);

	convert_options_to_string(options, options_str);
	convert_chan_mask_to_string(channel_mask, channel_string);
	convert_input_mode_to_string(input_mode, mode_string);
	convert_input_range_to_string(input_range, range_string);

	printf("\nRocketPi DAC\n");
	printf("    Input mode: %s\n", mode_string);
	printf("    Input range: %s\n", range_string);
	printf("    Channels: %s\n", channel_string);
	printf("    Requested scan rate: %-10.2f\n", scan_rate);
	printf("    Actual scan rate: %-10.2f\n", actual_scan_rate);
	printf("    Options: %s\n", options_str);

	
	//printf("\nPress ENTER to continue ...\n");
	//scanf("%c", &c);

	// Configure and start the scan.
	// Since the continuous option is being used, the samples_per_channel
	// parameter is ignored if the value is less than the default internal
	// buffer size (10000 * num_channels in this case). If a larger internal
	// buffer size is desired, set the value of this parameter accordingly.
	result = mcc128_a_in_scan_start(address, channel_mask, samples_per_channel,
		scan_rate, options);
	STOP_ON_ERROR(result);

	STOP_ON_ERROR(mcc128_a_in_scan_buffer_size(address,
		&internal_buffer_size_samples));
	printf("Internal data buffer size:  %d\n", internal_buffer_size_samples);


	//printf("\nStarting scan ... Press ENTER to stop\n\n");

	if (PRINT_DATA) {
		// Create the header containing the column names.
		strcpy(display_header, "Samples Read    Scan Count    Avg Count    ");
		for (i = 0; i < num_channels; i++)
		{
			sprintf(channel_string, "Channel %d   ", channel_array[i]);
			strcat(display_header, channel_string);
		}
		strcat(display_header, "\n");
		printf("%s", display_header);
	}



	// DAC Loop
	DAC_Data data;
	do
	{
		// Since the read_request_size is set to -1 (READ_ALL_AVAILABLE), this
		// function returns immediately with whatever samples are available (up
		// to user_buffer_size) and the timeout parameter is ignored.
		result = mcc128_a_in_scan_read(address, &read_status, read_request_size,
			timeout, read_buf, user_buffer_size, &samples_read_per_channel);
		STOP_ON_ERROR(result);
		if (read_status & STATUS_HW_OVERRUN)
		{
			printf("\n\nHardware overrun\n");
			break;
		}
		else if (read_status & STATUS_BUFFER_OVERRUN)
		{
			printf("\n\nBuffer overrun\n");
			break;
		}

		total_samples_read += samples_read_per_channel;

		if (samples_read_per_channel > 0)
		{
			// Data Acquisition
			//int index = samples_read_per_channel * num_channels - num_channels;
			double avgs[6] = {0};
			int avgCount = meanMulti(avgs, read_buf, samples_read_per_channel, num_channels);

			for (uint8_t i = 0; i < 6; i++)
			{
				avgs[i] = avgs[i] * slopes[i] + offsets[i];
			}
			

			data.avgCount = avgCount;

			std::copy(avgs, avgs+6, data.data);
			if (PRINT_DATA) {
				printf("\r%12.0d    %10.0d ", samples_read_per_channel, total_samples_read);
				printf("%12i ", avgCount);
				for (i = 0; i < num_channels; i++) {
					//printf("%10.5f V", read_buf[index + i]);
					printf("%10.5f V", avgs[i]);
				}
				fflush(stdout);
			}
			
			*sharedMem_DAC = data;


			// BangBang Control
			if (sharedMem_GL->BangBang_Enable) {
				if (PRINTTHIS) printf("\r%10.5f", avgs[BANGBANG_INDEX]);
				if (avgs[BANGBANG_INDEX] < sharedMem_GL->BangBang_Set) {
					if (PRINTTHIS) printf("\ttrue");
					digitalWrite(BANGBANG_PIN, HIGH);
					
					if (BANGBANG_PULSED) {
						usleep(PULSE_TIME);
						digitalWrite(BANGBANG_PIN, LOW);
						usleep(STOP_TIME);
					}
				}
				else {
					if (PRINTTHIS) printf("\tfalse");
					digitalWrite(BANGBANG_PIN, LOW);
				}
				//printf("\t%s %i", sharedMem_GL->BangBang_Enable ? "true" : "false", sharedMem_GL->BangBang_Set);
				if (PRINTTHIS) fflush(stdout);
			}
			else digitalWrite(BANGBANG_PIN, LOW);
		}

		//usleep(1000);
	}
	while ((result == RESULT_SUCCESS) &&
		   ((read_status & STATUS_RUNNING) == STATUS_RUNNING) );// &&
		   //!enter_press());

	printf("\n");


stop:
	print_error(mcc128_a_in_scan_stop(address));
	print_error(mcc128_a_in_scan_cleanup(address));
	print_error(mcc128_close(address));

	//memset(sharedMem_DAC,0,MEMSIZE_DAC);
	//memset(sharedMem_GL,0,MEMSIZE_GL);

	shmdt(sharedMem_DAC);
	shmdt(sharedMem_GL);

	shmctl(shmid_DAC, IPC_RMID, NULL);
	shmctl(shmid_GL, IPC_RMID, NULL);

	digitalWrite(BANGBANG_PIN, LOW);

	return 0;
}
