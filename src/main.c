#include "stm32f4xx.h"
#include "stm32f4_discovery.h"
#include "tm_stm32f4_lis302dl_lis3dsh.h"
#include "stm32_tm1637.h"

#include "SPI.h"
#include "TIM.h"
#include "GPIO.h"

#include "arm_math.h"

#define N 64
#define freq 128
extern int FFT_Flag = 0;
extern int STEPS = 0;
extern int PAUSE = 0;

double sample[N] = {0.0};
float32_t comp[2*N] = {0.0};
float32_t compfft[2*N] = { 0.0 };
double mag[N] = {0.0};

uint8_t results = 0;
int itsc=0;
double maxvalue;
uint32_t maxvalueindex;
double frequency = 0;

TM_LIS302DL_LIS3DSH_t Axes_Data;
TM_LIS302DL_LIS3DSH_Device_t IMU_Type;



// przerwanie!
void TIM3_IRQHandler(void) {
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {

		TM_LIS302DL_LIS3DSH_ReadAxes(&Axes_Data);
		if (results < N){

			sample[results++] = (double) Axes_Data.Y;

		}
		else
			FFT_Flag = 1;
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}


int main(void)
{
	SystemInit();
	initAccelerometr();
	initGPIODiodes();
	initTimer3();
	tm1637Init();
	tm1637DisplayInt(0);

	for (;;) {
		if (FFT_Flag) {
			TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);

			//----- FFT -----

			for(int i=0; i<2*N; i+=2){
				comp[i] = (float) sample[itsc++];
			}
			itsc=0;

			//tworzenie instancji struktury
			arm_cfft_radix4_instance_f32 S;
			arm_cfft_radix4_init_f32(&S, N, 0, 1); //zainicjowanie jej

			//widmo fft
			arm_cfft_radix4_f32(&S, comp);
			comp[0] = 0;

			//wyliczanie modulu liczby zespolonej
			arm_cmplx_mag_squared_f32(comp, mag, N);
			mag[0] = 0;

			//szukanie maksymalnej wartosci, zapisywanie jej indeksu
			arm_max_f32(mag, N, &maxvalue, &maxvalueindex);

			//czestotliwosc
			frequency = (double)maxvalueindex * (double)freq / (double)(N/2) ;

			if(maxvalueindex==1&&maxvalue>1000000000){
				if(PAUSE==0){
					//zliczanie krokow
					sendStep(++STEPS);
					PAUSE=1;
				} else PAUSE = 0;
			}

			Clear();

			GPIO_SetBits(GPIOD, GPIO_Pin_15);
			FFT_Flag = 0;
			TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
		}
	}
}


void Clear(){
	//czysci wszystkie tablices (wykonywana co sekunde)
	for(int i=0;i<N;i++){
		sample[i] = 0;
		//mag[i] = 0;
		}
	for(int i=0;i<2*N;i++){
		comp[i] = 0;
	}
	results = 0;
}



void sendStep(int step){
	tm1637DisplayInt(step);
}
