/*
 * Filename   : microfunctions.c
 *
 * Created    : 29.04.2016
 *
 * Modified   : jue 19 may 2016 14:38:55 CEST
 *
 * Author     : jatorre
 *
 * Purpose    : Microscopic functions for CG-Method 
 *
 */
#include "cg.h"

void Compute_Forces(gsl_matrix * Positions, gsl_matrix * Velocities, gsl_matrix * Neighbors, 
                    gsl_vector * ListHead, gsl_vector * List, int type1, int type2, 
                    gsl_matrix * Forces, gsl_vector * Energy, gsl_vector * Kinetic )
{

  // RESET MATRICES AND VECTORS
  // TODO: Redundant?
  gsl_matrix_set_zero(Forces);
  gsl_vector_set_zero(Energy);
  gsl_vector_set_zero(Kinetic);

  // Begin of parallel region
  
  int omp_get_max_threads();
  int chunks = NParticles / omp_get_max_threads();

  #pragma omp parallel
  {
    #pragma omp for schedule (dynamic,chunks) 
    for (int i=0;i<NParticles;i++)
    {
      gsl_vector_view vi = gsl_matrix_row(Velocities, i);

      double * fij = malloc(3*sizeof(double));

      // Compute the kinetic energy of particle i (0.5 mi vi^2)
      double ei = KineticEnergy(&vi.vector, (int) gsl_matrix_get(Positions,i,0));
      gsl_vector_set(Kinetic,i,ei);

      // Obtain the list of neighboring cells to iCell (the cell i belongs to)
      int iCell    = FindParticle(Positions,i);
      gsl_vector_view NeighboringCells = gsl_matrix_row(Neighbors, iCell);
           
      // Obtain the list of neighboring particles that interacts with i
      // i interacts with all Verlet[j] particles (j = 0 .. NNeighbors-1)
      int * Verlet = malloc(27 * NParticles * sizeof(int) / (Mx*My*Mz));
      int NNeighbors = Compute_VerletList(Positions, i, &NeighboringCells.vector, iCell, ListHead, List, Verlet);
      
      // Loop over all the j-neighbors of i-particle
      for (int j=0;j<NNeighbors;j++)
      {
        ei = Compute_Force_ij(Positions, i, Verlet[j], type1, type2, fij);
        Forces->data[i*Forces->tda + 0] += fij[0];
        Forces->data[i*Forces->tda + 1] += fij[1];
        Forces->data[i*Forces->tda + 2] += fij[2];
        Energy->data[i*Energy->stride]  += ei;
      }
      free(Verlet);
      free(fij);
    }
  }
  // End of parallel region
}

void GetLJParams(double type1, double type2, double * lj)
{
  if (type1 == type2)
  {
    if (type1 == 1.0) 
    {
      lj[0] = e1;
      lj[1] = s1;
      lj[2] = ecut1;
    }
    else
    {
      lj[0] = e2;
      lj[1] = s2;
      lj[2] = ecut2;
    }
  } 
  else
  {
      lj[0] = e12;
      lj[1] = s12;
      lj[2] = ecut12;
  }
}

double GetLJepsilon(int type1, int type2)
{
  double epsilon;
  if (type1 == type2)
  {
    if (type1 == 1.0) 
    {
      epsilon = e1;
    }
    else
    {
      epsilon = e2;
    }
  } 
  else
  {
    epsilon = e12;
  }
  return epsilon;
}

double GetLJsigma(int type1, int type2)
{
  double sigma;
  if (type1 == type2)
  {
    if (type1 == 1.0) 
    {
      sigma   = s1;
    }
    else
    {
      sigma = s2;
    }
  } 
  else
  {
    sigma = s12;
  }
  return sigma;
}

double Compute_Force_ij (gsl_matrix * Positions, int i, int j, int type1, int type2, double * fij)
{
   double r2i, r6i, ff;
   double eij = 0.0;
   
   double deltax  = Positions->data[i*Positions->tda + 1] - Positions->data[j*Positions->tda + 1];
          deltax -= Lx*round(deltax/Lx);
   double deltay  = Positions->data[i*Positions->tda + 2] - Positions->data[j*Positions->tda + 2];
          deltay -= Ly*round(deltay/Ly);
   double deltaz  = Positions->data[i*Positions->tda + 3] - Positions->data[j*Positions->tda + 3];
          deltaz -= Lz*round(deltaz/Lz);

   double r2      = deltax*deltax + deltay*deltay + deltaz*deltaz;

   // Obtain LJ parameters
   double * lj = malloc(3*sizeof(float));
   GetLJParams(gsl_matrix_get(Positions,i,0), gsl_matrix_get(Positions,j,0), lj);
   double epsilon = lj[0];
   double sigma   = lj[1];
   double ecut    = lj[2];
   free (lj);

   // Compute the force
   r2i      = sigma*sigma/r2;
   r6i      = pow(r2i,3);
   ff       = 48.0*epsilon*r2i*r6i*(r6i-0.5);

   fij[0] = 0.0;
   fij[1] = 0.0;
   fij[2] = 0.0;

   if (((type1 == 0)&&(type2 == 0)) || ((((int) gsl_matrix_get(Positions,i,0)) == type1)&&(((int) gsl_matrix_get(Positions,j,0) == type2))))
   {
     fij[0] = ff*deltax;  
     fij[1] = ff*deltay;  
     fij[2] = ff*deltaz;  
   }

   // Compute the potential energy
   // In lammps, pair_modify shift yes implies the existence of an ecut
   // eij = 4.0*epsilon*r6i*(r6i-1.0);
   eij = 4.0*epsilon*r6i*(r6i-1.0)-ecut;

   return eij;
}

double KineticEnergy (gsl_vector * v, int type)
{
  double K = pow(gsl_vector_get(v,0),2) + pow(gsl_vector_get(v,1),2) + pow(gsl_vector_get(v,2),2);
  
  switch (type)
  {
    case 1:
      K *= 0.5 * m1;
      break;
    case 2:
      K *= 0.5 * m2;
      break;
  }
  return K;
}

void Compute_Velocity_Module (gsl_matrix * Velocities, gsl_vector * Vmod)
{
  double vx, vy, vz;
  for (int i=0;i<NParticles;i++)
  {
    vx = gsl_matrix_get(Velocities,i,0);
    vy = gsl_matrix_get(Velocities,i,1);
    vz = gsl_matrix_get(Velocities,i,2);

    gsl_vector_set(Vmod,i,sqrt(vx*vx+vy*vy+vz*vz));
  }
}
    
void FixPBC(gsl_matrix * Positions)
{

  double xi,yi,zi;

  for (int i=0;i<NParticles;i++)
  {
    xi = gsl_matrix_get(Positions,i,1);
    if (xi < 0)
    {
      xi += Lx; 
    } 
    else if (xi >= Lx)
    {
      xi -= Lx; 
    }
    
    yi = gsl_matrix_get(Positions,i,2);
    if (yi < 0)
    {
      yi += Ly; 
    } 
    else if (yi >= Ly)
    {
      yi -= Ly; 
    }

    zi = gsl_matrix_get(Positions,i,3);
    if (zi < 0)
    {
      zi += Lz; 
    } 
    else if (zi >= Lz)
    {
      zi -= Lz; 
    }

    gsl_matrix_set(Positions,i,1,xi);
    gsl_matrix_set(Positions,i,2,yi);
    gsl_matrix_set(Positions,i,3,zi);

  }

}

void Compute_Momentum(gsl_matrix * Positions, gsl_matrix * Velocities, gsl_matrix * Momentum)
{
  gsl_matrix_set_zero(Momentum);

  gsl_matrix_memcpy(Momentum,Velocities);

  for (int i=0;i<NParticles;i++)
  {
    gsl_vector_view gi = gsl_matrix_row(Momentum,i);
    if ((int) gsl_matrix_get(Positions,i,0) == 1 )
    {
      gsl_vector_scale(&gi.vector,m1);
    }
    else
    {
      gsl_vector_scale(&gi.vector,m2);
    }
  }
}


void Compute_Momentum_Module (gsl_matrix * Momentum, gsl_vector * Mmod)
{
  double mx, my, mz;
  for (int i=0;i<NParticles;i++)
  {
    mx = gsl_matrix_get(Momentum,i,0);
    my = gsl_matrix_get(Momentum,i,1);
    mz = gsl_matrix_get(Momentum,i,2);

    gsl_vector_set(Mmod,i,sqrt(mx*mx+my*my+mz*mz));
  }
}
