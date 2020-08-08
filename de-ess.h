/*
 * de-ess.h - header file for de-ess.c
 * 
 * Copyright (C) 2008-2013 Theophilus (http://theaudiobible.org)
 *
 * http://theaudiobible.org/contact.php
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <fftw3.h>

/* Note that N must be odd */
#define N 81
#define PI 3.14159265358979323846
#define EQ1_FILE "/etc/eq1.cfg"
#define EQ2_FILE "/etc/eq2.cfg"
#define MAX_CHAR_PER_LINE 9

/* Description of the following definition */
#define QUALIFYING_HALF_CYCLES_THRESHOLD 100
#define ARRAY_SIZE 100000

#define FALSE 0
#define TRUE !FALSE

/***************************** Type definitions *****************************/
typedef unsigned char byte;


