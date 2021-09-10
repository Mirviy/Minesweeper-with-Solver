#include"random.h"

//MT19937-64
const uint64_t
	W=64,N=312,M=156,R=31,
	A=0xB5026F5AA96619E9,
	U=29,D=0x5555555555555555,
	S=17,B=0x71D67FFFEDA60000,
	T=37,C=0xFFF7EEE000000000,
	L=43,F=6364136223846793005,
	MASK_LOWER=0x7FFFFFFF,
	MASK_UPPER=~MASK_LOWER
;

static uint64_t mt[N];
static uint64_t index=N+1;

//Re-init with a given seed
void seedrandom(const uint64_t seed){
	index=N;
	mt[0]=seed;
	for(uint64_t i=1;i<N;++i)
		mt[i]=F*(mt[i-1]^(mt[i-1]>>W-2))+i;
}
void Twist(){
	for(uint64_t i=0;i<N;++i){
		uint64_t x=(mt[i]&MASK_UPPER)+(mt[(i+1)%N]&MASK_LOWER);
		uint64_t xA=x>>1;
		if(x&1)xA^=A;
		mt[i]=mt[(i+M)%N]^xA;
	}
	index=0;
}

//a 64-bit random number
uint64_t random64(){

	if(index>=N){
		if(index>N)seedrandom(F);
		Twist();
	}

	uint64_t y=mt[index];

	y^=y>>U&D;
	y^=y<<S&B;
	y^=y<<T&C;
	y^=y>>L;

	++index;

	return y;
}
//a random real in (0,1)
double random(){
	const double rbase=1.0/18446744073709551616.;
	return ((double)random64()+0.5)*rbase;
}
//a random real from normal distribution
double randomN(){
	const double dpi=6.2831853071795864;
	return sqrt(-2*log(random()))*cos(dpi*random());
}
