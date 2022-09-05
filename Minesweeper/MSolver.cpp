#include"MSolver.h"
#include<cstring>
#include<vector>
#include<set>
#include<map>
#include<queue>
#include<cmath>

#define Pair(i,j) ((i)<<16|(j)&0xFFFF)
#define Geti(x) ((x)>>16)
#define Getj(x) ((x)&0xFFFF)
#define LOGZERO (-1E30)
#define ISLOGZERO(x) ((x)<-1.0)
#define MAXPROB 100000000

typedef int32_t pair_t;

#define NOTMINE -1
#define UNKNOWN -2
#define MINE    -3
#define OUT_OF_RANGE -128
/*
Internal representation of the board state.
0~8 means that the square there is that number
-1 it's not a mine but not opened yet
-2 totally unknown
-3 certainly a mine
-128 out of range
*/
static int8_t *board=NULL,*backboard=NULL;
static int32_t height=0,width=0,mines=0;
#define Index(i,j) ((i)*width+(j))

#if true
//for vectors about supercells, 
//since maximum size for 1 supercell(and constraints about 1 supercell) will never exceed 8
//using std::vector is too heavy in allocation/deallocation
template<typename T>
class vec8{
	T _dat[8];
	size_t _size;
public:

	size_t size() const { return _size; }
	void push_back(T val){ _dat[_size++]=val; }
	T &operator[](size_t s){ return _dat[s]; }
	const T&operator[](size_t s) const{ return _dat[s]; }
	T *begin(){ return _dat; }
	const T*begin() const{ return _dat; }
	T *end(){ return &_dat[_size]; }
	const T*end() const{ return &_dat[_size]; }

	//Note this behavior is not consistent with std::vector
	void resize(size_t size,T val){
		for(size_t i=0;i<size;++i)_dat[i]=val;
		_size=size;
	}

	vec8(){ _size=0; }
	vec8(size_t size,T val){ resize(size,val); }
};
#else
#define vec8 std::vector
#endif

/*
region<supercell>:
	for all i, n[c]=sum(f[c][i][j],j)
	w=sum(n[c],c)
*/
class Region{
public:
	//all constraints, y[q] is q-th constraints related to the region, Pair(free,mines)
	std::vector<std::pair<int32_t,int32_t>> y;
	//Postulate position of mines, p[i] is number of mines in i-th supercell, used in Solve process
	std::vector<int32_t> p;
	//x[i][k] is index of k-th constraints around i-th supercell in region
	std::vector<vec8<int32_t>> x;
	//all supercells, r[i] is i-th supercell in region, s[i]=r[i].size
	std::vector<vec8<int32_t>> r;
	//all possible total number of mines in region, m[c] is c-th possible number
	std::vector<int32_t> m;
	//n[c] is log(number of cases in which region contains m[c] mines)
	std::vector<double> n;
	//f[c][i][j] is log(number of cases in which (r[i] contains j mines) and (region contains m[c] mines))
	std::vector<std::vector<vec8<double>>> f;
	//log(number of possible cases)
	double w;
	//p(r)
	void Solve(int32_t k,int32_t maxmines);
public:
	//.
	Region();
	//(m,n,f,w)(r)
	bool GetParam(int32_t maxmines);
	//(m,n,f,w)((*,R).(m,n,f,w))
	bool Combine(Region &R,int32_t maxmines);
};

//a=log(exp(a)+exp(b));
void LogAdd(double &a,double b){
	double m=a,nd=b-a;
	if(nd>=0){
		m=b;nd=-nd;
	}
	if(nd<-50.0)a=m;
	else a=m+log(1+exp(nd));
}
//for natural number p+q=w, return p/w, scaled to [0,MAXPROB].
int LogToInt(double logp,double logq,double logw){
	if(ISLOGZERO(logp))return 0;
	if(ISLOGZERO(logq))return MAXPROB;
	int resp=std::round(exp(logp-logw)*MAXPROB);
	if(resp<1)resp=1;
	if(resp>MAXPROB-1)resp=MAXPROB-1;
	return resp;
}
//log(C(n,k))
double LogBin(int32_t n,int32_t k){
/*	if(n<0||k<0||k>n)return LOGZERO;
	if(2*k>n)k=n-k;
	double bres=0;
	for(int32_t j=0;j<k;++j)
		bres+=log((double)(n-j)/(j+1));
	return bres;*/
	const double cnk[][12]={
		{0.0000000000000000},
		{0.0000000000000000, 0.0000000000000000},
		{0.0000000000000000, 0.6931471805599453, 0.0000000000000000},
		{0.0000000000000000, 1.0986122886681100, 1.0986122886681100, 0.0000000000000000},
		{0.0000000000000000, 1.3862943611198910, 1.7917594692280550, 1.3862943611198910, 0.0000000000000000},
		{0.0000000000000000, 1.6094379124341000, 2.3025850929940460, 2.3025850929940460, 1.6094379124341000, 0.0000000000000000},
		{0.0000000000000000, 1.7917594692280550, 2.7080502011022100, 2.9957322735539910, 2.7080502011022100, 1.7917594692280550, 0.0000000000000000},
		{0.0000000000000000, 1.9459101490553130, 3.0445224377234230, 3.5553480614894140, 3.5553480614894140, 3.0445224377234230, 1.9459101490553130, 0.0000000000000000},
		{0.0000000000000000, 2.0794415416798360, 3.3322045101752040, 4.0253516907351490, 4.2484952420493590, 4.0253516907351490, 3.3322045101752040, 2.0794415416798360, 0.0000000000000000}
	};
	return cnk[n][k];
}
//mines: known mines, frees: free blocks that could be a mine (i.e. not known not-mine-block)
void countAround(int32_t i,int32_t j,int32_t &frees,int32_t &mines){
	bool u=i>0,d=i+1<height,l=j>0,r=j+1<width;
	frees=0;
	mines=0;
	if(u){
		frees+=(board[Index(i-1,j)]<-1)+(l&&board[Index(i-1,j-1)]<-1);
		mines+=(board[Index(i-1,j)]==MINE)+(l&&board[Index(i-1,j-1)]==MINE);
	}
	if(l){
		frees+=(board[Index(i,j-1)]<-1)+(d&&board[Index(i+1,j-1)]<-1);
		mines+=(board[Index(i,j-1)]==MINE)+(d&&board[Index(i+1,j-1)]==MINE);
	}
	if(d){
		frees+=(board[Index(i+1,j)]<-1)+(r&&board[Index(i+1,j+1)]<-1);
		mines+=(board[Index(i+1,j)]==MINE)+(r&&board[Index(i+1,j+1)]==MINE);
	}
	if(r){
		frees+=(board[Index(i,j+1)]<-1)+(u&&board[Index(i-1,j+1)]<-1);
		mines+=(board[Index(i,j+1)]==MINE)+(u&&board[Index(i-1,j+1)]==MINE);
	}
}
//Boundry : unknown square with opened squares near it.
bool isBoundry(int32_t i,int32_t j){
	if(board[Index(i,j)]!=UNKNOWN)return false;
	bool u=i>0,d=i+1<height,l=j>0,r=j+1<width;
	if(u)if(board[Index(i-1,j)]>=0||l&&board[Index(i-1,j-1)]>=0)return true;
	if(l)if(board[Index(i,j-1)]>=0||d&&board[Index(i+1,j-1)]>=0)return true;
	if(d)if(board[Index(i+1,j)]>=0||r&&board[Index(i+1,j+1)]>=0)return true;
	if(r)if(board[Index(i,j+1)]>=0||u&&board[Index(i-1,j+1)]>=0)return true;
	return false;
}
//solve basic cases, gives basic consistency test.
bool BasicSolver(){
	bool diff;
	do{
		diff=false;
		for(int32_t i=0;i<height;++i)for(int32_t j=0;j<width;++j){
			int32_t bij=board[Index(i,j)];
			if(bij>=0){
				int32_t numf,numm;
				countAround(i,j,numf,numm);
				if(bij>numf||bij<numm)return false;

				if(bij==numf){
					int32_t is=std::max(       0,i-1);
					int32_t ie=std::min(height-1,i+1);
					int32_t js=std::max(       0,j-1);
					int32_t je=std::min( width-1,j+1);
					//Attempt to flag mines(if number of a square around it = its number)
					for(int32_t di=is;di<=ie;++di)for(int32_t dj=js;dj<=je;++dj)
						if(board[Index(di,dj)]==UNKNOWN){
							diff=true;
							board[Index(di,dj)]=MINE;
						}
				}
				else if(bij==numm){
					int32_t is=std::max(       0,i-1);
					int32_t ie=std::min(height-1,i+1);
					int32_t js=std::max(       0,j-1);
					int32_t je=std::min( width-1,j+1);
					//Find a square where the number of mines around it is the same as it
					//Then click every empty square around it
					for(int32_t di=is;di<=ie;++di)for(int32_t dj=js;dj<=je;++dj)
						if(board[Index(di,dj)]==UNKNOWN){
							diff=true;
							board[Index(di,dj)]=NOTMINE;
						}
				}
			}
		}
	}while(diff);
	return true;
}

Region::Region(){

}
void Region::Solve(int32_t i,int32_t maxmines){
	if(i==r.size()){
		//Solution found!
		int32_t mk=0;
		double weight=0;
		for(int32_t i=0;i<r.size();++i){
			mk+=p[i];
			weight+=LogBin(r[i].size(),p[i]);
		}
		int32_t mchs=std::find(m.begin(),m.end(),mk)-m.begin();
		if(mchs==m.size()){
			std::vector<vec8<double>> shape(r.size());
			for(int32_t i=0;i<r.size();++i)shape[i].resize(r[i].size()+1,LOGZERO);
			m.push_back(mk);
			n.push_back(LOGZERO);
			f.push_back(shape);
		}
		LogAdd(w,weight);
		LogAdd(n[mchs],weight);
		for(int32_t i=0;i<r.size();++i)
			LogAdd(f[mchs][i][p[i]],weight);
	}
	else{
		int32_t si=r[i].size();
		//possible numbers of mines at this stage
		int32_t maxmi=std::min(maxmines,si),minmi=0;
		for(const auto &idx:x[i]){
			auto &yidx=y[idx];
			maxmi=std::min(maxmi,yidx.second);
			minmi=std::max(minmi,yidx.second+si-yidx.first);
		}

		if(maxmi<minmi)return;

		//set initial mines
		for(const auto &idx:x[i]){
			auto &yidx=y[idx];
			yidx.first-=si;
			yidx.second-=minmi-1;
		}

		for(int32_t mi=minmi;mi<=maxmi;++mi){
			//update mine
			for(const auto &idx:x[i])
				--y[idx].second;

			// Recurse next supercell
			p[i]=mi;
			Solve(i+1,maxmines-mi);
		}

		//recover cells
		for(const auto &idx:x[i]){
			auto &yidx=y[idx];
			yidx.first+=si;
			yidx.second+=maxmi;
		}
	}
}
bool Region::GetParam(int32_t maxmines){
	if(
		true||/**/
		r.size()<16){
		p.resize(r.size(),0);
		w=LOGZERO;
		Solve(0,maxmines);
		if(ISLOGZERO(w))return false;
	}
	else{
		//LMR Method
		//TODO
	}
	return true;
}
bool Region::Combine(Region &R,int32_t maxmines){
	//all possible total number of mines in region, m[c] is c-th possible number
	std::vector<int32_t> nm;
	//n[c] is log(number of cases in which region contains m[c] mines)
	std::vector<double> nn;
	//f[c][i][j] is log(number of cases in which (r[i] contains j mines) and (region contains m[c] mines))
	std::vector<std::vector<vec8<double>>> nf;
	//log(number of possible cases)
	double nw=LOGZERO;

	bool retv=false;
	int32_t n1=r.size(),n2=R.r.size();
	r.resize(n1+n2);
	for(int32_t i=0;i<n2;++i)r[n1+i]=(R.r[i]);
	std::vector<vec8<double>> shape(n1+n2);
	for(int32_t i=0;i<r.size();++i)shape[i].resize(r[i].size()+1,LOGZERO);

	for(int32_t ch1=0;ch1<m.size();++ch1)for(int32_t ch2=0;ch2<R.m.size();++ch2){
		int32_t mn=m[ch1]+R.m[ch2];
		if(mn>maxmines)continue;
		retv=true;
		int32_t mchs=std::find(nm.begin(),nm.end(),mn)-nm.begin();
		if(mchs==nm.size()){
			nm.push_back(mn);
			nn.push_back(LOGZERO);
			nf.push_back(shape);
		}
		LogAdd(nn[mchs],n[ch1]+R.n[ch2]);
		LogAdd(nw,n[ch1]+R.n[ch2]);
		for(int32_t i=0;i<n1+n2;++i)for(int32_t j=0;j<=r[i].size();++j)
			LogAdd(nf[mchs][i][j],
				i<n1 ? f[ch1][i][j]+R.n[ch2]
				     : R.f[ch2][i-n1][j]+n[ch1]
			);
	}
	
	m.swap(nm);
	n.swap(nn);
	f.swap(nf);
	w=nw;
	return retv;
}
void Segregate(const std::vector<int32_t> &border,std::vector<Region> &result){
	std::set<int32_t> uncovered(border.begin(),border.end());
	while(1){
		std::set<int32_t> connects;
		result.push_back(Region());
		Region &finished=result.back();
		// Find a suitable starting point
		if(uncovered.empty())break;
		connects.insert(*uncovered.begin());
		uncovered.erase(uncovered.begin());

		while(!connects.empty()){
			finished.r.push_back(vec8<int32_t>(1,*connects.begin()));
			vec8<int32_t> &curscell=finished.r.back();
			int32_t i1=Geti(curscell[0]),j1=Getj(curscell[0]);
			connects.erase(connects.begin());

			//current supercell 0's all ajacent numbers are constraints
			finished.x.push_back(vec8<int32_t>());
			vec8<int32_t> &curcons=finished.x.back();

			//Find all connecting cells
			int32_t
				is=std::max(       0,i1-2),
				js=std::max(       0,j1-2),
				ie=std::min(height-1,i1+2),
				je=std::min( width-1,j1+2);
			for(int32_t i2=is;i2<=ie;++i2)for(int32_t j2=js;j2<=je;++j2)if(board[Index(i2,j2)]==UNKNOWN){
				int32_t pij=Pair(i2,j2);
				bool inunc=uncovered.find(pij)!=uncovered.end();
				if(!inunc&&connects.find(pij)==connects.end())continue;

				bool isconnect=false,issuper=true;
				int32_t
					mini=std::max(       0,std::min(i1,i2)-1),
					minj=std::max(       0,std::min(j1,j2)-1),
					maxi=std::min(height-1,std::max(i1,i2)+1),
					maxj=std::min( width-1,std::max(j1,j2)+1);
				for(int32_t ci=mini;ci<=maxi;++ci)for(int32_t cj=minj;cj<=maxj;++cj)
					if(board[Index(ci,cj)]>0){
						bool isl=(std::abs(ci-i1)<=1&&std::abs(cj-j1)<=1),
							 isr=(std::abs(ci-i2)<=1&&std::abs(cj-j2)<=1);
						if(isl&&isr)isconnect=true;
						else if(isl||isr)issuper=false;
				}

				if(isconnect){
					if(inunc){
						uncovered.erase(pij);
						if(!issuper)connects.insert(pij);
					}
					if(issuper){
						if(!inunc)connects.erase(pij);
						curscell.push_back(pij);
					}
				}
			}
			else if(board[Index(i2,j2)]>0){
				bool isl=(std::abs(i2-i1)<=1&&std::abs(j2-j1)<=1);
				if(isl)curcons.push_back(Pair(i2,j2));
			}
		}

		//convert all finished.x from pairs to index
		//and construct constraints y
		auto &consval=finished.y;
		std::map<int32_t,int32_t> yidx;
		for(auto &xi:finished.x)for(auto &xij:xi){
			auto retv=yidx.insert(std::pair<int32_t,int32_t>(xij,yidx.size()));
			xij=retv.first->second;
			if(retv.second){
				int32_t tile=retv.first->first;
				int32_t ix=Geti(tile),jx=Getj(tile);
				int32_t nf,nm;
				countAround(ix,jx,nf,nm);
				consval.push_back(std::pair<int32_t,int32_t>(nf-nm,board[Index(ix,jx)]-nm));
			}
		}
	}
}
bool MSolve(int32_t _width,int32_t _height,int32_t _mines,const int8_t *_board,int32_t *prob){
	if(_width<=0||_height<=0||_mines<0||_width>=32768||_height>=32768)return false;
	int32_t boardsize=_width*_height;
	if(boardsize!=width*height){
		if(board)delete[] board;
		board=new int8_t[boardsize*2];
		backboard=board+boardsize;
	}
	for(int32_t i=0;i<boardsize;++i){
		if(_board[i]>=0&&_board[i]<9)board[i]=_board[i];
		else board[i]=UNKNOWN;
	}
	width=_width;
	height=_height;
	mines=_mines;

	if(!BasicSolver())return false;

	std::vector<int32_t> border,other;
	std::vector<Region> regions;
	int32_t unknowns=0;
	for(int32_t i=0;i<height;++i)for(int32_t j=0;j<width;++j){
		int32_t numb=board[Index(i,j)];
		if(numb==MINE)--mines;
		else if(numb==UNKNOWN){
			++unknowns;
			if(isBoundry(i,j))border.push_back(Pair(i,j));
			else other.push_back(Pair(i,j));
		}
	}
	if(mines<0||mines>unknowns)return false;
	Segregate(border,regions);

	int no=other.size();
	//CN[mr] is log(Binomial coefficient, number of cases to select mr blocks in other)
	std::vector<double> CN(no+1);
	CN[0]=0;
	for(int32_t mr=0;mr<no;++mr)
		CN[mr+1]=CN[mr]+log((double)(no-mr)/(mr+1));
	//log(total number of possible cases)
	double allw=LOGZERO;
	//log(how many cases in which a specific tile in other is a mine)
	double otherf=LOGZERO;
	//is not
	double othercf=LOGZERO;

	if(regions.size()){
		for(int32_t i=0;i<regions.size();++i){
			if(!regions[i].GetParam(mines))return false;
			if(i)if(!regions[0].Combine(regions[i],mines))return false;
		}

		Region &r0=regions[0];
		//scellf[i]:log(how many cases in which a specific cell in r0.r[i] is a mine)
		std::vector<double> scellf(r0.r.size(),LOGZERO);
		//is not
		std::vector<double> scellcf(r0.r.size(),LOGZERO);

		//traverse all possible mchs choices
		for(int32_t mchs=0;mchs<r0.m.size();++mchs){
			//mr is number of mines in otherTiles
			int32_t mr=mines-r0.m[mchs];
			//if possible
			if(mr>=0&&mr<=no){
				//local weight: CN[mr]*r0.n[mchs] cases that
				//mr mines are in other and r0 contains r0.m[mchs] mines.
				double locweight=CN[mr]+r0.n[mchs];
				LogAdd(allw,locweight);//allw+=locweight
				for(int32_t i=0;i<r0.r.size();++i){
					int si=r0.r[i].size();
					for(int32_t j=0;j<=si;++j){
						//a specific cell in r0.r[i] has a probability of
						//sum( (j/r0.s[i]) * (f[mchs][i][j]/r0.n[mchs]) ,j) to be a mine
						if(j)LogAdd(scellf[i],log((double)j/si)+CN[mr]+r0.f[mchs][i][j]);
						if(j!=si)LogAdd(scellcf[i],log((double)(si-j)/si)+CN[mr]+r0.f[mchs][i][j]);
					}
				}
				//a specific tile in otherTiles has a probability of mr/N to be a mine
				if(mr)LogAdd(otherf,locweight+log((double)mr/no));
				if(mr!=no)LogAdd(othercf,locweight+log((double)(no-mr)/no));
			}
		}

		if(ISLOGZERO(allw))return false;

		for(int32_t i=0;i<r0.r.size();++i){
			int32_t cpi=LogToInt(scellf[i],scellcf[i],allw);
			for(int32_t j=0;j<r0.r[i].size();++j){
				int32_t rij=r0.r[i][j];
				prob[Geti(rij)*width+Getj(rij)]=cpi;
			}
		}
	}
	else if(no){
		allw=CN[mines];
		if(mines)otherf=log((double)mines/no)+allw;
		if(mines!=no)othercf=log((double)(no-mines)/no)+allw;
	}

	if(no){
		int32_t otherfi=LogToInt(otherf,othercf,allw);
		for(int32_t k=0;k<no;++k)
			prob[Geti(other[k])*width+Getj(other[k])]=otherfi;
	}

	for(int32_t i=0;i<boardsize;++i){
		if(board[i]==MINE)prob[i]=MAXPROB;
		else if(board[i]>=-1)prob[i]=0;
	}
	return true;
}

bool MSolve(int32_t _width,int32_t _height,const int8_t *_board,int32_t initx,int32_t inity){
	if(_width<=0||_height<=0||_width>=32768||_height>=32768)return false;
	if(initx<0||initx>=_width||inity<0||inity>=_height)return false;
	int32_t boardsize=_width*_height;
	if(boardsize!=width*height){
		if(board)delete[] board;
		board=new int8_t[boardsize*2];
		backboard=board+boardsize;
	}
	mines=0;
	memset(board,UNKNOWN,boardsize);
	for(int32_t i=0;i<boardsize;++i){
		if(_board[i]>=0&&_board[i]<9)backboard[i]=_board[i];
		else{
			backboard[i]=MINE;
			++mines;
		}
	}
	width=_width;
	height=_height;

	int32_t curfrees=boardsize;
	std::queue<pair_t> safetiles;
	//tiles with numbers that should be checked for progress
	std::set<pair_t> updates;
	//dead tiles are tiles contain non-zero numbers but no longer be able to provide any information
	std::vector<bool> deadmap(boardsize,false);
	safetiles.push(Pair(inity,initx));
	do{
		//Discover all safe tiles
		while(safetiles.size()){
			pair_t tile=safetiles.front();
			safetiles.pop();
			int32_t i=Geti(tile),j=Getj(tile);
			int32_t idx=Index(i,j);

			//already opened, nothing to do
			if(board[idx]>=0)continue;

			//open a safe tile
			--curfrees;
			if(board[idx]=backboard[idx]){
				//non-zero number should be updated
				updates.insert(tile);
			}
			else{
				//8 ajacent tiles should be opened
				bool u=i>0,d=i+1<height,l=j>0,r=j+1<width;
				if(u){
					safetiles.push(Pair(i-1,j));
					if(l)safetiles.push(Pair(i-1,j-1));
				}
				if(l){
					safetiles.push(Pair(i,j-1));
					if(d)safetiles.push(Pair(i+1,j-1));
				}
				if(d){
					safetiles.push(Pair(i+1,j));
					if(r)safetiles.push(Pair(i+1,j+1));
				}
				if(r){
					safetiles.push(Pair(i,j+1));
					if(u)safetiles.push(Pair(i-1,j+1));
				}
			}
		}

		//if all safe tiles are opened, the game is solvable free of guess
		if(curfrees==mines)return true;

		//check all updated numbers to find mines and new safetiles
		while(updates.size()){
			pair_t tile=*updates.begin();
			int32_t i=Geti(tile);
			int32_t j=Getj(tile);
			int32_t idx=Index(i,j);
			int8_t bij=board[idx];

			int32_t numf,numm;
			countAround(i,j,numf,numm);

			std::set<pair_t> update_around;
			if(bij==numf){
				int32_t is=std::max(0,i-1);
				int32_t ie=std::min(height-1,i+1);
				int32_t js=std::max(0,j-1);
				int32_t je=std::min(width-1,j+1);
				//Attempt to flag mines(if number of a square around it = its number)
				for(int32_t di=is;di<=ie;++di)for(int32_t dj=js;dj<=je;++dj)
					if(board[Index(di,dj)]==UNKNOWN){
						board[Index(di,dj)]=MINE;
						update_around.insert(Pair(di,dj));
					}
				deadmap[idx]=true;
			}
			else if(bij==numm){
				int32_t is=std::max(0,i-1);
				int32_t ie=std::min(height-1,i+1);
				int32_t js=std::max(0,j-1);
				int32_t je=std::min(width-1,j+1);
				//Find a square where the number of mines around it is the same as it
				//Then click every empty square around it
				for(int32_t di=is;di<=ie;++di)for(int32_t dj=js;dj<=je;++dj)
					if(board[Index(di,dj)]==UNKNOWN){
						board[Index(di,dj)]=NOTMINE;
						safetiles.push(Pair(di,dj));
						update_around.insert(Pair(di,dj));
					}
				deadmap[idx]=true;
			}
			else{//Pair-Solver
				int32_t is=std::max(0,i-2);
				int32_t ie=std::min(height-1,i+2);
				int32_t js=std::max(0,j-2);
				int32_t je=std::min(width-1,j+2);
				for(int32_t di=is;di<=ie;++di)for(int32_t dj=js;dj<=je;++dj){
					int32_t didx=Index(di,dj);
					if(board[didx]<=0||idx==didx||deadmap[didx])continue;
					pair_t mu[4],lu[8],ru[8];
					int32_t ms=0,ls=0,rs=0;

					int32_t a=bij,b=board[didx],
						mini=std::max(0,std::min(i,di)-1),
						minj=std::max(0,std::min(j,dj)-1),
						maxi=std::min(height-1,std::max(i,di)+1),
						maxj=std::min(width-1,std::max(j,dj)+1);
					for(int32_t ci=mini;ci<=maxi;++ci)for(int32_t cj=minj;cj<=maxj;++cj){
						int32_t cidx=Index(ci,cj);
						if(board[cidx]<-1){//MINE or UNKNOWN
							bool isl=(std::abs(ci- i)<=1&&std::abs(cj- j)<=1),
								isr=(std::abs(ci-di)<=1&&std::abs(cj-dj)<=1);
							if(board[cidx]==UNKNOWN){
								if(isl&&isr)mu[ms++]=Pair(ci,cj);
								else if(isl)lu[ls++]=Pair(ci,cj);
								else if(isr)ru[rs++]=Pair(ci,cj);
							}
							else{
								if(isl)--a;
								if(isr)--b;
							}
						}
					}

					bool lm=a==b+ls,rm=b==a+rs;
					if(lm||rm){
						int8_t lset=lm?MINE:NOTMINE;
						int8_t rset=rm?MINE:NOTMINE;
						for(int32_t i=0;i<ls;++i){
							int32_t li=lu[i];
							board[Index(Geti(li),Getj(li))]=lset;
							update_around.insert(li);
							if(lset==NOTMINE)safetiles.push(li);
						}
						for(int32_t i=0;i<rs;++i){
							int32_t ri=ru[i];
							board[Index(Geti(ri),Getj(ri))]=rset;
							update_around.insert(ri);
							if(rset==NOTMINE)safetiles.push(ri);
						}
					}
				}
			}
			for(const auto &uatile:update_around){
				int32_t di=Geti(uatile);
				int32_t dj=Getj(uatile);
				//update around di,dj
				int32_t dis=std::max(0,di-1);
				int32_t die=std::min(height-1,di+1);
				int32_t djs=std::max(0,dj-1);
				int32_t dje=std::min(width-1,dj+1);
				for(int32_t ddi=dis;ddi<=die;++ddi)for(int32_t ddj=djs;ddj<=dje;++ddj)
					if(board[Index(ddi,ddj)]>0&&!deadmap[Index(ddi,ddj)]){
						updates.insert(Pair(ddi,ddj));
					}
			}
			updates.erase(tile);
		}
		//...

		//if there are safe tiles, just open them
		if(safetiles.size())continue;

		//Basic solver fails, tank algorithm starts

		//naive implementation
		int32_t *prob=new int32_t[boardsize];
		int8_t *pushboard=new int8_t[boardsize];
		int32_t pushmines=mines;
		memcpy(pushboard,board,boardsize*sizeof(int8_t));
		//This function will change global board and mines
		MSolve(_width,_height,mines,board,prob);
		mines=pushmines;
		memcpy(board,pushboard,boardsize*sizeof(int8_t));
		delete[] pushboard;
		for(int32_t i=0;i<height;++i)for(int32_t j=0;j<width;++j){
			if(board[Index(i,j)]==UNKNOWN){
				if(prob[Index(i,j)]==0){
					board[Index(i,j)]=NOTMINE;
					safetiles.push(Pair(i,j));
				}
				if(prob[Index(i,j)]==MAXPROB){
					board[Index(i,j)]=MINE;
				}
			}
		}
		delete[] prob;
	} while(safetiles.size());
	return false;
}


