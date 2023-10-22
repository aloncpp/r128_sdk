#ifndef _EQ_H_
#define _EQ_H_
#include <stdlib.h>
#include <stdio.h>

#ifdef FIXED_POINT
#else
#include <math.h>
#endif
#include <math.h>
/* EQ filter type definition */

#define MAX_FILTER_N (10)

typedef enum
{
    /* low pass shelving filter */
    LOWPASS_SHELVING,
    /* band pass peak filter */
    BANDPASS_PEAK,
    /* high pass shelving filter */
    HIHPASS_SHELVING,
    LOWPASS,
    HIGHPASS
} eq_ftype_t;

/* equalizer parameters */
typedef struct
{
    /* boost/cut gain in dB */
    int G;
    /* cutoff/center frequency in Hz */
    int fc;
    /* quality factor */
    float Q;
    /* filter type */
    eq_ftype_t type;
} eq_core_prms_t;

typedef struct
{
	/*num of items(biquad)*/
	int biq_num;
	/* sampling rate */
	int sampling_rate;
	/* channel num */
	int chan;
	/* eq parameters for generate IIR coeff*/
	eq_core_prms_t core_prms[MAX_FILTER_N];
}eq_prms_t;

typedef struct
{
	/*num of items(biquad)*/
	int enable_[MAX_FILTER_N];	//enable == 1 ;  disable == 0

	/* sampling rate */
	int sampling_rate;
	/* channel num */
	int chan;
	/* eq parameters for generate IIR coeff*/
	eq_core_prms_t core_prms[MAX_FILTER_N];
}eq_prms_t_pc;


typedef struct
{
	/*num of items(biquad)*/
	int biq_num;
	/* sampling rate */
	int sampling_rate;
	/* bits_per_sample */
	int bits_per_sample;
	/* channel num */
	int chan;
	/* eq parameters for generate IIR coeff*/
	eq_core_prms_t core_prms[MAX_FILTER_N];
}eq_remote_prms_t;


/*
	function eq_create
description:
	use this function to create the equalizer object
prms:
	eq_prms_t: [in], desired frequency response array
returns:
	the equalizer handle
*/
void *eq_create(eq_prms_t *prms);
/*
	function eq_process
description:
	equalizer processing function
prms:
	handle:[in,out] equalzier handle
	x:[in,out],	input signal
	len:[in], input length(in samples)
returns:
	none
*/
void eq_process(void *handle, short *x, int len);
/*
	function eq_destroy
description:
	use this function to destroy the equalizer object
prms:
	handle:[in], equalizer handle
returns:
	none
*/
void eq_destroy(void *handle);

void* eq_setpara_reset(eq_prms_t* prms, void * eq_handle);

void eq_avert_parms(eq_prms_t * eq_para, eq_prms_t_pc *eq_para_pc);

#endif
