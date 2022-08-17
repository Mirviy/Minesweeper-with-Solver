#include"MSolver.h"
#include<cstring>
#include<vector>
#include<set>
#include<cmath>

#define Pair(i,j) ((i)<<16|(j)&0xFFFF)
#define Geti(x) ((x)>>16)
#define Getj(x) ((x)&0xFFFF)
#define LOGZERO (-1E30)
#define MAXPROB 100000000

/*
Internal representation of the board state.
0~8 means that the square there is that number
-1 it's not a mine but not opened yet
-2 totally unknown
-3 certainly a mine
-128 out of range
*/
static int8_t *board=NULL;
static int32_t height=0,width=0,mines=0;
/*
region<supercell>:
	for all i, n[c]=sum(f[c][i][j],j)
	w=p.size=sum(n[c],c)
*/
class Region{
public:
	//all supercells, r[i] is i-th supercell in region, s[i]=r[i].size
	std::vector<std::vector<int32_t>> r;
	//all position of mines, p[k][i] is number of mines in i-th supercell under k-th position
	std::vector<std::vector<int32_t>> p;
	//all possible total number of mines in region, m[c] is c-th possible number
	std::vector<int32_t> m;
	//n[c] is log(number of cases in which region contains m[c] mines)
	std::vector<double> n;
	//f[c][i][j] is log(number of cases in which (r[i] contains j mines) and (region contains m[c] mines))
	std::vector<std::vector<std::vector<double>>> f;
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
	if(logp<-1.)return 0;
	if(logq<-1.)return MAXPROB;
	int resp=floor(exp(logp-logw)*MAXPROB+0.5);
	if(resp<1)resp=1;
	if(resp>MAXPROB-1)resp=MAXPROB-1;
	return resp;
}
//log(C(n,k))
double LogBin(int32_t n,int32_t k){
	if(n<0||k<0||k>n)return LOGZERO;
	if(2*k>n)k=n-k;
	double bres=0;
	for(int32_t j=0;j<k;++j)
		bres+=log((double)(n-j)/(j+1));
	return bres;
}
int8_t &Board(int32_t i,int32_t j){
	static int8_t out_of_range;
	if(i<0||j<0||i>=height||j>=width){
		out_of_range=-128;
		return out_of_range;
	}
	return board[i*width+j];
}
int8_t &Board(int32_t ijpair){
	return Board(Geti(ijpair),Getj(ijpair));
}
//mines: known mines, frees: free blocks that could be a mine (i.e. not known not-mine-block)
void countAround(int32_t i,int32_t j,int32_t &frees,int32_t &mines){
	bool u=i>0,d=i+1<height,l=j>0,r=j+1<width;
	frees=0;
	mines=0;
	if(u){
		frees+=(Board(i-1,j)<-1)+(l&&Board(i-1,j-1)<-1);
		mines+=(Board(i-1,j)==-3)+(l&&Board(i-1,j-1)==-3);
	}
	if(l){
		frees+=(Board(i,j-1)<-1)+(d&&Board(i+1,j-1)<-1);
		mines+=(Board(i,j-1)==-3)+(d&&Board(i+1,j-1)==-3);
	}
	if(d){
		frees+=(Board(i+1,j)<-1)+(r&&Board(i+1,j+1)<-1);
		mines+=(Board(i+1,j)==-3)+(r&&Board(i+1,j+1)==-3);
	}
	if(r){
		frees+=(Board(i,j+1)<-1)+(u&&Board(i-1,j+1)<-1);
		mines+=(Board(i,j+1)==-3)+(u&&Board(i-1,j+1)==-3);
	}
}
//check after make a postulate in tank algorithm
bool ConsistencyAround(int32_t i,int32_t j){
	for(int32_t di=-1;di<=1;++di)for(int32_t dj=-1;dj<=1;++dj){
		int32_t numb=Board(i+di,j+dj);
		if(numb>=0){
			int32_t frees,mines;
			countAround(i+di,j+dj,frees,mines);
			if(numb>frees||numb<mines)return false;
		}
	}
	return true;
}
//Boundry : unknown square with opened squares near it.
bool isBoundry(int32_t i,int32_t j){
	if(Board(i,j)!=-2)return false;
	bool u=i>0,d=i+1<height,l=j>0,r=j+1<width;
	if(u)if(Board(i-1,j)>=0||l&&Board(i-1,j-1)>=0)return true;
	if(l)if(Board(i,j-1)>=0||d&&Board(i+1,j-1)>=0)return true;
	if(d)if(Board(i+1,j)>=0||r&&Board(i+1,j+1)>=0)return true;
	if(r)if(Board(i,j+1)>=0||u&&Board(i-1,j+1)>=0)return true;
	return false;
}
//solve basic cases, gives basic consistency test.
bool BasicSolver(){
	bool diff;
	do{
		diff=false;
		for(int32_t i=0;i<height;++i)for(int32_t j=0;j<width;++j){
			int32_t bij=Board(i,j);
			if(bij>=0){
				int32_t numf,numm;
				countAround(i,j,numf,numm);
				if(bij>numf||bij<numm)return false;

				if(bij==numf){
					//Attempt to flag mines(if number of a square around it = its number)
					for(int32_t di=-1;di<=1;++di)for(int32_t dj=-1;dj<=1;++dj)
						if(Board(i+di,j+dj)==-2){
							Board(i+di,j+dj)=-3;
							++numm;
						}
				}
				if(bij==numm){
					//Find a square where the number of mines around it is the same as it
					//Then click every empty square around it
					for(int32_t di=-1;di<=1;++di)for(int32_t dj=-1;dj<=1;++dj)
						if(Board(i+di,j+dj)==-2){
							diff=true;
							Board(i+di,j+dj)=-1;
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
		std::vector<int32_t> pos(r.size(),0);
		for(int32_t ii=0;ii<r.size();++ii)for(int32_t j=0;j<r[ii].size();++j)
			pos[ii]+=(Board(r[ii][j])==-3);
		p.resize(p.size()+1);
		p[p.size()-1].swap(pos);
	}
	else{
		//set zero-mine
		for(int32_t j=0;j<r[i].size();++j)Board(r[i][j])=-1;
	
		for(int32_t mi=0;mi<=r[i].size();++mi){
			//set mines.
			if(mi)Board(r[i][mi-1])=-3;

			//if at this point, it's already inconsistent
			if(mi>maxmines)break;
			if(!ConsistencyAround(Geti(r[i][0]),Getj(r[i][0])))continue;

			// Recurse next supercell
			Solve(i+1,maxmines-mi);
		}
		//recover cells
		for(int32_t j=0;j<r[i].size();++j)Board(r[i][j])=-2;
	}
}
bool Region::GetParam(int32_t maxmines){
	if(
		true||/**/
		r.size()<16){
		Solve(0,maxmines);
		if(p.size()==0)return false;

		std::vector<std::vector<double>> shape(r.size());
		for(int32_t i=0;i<r.size();++i)shape[i].resize(r[i].size()+1,LOGZERO);
		w=LOGZERO;

		for(int32_t k=0;k<p.size();++k){
			int32_t mk=0;
			double weight=0;
			for(int32_t i=0;i<r.size();++i){
				mk+=p[k][i];
				weight+=LogBin(r[i].size(),p[k][i]);
			}
			int32_t mchs=std::find(m.begin(),m.end(),mk)-m.begin();
			if(mchs==m.size()){
				m.push_back(mk);
				n.push_back(LOGZERO);
				f.push_back(shape);
			}
			LogAdd(w,weight);
			LogAdd(n[mchs],weight);
			for(int32_t i=0;i<r.size();++i)
				LogAdd(f[mchs][i][p[k][i]],weight);
		}
		p.resize(0);
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
	std::vector<std::vector<std::vector<double>>> nf;
	//log(number of possible cases)
	double nw=LOGZERO;

	bool retv=false;
	int32_t n1=r.size(),n2=R.r.size();
	r.resize(n1+n2);
	for(int32_t i=0;i<n2;++i)r[n1+i].swap(R.r[i]);
	std::vector<std::vector<double>> shape(n1+n2);
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
		std::set<int32_t> qtemp;
		Region finished;
		// Find a suitable starting point
		if(uncovered.empty())break;
		qtemp.insert(*uncovered.begin());
		uncovered.erase(uncovered.begin());

		while(!qtemp.empty()){
			std::vector<int32_t> curscell(1,*qtemp.begin());
			int32_t i1=Geti(curscell[0]),j1=Getj(curscell[0]);
			qtemp.erase(qtemp.begin());

			//Find all connecting cells
			for(int32_t i2=i1-2;i2<=i1+2;++i2)for(int32_t j2=j1-2;j2<=j1+2;++j2)if(Board(i2,j2)==-2){
				int32_t pij=Pair(i2,j2);
				bool inunc=uncovered.find(pij)!=uncovered.end(),inqtemp=false;
				if(!inunc){
					inqtemp=qtemp.find(pij)!=qtemp.end();
					if(!inqtemp)continue;
				}

				bool isconnect=false,issuper=true;
				int32_t
					mini=(i1<i2?i1:i2)-1,minj=(j1<j2?j1:j2)-1,
					maxi=(i1<i2?i2:i1)+1,maxj=(j1<j2?j2:j1)+1;
				if(mini<0)mini=0;
				if(minj<0)minj=0;
				if(maxi>=height)maxi=height-1;
				if(maxj>=width)maxj=width-1;
				for(int32_t ci=mini;ci<=maxi;++ci)for(int32_t cj=minj;cj<=maxj;++cj)if(Board(ci,cj)>0){
					bool isl=(abs(ci-i1)<=1&&abs(cj-j1)<=1),isr=(abs(ci-i2)<=1&&abs(cj-j2)<=1);
					if(isl&&isr)isconnect=true;
					else if(isl||isr)issuper=false;
				}

				if(isconnect){
					if(inunc){
						uncovered.erase(pij);
						if(!issuper)qtemp.insert(pij);
					}
					if(issuper){
						if(!inunc)qtemp.erase(pij);
						curscell.push_back(pij);
					}
				}
			}
			finished.r.push_back(curscell);
		}
		result.push_back(finished);
	}
}
bool MSolve(int32_t _width,int32_t _height,int32_t _mines,const int8_t *_board,int32_t *prob){
	if(_width<=0||_height<=0||_mines<0||_width>=32768||_height>=32768)return false;
	int32_t boardsize=_width*_height;
	if(boardsize>width*height){
		if(board)delete[] board;
		board=new int8_t[boardsize];
	}
	for(int32_t i=0;i<boardsize;++i){
		if(_board[i]>=0&&_board[i]<9)board[i]=_board[i];
		else board[i]=-2;
	}
	width=_width;
	height=_height;
	mines=_mines;

	if(!BasicSolver())return false;

	std::vector<int32_t> border,other;
	std::vector<Region> regions;
	int32_t unknowns=0;
	for(int32_t i=0;i<height;++i)for(int32_t j=0;j<width;++j){
		int32_t numb=Board(i,j);
		if(numb==-3)--mines;
		else if(numb==-2){
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

		if(allw<-1)return false;

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
		if(board[i]==-3)prob[i]=MAXPROB;
		else if(board[i]>=-1)prob[i]=0;
	}
	return true;
}
