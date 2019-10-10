/*
IIR2 coefficient compuation with values for graphing
https://www.earlevel.com/main/2012/11/26/biquad-c-source-code/

http://www.advsolned.com/wp-content/uploads/2018/05/Designing-and-implementing-biquad-filters-with-the-ASN-Filter-Designer-a-tutorial-review.pdf

// Line for compiling && executing--

// Integer/fixed-point computation (thinking of STM32F103)
gcc iir2_fix.c -o iir2_fix -lm -Wall && ./iir2_fix
*/
#include <math.h>
#include <stdio.h>
#include <stdint.h>

int main(void)
{
/* Specifications. */
double Fc = 0.05;
double Q = .707;

   double dnorm,da0,da1,da2,db1,db2;
   double K = tan(M_PI * Fc);
	double dscale = (1<<8);

/* Compute coefficients using fp doubles */
    dnorm = 1 / (1 + K / Q + K * K);
    da0 = 1; //K * K * norm;
    da1 = 2 * da0;
    da2 = da0;
    db1 = 2 * (K * K - 1) * dnorm;
    db2 = (1 - K / Q + K * K) * dnorm;

	double dKK = (K * K * dnorm);

/* Convert coefficients to ints. */
long long a0 = dscale * da0;
long long a1 = dscale * da1;
long long a2 = dscale * da2;
long long b1 = dscale * db1;
long long b2 = dscale * db2;

printf("Fc  %12.8f\n",Fc);
printf("Q   %12.8f\n\n",Q);

printf("KK  %12.8f  %12.8f\n\n",K * K * dnorm, 1/(K * K * dnorm) );

printf("K   %12.8f\n",K);
printf("a0  %12.8f %lli\n",da0, a0);
printf("a1  %12.8f %lli\n",da1, a1);
printf("a2  %12.8f %lli\n",da2, a2);
printf("b1  %12.8f %lli\n",db1, b1);
printf("b2  %12.8f %lli\n",db2, b2);

/* Generate a table for graphing. */
double dz1,dz2,dout;

long long out;
long long in = 1;
long long z1 = 0;
long long z2 = 0;
int i;
	for (i = 0; i < 30; i++)
	{
// By making a0 = a2 = 1, and using KK for gain scaling
// there is a net reduction of one multiply.
		 out = in * a0 + z1;
   	 z1  = in * a1 + z2 - b1 * out;
   	 z2  = in * a2 - b2 * out;
		dz1 = z1 / dscale;
		dz2 = z2 / dscale;
		dout = out / dscale;
		printf("%2i  %5lli %14.8f %14.8f %12.8f\n",i,in,dz1,dz2,out*dKK);
	}
return 0;
}

