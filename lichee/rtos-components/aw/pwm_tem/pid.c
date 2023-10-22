#include <stdint.h>

#include "pid.h"

PID pid;


void InitPID(void)
{
	pid.SetTemperature = 125;       //target temperature
	pid.SumError = 0;
	pid.Kp = 700;                  	//proportional constant

	pid.Ki = 0.600;                	//integral constant
	pid.Kd = 0.00;                 	//derivative constant

	pid.NowError = 0;               //current error
	pid.LastError = 0;              //last error

	pid.Out0 = 0;                   //output correction value
	pid.Out = 0;                    //final output value
}


/*******************************************************************************
* function : LocPIDCalc
* description : PID control calculation
* input : None
* output : Results of PID
* output = kp*et + ki*etSum + kd*det+ out0;
*******************************************************************************/

int LocPIDCalc(void)
{
	static int out1, out2, out3;

	pid.NowError = pid.SetTemperature - pid.ActualTemperature;
	pid.SumError += pid.NowError;

	out1 = pid.Kp * pid.NowError;                                    //proportional
	out2 = pid.Kp * pid.Ki * pid.SumError;                           //integral
	out3 = pid.Kp * pid.Kd * (pid.NowError - pid.LastError);         //derivative
//			  pid.Out;                                              //correction

	pid.LastError = pid.NowError;

	return out1 + out2 + out3 + pid.Out;
}

void SetPID_Kp(int value)
{
	pid.Kp = (float)value/1000;
}

void SetPID_Ki(int value)
{
	pid.Ki = (float)value/1000;
}

void SetPID_Kd(int value)
{
	pid.Kd = (float)value/1000;
}

void SetPID_Temperature(int value)
{
	pid.SetTemperature = value;
}