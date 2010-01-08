/*  * Copyright (C) 2008 sarah dégallier BIRG, EPFL, Lausanne
 * RobotCub Consortium, European Commission FP6 Project IST-004370
 * email:   sarah.degallier@robotcub.org
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include "cpgs.h"
#include <iostream>
#include <ace/OS.h>
using namespace std;



cpg_manager::cpg_manager(int nbDOFs){

  this->nbDOFs = nbDOFs;
  cpgs_size = 4*nbDOFs + 3;
  controlled_param = 2*nbDOFs;
  

  //we initialize coupling strength and phase angle and set everything to 0
  epsilon = new double*[nbDOFs];
  theta = new double*[nbDOFs];
  theta_init = new double*[nbDOFs];

  for(int i =0;i<nbDOFs;i++)
    {
      epsilon[i] = new double[nbDOFs+1];
      theta[i] = new double[nbDOFs+1];
      theta_init[i]= new double[nbDOFs+1];

      for(int j=0;j<nbDOFs+1;j++)
	 {
		 epsilon[i][j] = 0.0;
		 theta[i][j] = 0.0;
		 theta_init[i][j] =0.0;
	 }
    }

  next_theta = theta[0][0];

  //open parameters
  m = new double [nbDOFs];//amplitudes
  g = new double [nbDOFs];//target angles
 
  //radii
  r = new double [nbDOFs];
  r2= new double [nbDOFs];
  dydt = new double [2*cpgs_size];
  for(int i=0; i<2*cpgs_size; i++)
    {
      dydt[i]=0.0;
    }

  //*********open parameters**********

  parameters = new double[controlled_param];
  next_parameters = new double[controlled_param];
  
  for(int i=0; i< nbDOFs; i++)
    {
      parameters[2*i]=-15.0; // amplitude of oscillations
      parameters[2*i+1]=0.0; //target of the discrete movement

      next_parameters[2*i] =-15.0;
      next_parameters[2*i+1] = 0.0;
    }

  //********fixed parameters********* 

  //equation
  a = 2.0; //rate of convergence of the rhythmic system
  b = 5.0;//rate of convergence of the discrete system
  m_off = -5.0; //value to turn of the oscillations 
  m_on = 1.0; // value to turn on the oscillations
  b_go = 0.5; //rate of conv. of the go command (for the discrete system)
  u_go = 4.0; //max value of the go command
  dt = 0.0001; //integration step in seconds
  alpha_x = 100.0; //gain of the feedback on the position
  alpha_y = 100.0;//gain of the feedback on the speed
  c = 100.0;//parameter of for the swing/stance switching

  //frequency and amplitude
  nu=0.25; //in Hz
  next_nu = 0.25;

  ampl = new double[nbDOFs];
  for(int i=0;i<nbDOFs;i++)
    ampl[i]=0.1;

  //feedback
  nuStance=0.1;
  drumHit=0;
  stuckCounter=0;
  stuckPos = new double[nbDOFs];
  disStuckPos = new double[nbDOFs];


}



cpg_manager::~cpg_manager()

{
  delete[] parameters;

  for(int i=0;i<nbDOFs;i++)
    {
      delete[] epsilon[i];
      delete[] theta[i];
      delete[] theta_init[i];
    }

  delete[] epsilon;
  delete[] theta;
  delete[] theta_init;
  delete[] m;
  delete[] g;
  delete[] dydt;
  delete[] r;
  delete[] ampl;
  delete[] stuckPos;
  delete[] disStuckPos;
}



void cpg_manager::integrate_step(double *y, double *at_states)
{

  //************ getting open parameters******************
  
  double omega;

  omega = 2*M_PI*nu;
  //om_st = 2*M_PI*nuStance;
  //om_sw = om*om_st/(2*om_st-om); //pulsation
 
   for(int i=0; i<nbDOFs;i++)
   {
      m[i]= parameters[2*i];
      g[i]= parameters[2*i+1]/ampl[i];
    }

  //***********CPGS - EQUATIONS***************************************

//radii
  r_clock=y[0]*y[0]+y[1]*y[1]; 

  for(int i=0;i<nbDOFs;i++)
    {
      r[i]=y[4+i*4]*y[4+i*4]+y[5+i*4]*y[5+i*4]; 
    }

  //******** external clock************  

  dydt[0] = a * (m_on-r_clock)*y[0]-omega*y[1];
  dydt[1] = a * (m_on-r_clock)*y[1]+omega*y[0];

   //********** go_command ********

  dydt[cpgs_size-1]=b_go*(u_go-y[cpgs_size-1]);

  //***********JOINTS***********************************

  ////internal dynamics
  for(int i =0;i<nbDOFs;i++)
    {
      ///discrete system
      dydt[i*4+2] = y[cpgs_size-1]*y[cpgs_size-1]*y[cpgs_size-1]*y[cpgs_size-1]*y[i*4+3];
      dydt[i*4+3] = u_go*u_go*u_go*u_go*b * (b/4.0 * (g[i] - y[i*4+2]) - y[i*4+3]);

	   //rhythmic one
      dydt[i*4+4] = a * (m[i]-r[i]) * y[i*4+4] - omega * y[i*4+5];
      dydt[i*4+5] = a * (m[i]-r[i]) * y[i*4+5] + omega* y[i*4+4];
   }

  ///couplings
  for(int i = 0;i<nbDOFs;i++)
    for(int j=0;j<nbDOFs+1;j++)
      {
        int indice;
        if(j == 0)
        {
            dydt[i*4+4] += epsilon[i][j]*(cos(theta[i][j])*y[0] - sin(theta[i][j])*y[1]);
            dydt[i*4+5] +=  epsilon[i][j]*(sin(theta[i][j])*y[0] + cos(theta[i][j])*y[1]);
        }
        else
        {
            indice = j*4;
	    
            dydt[i*4+4] += epsilon[i][j]*(cos(theta[i][j])*(y[indice]) - sin(theta[i][j])*y[indice+1]);
            dydt[i*4+5] +=  epsilon[i][j]*(sin(theta[i][j])*(y[indice]) + cos(theta[i][j])*y[indice+1]);
        }
      }

  //*********OBSERVER***************

//radii
  r_clock2=y[cpgs_size]*y[cpgs_size]+y[1+cpgs_size]*y[1+cpgs_size]; 

  for(int i=0;i<nbDOFs;i++)
    {
      r2[i]=(y[4+i*4+cpgs_size])*(y[4+i*4+cpgs_size])+y[5+i*4+cpgs_size]*y[5+i*4+cpgs_size]; 
    }

  //******** external clock************  

  dydt[cpgs_size] = a * (m_on-r_clock2)*y[cpgs_size]-omega*y[cpgs_size+1];
  dydt[cpgs_size+1] = a * (m_on-r_clock2)*y[1+cpgs_size]+omega*y[cpgs_size];

   //********** go_command ********

  dydt[2*cpgs_size-1]=b_go*(u_go-y[2*cpgs_size-1]);


  ////internal dynamics
  for(int i =0;i<nbDOFs;i++)
    {
      ///discrete system
      dydt[i*4+2+cpgs_size] = y[2*cpgs_size-1]*y[2*cpgs_size-1]*y[2*cpgs_size-1]*y[2*cpgs_size-1]*y[i*4+3+cpgs_size];
      dydt[i*4+3+cpgs_size] = u_go*u_go*u_go*u_go*b * (b/4.0 * (g[i] - y[i*4+2+cpgs_size]) - y[i*4+3+cpgs_size]);

	   //rhythmic one
      dydt[i*4+4+cpgs_size] = a * (m[i]-r2[i]) * (y[i*4+4+cpgs_size]) - omega * y[i*4+5+cpgs_size];
      dydt[i*4+5+cpgs_size] = a * (m[i]-r2[i]) * y[i*4+5+cpgs_size] + omega * (y[i*4+4+cpgs_size]);
   }

  ///couplings
  for(int i = 0;i<nbDOFs;i++)
    for(int j=0;j<nbDOFs+1;j++)
      {
        int indice;
        if(j == 0)
          {
            dydt[i*4+4+cpgs_size] += epsilon[i][j]*(cos(theta[i][j])*y[cpgs_size] - sin(theta[i][j])*y[1+cpgs_size]);
            dydt[i*4+5+cpgs_size] +=  epsilon[i][j]*(sin(theta[i][j])*y[cpgs_size] + cos(theta[i][j])*y[1+cpgs_size]);
          }
        else
          {
            indice = j*4;
            
            dydt[i*4+4+cpgs_size] += epsilon[i][j]*(cos(theta[i][j])*(y[indice+cpgs_size]) - sin(theta[i][j])*y[indice+1+cpgs_size]);
            dydt[i*4+5+cpgs_size] +=  epsilon[i][j]*(sin(theta[i][j])*(y[indice+cpgs_size]) + cos(theta[i][j])*y[indice+1+cpgs_size]);
          }
      }
  
  
//*******SOUND FEEDBACK*********

	if(drumHit==-1 && y[4+cpgs_size]*up_down>up_down*0.9)
	{
		ACE_OS::printf("feedback enabled again\n");
		drumHit=0;
	}

    if(drumHit==1)
    {
        if(stuckCounter==0)
        {
            for(int i=0; i<nbDOFs; i++)
            {
                stuckPos[i]=y[4*i+4];
                disStuckPos[i]=y[4*i+2];                 
            }

			if(y[5]>0)
			{
				up_down=1;
			} //arm is going up
        
			else
			{
				up_down=-1;
			}

			stuckCounter=1;
			ACE_OS::printf("FEEDBACK ON\n");
			ACE_OS::printf("stuck value %f, target value %f, observer %f\n", stuckPos[0], y[4], y[4+cpgs_size]);
		} 
			
		if(stuckCounter>0)
		{
			if(part_name=="right_leg" || part_name=="left_leg")
			{
				if(stuckCounter>10 && up_down*y[4+cpgs_size]>up_down*stuckPos[0])
				{
					ACE_OS::printf("FEEDBACK OFF\n");
					ACE_OS::printf("stuck value %f, target value %f, observer %f\n", stuckPos[0], y[4], y[4+cpgs_size]);
					drumHit=0;
					stuckCounter=-1;
            
					for(int i=0;i<cpgs_size;i++)//we adapt the observer to the current state of the oscillator
					{
						y[i+cpgs_size] = y[i];
						dydt[i+cpgs_size] = dydt[i]; 
					}	
				}
				
				else
				{
					for(int i=0;i<nbDOFs;i++)
					{
						stuckCounter++;
						dydt[i*4+4] += alpha_x*(stuckPos[i]-(y[4*i+4]));
						dydt[i*4+5] = dydt[i*4+5]/(1+alpha_y*(stuckPos[i]-(y[4*i+4]))*(stuckPos[i]-(y[4*i+4])));
						dydt[i*4+2] += alpha_x*(disStuckPos[i]-y[4*i+2]);
						dydt[i*4+3] = dydt[i*4+3]/(1+alpha_y*(disStuckPos[i]-y[4*i+2])*(disStuckPos[i]-y[4*i+2]));
					}
				}
			}
			
			if(part_name=="right_arm" || part_name=="left_arm")
			{
				if(stuckCounter>5 && up_down*y[4+cpgs_size]>up_down*stuckPos[0])
				{
					ACE_OS::printf("FEEDBACK OFF\n");
					ACE_OS::printf("stuck value %f, target value %f, observer %f\n", stuckPos[0], y[4], y[4+cpgs_size]);
					drumHit=-1;
					stuckCounter=0;
            
					for(int i=0;i<cpgs_size;i++)//we adapt the observer to the current state of the oscillator
					{
						y[i+cpgs_size] = y[i];
						dydt[i+cpgs_size] = dydt[i]; 
					}	
				}
				
				else
				{
					for(int i=0;i<nbDOFs;i++)
					{
						stuckCounter++;
						dydt[i*4+4] += alpha_x*(stuckPos[i]-(y[4*i+4]));
						dydt[i*4+5] = dydt[i*4+5]/(1+alpha_y*(stuckPos[i]-(y[4*i+4]))*(stuckPos[i]-(y[4*i+4])));
						dydt[i*4+2] += alpha_x*(disStuckPos[i]-y[4*i+2]);
						dydt[i*4+3] = dydt[i*4+3]/(1+alpha_y*(disStuckPos[i]-y[4*i+2])*(disStuckPos[i]-y[4*i+2]));
					}
				}
			}
		}
    
    }
      
  /* if(drumHit==0 && stuckCounter==1)
    {
      ACE_OS::printf("FEEDBACK OFF\n");
      stuckCounter=0;
    }
  */

  //********INTEGRATION****************************************

   for(int i=0; i<2*cpgs_size; i++)
    {
       y[i]=y[i]+dydt[i]*dt;
    }



    //***** SETTING TARGET POSITION
    for(int i=0;i<nbDOFs;i++)
    {
        at_states[i]= ampl[i]*180/M_PI*(y[4*i+4]+y[4*i+2]);
		if(at_states[i] != at_states[i])
		{
			printf("FATAL ERROR : CPG has diverged above the DOUBLE_MAX value, check the CPG parameters");
			exit(0);
		}
    }
}


void cpg_manager::printInternalVariables()
{

 ACE_OS::printf("freq: %f\n",nu);
 ACE_OS::printf("nbDOFs %d, cpgs_size %d, controlled param %d\n",
		nbDOFs,cpgs_size,controlled_param);
 ACE_OS::printf("a %f, b %f, m_off %f, m_on %f, b_go %f, u_go %f, dt %f\n",
		a,b,m_off,m_on,b_go,u_go,dt);

 for(int i=0;i<nbDOFs;i++)
   {
     ACE_OS::printf("for DOF %d, mu=%f and g=%f - ampl=%f\n",i,parameters[2*i],parameters[2*i+1],ampl[i]);
     ACE_OS::printf("coupling strength");

     for(int j=0;j<nbDOFs+1;j++)
       ACE_OS::printf(" - %f",epsilon[i][j]); 

     ACE_OS::printf("\n");
     ACE_OS::printf("phase diff");

     for(int j=0;j<nbDOFs+1;j++)
       ACE_OS::printf(" - %f",theta[i][j]); 

     ACE_OS::printf("\n");
   }
}

