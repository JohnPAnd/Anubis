#include "/home/anubis/daqhats/examples/c/daqhats_utils.h"

class PressureTrans {
    public:
        uint8_t address;
        uint8_t channel_mask;
        uint32_t samples_per_channel;
        int32_t read_request_size;
        double scan_rate;
        uint32_t options = OPTS_DEFAULT;
        double timeout = 5.0;

        PressureTrans(uint8_t address, uint8_t channel_mask, uint32_t samples_per_channel, int32_t read_request_size, double scan_rate){
            //Defualt in examples
            //uint8_t address = 0 ;
            //uint8_t channel_mask = {CHAN0};
            //uint32_t samples_per_channel = 10000;
            //int32_t read_request_size = 500;
            //double timeout = 5.0;
            //double scan_rate = 1000.0;
            
            this->address = address;
            this->channel_mask = channel_mask;
            this->samples_per_channel = samples_per_channel;
            this->read_request_size = read_request_size;
            this->scan_rate = scan_rate;
        }

        int openPressTrans(){
            int result = RESULT_SUCCESS;
            // uint8_t address = 0;
        
            // Set the channel mask which is used by the library function
            // mcc118_a_in_scan_start to specify the channels to acquire.
            // The functions below, will parse the channel mask into a
            // character string for display purposes.
            // uint8_t channel_mask = {CHAN0};
        
            // uint32_t samples_per_channel = 10000;
            // int32_t read_request_size = 500;
            // double timeout = 5.0;
            // double scan_rate = 1000.0;
            // uint32_t options = OPTS_DEFAULT;
        
        
            // Select an MCC118 HAT device to use.
            if (select_hat_device(HAT_ID_MCC_118, &address))
            {
                // Error getting device.
                return 1;
            }

            // Open a connection to the device.
            result = mcc118_open(address);
            //STOP_ON_ERROR(result)

            return 0;
        }

        int readPressTrans(double* output){
            int result = RESULT_SUCCESS;
            //STOP_ON_ERROR(result);

            // uint8_t address = 0;
            double summation = 0.0;
            int i = 0;
            //uint8_t channel_mask = {CHAN0};
            //uint32_t samples_per_channel = 10000;
            //double scan_rate = 1000.0;
            //uint32_t options = OPTS_DEFAULT;
            uint16_t read_status = 0;
            uint32_t samples_read_per_channel = 0;
            //int32_t read_request_size = 500;
            //double timeout = 5.0;

            int max_channel_array_length = mcc118_info()->NUM_AI_CHANNELS;
            int channel_array[max_channel_array_length];
            uint8_t num_channels = convert_chan_mask_to_array(channel_mask,
                channel_array);

            uint32_t buffer_size = samples_per_channel * num_channels;
            double read_buf[buffer_size];
            int total_samples_read = 0;
        
            // Configure and start the scan.
            result = mcc118_a_in_scan_start(address, channel_mask, samples_per_channel,
                scan_rate, options);
            //STOP_ON_ERROR(result);
        
            // Read the specified number of samples.
            result = mcc118_a_in_scan_read(address, &read_status, read_request_size,
                timeout, read_buf, buffer_size, &samples_read_per_channel);
            //STOP_ON_ERROR(result);
            if (read_status & STATUS_HW_OVERRUN)
            {
                printf("\n\nHardware overrun\n");
            }
            else if (read_status & STATUS_BUFFER_OVERRUN)
            {
                printf("\n\nBuffer overrun\n");
            }
    
            //total_samples_read += samples_read_per_channel;
            
            for(i = 0; i < samples_read_per_channel; i++){
                summation += read_buf[i];
            }
            total_samples_read += samples_read_per_channel;

            double average = summation/((double)read_request_size);
            
            *output = average;

            mcc118_a_in_scan_stop(address);
            mcc118_a_in_scan_cleanup(address);

            return 0;
        }

        int closePressTrans(){
            mcc118_a_in_scan_stop(address);
            mcc118_a_in_scan_cleanup(address);
            mcc118_close(address);

            return 0;
        }


};