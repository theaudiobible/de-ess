/*
 * de-ess.c - de-ess a PCM-coded audio file.
 * 
 * Copyright (C) 2008-2013 Theophilus (http://theaudiobible.org)
 *
 * http://theaudiobible.org/contact.php
 *
 */

#include "de-ess.h"

/***************************** Global Variables *****************************/
unsigned int delta1 = 80, delta2 = 200;	/* time in ms of ess burst ignore */
unsigned int samples_per_sec = 44100;	/* sampling rate of PCM file */
unsigned int fundamental = 2000;	/* must be > all voice fundamentals */
short noise_floor = 100;		/* absolute value of noise floor */
int verbose = FALSE;			/* verbosity during attenutation */

/********************************* Functions *********************************/

void print_decimal(char* filename) {
	FILE *pcm_file;
	unsigned int sample_no;
	short sample;

	pcm_file = fopen(filename, "r");
	if (pcm_file == NULL) {
		fprintf(stdout, "cannot open file named %s\n", filename);
		exit(4);
	}

	if (fseek(pcm_file, 44, SEEK_SET)) {
		fprintf(stderr, "Could not go to the 44th byte in %s\n", filename);
		exit(5);
	}

	sample_no = 0;
	while (fread(&sample, sizeof(sample), 1, pcm_file)) {
		printf("%d:\t%d\n", sample_no, sample);
		sample_no++;
	}

	fclose(pcm_file);
}




//  Optimized Binomial Theorem
unsigned long long sqroot( unsigned long long num ) {
	unsigned long long l2, u, v, u2, n;
	if( 2 > num ) return( num );
	u  = num;
	l2 = 0;
	while( u >>= 2 ) l2++;
	u  = 1L << l2;
	v  = u;
	u2 = u << l2;
	while( l2-- ) {
		v >>= 1;
		n   = (u + u + v) << l2;
		n  += u2;
		if( n <= num ) {
			u += v;
			u2 = n;
		}
	}
	return( u );
}




void impulse_response(char *eq_filename, double *H, double *h) {
	int k, n, alpha;
	double sum;
	char line[MAX_CHAR_PER_LINE + 1];
	FILE *eq_file;

	alpha = (N-1)/2;

	if (! (eq_file = fopen(eq_filename, "r"))) {
		fprintf(stderr, "Could not open config file.\n");
		exit;
	}

	for (k=0; k<=alpha; k++) {
		if (fgets(line, MAX_CHAR_PER_LINE, eq_file)) {
			H[k] = atof(line);
			//fprintf(stderr, "%f ", H[k]);
		} else {
			fprintf(stderr, "Not enough frequency samples in config file.\n\a");
		}
	}
	//fprintf(stderr, "\n\n");

	for (n=0; n<=alpha; n++) {
		sum = 0;
		for (k=1; k<=(N-1)/2; k++) {
			sum += 2*H[k]*cos(2*PI*k*(n-alpha)/N) + H[0];
		}
		h[n] = sum/N;
		//fprintf(stderr, "%f ", h[n]);
	}
	//fprintf(stderr, "\n\n");

	for (n=alpha; n<N; n++) {
		h[n] = h[N-n-1];
	}

	fclose(eq_file);
}




/* FIR Filter
 * Returns FALSE if overflow during convolution, otherwise returns TRUE */
int filter(short *x, short *y, int size, double *h) {
	double sum, divisor;
	int k, n, overflowed = FALSE;

	divisor = (double)0x7fff;

	for (n = 0; n < size; n++) {
		sum = 0;
		for (k = 0; k < N; k++) {
			if (k > n) break;
			sum += h[k] * ((double)x[n - k])/divisor;
		}
		/*if (fabs(sum) > 1.0) {
			fprintf(stderr, "Overflow! Convolution result = %f\n\a", sum);
			overflowed = TRUE;
			sum = copysign(1.0, sum);
		}*/
		y[n] = (short)(sum * divisor);
	}

	if (overflowed) {
		return FALSE;
	} else {
		return TRUE;
	}
}




/* Find the RMS magnitude of the entire PCM file */
unsigned int file_rms(FILE *pcm_file) {
	unsigned long long sum_of_squares;
	unsigned int n, abs_sum;
	short sample, sliding_abs_mean, abs_sample[16];
	unsigned char i, j;

	//printf("Calculating RMS magnitude of entire PCM file...\n");

	abs_sum = 0;
	sliding_abs_mean = 0;

	for (i = 0; i < 16; i++) {
		abs_sample[i] = 0;
	}

	i = 0; j = 1;

	sum_of_squares = 0;
	n = 0;

	while (fread(&sample, sizeof(sample), 1, pcm_file)) {

		abs_sample[i] = abs(sample);
		abs_sum += abs_sample[i] - abs_sample[j];
		sliding_abs_mean = abs_sum >> 4;
		i++; j++;
		i &= 0x0f;
		j &= 0x0f;

		if (sliding_abs_mean > noise_floor) {
			sum_of_squares += sample * sample;
			n++;
		}
	}

	return (unsigned int)sqroot(sum_of_squares/n);
}


/* ess_detect is used to detect the start and end sample indices of a burst of
 * "ess" sound.  ess_detect returns FALSE (0) if an EOF occurs before detecting
 * the start and end of the next "ess" in the file stream, otherwise it returns
 * TRUE.  sample_no will be incremented before sample is read, so, at the time
 * of calling ess_detect, sample_no typically points to the last sample read
 * the previous time it was called (on the very first call for a particular file
 * stream, it should be set to -1).  When ess_detect is called, pcm_file must
 * already be open and the file position indicator must be at the next sample
 * to be read.  ess_detect calculates the total RMS contained in the ess
 * burst.
 */

int ess_detect(FILE *pcm_file, int *sample_no, int *start_sample_no, int *end_sample_no, unsigned int *rms) {
	int samples_per_fundamental_half_cycle, qualifying_half_cycles;
	int samples_in_this_half_cycle, qualifying_samples;
	short prev_sample, sample, sliding_abs_mean, abs_sample[16];
	unsigned long long sum_of_squares;
	unsigned int n, abs_sum;
	unsigned char i, j;
	int detection;

	samples_per_fundamental_half_cycle = (samples_per_sec / fundamental) / 2;

	*start_sample_no = 0;
	*end_sample_no = 0;

	abs_sum = 0;
	sliding_abs_mean = 0;

	detection = FALSE;

	for (i = 0; i < 16; i++) {
		abs_sample[i] = 0;
	}

	i = 0; j = 1;

	sample = 0;
	prev_sample = 0;

clean_slate:
	qualifying_half_cycles = 0;
	qualifying_samples = 0;
	sum_of_squares = 0;
	n = 0;

new_half_cycle:
	samples_in_this_half_cycle = 0;

	do {
		(*sample_no)++;

		prev_sample = sample;

		if (feof(pcm_file)) {
			return FALSE;
		}

		fread(&sample, sizeof(sample), 1, pcm_file);

		abs_sample[i] = abs(sample);

		abs_sum += abs_sample[i] - abs_sample[j];
		sliding_abs_mean = abs_sum >> 4;

		i++; j++;
		i &= 0x0f;
		j &= 0x0f;

		// printf("i=%d j=%d sample=%d abs_sum=%d sliding_abs_mean=%d\n", i, j, sample, abs_sum, sliding_abs_mean);

		sum_of_squares += sample * sample;
		n++;

		samples_in_this_half_cycle++;

	} while ((sample & 0x8000) == (prev_sample & 0x8000));

	// below noise floor?
	if (sliding_abs_mean < noise_floor) {
		goto voiced;
	}

	// voiced?
	if (samples_in_this_half_cycle > samples_per_fundamental_half_cycle) {
voiced:
		if (detection == TRUE) {
			*rms = (unsigned int)sqroot(sum_of_squares/n);
			*end_sample_no = *sample_no - samples_in_this_half_cycle;
			return TRUE;
		} else {
			goto clean_slate;
		}
	} else {
		qualifying_half_cycles++;

		qualifying_samples += samples_in_this_half_cycle;

		if (detection == TRUE) {
			goto new_half_cycle;
		}

		// genuine fricative?
		if (!(qualifying_half_cycles > QUALIFYING_HALF_CYCLES_THRESHOLD)) {
			goto new_half_cycle;
		}

		detection = TRUE;
		*start_sample_no = *sample_no - qualifying_samples;
		goto new_half_cycle;
	}
}

/* ess_attenuate reads in_file, attenuates just the ess bursts by an amount
 * determined by their respective RMS levels, and writes
 * everything back to out_file.  It returns FALSE (0) if there is an un-expected
 * EOF while reading in_file (i.e. before all ess bursts have been processed).
 * otherwise it returns TRUE.
 */

int ess_attenuate(char *in_file, char *out_file, int *start_array, int *end_array, unsigned int *RMS_array, int total_ess_bursts) {
	FILE *infile, *outfile;
	int i, sample_no, ess_burst, start_sample_no, end_sample_no, attenuation;
	int num_samples, rw_count;
	unsigned int RMS;
	long long interim;
	int attenuate_by;
	short sample, *buffer, *out_buffer, *filter_init_buff;
	unsigned int threshold1, threshold2, filerms;
	size_t retval;
	double h1[N], H1[(N-1)/2];		/* impulse and freq response for delta1 */
	double h2[N], H2[(N-1)/2];		/* impulse and freq response for delta2 */
	double *h;

	delta1 = (samples_per_sec * delta1) / 1000;
	delta2 = (samples_per_sec * delta2) / 1000;

	/* define h[n] from the freq profile in the config file */
	impulse_response(EQ1_FILE, H1, h1);
	impulse_response(EQ2_FILE, H2, h2);

	infile = fopen(in_file, "r");
	if (infile == NULL) {
		fprintf(stdout, "cannot open file named %s\n", infile);
		exit(6);
	}

	outfile = fopen(out_file, "w");
	if (outfile == NULL) {
		fprintf(stdout, "cannot open file named %s\n", outfile);
		exit(7);
	}

	filerms = file_rms(infile);

	threshold1 = filerms/8;
	threshold2 = filerms/8;
	//printf("threshold1 = %d\tthreshold2 = %d\n", threshold1, threshold2);

    rewind(infile);

	buffer = malloc(44);
	fread(buffer, 1, 44, infile);
	fwrite(buffer, 1, 44, outfile);
	free(buffer);

	filter_init_buff = calloc(N, sizeof(sample));

	sample_no = 0;
	ess_burst = 0;

	do {
		start_sample_no = start_array[ess_burst];
		end_sample_no = end_array[ess_burst];
		RMS = RMS_array[ess_burst];

		if (sample_no >= start_sample_no) {
			num_samples = end_sample_no - start_sample_no;
		} else {
			num_samples = start_sample_no - sample_no;
			goto no_atten;
		}

		ess_burst++;

		if (num_samples > delta1) {
			if (num_samples > delta2) {
				if (RMS > threshold2) {
					h = h2;
					attenuation = TRUE;
				} else {
					goto no_atten;
				}
			} else {
				if (RMS > threshold1) {
					h = h1;
					attenuation = TRUE;
				} else {
					goto no_atten;
				}
			}
		} else {
no_atten:
			attenuation = FALSE;
			attenuate_by = 0;
		}

		buffer = malloc((N + num_samples) * sizeof(sample));
		out_buffer = malloc((N + num_samples) * sizeof(sample));

		/* copy filter_init_buff into beginning of buffer */
		for (i = 0; i < N; i++) {
			*(buffer + i) = *(filter_init_buff + i);
		}

		/* read samples from file into buffer */
		rw_count = fread(buffer + N, sizeof(sample), num_samples, infile);
		if (rw_count != num_samples) {
			fwrite(buffer + N, sizeof(sample), rw_count, outfile);
			free(buffer);
			free(out_buffer);
			free(filter_init_buff);
			fclose(infile);
			fclose(outfile);
			return FALSE;
		}

		/* copy N samples from end of buffer into filter_init_buff */
		for (i = 0; i < N; i++) {
			*(filter_init_buff + i) = *(buffer + num_samples + i);
		}

		if (attenuation) {
			filter(buffer, out_buffer, num_samples + N, h);
			rw_count = fwrite(out_buffer + N, sizeof(sample), num_samples,
							outfile);
		} else {
			rw_count = fwrite(buffer + N, sizeof(sample), num_samples, outfile);
		}

		if (rw_count != num_samples) {
			fprintf(stderr, "wrote only %d out of %d bytes (sample_no = %d)\n",
							rw_count, num_samples, sample_no);
		}

		sample_no += num_samples;

		free(buffer);
		free(out_buffer);

	} while	(ess_burst < total_ess_bursts);

	while (fread(&sample, sizeof(sample), 1, infile)) {
		fwrite(&sample, sizeof(sample), 1, outfile);
	}

	fclose(infile);
	fclose(outfile);

	free(filter_init_buff);

	return TRUE;
}



/*********************************** Main ***********************************/

int main(int argc, char *argv[]) {
	int option;
	char *filename, *in_file, *out_file = "out.wav";
	FILE *pcm_file;
	int sample_no, start_sample_no, end_sample_no, length, total_ess_bursts;
	int start_array[ARRAY_SIZE], end_array[ARRAY_SIZE];
	unsigned int RMS, RMS_array[ARRAY_SIZE];
	short *buffer, *out_buffer;

	if (argc < 2) {
		printf("Usage: %s [OPTION]... [ARGUMENT]...\n", argv[0]);
		printf("Options and their arguments are as follows:\n");
		printf("-a <file>\t- attenuate the ess sections of <file>\n");
		printf("-d <file>\t- detect the ess sections of <file> and print start and end\n\t\t  sample numbers, time and RMS value\n");
		printf("-o <out_file>\t- specify output file name (use before -a)\n");
		printf("-p <file>\t- print decimal values of samples in <file>\n");
		printf("-t <time>\t- time in ms of ess burst to ignore 0-200, default 80 \n");
		printf("-v\t\t- verbose output (use before -a)\n");
		exit(0);
	}

	while ((option = getopt(argc, argv, "a:d:o:p:t:v")) != -1) {
		switch (option) {

			case 'a':
				in_file = optarg;

				total_ess_bursts = 0;

				pcm_file = fopen(in_file, "r");
				if (pcm_file == NULL) {
					fprintf(stdout, "cannot open file named %s\n", in_file);
					exit(9);
				}
				
				if (fseek(pcm_file, 24, SEEK_SET)) {
					fprintf(stderr, "Could not go to the 24th byte in %s\n", filename);
					exit(12);
				}						
				fread(&samples_per_sec, sizeof(int), 1, pcm_file);

				if (fseek(pcm_file, 44, SEEK_SET)) {
					fprintf(stderr, "Could not go to the 44th byte in %s\n", in_file);
					exit(10);
				}

				sample_no = -1;
				while (ess_detect(pcm_file, &sample_no, &start_sample_no, &end_sample_no, &RMS)) {
					*(start_array + total_ess_bursts) = start_sample_no;
					*(end_array +  total_ess_bursts) = end_sample_no;
					*(RMS_array +  total_ess_bursts) = RMS;
					total_ess_bursts++;
					if (total_ess_bursts >= ARRAY_SIZE) {
						fprintf(stderr, "Exceeded max array size\n");
						exit(11);
					}
				}

				fclose(pcm_file);

				ess_attenuate(in_file, out_file, start_array, end_array, RMS_array, total_ess_bursts);
				break;


			case 'd':
				filename = optarg;
				pcm_file = fopen(filename, "r");
				if (pcm_file == NULL) {
					fprintf(stdout, "cannot open file named %s\n", filename);
					exit(1);
				}
				
				if (fseek(pcm_file, 24, SEEK_SET)) {
					fprintf(stderr, "Could not go to the 24th byte in %s\n", filename);
					exit(13);
				}						
				fread(&samples_per_sec, sizeof(int), 1, pcm_file);
				

				if (fseek(pcm_file, 44, SEEK_SET)) {
					fprintf(stderr, "Could not go to the 44th byte in %s\n", filename);
					exit(2);
				}

				sample_no = -1;
				while (ess_detect(pcm_file, &sample_no, &start_sample_no, &end_sample_no, &RMS)) {
					length = end_sample_no - start_sample_no;
					printf("start: %d\tend: %d\t%dms\tRMS: %d\n", start_sample_no, end_sample_no, length*1000/samples_per_sec, RMS );
				}

				fclose(pcm_file);
				break;


			case 'o':
				out_file = optarg;
				break;


			case 'p':
				filename = optarg;
				print_decimal(filename);
				break;

			case 't':
				delta1 = atoi(optarg);
				break;


			case 'v':
				verbose = TRUE;
				break;


			case '?':
				if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
				exit(3);

			default:
				abort();
								 
		}
	}

}
