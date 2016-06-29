/*
 * Filename   : mesofunctions.c
 *
 * Created    : 07.04.2016
 *
 * Modified   : mié 29 jun 2016 16:37:49 CEST
 *
 * Author     : jatorre
 *
 * Purpose    : Mesoscopic functions for cg.c 
 *
 */
#include "cg.h"

void Compute_Node_Positions(gsl_vector * z)
{
  // Valid only for a regular lattice
  // TODO: Consider irregular lattices
  for (int mu=0;mu<NNodes;mu++)
    gsl_vector_set(z,mu,((double) mu)*Lz/NNodes);
}

void Compute_Meso_Density(gsl_matrix * Micro, gsl_vector * z, gsl_matrix * n)
{

  // RESET vector
  gsl_matrix_set_zero(n);

  // Valid for PBC,  this function obtains the  density of a slab of volume Lx *
  //  Ly *  dz The  slab is  build as  a  finite  element  based  on  a Delaunay
  // tessellation
  double dv = ((float) Lx * Ly * Lz) / NNodes;
  double dz = ((float) Lz) / NNodes;

  for (int i=0;i<NParticles;i++)
  {
    int    type    = (int) gsl_matrix_get(Micro,i,0);
    double zi      = gsl_matrix_get(Micro,i,3);
    int    muLeft  = (int) floor(zi*NNodes/Lz);
    int    muRight = muLeft+1;
    double zLeft   = gsl_vector_get(z, muLeft);
    double zRight  = gsl_vector_get(z,muRight);

    if (muRight == NNodes)
    {
      n->data[     0*n->tda+type] += (zi - zLeft)/dz;
      n->data[muLeft*n->tda+type] += (Lz    - zi)/dz;
    }
    else if (muLeft == NNodes)
    {
      n->data[0*n->tda+type] += 1.0;
    }
    else
    {
      n->data[muRight*n->tda+type] += (zi  - zLeft)/dz;
      n->data[ muLeft*n->tda+type] += (zRight - zi)/dz;
    }
  }
  gsl_matrix_scale(n,1.0/dv);
}

void Compute_Meso_Force(gsl_matrix * Positions, gsl_matrix * Forces, 
                        gsl_vector * z, gsl_matrix * MesoForce)
{
  double zi, fx, fy, fz;
  int muRight, muLeft;
  double dv = ((float) Lx * Ly * Lz) / NNodes;
  double dz = ((float) Lz) / NNodes;

  for (int i=0;i<NParticles;i++)
  {
    zi      = gsl_matrix_get(Positions,i,3);
    fx      = gsl_matrix_get(Forces,i,0);
    fy      = gsl_matrix_get(Forces,i,1);
    fz      = gsl_matrix_get(Forces,i,2);
    
    muLeft  = (int) floor(zi*NNodes/Lz);
    muRight = muLeft+1;
    double zLeft  = gsl_vector_get(z, muLeft);
    double zRight = gsl_vector_get(z,muRight);
      
    if (muRight == NNodes)
    {
      MesoForce->data[     0*MesoForce->tda+0] += fx * (zi - zLeft)/dz;
      MesoForce->data[muLeft*MesoForce->tda+0] += fx * (Lz -    zi)/dz;
      MesoForce->data[     0*MesoForce->tda+1] += fy * (zi - zLeft)/dz;
      MesoForce->data[muLeft*MesoForce->tda+1] += fy * (Lz -    zi)/dz;
      MesoForce->data[     0*MesoForce->tda+2] += fz * (zi - zLeft)/dz;
      MesoForce->data[muLeft*MesoForce->tda+2] += fz * (Lz -    zi)/dz;
    }
    else if (muLeft == NNodes)
    {
      MesoForce->data[0*MesoForce->tda+0] += fx;
      MesoForce->data[0*MesoForce->tda+1] += fy;
      MesoForce->data[0*MesoForce->tda+2] += fz;
    }
    else
    {
      MesoForce->data[muRight*MesoForce->tda+0] += fx * (zi -  zLeft)/dz;
      MesoForce->data[ muLeft*MesoForce->tda+0] += fx * (zRight - zi)/dz;
      MesoForce->data[muRight*MesoForce->tda+1] += fy * (zi -  zLeft)/dz;
      MesoForce->data[ muLeft*MesoForce->tda+1] += fy * (zRight - zi)/dz;
      MesoForce->data[muRight*MesoForce->tda+2] += fz * (zi -  zLeft)/dz;
      MesoForce->data[ muLeft*MesoForce->tda+2] += fz * (zRight - zi)/dz;
    }
  }
  gsl_matrix_scale(MesoForce,1.0/dv);
}

void Compute_Meso_Sigma1 (gsl_matrix * Positions, gsl_matrix * Velocities, 
                          gsl_matrix * MesoSigma1)
{
  double dv = ((float) Lx * Ly * Lz) / NNodes;
  
  gsl_matrix_set_zero(MesoSigma1);
  
  // Loop over all i-particles
  for (int i=0;i<NParticles;i++)
  {
    // Consider only type2 (fluid) particles 
    // mass of type2 particles is m2
    if ((int) gsl_matrix_get(Positions,i,0) == 2)
    {
      // Obtain the bin to where the i-particle belongs to
      int mu = floor(gsl_matrix_get(Positions,i,3)*NNodes/Lz);

      double * sigma1 = malloc(9*sizeof(double));

      double vx = gsl_matrix_get(Velocities,i,0);
      double vy = gsl_matrix_get(Velocities,i,1);
      double vz = gsl_matrix_get(Velocities,i,2);

      sigma1[0] = vx * vx;
      sigma1[1] = vx * vy;
      sigma1[2] = vx * vz;
      sigma1[3] = vy * vx;
      sigma1[4] = vy * vy;
      sigma1[5] = vy * vz;
      sigma1[6] = vz * vx;
      sigma1[7] = vz * vy;
      sigma1[8] = vz * vz;
     
      for (int j=0;j<9;j++)
        MesoSigma1->data[mu*MesoSigma1->tda+j] += m2 * sigma1[j];
        // MesoSigma1->data[mu*MesoSigma1->tda+j] += mass * sigma1[j];

      free(sigma1);
    }
  }
  gsl_matrix_scale(MesoSigma1,1.0/dv);
}

void Compute_Meso_Sigma2 (gsl_matrix * Positions, gsl_matrix * Neighbors, gsl_vector * ListHead,
                          gsl_vector * List, gsl_matrix * MesoSigma2, gsl_vector * z)
{

  gsl_matrix_set_zero(MesoSigma2);

  double dv = ((float) Lx * Ly * Lz) / NNodes;
  
  // Forall i particles
  for (int i=0;i<NParticles;i++)
  {
    // Only for fluid (type 2) particle
    if ((int) gsl_matrix_get(Positions,i,0) == 2)
    {
      double zi = gsl_matrix_get(Positions,i,3);
      // Find the bin mu to which the particle i belongs
      int mu = floor(zi*NNodes/Lz);

      // Find the cell to which the particle i belongs and all its neighboring cells
      int iCell = FindParticle(Positions,i);
      gsl_vector_view NeighboringCells = gsl_matrix_row(Neighbors, iCell);
      
      // Find the neighbors of particle i
      int * Verlet = malloc(27 * NParticles * sizeof(int) / (Mx*My*Mz) );
      int NNeighbors = Compute_VerletList(Positions, i, &NeighboringCells.vector, iCell, ListHead, List, Verlet);
      
      // Forall Verlet[j] neighboring particles
      for (int j=0;j<NNeighbors;j++)
      {
        // Only for fluid (type 2) particles
        if ((int) gsl_matrix_get(Positions,Verlet[j],0) == 2)
        {
          double zj = gsl_matrix_get(Positions,Verlet[j],3);
          // Find the bin nu to which the particle Verlet[j] belongs
          int nu = floor(zj*NNodes/Lz);

          double * fij = malloc(3*sizeof(double));
          Compute_Force_ij (Positions, i, Verlet[j], 2, 2, fij);
  
          double * rij = malloc(3*sizeof(double));
          rij[0]  = gsl_matrix_get(Positions,i,1) - gsl_matrix_get(Positions,Verlet[j],1);
          rij[0] -= Lx*round(rij[0]/Lx);
          rij[1]  = gsl_matrix_get(Positions,i,2) - gsl_matrix_get(Positions,Verlet[j],2);
          rij[1] -= Ly*round(rij[1]/Ly);
          rij[2]  = zi - zj;
          rij[2] -= Lz*round(rij[2]/Lz);

          double * sigma2 = malloc(9*sizeof(double));

          for (int sigma=((int)min(mu,nu)); sigma<=((int)max(mu,nu));sigma++)
          {
            double zsigma = zmuij(z,sigma,zi,zj);
            sigma2[0] = rij[0]*fij[0]*zsigma;
            sigma2[1] = rij[0]*fij[1]*zsigma;
            sigma2[2] = rij[0]*fij[2]*zsigma;
            sigma2[3] = rij[1]*fij[0]*zsigma;
            sigma2[4] = rij[1]*fij[1]*zsigma;
            sigma2[5] = rij[1]*fij[2]*zsigma;
            sigma2[6] = rij[2]*fij[0]*zsigma;
            sigma2[7] = rij[2]*fij[1]*zsigma;
            sigma2[8] = rij[2]*fij[2]*zsigma;

            for (int k=0;k<9;k++)
              MesoSigma2->data[sigma*MesoSigma2->tda+k] += sigma2[k];
          }
          
          free(fij);
          free(rij);
          free(sigma2);
        }
      }
      free(Verlet);
    }
  }
  gsl_matrix_scale(MesoSigma2,0.5/dv);
}

void Compute_Meso_Temp(gsl_vector * MesoKinetic, gsl_vector * MesoDensity, gsl_vector * MesoTemp)
{
  double val;

  for (int mu=0;mu<NNodes;mu++)
  {
    if (gsl_vector_get(MesoDensity,mu) != 0)
    {
      val = gsl_vector_get(MesoKinetic,mu) / gsl_vector_get(MesoDensity,mu);
    }
    else
    {
      val = 0.0;
    }
    gsl_vector_set(MesoTemp,mu,val);
  }
  gsl_vector_scale(MesoTemp,2.0/3.0);
}
  
void Compute_Mean_Values(char * basename, char * filename, gsl_vector * MeanValues)
{

  gsl_matrix * InputMatrix = gsl_matrix_calloc (NSteps,NNodes);     
  
  FILE *iFile;
  char str[100]; 
  memset(str,'\0',sizeof(str));
  sprintf(str, "./output/%s%s", basename,filename);
  iFile = fopen(str, "r");

  #if __BINARY_OUTPUT__
    gsl_matrix_fread(iFile, InputMatrix);
  #else
    gsl_matrix_fscanf(iFile, InputMatrix);
  #endif

  fclose(iFile);

  gsl_vector_set_zero(MeanValues);

  for (int i=0;i<NSteps;i++)
  {
    for (int mu=0;mu<NNodes;mu++)
    {
      // MeanValues->data[(mu-1)*MeanValues->stride] += gsl_matrix_get(InputMatrix,i,mu); 
      MeanValues->data[mu*MeanValues->stride] += gsl_matrix_get(InputMatrix,i,mu); 
    }
  }

  gsl_vector_scale(MeanValues,1.0/NSteps);
}

void Compute_Meso_Velocity(gsl_matrix * MesoMomentum, gsl_vector * MesoDensity, gsl_matrix * MesoVelocity)
{
  gsl_matrix_memcpy(MesoVelocity,MesoMomentum);
  
  gsl_vector_view vx = gsl_matrix_column(MesoVelocity,0);
  gsl_vector_div(&vx.vector,MesoDensity);
  gsl_vector_view vy = gsl_matrix_column(MesoVelocity,1);
  gsl_vector_div(&vy.vector,MesoDensity);
  gsl_vector_view vz = gsl_matrix_column(MesoVelocity,2);
  gsl_vector_div(&vz.vector,MesoDensity);

  for (int mu=0;mu<NNodes;mu++)
  {
    gsl_vector_view vmu = gsl_matrix_row(MesoVelocity,mu); 
    if (fabs(gsl_vector_get(MesoDensity,mu)) < 10E-8)
      gsl_vector_set_zero(&vmu.vector);
  }
}
          
void Compute_Meso_Profile(gsl_matrix * Positions, gsl_vector * Micro, gsl_vector * z, gsl_vector * Meso, int type)
{
  double dv = ((float) Lx * Ly * Lz) / NNodes;
  double dz = ((float) Lz) / NNodes;
  double zi, ei;
  int muRight, muLeft;

  // RESET vector
  gsl_vector_set_zero(Meso);

  for (int i=0;i<NParticles;i++)
  {
    if ((int) gsl_matrix_get(Positions,i,0) == type)
    {
      zi      = gsl_matrix_get(Positions,i,3);
      ei      = gsl_vector_get(Micro,i);
      muLeft  = (int) floor(zi*NNodes/Lz);
      muRight = muLeft+1;
      double zLeft  = gsl_vector_get(z, muLeft);
      double zRight = gsl_vector_get(z,muRight);
      
      if (muRight == NNodes)
      {
        Meso->data[     0*Meso->stride] += ei * (zi - zLeft)/dz;
        Meso->data[muLeft*Meso->stride] += ei * (Lz -    zi)/dz;
      }
      else if (muLeft == NNodes)
      {
        Meso->data[0*Meso->stride] += ei;
      }
      else
      {
        Meso->data[muRight*Meso->stride] += ei * (zi -  zLeft)/dz;
        Meso->data[ muLeft*Meso->stride] += ei * (zRight - zi)/dz;
      }
    }
  }
  gsl_vector_scale(Meso,1.0/dv);
}
        
void Compute_InternalEnergy(gsl_vector * MesoEnergy, gsl_matrix * MesoMomentum, 
                            gsl_vector * MesoDensity, gsl_vector * InternalEnergy)
{
  gsl_vector * GMod2 = gsl_vector_calloc(NNodes);

  for (int mu=0;mu<NNodes;mu++)
  {
    double gmu2 =   pow(gsl_matrix_get(MesoMomentum,mu,0),2) 
                  + pow(gsl_matrix_get(MesoMomentum,mu,1),2) 
                  + pow(gsl_matrix_get(MesoMomentum,mu,2),2);
    gsl_vector_set(GMod2,mu,gmu2);
  }

  gsl_vector_memcpy(InternalEnergy,GMod2);
  gsl_vector_scale(InternalEnergy,-1.0);
  gsl_vector_div(InternalEnergy,MesoDensity);
  
  for (int mu=0;mu<NNodes;mu++)
  {
    if (fabs(gsl_vector_get(MesoDensity,mu)) < 10E-8)
      gsl_vector_set(InternalEnergy,mu,0.0);
  }
  
  gsl_vector_add(InternalEnergy,MesoEnergy);

  gsl_vector_free(GMod2);
}

double zmuij(gsl_vector * z, int mu, double zi, double zj)
{
 
  double val;
  double z1 = gsl_vector_get(z,mu);
  double z2 = gsl_vector_get(z,mu+1);

  if (fabs(zi-zj) <= 1e-10)
  {
    val = Heaviside(z2-zi)*Heaviside(zi-z1);  
  } 
  else
  {
    val = ( (zj-z1)*Heaviside(z2-zj)*Heaviside(z1-zj)
           -(zi-z1)*Heaviside(z2-zi)*Heaviside(z1-zi)
           -(zj-z2)*Heaviside(z2-zj)
           +(zi-z2)*Heaviside(z2-zi)) / (zi-zj);
  }
        
  return val;
}
