#include "Functions.hpp"

// Function to calculate average of
// an array using efficient method
double mean(double arr[], int N)
{
	// Store the average of the array
	double avg = 0;
 
	// Traverse the array arr[]
	for (int i = 0; i < N; i++) {
		// Update avg
		avg += (arr[i] - avg) / (i + 1);
	}
 
	// Return avg
	return avg;
}


int meanMulti(double* avgs, double arr[], int samples_read_per_channel, int num_channels)
{
	int avgCount = 0;

	// Traverse the array arr[]
	int index = 0;
	int j = 0;
	for (int i = 0; i < samples_read_per_channel; i++) {
		index = i * num_channels;
		for (j = 0; j < num_channels; j++) {
			// Update avg
			avgs[j] += (arr[index + j] - avgs[j]) / (i + 1);
		}
		avgCount++;
	}

	return avgCount;
}