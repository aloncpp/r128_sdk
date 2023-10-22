#ifndef PID_H_
#define PID_H_


typedef struct PID
{
	int  SetTemperature;        //target temperature
	int  ActualTemperature;     //current value

	int SumError;               //error accumulation
	float  Kp;                  //proportional constant
	float  Ki;                  //integral constant
	float  Kd;                  //derivative constant
	int NowError;               //current error
	int LastError;              //last error

	int Out0;                   //output correction value
	int Out;                    //final output value
} PID;

extern PID pid;
void InitPID(void);
int LocPIDCalc(void);
void SetPID_Kp(int value);
void SetPID_Ki(int value);
void SetPID_Kd(int value);
void SetPID_Temperature(int value);

#endif /* PID_H_ */
