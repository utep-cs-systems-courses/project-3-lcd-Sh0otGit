#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"

// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

#define M_PI (3.1415927f)
#define M_PI_2 (M_PI/2.0f)
#define M_PI_M_2 (M_PI*2.0f)

int compare_float(double f1, double f2){
  double precision = 0.00000000000000000001;
  if ((f1 - precision) < f2){
    return -1;
  } else if ((f1 + precision) > f2){
    return 1;
  } else {
    return 0;
  }
}

double my_cos(double x){
  if (x < 0.0f)
    x = -x;
  if (0 <= compare_float(x,M_PI_M_2)){
    do {
      x -= M_PI_M_2;
    } while(0 <= compare_float(x, M_PI_M_2));
  }
  if ((0 <= compare_float(x, M_PI)) && (-1 == compare_float(x, M_PI_M_2))){
    x -= M_PI;
    return ((-1)*(1.0f - (x*x/2.0f)*( 1.0f - (x*x/12.0f) * (1.0f - (x*x/30.0f) * (1.0f - (x*x/56.0f)*(1.0f - (x*x/90.0f)*(1.0f-(x*x/132.0f)*(1.0f-(x*x/182.0f)))))))));
  }
  return 1.0f - (x*x/2.0f)*( 1.0f - (x*x/12.0f) * (1.0f - (x*x/30.0f) * (1.0f - (x*x/56.0f)*(1.0f - (x*x/90.0f)*(1.0f - (x*x/132.0f)*(1.0f - (x*x/182.0f)))))));
}

double my_sin(double x){
  return my_cos(x-M_PI_2);
}

static char switch_update_interrupt_sense(){
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void switch_init(){  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

int switches = 0;

void switch_interrupt_handler(){
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;
}

// axis zero for col, axis 1 for row

short drawPos[2] = {0,0}, controlPos[2] = {1, 0};
short colVelocity = 1, colLimits[2] = {0, screenWidth};

void draw_ball(int col, int row, unsigned short color){
  fillRectangle(col-1, row-1, 3, 3, color);
}


void screen_update_ball(){ 
  for (char axis = 0; axis < 2; axis ++) 
    if (drawPos[axis] != controlPos[axis]) /* position changed? */
      goto redraw;
  return;			/* nothing to do */
 redraw:
  draw_ball(drawPos[0], drawPos[1], COLOR_RED); /* erase */
  for (char axis = 0; axis < 2; axis ++) 
    drawPos[axis] = controlPos[axis];
  draw_ball(drawPos[0], drawPos[1], COLOR_WHITE); /* draw */
}

short circleCenter[2] = {screenWidth/2,screenHeight/2};
short circleRadius = 50;
double angle = 1.0;

void screen_update_circle(){
  float xPos, yPos;
  double radians = angle * M_PI / 180.0;
  draw_ball(circleCenter[0], circleCenter[1], COLOR_GREEN);
  xPos = circleCenter[0] + (circleRadius * my_cos(radians));
  yPos = circleCenter[1] + (circleRadius * my_sin(radians));
  draw_ball(xPos, yPos, COLOR_WHITE);
}  

short redrawScreen = 1;

void wdt_c_handler()
{
  static int secCount1 = 0;
  static int secCount2 = 0;

  if (switches & SW1){
  
    secCount1 ++;
  
    if (secCount1 >= 10) {		/* 10/sec */
      {				/* move ball */
	short oldCol = controlPos[0];
	short newCol = oldCol + colVelocity;
	if (newCol <= colLimits[0] || newCol >= colLimits[1]){
	  colVelocity = -colVelocity;
	  controlPos[1] = controlPos[1] + 2;
	}
	else
	  controlPos[0] = newCol;
      }
      redrawScreen = 1;
    }
  } else if (switches & SW2){

    secCount2++;

    if (secCount2 >= 50){
      {
	angle++;
	if (angle >= 360){
	  angle = 1;
	}
      }
      redrawScreen = 1;
    }
  }
}

void main()
{
  
  P1DIR |= LED;		/**< Green led on when CPU on */
  P1OUT |= LED;
  configureClocks();
  lcd_init();
  switch_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  clearScreen(COLOR_BLUE);
  while (1) {			/* forever */
    if (redrawScreen) {
      redrawScreen = 0;
      if (switches & SW1){
	if (controlPos[1] > screenHeight){
	  fillRectangle(screenWidth/2,screenHeight/2, 3, 3, COLOR_WHITE);
	} else {
	  screen_update_ball();
	}
      } else if (switches & SW2){
	screen_update_circle();
      }
     
    }
    P1OUT &= ~LED;	/* led off */
    or_sr(0x10);	/**< CPU OFF */
    P1OUT |= LED;	/* led on */
  }
}


void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}
