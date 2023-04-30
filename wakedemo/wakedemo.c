#include <msp430.h>
#include <libTimer.h>
#include <stdio.h>
#include <math.h>
#include "lcdutils.h"
#include "lcddraw.h"

// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

int switches = 0;

void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;
}


// axis zero for col, axis 1 for row

short drawPos[2] = {0,0}, controlPos[2] = {1, 0};
short colVelocity = 1, colLimits[2] = {0, screenWidth};

void
draw_ball(int col, int row, unsigned short color)
{
  fillRectangle(col-1, row-1, 3, 3, color);
}


void
screen_update_ball(){ 
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
short circleRadius = 10;
double angle = 1.0;
double pi = 3.14159265;

void screen_update_circle(){
  double xPos, yPos;
  xPos = cos(angle) * circleRadius;
  yPos = sin(angle) * circleRadius;
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
    }
  } else if (switches & SW2){

    secCount2++;

    if (secCount2 >= 10){
      {
	angle++;
	if (angle >= 360){
	  angle = 1;
	}
      }
      //stuff
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
