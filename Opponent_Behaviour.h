#ifndef _OPPONENT_BEHAVIOUR
#define _OPPONENT_BEHAVIOUR

#define NO_OPPONENT (-1)

extern void OpponentBehaviour(long *x,
							  long *y,
							  long *z,
							  float *x_angle,
							  float *y_angle,
							  float *z_angle,
							  bool bOpponentPaused);

extern void CarToCarCollision(void);

extern long CalculateIfWinning(long start_finish_piece);

extern long CalculateOpponentsDistance(void);

#endif /* _OPPONENT_BEHAVIOUR */