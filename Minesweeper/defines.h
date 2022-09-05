#ifndef MS_DEFINES_H
#define MS_DEFINES_H

#define MS_CUSTOM              0
#define MS_BEG                 1
#define MS_INT                 2
#define MS_EXP                 3
#define MS_BEG_W               8
#define MS_BEG_H               8
#define MS_BEG_M              10
#define MS_INT_W              16
#define MS_INT_H              16
#define MS_INT_M              40
#define MS_EXP_W              30
#define MS_EXP_H              16
#define MS_EXP_M              99
#define MS_MIN_W               8
#define MS_MIN_H               8
#define MS_MAX_W             256
#define MS_MAX_H             256
#define MS_INIT_T       99999999
#define MS_MAX_NUMSCR          5
#define MS_MAX_TIMER       99999
#define MS_MAX_NAME           32
#define MS_UNKNOWN             9
#define MS_PRESS              10
#define MS_FLAG               11
#define MS_MARK               12
#define MS_MARKPRESS          13
#define MS_MISFLAG            14
#define MS_BOMB               15
#define MS_MINE               16
#define MS_UNINITIALIZED    (-1)
#define MS_OUT_OF_RANGE     (-2)

#define MS_PROB_MAX    100000000
#define MS_PROB_DRAWMAX       21
#define MS_SAFE_COLOR          \
	128,0,255,255
#define MS_TOSAFE_COLOR        \
	64,0,255,0
#define MS_TOMINE_COLOR        \
	128,255,0,0
#define MS_MINE_COLOR          \
	192,255,0,0
#define MS_BESTSTEP_MMIN      40
#define MS_BESTSTEP_MMAX    1000
#define MS_INVALIDBEST         0
#define MS_WAITBEST            1
#define MS_PRESSBEST           2
#define MS_INITBEST            3
#define MS_PRESSPORTION      0.6
#define MS_INTERVAL_MIN     1000
#define MS_INTERVAL_MAX     9000

#define MS_WAITING             0
#define MS_PLAYING             1
#define MS_WIN                 2
#define MS_LOSE                3
#define MS_NEWREC_BEG          1
#define MS_NEWREC_INT          2
#define MS_NEWREC_EXP          3
#define MS_LCLICK              0
#define MS_RCLICK              1
#define MS_DBLCLICK            2
#define MS_ONFACE           (-1)
#define MS_ONMINE			(-2)
#define MS_ONELSE           (-3)
#define MS_ONBOARD(p)   ((p)>=0)
#define MS_PAIR(x,y)           \
            ((x)<<16|(y)&0xFFFF)
#define MS_GETX(p)     ((p)>>16)
#define MS_GETY(p)  ((p)&0xFFFF)
#define MS_ISFUNC(x)     ((x)>8)

#define MS_NORMAL_NUM 0x000000FF
#define MS_CHOCO_NUM  0x0000FF00
#define MS_LAYOUT_NUM 0x00FFFF00

#define MS_CELL_METRIC        16
#define MS_BEST_SHC            2
#define MS_NUMBER_CX          11
#define MS_NUMBER_CY          21
#define MS_NUMSCR_CXK         13
#define MS_NUMSCR_CY          25
#define MS_FACE_METRIC        26
#define MS_BORDER_TOP         11
#define MS_BORDER_MID         11
#define MS_BORDER_BOTTOM      12
#define MS_BORDER_LEFT        12
#define MS_BORDER_RIGHT       12
#define MS_HEADER_CY          33
#define MS_NBOARD_CX           \
	           (MS_BORDER_LEFT \
			  +MS_BORDER_RIGHT )
#define MS_NBOARD_CY           \
	            (MS_BORDER_TOP \
                +MS_BORDER_MID \
             +MS_BORDER_BOTTOM \
                 +MS_HEADER_CY )
#define MS_TIP_CX             40
#define MS_TIP_CY             14
#define MS_TIPNUM_CX           6
#define MS_TIPNUM_CY           8
#define MS_TIPNUM_SHXK         6
#define MS_TIPNUM_SHY         14
#define MS_TIPNUM_SHL          2
#define MS_TIPNUM_SHT          3

#define MS_HEADER_SHT          4
#define MS_NUMSCRL_SHL         4
#define MS_NUMSCRR_SHR         4
#define MS_NUMSCR_SHT          6

#define MS_CELL_SHXK          16
#define MS_CELL_SHYK          16
#define MS_CELLNUM_NUM         9
#define MS_CELLFUNC_NUM        8
#define MS_CELLPROB_NUM        5
#define MS_NUMBER_SHY         33
#define MS_NUMBER_SHXK        12
#define MS_FACE_SHY           55
#define MS_FACE_SHX(x)  ((x)*27)
#define MS_FACE_SMILE          0
#define MS_FACE_OH             1
#define MS_FACE_LOSE           2
#define MS_FACE_WIN            3
#define MS_FACE_DOWN           4
#define MS_FACE_INVALID        5
#define MS_FRAME_SHY1         82
#define MS_FRAME_SHY2         94
#define MS_FRAME_SHY3         96
#define MS_FRAME_SHY4        108
#define MS_FRAME_SHY5        110
#define MS_FRAME_SHX1          0
#define MS_FRAME_SHX2         13
#define MS_FRAME_SHX3         15
#define MS_FRAME_SHX4         28
#define MS_FRAME_SHX5         29
#define MS_FRAME_SHX6         68
#define MS_FRAME_SHX7         70

#endif
