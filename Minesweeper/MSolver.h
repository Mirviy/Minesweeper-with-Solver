#ifndef MSOLVER_H
#define MSOLVER_H
#include<cstdint>

//given a board and total number of mines,
//if this game is consistent
//   set prob to MAX_PROB * (probability of a corresponding tile to be a mine)
//   and return true
//elsewise return false
//_board: 0~8 is numbers, anything else is unknown
bool MSolve(int32_t _width,int32_t _height,int32_t _mines,const int8_t *_board,int32_t *prob);

//given a (back-)board and initial open position,
//return true if this game is guess free
//elsewise return false
//_board: 0~8 is numbers, anything else is mines
//should be self-consistent, and total number of mines is known and correct
bool MSolve(int32_t _width,int32_t _height,const int8_t *_board,int32_t initx,int32_t inity);

//Compare mine map to pre-computed must-guess configurations
//And adjust must-guess patch of the map to try to avoid guess
//return true if guess is required and adjusting is failed
//note: when false is returned, game is not guaranteed to be guess-free
bool MustGuess(int32_t _width,int32_t _height,int8_t *_board,int32_t initx,int32_t inity);

#endif

