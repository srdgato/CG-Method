/*
 * Filename   : io.c
 *
 * Created    : 19.04.2016
 *
 * Modified   : mar 17 may 2016 18:52:41 CEST
 *
 * Author     : jatorre
 *
 * Purpose    : IO functions for CG-Method 
 *
 */
#include "cg.h"

void PrintMsg(char *msg)
{
  time_t rawtime;
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  char * timestamp = asctime(timeinfo);
  timestamp[strlen(timestamp)-1] = '\0';
  printf("[%s]\t%s\n", timestamp, msg);
}
    
void SaveMatrixWithIndex(gsl_vector * z, gsl_matrix * Matrix, char * File)
{
    FILE *iFile;
    iFile = fopen(File, "w");
    int Nrows = Matrix->size1;
    int Ncols = Matrix->size2;
    for (int i=0;i<Nrows;i++)
    {
      fprintf(iFile, "%8.6f",gsl_vector_get(z,i));
      for (int j=0;j<Ncols;j++)
        fprintf(iFile, "\t%20.14f",gsl_matrix_get(Matrix,i,j));
      fprintf(iFile, "\n");
    }
    fclose(iFile);
}

void SaveVectorWithIndex(char * basename, char * filename, gsl_vector * z1, gsl_vector * z2)
{
  
  char str[100]; 
  strcpy (str, "./output/");
  strcat (str, basename);
  strcat (str, filename);
  
  int NRows = z1->size;

  FILE * iFile = fopen(str, "w");
  for (int i=0;i<NRows;i++)
  {
    fprintf(iFile, "%8.6e\t %8.6e\n",gsl_vector_get(z1,i), gsl_vector_get(z2,i));
  }
  fclose(iFile);
}

void SaveVectorWithoutIndex(gsl_vector * z, char * File)
{
    FILE *iFile;
    iFile = fopen(File, "w");
    int NRows = z->size;
    for (int i=0;i<NRows;i++)
      fprintf(iFile, "%d\t %8.6f\n",i, gsl_vector_get(z,i));

    fclose(iFile);
}

long timediff(clock_t t1, clock_t t2)
{
  long elapsed;
  elapsed = ((double)t2 -t1) / CLOCKS_PER_SEC * 1000;
  return elapsed;
}
    
void PrintInitInfo(void) 
{
  printf("##############################################################################\n");
  printf("#                                                                            #\n");
  printf("# CG-METHOD                                                                  #\n");
  printf("#                                                                            #\n");
  printf("# This program computes mesoscopic variables from microscopic configurations #\n");
  printf("#                                                                            #\n");
  printf("# Last stable version can be found in                                        #\n");
  printf("#    https://github.com/UNEDSoftMatter/CG-Method.git                         #\n");
  printf("#                                                                            #\n");
  printf("##############################################################################\n");
  printf("#                                                                            #\n");
  printf("# If called without arguments, CG-Method will create snapshots from lammps   #\n");
  printf("# output files (see README.md to find information about the correct format)  #\n");
  printf("#                                                                            #\n");
  printf("# If called with an argument, CG-Method will use the argument as an input    #\n");
  printf("# file which contains the list of snapshots                                  #\n");
  printf("#                                                                            #\n");
  printf("# In both cases, the snapshots are located in data/positions/x?????.pos      #\n");
  printf("# with the format 'TYPE x y z' for the positions, and                        #\n");
  printf("# in data/velocities/*.vel with the format 'vx vy vz' for the velocities     #\n");
  printf("#                                                                            #\n");
  printf("# (See README.md to know how to format lammps dump files)                    #\n");
  printf("#                                                                            #\n");
  printf("##############################################################################\n");
  printf("#                                                                            #\n");
  printf("# Mesoscopic profiles will be stored in ./output/                            #\n");
  printf("#                                                                            #\n");
  printf("# Microscopic visualization (if any) will be stored in ./povray/             #\n");
  printf("#                                                                            #\n");
  printf("# Log files (if any) will be stored in ./log/                                #\n");
  printf("#                                                                            #\n");
  printf("##############################################################################\n\n");
}

void ReadInputFiles(char * iFileStr, char iFiles[][7])
{
    FILE *iFile;
    iFile = fopen(iFileStr, "r");
    if (!iFile)
    {
        PrintMsg("Error reading input file. Exiting now...");
        printf("\tThe input file was: %s\n", iFileStr);
    }

    int line=NSteps;
    int i=0;

    while (line--)
    {
        fscanf(iFile,"%s",iFiles[i]);
        i++;
    }

    fclose(iFile);
}

void PrintInfo(int Step, gsl_vector * vector, FILE* fileptr)
{
  fprintf(fileptr, "%10d", Step);

  for (int i=0;i<vector->size;i++)
    fprintf(fileptr, "\t%8.6e", gsl_vector_get(vector,i));

  fprintf(fileptr,"\n");
}

void PrepareInputFiles(void)
{
  char str[100];
  char NLines[6];
  int SizeOfChunk = NParticles+9;

  #pragma omp parallel sections num_threads(2)
  {
    #pragma omp section
    {
      // Processing positions file
      PrintMsg("Processing positions file...");
      system("if [ ! -d data/positions ]; then mkdir -p data/positions; fi");
      strcpy(str,"cd data/positions; ln -s ../../");
      strcat(str,PositionsFileStr);
      strcat(str," ./output.positions");
      system(str);
      sprintf(NLines,"%d",SizeOfChunk);
      strcpy(str,"cd data/positions; split -a 5 -d --lines=");
      strcat(str,NLines);
      strcat(str," output.positions");
      system(str);
      system("cd data/positions; for i in $(ls |grep x); do cat $i | tail -n +10 | sort -n |awk '{print $2,$3,$4,$5}' > $i.pos ; rm $i ; done");
      system("rm data/positions/output.positions");
    }
    #pragma omp section
    {
      // Processing velocities file
      PrintMsg("Processing velocities file...");
      system("if [ ! -d data/velocities ]; then mkdir -p data/velocities; fi");
      strcpy(str,"cd data/velocities; ln -s ../../");
      strcat(str,VelocitiesFileStr);
      strcat(str," ./output.velocities");
      system(str);
      strcpy(str,"cd data/velocities; split -a 5 -d --lines=");
      strcat(str,NLines);
      strcat(str," output.velocities");
      system(str);
      system("cd data/velocities/; for i in $(ls |grep x); do cat $i | tail -n +10 | sort -n |awk '{print $3,$4,$5}' > $i.vel ; rm $i ; done");
      system("rm data/velocities/output.velocities");
    }
  }

  // Create snapshot list
  PrintMsg("Creating snapshot list in file 'sim'...");
  system("for i in $(ls data/positions/ | grep .pos); do basename $i .pos; done > sim");
}


void PrintScalarWithIndex(int Step, double Value, FILE*fileptr)
{
    fprintf(fileptr, "%10d\t%8.6e\n", Step, Value);
}
