#include<cstdlib>
#include<cstdio>
#include<cstdint>
#include<ctime>
#include<queue>
#include"random.h"
#include"MSolver.h"
#define NOMINMAX
#include<windows.h>
#include<windowsx.h>
#include"defines.h"
#include"resource.h"
const char config_file_name[]="Minesweeper.config";
const TCHAR default_name[]=TEXT("ÄäÃû");
static struct{
	int w,h,m,tbeg,tint,texp,mark,probtip,bestmax,interval,single;
	TCHAR begname[MS_MAX_NAME],intname[MS_MAX_NAME],expname[MS_MAX_NAME];
	int zeroend;
} config;
static int bsize=0,game_type;
static INT8 *board=NULL,*chocoboard=NULL,*backboard=NULL,*showboard=NULL;
static int32_t *fboard=NULL;
static HDC mskin,mprob,mtip,mdc;
static bool lb_down=false,lb_downf=false,lb_downb=false,rb_down=false;
static bool fvalid=false,showprob=false,choco=false,chocoplaying=false,endgame=false,autogame=false,cheated=false;
static bool layout=false,rehide=false;
static int layoutmines=0,wheeldeltam=1;
static COLORREF curskin=MS_NORMAL_NUM;
static int bestvalid,bestx,besty,lastx,lasty;
static int cur_mx,cur_my,curmines,curfrees,game_state,wintime,rectype;
static int showmines,showtimer,shownumw,showface;
static LARGE_INTEGER pfrq,psknt,pknt;
static HINSTANCE hInst;
static HWND hWnd;

template<typename T>
T &On(T *b,int x,int y){
	static T oor;
	if(x<0||y<0||x>=config.w||y>=config.h){
		oor=MS_OUT_OF_RANGE;
		return oor;
	}
	return b[x+config.w*y];
}
int CountFlagAround(INT8 *b,int x,int y){
	return
		 (On(b,x-1,y-1)==MS_FLAG)
		+(On(b,x-1,y  )==MS_FLAG)
		+(On(b,x-1,y+1)==MS_FLAG)
		+(On(b,x  ,y-1)==MS_FLAG)
		+(On(b,x  ,y+1)==MS_FLAG)
		+(On(b,x+1,y-1)==MS_FLAG)
		+(On(b,x+1,y  )==MS_FLAG)
		+(On(b,x+1,y+1)==MS_FLAG);
}
void LongPaint(int dstx,int dsty,int nx,int ny,int srcx,int srcy,int cx,int cy){
	if(nx<=0||ny<=0||cx<=0||cy<=0)return;
	BitBlt(mdc,dstx,dsty,cx,cy,mskin,srcx,srcy,SRCCOPY);
	if(nx){
		int ix=1;
		while(nx>ix){
			int nix=2*ix;
			if(nix>nx)nix=nx;
			BitBlt(mdc,dstx+ix*cx,dsty,cx*(nix-ix),cy,mdc,dstx,dsty,SRCCOPY);
			ix=nix;
		}
	}
	if(ny){
		int iy=1;
		while(ny>iy){
			int niy=2*iy;
			if(niy>ny)niy=ny;
			BitBlt(mdc,dstx,dsty+iy*cy,cx*nx,cy*(niy-iy),mdc,dstx,dsty,SRCCOPY);
			iy=niy;
		}
	}
}
void ShortPaint(int dstx,int dsty,int srcx,int srcy,int cx,int cy){
	BitBlt(mdc,dstx,dsty,cx,cy,mskin,srcx,srcy,SRCCOPY);
}
void PaintFrame(){
	int lw=MS_CELL_METRIC*config.w,lh=MS_CELL_METRIC*config.h;
	int x0=0,x1=x0+MS_BORDER_LEFT,x2=x1+lw;
	int y0=0,y1=y0+MS_BORDER_TOP,y2=y1+MS_HEADER_CY,y3=y2+MS_BORDER_MID,y4=y3+lh;

	ShortPaint(x0,y0,MS_FRAME_SHX1,MS_FRAME_SHY1,MS_BORDER_LEFT,MS_BORDER_TOP);
	ShortPaint(x2,y0,MS_FRAME_SHX3,MS_FRAME_SHY1,MS_BORDER_RIGHT,MS_BORDER_TOP);
	ShortPaint(x0,y2,MS_FRAME_SHX1,MS_FRAME_SHY3,MS_BORDER_LEFT,MS_BORDER_MID);
	ShortPaint(x2,y2,MS_FRAME_SHX3,MS_FRAME_SHY3,MS_BORDER_RIGHT,MS_BORDER_MID);
	ShortPaint(x0,y4,MS_FRAME_SHX1,MS_FRAME_SHY5,MS_BORDER_LEFT,MS_BORDER_BOTTOM);
	ShortPaint(x2,y4,MS_FRAME_SHX3,MS_FRAME_SHY5,MS_BORDER_RIGHT,MS_BORDER_BOTTOM);

	LongPaint(x1,y0,lw,1,MS_FRAME_SHX2,MS_FRAME_SHY1,1,MS_BORDER_TOP);
	LongPaint(x1,y2,lw,1,MS_FRAME_SHX2,MS_FRAME_SHY3,1,MS_BORDER_MID);
	LongPaint(x1,y4,lw,1,MS_FRAME_SHX2,MS_FRAME_SHY5,1,MS_BORDER_BOTTOM);
	LongPaint(x0,y1,1,MS_HEADER_CY,MS_FRAME_SHX1,MS_FRAME_SHY2,MS_BORDER_LEFT,1);
	LongPaint(x2,y1,1,MS_HEADER_CY,MS_FRAME_SHX3,MS_FRAME_SHY2,MS_BORDER_RIGHT,1);
	LongPaint(x0,y3,1,lh,MS_FRAME_SHX1,MS_FRAME_SHY4,MS_BORDER_LEFT,1);
	LongPaint(x2,y3,1,lh,MS_FRAME_SHX3,MS_FRAME_SHY4,MS_BORDER_RIGHT,1);

	LongPaint(x1,y1,lw,MS_HEADER_CY,MS_FRAME_SHX7,MS_FRAME_SHY1,1,1);

}
int GetTotalMines(){
	return layout?layoutmines:config.m;
}
void ClearBoard(){
	game_state=MS_WAITING;
	curmines=config.m;
	layoutmines=config.m;
	curfrees=bsize;
	memset(board,MS_UNKNOWN,bsize);
	memset(backboard,MS_UNINITIALIZED,bsize);
	memset(showboard,MS_UNINITIALIZED,bsize);
	showmines=bsize;
	showtimer=MS_MAX_TIMER+1;
	shownumw=0;
	showface=MS_FACE_INVALID;
	fvalid=false;
	bestvalid=MS_INVALIDBEST;
	endgame=false;
	CheckMenuItem(GetMenu(hWnd),ID_FENDGAME,MF_UNCHECKED);
	cheated=false;
	chocoplaying=false;
	PaintFrame();
	InvalidateRect(hWnd,NULL,FALSE);
}
void ChangeSkinColor(COLORREF colormask){
	if(curskin==colormask)return;
	for(int y=MS_NUMBER_SHY;y<MS_NUMBER_SHY+MS_NUMBER_CY;++y)
		for(int x=0;x<MS_NUMBER_SHXK*11;++x){
		COLORREF c=GetPixel(mskin,x,y);
		if(!c)continue;
		UCHAR r=GetRValue(c);
		UCHAR g=GetGValue(c);
		UCHAR b=GetBValue(c);
		UCHAR a=r|g|b;
		c=RGB(a,a,a)&colormask;
		SetPixel(mskin,x,y,c);
	}
	curskin=colormask;
	showmines=bsize;
	showtimer=MS_MAX_TIMER;
}
void LayoutUpdate(){
	fvalid=false;
	game_state=MS_WAITING;
}
void StopAutoGame(){
	bestvalid=MS_INVALIDBEST;
	endgame=false;
	CheckMenuItem(GetMenu(hWnd),ID_FENDGAME,MF_UNCHECKED);
	autogame=false;
	CheckMenuItem(GetMenu(hWnd),ID_FAUTO,MF_UNCHECKED);
}
int MaxChocoMines(){
	// equations are fit from benchmarks to ensure ( in 99% cases )
	// maximum time used to generate chocolate game does not exceed 1.5 s.
	double x=log((double)config.h);
	double y=log((double)config.w);
	double s=x+y,x2=x*x,y2=y*y;
	const double A=+1.2994885419624760;
	const double B=+0.4145579744285156;
	const double C=+0.0823501516492460;
	const double D=-0.0080262639396200;
	const double E=+0.0103488860309975;
	const double F=+0.0129018702933898;
	return floor(exp(A+B*x*y+(C+D*s*s)*s+E*(x2+y2)+F*(x*x2+y*y2)));
}
void StopWaiting(){
	if(game_state==MS_WAITING&&!layout){
		game_state=MS_PLAYING;
		QueryPerformanceCounter(&psknt);
		pknt.QuadPart=psknt.QuadPart;
	}
}
bool GetFreq(){
	if(game_state!=MS_PLAYING&&!layout)return false;
	if(!fvalid){
		if(fvalid=MSolve(config.w,config.h,GetTotalMines(),board,fboard))cheated=true;
	}
	return fvalid;
}

void SaveConfig(){
	FILE *configfp=fopen(config_file_name,"wb");
	fwrite(&config,sizeof(config),1,configfp);
	fclose(configfp);
}
void Config(){
	if(config.w<MS_MIN_W)config.w=MS_MIN_W;
	if(config.w>MS_MAX_W)config.w=MS_MAX_W;
	if(config.h<MS_MIN_H)config.h=MS_MIN_H;
	if(config.h>MS_MAX_H)config.h=MS_MAX_H;

	int nclw,nclh,maxw,maxh;
	RECT rcWindow,rcClient;
	GetWindowRect(hWnd,&rcWindow);
	GetClientRect(hWnd,&rcClient);
	nclw=(rcWindow.right-rcWindow.left)-(rcClient.right-rcClient.left);
	nclh=(rcWindow.bottom-rcWindow.top)-(rcClient.bottom-rcClient.top);
	maxw=(GetSystemMetrics(SM_CXSCREEN)-nclw-MS_NBOARD_CX)/MS_CELL_METRIC;
	maxh=(GetSystemMetrics(SM_CYSCREEN)-nclh-MS_NBOARD_CY)/MS_CELL_METRIC;
	if(config.w>maxw)config.w=maxw;
	if(config.h>maxh)config.h=maxh;
	bsize=config.w*config.h;
	if(config.m<=0)config.m=1;
	if(config.m>=bsize)config.m=bsize-1;
	if(choco){
		int mcm=MaxChocoMines();
		if(config.m>mcm)config.m=mcm;
	}
	SaveConfig();

	if(board){
		delete[] board;
		delete[] chocoboard;
		delete[] backboard;
		delete[] fboard;
		delete[] showboard;
	}
	board=new INT8[bsize];
	chocoboard=new INT8[bsize];
	backboard=new INT8[bsize];
	showboard=new INT8[bsize];
	fboard=new int32_t[bsize];

	HMENU hMenu=GetMenu(hWnd);
	     if(config.w==MS_BEG_W&&config.h==MS_BEG_H&&config.m==MS_BEG_M)game_type=MS_BEG;
	else if(config.w==MS_INT_W&&config.h==MS_INT_H&&config.m==MS_INT_M)game_type=MS_INT;
	else if(config.w==MS_EXP_W&&config.h==MS_EXP_H&&config.m==MS_EXP_M)game_type=MS_EXP;
	else game_type=MS_CUSTOM;
	CheckMenuItem(hMenu,ID_GBEG,game_type==MS_BEG?MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(hMenu,ID_GINT,game_type==MS_INT?MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(hMenu,ID_GEXP,game_type==MS_EXP?MF_CHECKED:MF_UNCHECKED);
	//EnableMenuItem(hMenu,ID_FCHOCO,game_type==MS_CUSTOM?MF_DISABLED:MF_ENABLED);

	int cw=config.w*MS_CELL_METRIC+MS_NBOARD_CX;
	int ch=config.h*MS_CELL_METRIC+MS_NBOARD_CY;

	MoveWindow(hWnd,rcWindow.left,rcWindow.top,
		cw+nclw,
		ch+nclh,
		TRUE);
	HDC hdc=GetDC(hWnd);
	DeleteObject(SelectObject(mdc,CreateCompatibleBitmap(hdc,cw,ch)));
	ReleaseDC(hWnd,hdc);
	ClearBoard();
}

void WriteTime(TCHAR *pos,int time){
	*--pos=TEXT('0')+time%10;
	time/=10;
	*--pos=TEXT('0')+time%10;
	time/=10;
	*--pos=TEXT('0')+time%10;
	time/=10;
	*--pos=TEXT('.');
	do{
		*--pos=TEXT('0')+time%10;
		time/=10;
	}while(time);
}
void WriteHero(HWND hDlg){
	TCHAR 
		txtbeg[32+MS_MAX_NAME]=TEXT("       9.999Ãë"),
		txtint[32+MS_MAX_NAME]=TEXT("       9.999Ãë"),
		txtexp[32+MS_MAX_NAME]=TEXT("       9.999Ãë");
	WriteTime(txtbeg+12,config.tbeg);
	WriteTime(txtint+12,config.tint);
	WriteTime(txtexp+12,config.texp);
	SetWindowText(GetDlgItem(hDlg,IDC_HERO_BEGTIME),txtbeg);
	SetWindowText(GetDlgItem(hDlg,IDC_HERO_INTTIME),txtint);
	SetWindowText(GetDlgItem(hDlg,IDC_HERO_EXPTIME),txtexp);
	SetWindowText(GetDlgItem(hDlg,IDC_HERO_BEGNAME),config.begname);
	SetWindowText(GetDlgItem(hDlg,IDC_HERO_INTNAME),config.intname);
	SetWindowText(GetDlgItem(hDlg,IDC_HERO_EXPNAME),config.expname);
}
INT_PTR CALLBACK HeroProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam){
	switch(message){
	case WM_INITDIALOG:
		WriteHero(hDlg);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case ID_HERO_RESET:
			config.tbeg=MS_INIT_T;
			config.tint=MS_INIT_T;
			config.texp=MS_INIT_T;
			memcpy(config.begname,default_name,sizeof(default_name));
			memcpy(config.intname,default_name,sizeof(default_name));
			memcpy(config.expname,default_name,sizeof(default_name));
			SaveConfig();
			WriteHero(hDlg);
			break;
		case IDOK:
		case IDCANCEL:
		case IDABORT:
		case IDYES:
		case IDNO:
			EndDialog(hDlg,0);
			break;
		}
		break;
	}
	return 0;
}
void Hero(){
	DialogBox(hInst,MAKEINTRESOURCE(IDD_HERO),hWnd,HeroProc);
}

INT_PTR CALLBACK NewRecProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam){
	switch(message){
	case WM_INITDIALOG:
		{
			HWND hEdit=GetDlgItem(hDlg,IDC_NEWREC_EDIT);
			TCHAR txtrec[]=TEXT("ÒÑÆÆÉ¶¼¶¼ÇÂ¼¡£\nÇëÁô×ðÐÕ´óÃû¡£");
			switch(rectype){
			case MS_NEWREC_BEG:
				memcpy(txtrec+2,TEXT("³õ"),sizeof(TCHAR));
				SetWindowText(hEdit,config.begname);
				break;
			case MS_NEWREC_INT:
				memcpy(txtrec+2,TEXT("ÖÐ"),sizeof(TCHAR));
				SetWindowText(hEdit,config.intname);
				break;
			case MS_NEWREC_EXP:
				memcpy(txtrec+2,TEXT("¸ß"),sizeof(TCHAR));
				SetWindowText(hEdit,config.expname);
				break;
			}
			SetWindowText(GetDlgItem(hDlg,IDC_NEWREC_TEXT),txtrec);
			Edit_SetSel(hEdit,0,-1);
			SetFocus(hEdit);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
		case IDCANCEL:
		case IDABORT:
		case IDYES:
		case IDNO:
			if(rectype==MS_NEWREC_BEG){
				GetWindowText(GetDlgItem(hDlg,IDC_NEWREC_EDIT),config.begname,MS_MAX_NAME);
				config.tbeg=wintime;
			}
			else if(rectype==MS_NEWREC_INT){
				GetWindowText(GetDlgItem(hDlg,IDC_NEWREC_EDIT),config.intname,MS_MAX_NAME);
				config.tint=wintime;
			}
			else if(rectype==MS_NEWREC_EXP){
				GetWindowText(GetDlgItem(hDlg,IDC_NEWREC_EDIT),config.expname,MS_MAX_NAME);
				config.texp=wintime;
			}
			SaveConfig();
			EndDialog(hDlg,0);
			Hero();
			break;
		}
		break;
	}
	return 0;
}
void WinGame(){
	QueryPerformanceCounter(&pknt);
	wintime=(pknt.QuadPart-psknt.QuadPart)*1000/pfrq.QuadPart+1;
	if(wintime>MS_INIT_T)wintime=MS_INIT_T;
	if(cheated)return;
	rectype=FALSE;
	if(game_type==MS_BEG&&wintime<config.tbeg)rectype=MS_NEWREC_BEG;
	if(game_type==MS_INT&&wintime<config.tint)rectype=MS_NEWREC_INT;
	if(game_type==MS_EXP&&wintime<config.texp)rectype=MS_NEWREC_EXP;

	if(rectype)DialogBox(hInst,MAKEINTRESOURCE(IDD_NEWREC),hWnd,NewRecProc);
}

TCHAR *IntToTstr(int x,TCHAR *ptstr){
	*--ptstr=0;
	do{
		*--ptstr=TEXT('0')+x%10;
		x/=10;
	}while(x);
	return ptstr;
}
int TstrToInt(const TCHAR *ptstr){
	int x=0;
	while(*ptstr){
		x*=10;
		x+=*ptstr-TEXT('0');
		++ptstr;
	}
	return x;
}
INT_PTR CALLBACK CustomProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam){
	TCHAR buf[16];
	switch(message){
	case WM_INITDIALOG:
		{
			SetWindowText(GetDlgItem(hDlg,IDC_CUSTOM_H),IntToTstr(config.h,buf+16));
			SetWindowText(GetDlgItem(hDlg,IDC_CUSTOM_W),IntToTstr(config.w,buf+16));
			SetWindowText(GetDlgItem(hDlg,IDC_CUSTOM_M),IntToTstr(config.m,buf+16));
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
		case IDYES:
			GetWindowText(GetDlgItem(hDlg,IDC_CUSTOM_H),buf,8);
			config.h=TstrToInt(buf);
			GetWindowText(GetDlgItem(hDlg,IDC_CUSTOM_W),buf,8);
			config.w=TstrToInt(buf);
			GetWindowText(GetDlgItem(hDlg,IDC_CUSTOM_M),buf,8);
			config.m=TstrToInt(buf);
			Config();
		case IDCANCEL:
		case IDABORT:
		case IDNO:
			EndDialog(hDlg,0);
			break;
		}
		break;
	}
	return 0;
}
void CustConfig(){
	DialogBox(hInst,MAKEINTRESOURCE(IDD_CUSTOM),hWnd,CustomProc);
}

INT_PTR CALLBACK FSetProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam){
	TCHAR buf[16];
	switch(message){
	case WM_INITDIALOG:
		{
			CheckDlgButton(hDlg,IDC_FSET_CHECK,config.single?BST_UNCHECKED:BST_CHECKED);
			SetWindowText(GetDlgItem(hDlg,IDC_FSET_EDIT),IntToTstr(config.bestmax,buf+16));
			SetWindowText(GetDlgItem(hDlg,IDC_FSET_EDIT2),IntToTstr(config.interval,buf+16));
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
		case IDYES:
			GetWindowText(GetDlgItem(hDlg,IDC_FSET_EDIT),buf,8);
			config.bestmax=TstrToInt(buf);
			if(config.bestmax>MS_BESTSTEP_MMAX)config.bestmax=MS_BESTSTEP_MMAX;
			if(config.bestmax<MS_BESTSTEP_MMIN)config.bestmax=MS_BESTSTEP_MMIN;
			GetWindowText(GetDlgItem(hDlg,IDC_FSET_EDIT2),buf,8);
			config.interval=TstrToInt(buf);
			if(config.interval>MS_INTERVAL_MAX)config.interval=MS_INTERVAL_MAX;
			if(config.interval<MS_INTERVAL_MIN)config.interval=MS_INTERVAL_MIN;
			config.single=(IsDlgButtonChecked(hDlg,IDC_FSET_CHECK)==BST_UNCHECKED);
			SaveConfig();
			bestvalid=MS_INVALIDBEST;
		case IDCANCEL:
		case IDABORT:
		case IDNO:
			EndDialog(hDlg,0);
			break;
		}
		break;
	}
	return 0;
}
void FSet(){
	DialogBox(hInst,MAKEINTRESOURCE(IDD_FSET),hWnd,FSetProc);
}

INT_PTR CALLBACK AboutProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam){
	if(message==WM_COMMAND)EndDialog(hDlg,0);
	return 0;
}
void About(){
	DialogBox(hInst,MAKEINTRESOURCE(IDD_ABOUT),hWnd,AboutProc);
}
BOOL Initialize(){
	HDC hdc=GetDC(hWnd);
	mskin=CreateCompatibleDC(hdc);
	mprob=CreateCompatibleDC(hdc);
	mtip=CreateCompatibleDC(hdc);
	mdc=CreateCompatibleDC(hdc);
	ReleaseDC(hWnd,hdc);
	SelectObject(mskin,LoadBitmap(hInst,MAKEINTRESOURCE(IDB_SKIN)));
	SelectObject(mprob,LoadBitmap(hInst,MAKEINTRESOURCE(IDB_PROB)));
	SelectObject(mtip,LoadBitmap(hInst,MAKEINTRESOURCE(IDB_TIP)));

	cur_mx=cur_my=0;
	lastx=lasty=0;
	QueryPerformanceFrequency(&pfrq);
	seedrandom(time(NULL));

	bool badconfig=false;
	FILE *configfp=fopen(config_file_name,"rb");
	if(!configfp)badconfig=true;
	else{
		if(fread(&config,sizeof(config),1,configfp)!=1)badconfig=true;
		fclose(configfp);
	}
	if(config.zeroend)badconfig=true;
	else if(config.w<MS_MIN_W||config.w>MS_MAX_W)badconfig=true;
	else if(config.h<MS_MIN_H||config.h>MS_MAX_H)badconfig=true;
	else if(config.m<=0||config.m>=config.w*config.h)badconfig=true;
	else if(config.tbeg<=0||config.tbeg>MS_INIT_T)badconfig=true;
	else if(config.tint<=0||config.tint>MS_INIT_T)badconfig=true;
	else if(config.texp<=0||config.texp>MS_INIT_T)badconfig=true;
	else if(config.mark!=TRUE&&config.mark!=FALSE)badconfig=true;
	else if(config.probtip!=TRUE&&config.probtip!=FALSE)badconfig=true;
	else if(config.bestmax<MS_BESTSTEP_MMIN||config.bestmax>MS_BESTSTEP_MMAX)badconfig=true;
	else if(config.interval<MS_INTERVAL_MIN||config.interval>MS_INTERVAL_MAX)badconfig=true;
	else if(config.single!=TRUE&&config.single!=FALSE)badconfig=true;
	if(!badconfig){
		int nlbeg=lstrlen(config.begname);
		if(nlbeg<=0||nlbeg>=MS_MAX_NAME)badconfig=true;
	}
	if(!badconfig){
		int nlint=lstrlen(config.intname);
		if(nlint<=0||nlint>=MS_MAX_NAME)badconfig=true;
	}
	if(!badconfig){
		int nlexp=lstrlen(config.expname);
		if(nlexp<=0||nlexp>=MS_MAX_NAME)badconfig=true;
	}

	if(badconfig){
		config.w=MS_BEG_W;
		config.h=MS_BEG_H;
		config.m=MS_BEG_M;
		config.tbeg=MS_INIT_T;
		config.tint=MS_INIT_T;
		config.texp=MS_INIT_T;
		config.mark=FALSE;
		config.probtip=TRUE;
		config.bestmax=sqrt((double)MS_BESTSTEP_MMIN*MS_BESTSTEP_MMAX);
		config.interval=sqrt((double)MS_INTERVAL_MIN*MS_INTERVAL_MAX);
		config.single=FALSE;
		memcpy(config.begname,default_name,sizeof(default_name));
		memcpy(config.intname,default_name,sizeof(default_name));
		memcpy(config.expname,default_name,sizeof(default_name));
		config.zeroend=0;
	}

	Config();
	return 1;
}
int Translate(int mx,int my){
	int lw=MS_CELL_METRIC*config.w,lh=MS_CELL_METRIC*config.h;
	int bx=MS_BORDER_LEFT,by=MS_BORDER_TOP+MS_HEADER_CY+MS_BORDER_MID;
	if(mx>=bx&&mx<bx+lw&&my>=by&&my<by+lh)
		return MS_PAIR((mx-bx)/MS_CELL_METRIC,(my-by)/MS_CELL_METRIC);

	int fx=MS_BORDER_LEFT+(lw-MS_FACE_METRIC)/2;
	int fy=MS_BORDER_TOP+MS_HEADER_SHT;
	if(mx>=fx&&mx<fx+MS_FACE_METRIC&&my>=fy&&my<fy+MS_FACE_METRIC)
		return MS_ONFACE;

	int minescrx=MS_BORDER_LEFT+MS_NUMSCRL_SHL+1;
	int hy=MS_BORDER_TOP+MS_HEADER_SHT+1;
	if(mx>=minescrx&&mx<minescrx+shownumw*MS_NUMSCR_CXK&&my>=hy&&my<hy+MS_NUMSCR_CY-2){
		const int deltam[]={1,10,100,1000,10000};
		wheeldeltam=deltam[shownumw-1-(mx-minescrx)/MS_NUMSCR_CXK];
		return MS_ONMINE;
	}

	return MS_ONELSE;
}
void CheckGame(){
	fvalid=false;
	bestvalid=MS_INVALIDBEST;
	if(game_state==MS_LOSE){
		for(int i=0;i<bsize;++i){
			int bnum=board[i];
			if(bnum==MS_UNKNOWN||bnum==MS_MARK){
				if(chocoplaying?fboard[i]:backboard[i]==MS_FLAG)board[i]=MS_MINE;
			}
			if(bnum==MS_FLAG){
				if(chocoplaying?fboard[i]!=MS_PROB_MAX:backboard[i]!=MS_FLAG)board[i]=MS_MISFLAG;
			}
		}
	}
	else if(curfrees==config.m){
		game_state=MS_WIN;
		for(int i=0;i<bsize;++i)
			if(board[i]==MS_UNKNOWN||board[i]==MS_MARK){
				board[i]=MS_FLAG;
				--curmines;
			}
		WinGame();
	}
}
void KeyDownEvent(WPARAM wParam){
	switch(wParam){
	case VK_F2:
		PostMessage(hWnd,WM_COMMAND,ID_GSTART,0);
		break;
	case VK_F6:
		PostMessage(hWnd,WM_COMMAND,ID_FPROB,0);
		break;
	}
	if(layout){
		int pos=Translate(cur_mx,cur_my);
		int x=MS_GETX(pos),y=MS_GETY(pos);
		INT8 &bxy=On(board,x,y);
		if(wParam==VK_OEM_3
			||wParam<=0x38&&wParam>=0x30
			||wParam<=0x68&&wParam>=0x60){
			curmines+=bxy==MS_FLAG;
			bxy=wParam&0xF;
			LayoutUpdate();
		}
		else switch(wParam){
		case VK_SPACE:
		case VK_OEM_MINUS:
		case VK_SUBTRACT:
			curmines+=bxy==MS_FLAG;
			bxy=MS_UNKNOWN;
			LayoutUpdate();
			break;
		case 0x46://F
		case 0x39:
		case 0x69:
			curmines-=bxy!=MS_FLAG;
			bxy=MS_FLAG;
			break;
		}
	}
	else{
		switch(wParam){
		case VK_F4:
			PostMessage(hWnd,WM_COMMAND,ID_FAUTO,0);
			break;
		case VK_F12:
			PostMessage(hWnd,WM_COMMAND,ID_FENDGAME,0);
			break;
		case VK_F11:
			PostMessage(hWnd,WM_COMMAND,ID_FBEST,0);
			break;
		}
	}
}
void PaintEvent(){
	HDC hdc=GetDC(hWnd);
	int lw=MS_CELL_METRIC*config.w,lh=MS_CELL_METRIC*config.h;
	int x0=0,x1=x0+MS_BORDER_LEFT,x2=x1+lw;
	int y0=0,y1=y0+MS_BORDER_TOP,y2=y1+MS_HEADER_CY,y3=y2+MS_BORDER_MID,y4=y3+lh;
	bool drawprob=showprob&&(layout||backboard[0]!=MS_UNINITIALIZED);
	bool autoclick=(bestvalid==MS_PRESSBEST);
	if(drawprob)drawprob=GetFreq();

	RECT ivld;
	ivld.top=lh+MS_NBOARD_CY;
	ivld.bottom=0;
	ivld.left=lw+MS_NBOARD_CX;
	ivld.right=0;
	int curpos=Translate(cur_mx,cur_my);
	int curx=MS_ONELSE,cury=MS_ONELSE;
	if(MS_ONBOARD(curpos)){
		curx=MS_GETX(curpos);
		cury=MS_GETY(curpos);
	}

	int minf=MS_PROB_MAX+1;
	if(drawprob||autoclick){
		for(int i=0;i<bsize;++i)if(MS_ISFUNC(board[i])&&fboard[i]<minf){
			minf=fboard[i];
			if(minf==0){
				minf=MS_PROB_MAX+1;
				break;
			}
		}
	}
	if(minf==MS_PROB_MAX)++minf;

	for(int x=0;x<config.w;++x)for(int y=0;y<config.h;++y){
		bool bestchs=false;
		int cell=On(board,x,y),cellf=On(fboard,x,y),showcell=cell;
		if(MS_ISFUNC(cell)){
			if(game_state==MS_WAITING||game_state==MS_PLAYING){
				bool isdblcd=(lb_down&&rb_down&&abs(curx-x)<=1&&abs(cury-y)<=1);
				bool issinglecd=(lb_downb&&curx==x&&cury==y)&&!(rehide&&layout);
				bool isautocd=autoclick;
				if(isautocd){
					isautocd=(x==bestx&&y==besty);
					if(!config.single)
						isautocd=(minf<=MS_PROB_MAX&&isautocd||cellf==0);
				}
				if(isautocd&&cell==MS_FLAG){
					++curmines;
					On(board,x,y)=cell=MS_UNKNOWN;
				}
				if(isdblcd||issinglecd||isautocd){
					if(cell==MS_UNKNOWN)cell=MS_PRESS;
					if(cell==MS_MARK)cell=MS_MARKPRESS;
				}
			}
			if(drawprob){
				bestchs=(cellf==minf);
				if(cellf==0)cellf=0;
				else if(cellf==MS_PROB_MAX)cellf=MS_PROB_DRAWMAX;
				else cellf=1+cellf/(1+MS_PROB_MAX/(MS_PROB_DRAWMAX-1));
				if(bestchs)cellf+=MS_PROB_DRAWMAX;
				showcell=cell+MS_CELLFUNC_NUM+cellf*MS_CELLPROB_NUM;
			}
			else showcell=cell;
		}
		if((INT8)showcell!=On(showboard,x,y)){
			On(showboard,x,y)=showcell;
			int shpx=x1+x*MS_CELL_METRIC,shpy=y3+y*MS_CELL_METRIC;
			if(!MS_ISFUNC(cell))
				BitBlt(mdc,shpx,shpy,MS_CELL_METRIC,MS_CELL_METRIC,
					mskin,cell*MS_CELL_SHXK,0,SRCCOPY);
			else if(drawprob)
				BitBlt(mdc,shpx,shpy,MS_CELL_METRIC,MS_CELL_METRIC,
					mprob,(cell-MS_CELLNUM_NUM)*MS_CELL_SHXK,cellf*MS_CELL_SHYK,SRCCOPY);
			else
				BitBlt(mdc,shpx,shpy,MS_CELL_METRIC,MS_CELL_METRIC,
					mskin,(cell-MS_CELLNUM_NUM)*MS_CELL_SHXK,MS_CELL_SHYK,SRCCOPY);
			if(shpx<ivld.left)ivld.left=shpx;
			if(shpx+MS_CELL_METRIC>ivld.right)ivld.right=shpx+MS_CELL_METRIC;
			if(shpy<ivld.top)ivld.top=shpy;
			if(shpy+MS_CELL_METRIC>ivld.bottom)ivld.bottom=shpy+MS_CELL_METRIC;
		}
	}
	if(ivld.bottom>ivld.top&&ivld.right>ivld.left){
		BitBlt(
			hdc,ivld.left,ivld.top,
			ivld.right-ivld.left,ivld.bottom-ivld.top,
			mdc,ivld.left,ivld.top,SRCCOPY);
	}

	static int tipposx,tipposy;
	static bool tipvalid=false;
	if(tipvalid){
		BitBlt(hdc,tipposx,tipposy,MS_TIP_CX,MS_TIP_CY,mdc,tipposx,tipposy,SRCCOPY);
		tipvalid=false;
	}
	if(drawprob&&config.probtip){
		if(MS_ONBOARD(curpos)&&MS_ISFUNC(On(board,curx,cury)))
			tipvalid=(On(fboard,curx,cury)>0&&On(fboard,curx,cury)<MS_PROB_MAX);
	}
	if(tipvalid){
		int tipval=On(fboard,curx,cury)/(MS_PROB_MAX/10000);
		if(tipval<=0)tipval=1;
		if(tipval>=10000)tipval=9999;
		int curcx=GetSystemMetrics(SM_CXCURSOR)/2,curcy=GetSystemMetrics(SM_CYCURSOR)/2;
		tipposx=cur_mx+curcx,tipposy=cur_my+curcy;
		if(tipposx+MS_TIP_CX>MS_NBOARD_CX+lw)tipposx=MS_NBOARD_CX+lw-MS_TIP_CX;
		if(tipposy+MS_TIP_CY>MS_NBOARD_CY+lh)tipposy=cur_my-MS_TIP_CY*2;
		int tnshx=MS_TIPNUM_SHL;
		if(tipval<1000){
			BitBlt(mtip,tnshx,MS_TIPNUM_SHT,
				MS_TIPNUM_CX,MS_TIPNUM_CY,
				mtip,MS_TIP_CX,MS_TIPNUM_SHT,SRCCOPY);
			BitBlt(mtip,tnshx+MS_TIPNUM_CX*5,MS_TIPNUM_SHT,
				MS_TIPNUM_CX,MS_TIPNUM_CY,
				mtip,MS_TIP_CX,MS_TIPNUM_SHT,SRCCOPY);
			tnshx+=MS_TIPNUM_CX/2;
		}
		else {
			BitBlt(mtip,tnshx,MS_TIPNUM_SHT,
				MS_TIPNUM_CX,MS_TIPNUM_CY,
				mtip,tipval/1000*MS_TIPNUM_SHXK,MS_TIPNUM_SHY,SRCCOPY);
			tipval%=1000;
			tnshx+=MS_TIPNUM_CX;
		}
		BitBlt(mtip,tnshx,MS_TIPNUM_SHT,
			MS_TIPNUM_CX,MS_TIPNUM_CY,
			mtip,tipval/100*MS_TIPNUM_SHXK,MS_TIPNUM_SHY,SRCCOPY);
		tipval%=100;
		tnshx+=MS_TIPNUM_CX;
		BitBlt(mtip,tnshx,MS_TIPNUM_SHT,
			MS_TIPNUM_CX,MS_TIPNUM_CY,
			mtip,10*MS_TIPNUM_SHXK,MS_TIPNUM_SHY,SRCCOPY);
		tnshx+=MS_TIPNUM_CX;
		BitBlt(mtip,tnshx,MS_TIPNUM_SHT,
			MS_TIPNUM_CX,MS_TIPNUM_CY,
			mtip,tipval/10*MS_TIPNUM_SHXK,MS_TIPNUM_SHY,SRCCOPY);
		tnshx+=MS_TIPNUM_CX;
		BitBlt(mtip,tnshx,MS_TIPNUM_SHT,
			MS_TIPNUM_CX,MS_TIPNUM_CY,
			mtip,tipval%10*MS_TIPNUM_SHXK,MS_TIPNUM_SHY,SRCCOPY);
		tnshx+=MS_TIPNUM_CX;
		BitBlt(mtip,tnshx,MS_TIPNUM_SHT,
			MS_TIPNUM_CX,MS_TIPNUM_CY,
			mtip,11*MS_TIPNUM_SHXK,MS_TIPNUM_SHY,SRCCOPY);
		BitBlt(hdc,tipposx,tipposy,MS_TIP_CX,MS_TIP_CY,mtip,0,0,SRCCOPY);
	}

	int face=MS_FACE_SMILE;
	if(lb_downb||lb_down&&rb_down||autoclick)face=MS_FACE_OH;
	if(game_state==MS_WIN)face=MS_FACE_WIN;
	if(game_state==MS_LOSE)face=MS_FACE_LOSE;
	if(curpos==MS_ONFACE&&lb_downf)face=MS_FACE_DOWN;
	
	int fx=(lw-MS_FACE_METRIC)/2,hy=MS_BORDER_TOP+MS_HEADER_SHT;
	if(face!=showface){
		showface=face;
		ShortPaint(MS_BORDER_LEFT+fx,hy,MS_FACE_SHX(face),MS_FACE_SHY,MS_FACE_METRIC,MS_FACE_METRIC);
		BitBlt(hdc,MS_BORDER_LEFT+fx,hy,MS_FACE_METRIC,MS_FACE_METRIC,mdc,MS_BORDER_LEFT+fx,hy,SRCCOPY);
	}
	int minenum=curmines,timer=0;
	if(game_state!=MS_WAITING){
		timer=1+(pknt.QuadPart-psknt.QuadPart)/pfrq.QuadPart;
	}
	int maxnum=(fx-MS_NUMSCRL_SHL*2-2)/MS_NUMSCR_CXK,maxnumwr=(fx-MS_NUMSCRR_SHR*2-2)/MS_NUMSCR_CXK;
	if(maxnum>maxnumwr)maxnum=maxnumwr;
	if(maxnum>MS_MAX_NUMSCR)maxnum=MS_MAX_NUMSCR;
	if(timer>MS_MAX_TIMER)timer=MS_MAX_TIMER;

	int nscrw=3,maxval=999,totalmines=GetTotalMines();
	while((maxval<totalmines||maxval<timer)&&nscrw<maxnum){
		++nscrw;
		maxval=maxval*10+9;
	}
	if(maxval<minenum)minenum=maxval;
	if(maxval/10<-minenum)minenum=-(maxval/10);
	if(maxval<timer)timer=maxval;
	if(timer<0)timer=0;

	int mscrw=std::max(nscrw,shownumw);
	int minescrx=MS_BORDER_LEFT+MS_NUMSCRL_SHL+1;
	int timerscrx2=MS_BORDER_LEFT+lw-MS_NUMSCRR_SHR-1;
	int timerscrx=timerscrx2-mscrw*MS_NUMSCR_CXK;
	if(nscrw!=shownumw){
		shownumw=nscrw;
		showmines=minenum-1;
		showtimer=timer-1;
		ShortPaint(minescrx-1,hy,
			MS_FRAME_SHX4,MS_FRAME_SHY1,1,MS_NUMSCR_CY);
		LongPaint(minescrx,hy,nscrw*MS_NUMSCR_CXK,1,
			MS_FRAME_SHX5,MS_FRAME_SHY1,1,MS_NUMSCR_CY);
		ShortPaint(minescrx+nscrw*MS_NUMSCR_CXK,hy,
			MS_FRAME_SHX6,MS_FRAME_SHY1,1,MS_NUMSCR_CY);
		LongPaint(minescrx+nscrw*MS_NUMSCR_CXK+1,MS_BORDER_TOP,
			(mscrw-nscrw)*MS_NUMSCR_CXK,MS_HEADER_CY,MS_FRAME_SHX7,MS_FRAME_SHY1,1,1);
		ShortPaint(timerscrx2,hy,
			MS_FRAME_SHX6,MS_FRAME_SHY1,1,MS_NUMSCR_CY);
		timerscrx2-=nscrw*MS_NUMSCR_CXK;
		LongPaint(timerscrx2,hy,nscrw*MS_NUMSCR_CXK,1,
			MS_FRAME_SHX5,MS_FRAME_SHY1,1,MS_NUMSCR_CY);
		ShortPaint(timerscrx2-1,hy,
			MS_FRAME_SHX4,MS_FRAME_SHY1,1,MS_NUMSCR_CY);
		LongPaint(timerscrx,MS_BORDER_TOP,
			(mscrw-nscrw)*MS_NUMSCR_CXK,MS_HEADER_CY,MS_FRAME_SHX7,MS_FRAME_SHY1,1,1);
		BitBlt(hdc,minescrx-1,hy,mscrw*MS_NUMSCR_CXK+2,MS_NUMSCR_CY,mdc,minescrx-1,hy,SRCCOPY);
		BitBlt(hdc,timerscrx,hy,mscrw*MS_NUMSCR_CXK+2,MS_NUMSCR_CY,mdc,timerscrx,hy,SRCCOPY);
		timerscrx=timerscrx2;
	}
	int ncshxw,ny=MS_BORDER_TOP+MS_NUMSCR_SHT;
	if(minenum!=showmines){
		showmines=minenum;
		ncshxw=nscrw;
		int stx=minescrx;
		if(minenum<0){
			ShortPaint(minescrx+1,ny,10*MS_NUMBER_SHXK,MS_NUMBER_SHY,MS_NUMBER_CX,MS_NUMBER_CY);
			minenum=-minenum;
			minescrx+=MS_NUMSCR_CXK;
			--ncshxw;
		}
		while(ncshxw){
			--ncshxw;
			ShortPaint(
				minescrx+1+MS_NUMSCR_CXK*ncshxw,ny,
				(minenum%10)*MS_NUMBER_SHXK,MS_NUMBER_SHY,
				MS_NUMBER_CX,MS_NUMBER_CY);
			minenum/=10;
		}
		BitBlt(hdc,stx,ny,MS_NUMSCR_CXK*nscrw,MS_NUMBER_CY,mdc,stx,ny,SRCCOPY);
	}
	if(timer!=showtimer){
		showtimer=timer;
		ncshxw=nscrw;
		while(ncshxw){
			--ncshxw;
			ShortPaint(
				timerscrx+1+MS_NUMSCR_CXK*ncshxw,ny,
				(timer%10)*MS_NUMBER_SHXK,MS_NUMBER_SHY,
				MS_NUMBER_CX,MS_NUMBER_CY);
			timer/=10;
		}
		BitBlt(hdc,timerscrx,ny,MS_NUMSCR_CXK*nscrw,MS_NUMBER_CY,mdc,timerscrx,ny,SRCCOPY);
	}
	ReleaseDC(hWnd,hdc);
}
void MenuEvent(WORD wParam){
	HMENU hMenu;
	switch(wParam){
	case ID_GSTART:
		ClearBoard();
		break;
	case ID_GEXIT:
		PostQuitMessage(0);
		break;
	case ID_GBEG:
		config.w=MS_BEG_W;
		config.h=MS_BEG_H;
		config.m=MS_BEG_M;
		Config();
		break;
	case ID_GINT:
		config.w=MS_INT_W;
		config.h=MS_INT_H;
		config.m=MS_INT_M;
		Config();
		break;
	case ID_GEXP:
		config.w=MS_EXP_W;
		config.h=MS_EXP_H;
		config.m=MS_EXP_M;
		Config();
		break;
	case ID_GCUST:
		CustConfig();
		break;
	case ID_GMARK:
		hMenu=GetMenu(hWnd);
		config.mark=((MF_CHECKED&GetMenuState(hMenu,ID_GMARK,MF_BYCOMMAND))==0);
		CheckMenuItem(hMenu,ID_GMARK,config.mark?MF_CHECKED:MF_UNCHECKED);
		SaveConfig();
		break;
	case ID_FPROB:
		hMenu=GetMenu(hWnd);
		showprob=((MF_CHECKED&GetMenuState(hMenu,ID_FPROB,MF_BYCOMMAND))==0);
		CheckMenuItem(hMenu,ID_FPROB,showprob?MF_CHECKED:MF_UNCHECKED);
	//	SaveConfig();
		break;
	case ID_FBEST:
		if(game_state==MS_PLAYING||game_state==MS_WAITING){
			StopWaiting();
			if(bestvalid<=MS_WAITBEST)bestvalid=MS_INITBEST;
		}
		break;
	case ID_FENDGAME:
		hMenu=GetMenu(hWnd);
		endgame=((MF_CHECKED&GetMenuState(hMenu,ID_FENDGAME,MF_BYCOMMAND))==0);
		CheckMenuItem(hMenu,ID_FENDGAME,endgame?MF_CHECKED:MF_UNCHECKED);
		if(endgame){
			autogame=false;
			CheckMenuItem(hMenu,ID_FAUTO,MF_UNCHECKED);
			StopWaiting();
		}
		break;
	case ID_FAUTO:
		hMenu=GetMenu(hWnd);
		autogame=((MF_CHECKED&GetMenuState(hMenu,ID_FAUTO,MF_BYCOMMAND))==0);
		CheckMenuItem(hMenu,ID_FAUTO,autogame?MF_CHECKED:MF_UNCHECKED);
		if(autogame){
			endgame=false;
			CheckMenuItem(hMenu,ID_FENDGAME,MF_UNCHECKED);
			if(game_state!=MS_PLAYING&&game_state!=MS_WAITING)ClearBoard();
			StopWaiting();
		}
		break;
	case ID_FCHOCO:
		hMenu=GetMenu(hWnd);
		choco=((MF_CHECKED&GetMenuState(hMenu,ID_FCHOCO,MF_BYCOMMAND))==0);
		layout=false;
		CheckMenuItem(hMenu,ID_FCHOCO,choco?MF_CHECKED:MF_UNCHECKED);
		CheckMenuItem(hMenu,ID_FLAYOUT,MF_UNCHECKED);
		ChangeSkinColor(choco?MS_CHOCO_NUM:MS_NORMAL_NUM);
		ClearBoard();
		EnableMenuItem(hMenu,ID_FBEST,MF_ENABLED);
		EnableMenuItem(hMenu,ID_FENDGAME,MF_ENABLED);
		EnableMenuItem(hMenu,ID_FAUTO,MF_ENABLED);
		if(config.m>MaxChocoMines())Config();
		break;
	case ID_FLAYOUT:
		hMenu=GetMenu(hWnd);
		layout=((MF_CHECKED&GetMenuState(hMenu,ID_FLAYOUT,MF_BYCOMMAND))==0);
		CheckMenuItem(hMenu,ID_FLAYOUT,layout?MF_CHECKED:MF_UNCHECKED);
		ChangeSkinColor(layout?MS_LAYOUT_NUM:(choco?MS_CHOCO_NUM:MS_NORMAL_NUM));
		if(layout)StopAutoGame();
		if(game_state!=MS_PLAYING&&!(game_state==MS_WAITING&&layout))ClearBoard();
		EnableMenuItem(hMenu,ID_FBEST,layout?MF_DISABLED:MF_ENABLED);
		EnableMenuItem(hMenu,ID_FENDGAME,layout?MF_DISABLED:MF_ENABLED);
		EnableMenuItem(hMenu,ID_FAUTO,layout?MF_DISABLED:MF_ENABLED);
		break;
	case ID_FVALUE:
		hMenu=GetMenu(hWnd);
		config.probtip=((MF_CHECKED&GetMenuState(hMenu,ID_FVALUE,MF_BYCOMMAND))==0);
		CheckMenuItem(hMenu,ID_FVALUE,config.probtip?MF_CHECKED:MF_UNCHECKED);
		SaveConfig();
		break;
	case ID_FSET:
		FSet();
		break;
	case ID_GHERO:
		Hero();
		break;
	case ID_HABOUT:
		About();
		break;
	}
}
void Discover(int x,int y);
void NewGame(int initx,int inity,bool mustopen=false){
	int ntc=config.m;
	if(ntc>=bsize/2){
		ntc=bsize-1-ntc;
		memset(backboard,MS_FLAG,bsize);
		On(backboard,initx,inity)=MS_UNINITIALIZED;
		if(mustopen){
			for(int idx=-1;idx<=1;++idx)for(int idy=-1;idy<=1;++idy){
				int nx=initx+idx,ny=inity+idy;
				if(On(backboard,nx,ny)==MS_FLAG){
					On(backboard,nx,ny)=MS_UNINITIALIZED;
					--ntc;
				}
			}
		}
		while(ntc){
			int nx=random64()%config.w,ny=random64()%config.h;
			if(On(backboard,nx,ny)==MS_FLAG){
				On(backboard,nx,ny)=MS_UNINITIALIZED;
				--ntc;
			}
		}
	}
	else{
		memset(backboard,MS_UNINITIALIZED,bsize);
		while(ntc){
			int nx=random64()%config.w,ny=random64()%config.h;
			if(On(backboard,nx,ny)==MS_UNINITIALIZED&&(
				mustopen?(abs(nx-initx)>1||abs(ny-inity)>1):(nx!=initx||ny!=inity)
				)){
				On(backboard,nx,ny)=MS_FLAG;
				--ntc;
			}
		}
	}
	for(int x=0;x<config.w;++x)for(int y=0;y<config.h;++y)
		if(On(backboard,x,y)==MS_UNINITIALIZED)
			On(backboard,x,y)=CountFlagAround(backboard,x,y);
}
void NewChocoGame(int initx,int inity){
	do{
		NewGame(initx,inity,true);
	} while(!MSolve(config.w,config.h,backboard,initx,inity));
	chocoplaying=true;
}
void Discover(std::queue<int> &dislist){
	int needchoco=chocoplaying?dislist.size():0;
	bool gamelose=false;
	while(dislist.size()){
		int x=MS_GETX(dislist.front());
		int y=MS_GETY(dislist.front());
		dislist.pop();

		if(On(board,x,y)==MS_UNKNOWN||On(board,x,y)==MS_MARK){
			if(layout){
				On(board,x,y)=CountFlagAround(board,x,y);
				LayoutUpdate();
			}
			else{
			if(needchoco>0&&GetFreq()&&On(fboard,x,y)||On(backboard,x,y)==MS_FLAG){
				On(board,x,y)=MS_BOMB;
				gamelose=true;
			}
			if(On(backboard,x,y)==MS_UNINITIALIZED){
				if(!choco)NewGame(x,y);
				else NewChocoGame(x,y);
				QueryPerformanceCounter(&psknt);
				pknt.QuadPart=psknt.QuadPart;
			}
			--curfrees;
			if(On(board,x,y)!=MS_BOMB&&(On(board,x,y)=On(backboard,x,y))==0){
				dislist.push(MS_PAIR(x-1,y-1));
				dislist.push(MS_PAIR(x-1,y));
				dislist.push(MS_PAIR(x-1,y+1));
				dislist.push(MS_PAIR(x,y-1));
				dislist.push(MS_PAIR(x,y+1));
				dislist.push(MS_PAIR(x+1,y-1));
				dislist.push(MS_PAIR(x+1,y));
				dislist.push(MS_PAIR(x+1,y+1));
			}
		}}
		--needchoco;
	}
	if(gamelose)game_state=MS_LOSE;
}
void Discover(int x,int y){
	std::queue<int> dislist;
	dislist.push(MS_PAIR(x,y));
	Discover(dislist);
}
void DiscoverAround(int x,int y){
	std::queue<int> dislist;
	dislist.push(MS_PAIR(x-1,y-1));
	dislist.push(MS_PAIR(x-1,y  ));
	dislist.push(MS_PAIR(x-1,y+1));
	dislist.push(MS_PAIR(x  ,y-1));
	dislist.push(MS_PAIR(x  ,y+1));
	dislist.push(MS_PAIR(x+1,y-1));
	dislist.push(MS_PAIR(x+1,y  ));
	dislist.push(MS_PAIR(x+1,y+1));
	Discover(dislist);
}
void ClickEvent(int pos,int type){
	if(pos==MS_ONFACE){
		if(type==MS_LCLICK)ClearBoard();
		return;
	}
	
	if(MS_ONBOARD(pos)&&(game_state==MS_WAITING||game_state==MS_PLAYING)){
		int x=MS_GETX(pos),y=MS_GETY(pos);
		if(type==MS_RCLICK){
			if(On(board,x,y)==MS_UNKNOWN){
				On(board,x,y)=MS_FLAG;
				--curmines;
			}
			else if(On(board,x,y)==MS_FLAG){
				++curmines;
				if(config.mark)On(board,x,y)=MS_MARK;
				else On(board,x,y)=MS_UNKNOWN;
			}
			else if(On(board,x,y)==MS_MARK)On(board,x,y)=MS_UNKNOWN;
		}
		else if(type==MS_LCLICK||type==MS_DBLCLICK){
			StopWaiting();
			lastx=x;
			lasty=y;
			if(type==MS_LCLICK)Discover(x,y);
			else if(type==MS_DBLCLICK&&!MS_ISFUNC(On(board,x,y))){
				if(CountFlagAround(board,x,y)==On(board,x,y)){
					DiscoverAround(x,y);
				}
			}
			CheckGame();
			PaintEvent();
		}
	}
}
void TimerEvent(){
	if(game_state==MS_PLAYING){
		if(bestvalid==MS_INVALIDBEST&&(endgame||autogame))bestvalid=MS_INITBEST;
		if(bestvalid==MS_INITBEST){
			if(!GetFreq())bestvalid=MS_INVALIDBEST;
		}
		static int stepstart;
		static int minf;
		static double timek;
		static double beststep;
		int nowtime;
		QueryPerformanceCounter(&pknt);
		nowtime=1+(pknt.QuadPart-psknt.QuadPart)*1000/pfrq.QuadPart;
		if(bestvalid==MS_INITBEST){
			int mindis=(config.w+config.h);
			mindis*=mindis;
			stepstart=nowtime;
			minf=MS_PROB_MAX+1;
			bestvalid=MS_PRESSBEST;
			beststep=0;
			timek=config.bestmax;
			for(int x=0;x<config.w;++x)for(int y=0;y<config.h;++y){
				if(MS_ISFUNC(On(board,x,y))&&On(fboard,x,y)<=minf){
					int dx=x-lastx,
						dy=y-lasty;
					int dis=dx*dx+dy*dy;
					if(On(fboard,x,y)<minf){
						minf=On(fboard,x,y);
						mindis=dis+1;
						if(minf==0&&!config.single)break;
					}
					if(dis<mindis){
						mindis=dis;
						bestx=x;
						besty=y;
					}
				}
			}
		}
		beststep=(nowtime-stepstart)/timek;

		if(bestvalid==MS_PRESSBEST&&beststep>MS_PRESSPORTION){
			if(config.single||minf){
				if(On(board,bestx,besty)!=MS_UNKNOWN){
					curmines+=(On(board,bestx,besty)==MS_FLAG);
					On(board,bestx,besty)=MS_UNKNOWN;
				}
				lastx=bestx;
				lasty=besty;
				Discover(bestx,besty);
			}
			else for(int x=0;x<config.w;++x)for(int y=0;y<config.h;++y)
				if(MS_ISFUNC(On(board,x,y))&&On(fboard,x,y)==minf){
					if(On(board,x,y)!=MS_UNKNOWN){
						curmines+=(On(board,x,y)==MS_FLAG);
						On(board,x,y)=MS_UNKNOWN;
					}
					Discover(x,y);
				}
			CheckGame();
			if(game_state==MS_PLAYING)bestvalid=MS_WAITBEST;
		}
		if(beststep>1)bestvalid=MS_INVALIDBEST;
	}
	else if(autogame){
		LARGE_INTEGER temppknt;
		QueryPerformanceCounter(&temppknt);
		int tempinterval=(temppknt.QuadPart-pknt.QuadPart)*1000/pfrq.QuadPart;
		if(tempinterval>config.interval&&game_state!=MS_WAITING)ClearBoard();
		if(tempinterval>config.interval+config.bestmax)StopWaiting();
	}
}

LRESULT CALLBACK WndProc(HWND _hWndp,UINT message,WPARAM wParam,LPARAM lParam){
	HDC hdc;
	PAINTSTRUCT ps;
	int curx=GET_X_LPARAM(lParam),cury=GET_Y_LPARAM(lParam);
	int pos=Translate(curx,cury);
	int x=MS_GETX(pos),y=MS_GETY(pos);
	switch(message){
	case WM_TIMER:
		if(wParam==10086){
			TimerEvent();
			PaintEvent();
		}
		break;
	case WM_KEYDOWN:
		KeyDownEvent(wParam);
		break;
	case WM_LBUTTONDOWN:
		if(pos==MS_ONFACE)lb_downf=true;
		else if(rb_down)lb_down=true;
		else{
			lb_downb=true;
			if(layout&&!MS_ISFUNC(On(board,x,y))){
				rehide=true;
				On(board,x,y)=MS_UNKNOWN;
				LayoutUpdate();
			}
		}
		break;
	case WM_LBUTTONUP:
		if(lb_down&&rb_down)ClickEvent(pos,MS_DBLCLICK);
		else if(pos==MS_ONFACE&&lb_downf||MS_ONBOARD(pos)&&lb_downb&&!(layout&&rehide))
			ClickEvent(pos,MS_LCLICK);
		lb_downf=false;
		lb_downb=false;
		lb_down=false;
		rehide=false;
		break;
	case WM_RBUTTONDOWN:
		rb_down=true;
		if(lb_downb){
			lb_down=true;
			lb_downb=false;
		}
		if(!lb_down)ClickEvent(pos,MS_RCLICK);
		break;
	case WM_RBUTTONUP:
		if(lb_down&&rb_down)ClickEvent(pos,MS_DBLCLICK);
		rb_down=false;
		break;
	case WM_MOUSEMOVE:
		if(!(MK_LBUTTON&wParam)&&(lb_downf||lb_down||lb_downb)){
			lb_downf=false;
			lb_downb=false;
			lb_down=false;
			rehide=false;
		}
		if(!(MK_RBUTTON&wParam)&&rb_down){
			rb_down=false;
		}
		cur_mx=curx;
		cur_my=cury;
		if(layout&&MS_ONBOARD(pos)){
			if(lb_down&&rb_down)ClickEvent(pos,MS_DBLCLICK);
			else if(lb_downb){
				if(!rehide)ClickEvent(pos,MS_LCLICK);
				else if(!MS_ISFUNC(On(board,x,y))){
					On(board,x,y)=MS_UNKNOWN;
					LayoutUpdate();
				}
			}
		}
		break;
	case WM_MOUSEWHEEL:
		if(layout){
			int zDelta=GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			int pos=Translate(cur_mx,cur_my);
			if(MS_ONBOARD(pos)){
				int x=MS_GETX(pos),y=MS_GETY(pos);
				INT8 &bxy=On(board,x,y);
				curmines+=bxy==MS_FLAG;
				bxy-=zDelta;
				bxy%=1+MS_UNKNOWN;
				if(bxy<0)bxy+=1+MS_UNKNOWN;
				//curmines-=bxy==MS_FLAG;//not possible...
				LayoutUpdate();
			}
			else if(pos==MS_ONMINE){
				int oldlayout=layoutmines;
				layoutmines-=zDelta*wheeldeltam;
				if(layoutmines<=1)layoutmines=1;
				if(layoutmines>=config.w*config.h)layoutmines=config.w*config.h-1;
				zDelta=oldlayout-layoutmines;
				curmines-=zDelta;
				LayoutUpdate();
			}
		}
		break;
	case WM_KILLFOCUS:
		rehide=false;
		rb_down=false;
		lb_down=false;
		lb_downb=false;
		lb_downf=false;
		break;
	case WM_PAINT:
		hdc=BeginPaint(_hWndp,&ps);
		BitBlt(
			hdc,ps.rcPaint.left,ps.rcPaint.top,
			ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,
			mdc,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
		EndPaint(_hWndp,&ps);
		break;
	case WM_COMMAND:
		MenuEvent(LOWORD(wParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(_hWndp,message,wParam,lParam);
		break;
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow){
	WNDCLASSEX wcex={0};
	MSG msg;
	TCHAR szWindowClass[]=TEXT("Minesweeper lef MD203");
	TCHAR szTitle[]=TEXT("É¨À×");
	
	hInst=hInstance;
	wcex.cbSize=sizeof(WNDCLASSEX);
	wcex.style=0;
	wcex.lpfnWndProc=WndProc;
	wcex.cbClsExtra=0;
	wcex.cbWndExtra=0;
	wcex.hInstance=hInst;
	wcex.hIcon=LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor=LoadCursor(NULL,IDC_ARROW);
	wcex.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName=MAKEINTRESOURCE(IDR_MENU);
	wcex.lpszClassName=szWindowClass;
	wcex.hIconSm=LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));

	if(!RegisterClassEx(&wcex))return 1;

	hWnd=CreateWindow(
		szWindowClass,
		szTitle,
		(WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX),
		CW_USEDEFAULT,CW_USEDEFAULT,
		CW_USEDEFAULT,CW_USEDEFAULT,
		NULL,NULL,
		hInst,
		NULL
		);

	if(!hWnd)return 1;

	if(!Initialize())return 1;
	
	CheckMenuItem(GetMenu(hWnd),ID_GMARK,config.mark?MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(GetMenu(hWnd),ID_FVALUE,config.probtip?MF_CHECKED:MF_UNCHECKED);
	ShowWindow(hWnd,nCmdShow);
	UpdateWindow(hWnd);
	SetTimer(hWnd,10086,2,NULL);

	while(GetMessage(&msg,NULL,0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}
