/*************************************************************************

  snaphu main source file
  Written by Curtis W. Chen
  Copyright 2002 Board of Trustees, Leland Stanford Jr. University
  Please see the supporting documentation for terms of use.
  No warranty.

*************************************************************************/

#include <cstdlib>
#include <csignal>
#include <iomanip>
#include <unistd.h>
#include <sys/wait.h>

#include <isce3/except/Error.h>

#include "snaphu.h"
#include "snaphu_unwrap.h"

namespace isce3::unwrap {

/* global (external) variable definitions */

/* flags used for signal handling */
char dumpresults_global = FALSE;
char requestedstop_global = FALSE;

/* node pointer for marking arc not on tree in apex array */
/* this should be treated as a constant */
nodeT NONTREEARC[1]={};

/* static (local) function prototypes */
static
int Unwrap(infileT *infiles, outfileT *outfiles, paramT *params,
           long linelen, long nlines);
template<class CostTag>
static
int Unwrap(infileT *infiles, outfileT *outfiles, paramT *params,
           long linelen, long nlines, CostTag tag);
template<class CostTag>
static
int UnwrapTile(infileT *infiles, outfileT *outfiles, paramT *params,
               tileparamT *tileparams, long nlines, long linelen, CostTag tag);



/***************************/
/* main program for snaphu */
/***************************/

void snaphuUnwrap(const std::string& configfile){

  /* variable declarations */
  infileT infiles[1]={};
  outfileT outfiles[1]={};
  paramT params[1]={};
  time_t tstart;
  double cputimestart;
  long linelen, nlines;

  auto info=pyre::journal::info_t("isce3.unwrap.snaphu");

  /* get current wall clock and CPU time */
  StartTimers(&tstart,&cputimestart);

  /* print greeting */
  info << pyre::journal::at(__HERE__)
       << PROGRAMNAME << " v" << VERSION
       << pyre::journal::endl;

  /* set default parameters */
  SetDefaults(infiles,outfiles,params);
  ReadConfigFile(DEF_SYSCONFFILE,infiles,outfiles,&linelen,params);

  /* read input config file */
  ReadConfigFile(configfile.data(),infiles,outfiles,&linelen,params);

  /* set names of dump files if necessary */
  SetDumpAll(outfiles,params);

  /* get number of lines in file */
  nlines=GetNLines(infiles,linelen,params);

  /* check validity of parameters */
  CheckParams(infiles,outfiles,linelen,nlines,params);

  /* log the runtime parameters */
  WriteConfigLogFile(infiles,outfiles,linelen,params);

  /* unwrap, forming tiles and reassembling if necessary */
  Unwrap(infiles,outfiles,params,linelen,nlines);
    
  /* finish up */
  info << pyre::journal::at(__HERE__)
       << "Program " << PROGRAMNAME << " done"
       << pyre::journal::endl;
  DisplayElapsedTime(tstart,cputimestart);

} /* end of snaphuUnwrap() */


/* function: Unwrap()
 * ------------------
 * Sets parameters for each tile and calls UnwrapTile() to do the
 * unwrapping.
 */
static
int Unwrap(infileT *infiles, outfileT *outfiles, paramT *params,
           long linelen, long nlines){

  /* tag dispatch based on cost mode */
  if(params->p<0){
    if(params->costmode==TOPO){
      Unwrap(infiles,outfiles,params,linelen,nlines,TopoCostTag{});
    }else if(params->costmode==DEFO){
      Unwrap(infiles,outfiles,params,linelen,nlines,DefoCostTag{});
    }else if(params->costmode==SMOOTH){
      Unwrap(infiles,outfiles,params,linelen,nlines,SmoothCostTag{});
    }
  }else{
    if(params->bidirlpn){
      if(params->p==0){
        Unwrap(infiles,outfiles,params,linelen,nlines,L0BiDirCostTag{});
      }else if(params->p==1){
        Unwrap(infiles,outfiles,params,linelen,nlines,L1BiDirCostTag{});
      }else if(params->p==2){
        Unwrap(infiles,outfiles,params,linelen,nlines,L2BiDirCostTag{});
      }else{
        Unwrap(infiles,outfiles,params,linelen,nlines,LPBiDirCostTag{});
      }
    }else{
      if(params->p==0){
        Unwrap(infiles,outfiles,params,linelen,nlines,L0CostTag{});
      }else if(params->p==1){
        Unwrap(infiles,outfiles,params,linelen,nlines,L1CostTag{});
      }else if(params->p==2){
        Unwrap(infiles,outfiles,params,linelen,nlines,L2CostTag{});
      }else{
        Unwrap(infiles,outfiles,params,linelen,nlines,LPCostTag{});
      }
    }
  }
  return(0);

} /* end of Unwrap() */


/* function: Unwrap()
 * ------------------
 * Sets parameters for each tile and calls UnwrapTile() to do the
 * unwrapping.
 */
template<class CostTag>
static
int Unwrap(infileT *infiles, outfileT *outfiles, paramT *params,
           long linelen, long nlines, CostTag tag){

  long optiter, noptiter;
  long nexttilerow, nexttilecol, ntilerow, ntilecol, nthreads, nchildren;
  long sleepinterval;
  tileparamT tileparams[1]={};
  infileT iterinfiles[1]={};
  outfileT iteroutfiles[1]={};
  outfileT tileoutfiles[1]={};
  paramT iterparams[1]={};
  char tileinitfile[MAXSTRLEN]={};
  pid_t pid;
  int childstatus;
  double tilecputimestart;
  time_t tiletstart;

  auto info=pyre::journal::info_t("isce3.unwrap.snaphu");

  /* see if we need to do single-tile reoptimization and set up if so */
  if(params->onetilereopt){
    noptiter=2;
  }else{
    noptiter=1;
  }
  
  /* iterate if necessary for single-tile reoptimization */
  for(optiter=0;optiter<noptiter;optiter++){

    /* initialize input and output file structures for this iteration */
    iterinfiles[0]=*infiles;
    iteroutfiles[0]=*outfiles;
    iterparams[0]=*params;

    /* set up for iteration if doing tile init and one-tile reoptimization*/
    if(optiter==0){

      /* first iteration: see if there will be another iteration */
      if(noptiter>1){

        /* set up to write tile-mode unwrapped result to temporary file */
        SetTileInitOutfile(iteroutfiles->outfile,iterparams->parentpid);
        StrNCopy(tileinitfile,iteroutfiles->outfile,MAXSTRLEN);
        iteroutfiles->outfileformat=TILEINITFILEFORMAT;
        info << pyre::journal::at(__HERE__)
             << "Starting first-round tile-mode unwrapping"
             << pyre::journal::endl;
        
      }
      
    }else if(optiter==1){

      /* second iteration */
      /* set up to read unwrapped tile-mode result as single tile */
      StrNCopy(iterinfiles->infile,tileinitfile,MAXSTRLEN);
      iterinfiles->unwrappedinfileformat=TILEINITFILEFORMAT;
      iterparams->unwrapped=TRUE;
      iterparams->ntilerow=1;
      iterparams->ntilecol=1;
      iterparams->rowovrlp=0;
      iterparams->colovrlp=0;
      info << pyre::journal::at(__HERE__)
           << "Starting second-round single-tile unwrapping"
           << pyre::journal::endl;
      
    }else{
      throw isce3::except::RuntimeError(ISCE_SRCINFO(),
              "Illegal optiter value in Unwrap()");
    }
    
    /* set up for unwrapping */
    ntilerow=iterparams->ntilerow;
    ntilecol=iterparams->ntilecol;
    nthreads=iterparams->nthreads;
    dumpresults_global=FALSE;
    requestedstop_global=FALSE;

    /* do the unwrapping */
    if(ntilerow==1 && ntilecol==1){

      /* only single tile */

      /* do the unwrapping */
      tileparams->firstrow=iterparams->piecefirstrow;
      tileparams->firstcol=iterparams->piecefirstcol;
      tileparams->nrow=iterparams->piecenrow;
      tileparams->ncol=iterparams->piecencol;
      UnwrapTile(iterinfiles,iteroutfiles,iterparams,tileparams,nlines,linelen,tag);

    }else{

      /* don't unwrap if in assemble-only mode */
      if(!iterparams->assembleonly){

        /* set up mask for which tiles should be unwrapped */
        auto dotilemask=SetUpDoTileMask(iterinfiles,ntilerow,ntilecol);

        /* make a temporary directory into which tile files will be written */
        MakeTileDir(iterparams,iteroutfiles);

        /* different code for parallel or nonparallel operation */
        if(nthreads>1){

          /* parallel code */

          /* initialize */
          nexttilerow=0;
          nexttilecol=0;
          nchildren=0;
          sleepinterval=(long )ceil(nlines*linelen
                                    /((double )(ntilerow*ntilecol))
                                    *SECONDSPERPIXEL);

          /* trap signals so children get killed if parent dies */
          CatchSignals(KillChildrenExit);

          /* loop until we're done unwrapping */
          while(TRUE){

            /* unwrap next tile if there are free processors and tiles left */
            if(nchildren<nthreads && nexttilerow<ntilerow){

              /* see if next tile needs to be unwrapped */
              if(dotilemask(nexttilerow,nexttilecol)){

                /* wait to make sure file i/o, threads, and OS are synched */
                sleep(sleepinterval);
                
                /* fork to create new process */
                fflush(NULL);
                pid=fork();

              }else{

                /* tile did not need unwrapping, so set pid to parent pid */
                pid=iterparams->parentpid;

              }

              /* see if parent or child (or error) */
              if(pid<0){

                /* parent kills children and exits if there was a fork error */
                fflush(NULL);
                kill(0,SIGKILL);
                throw isce3::except::RuntimeError(ISCE_SRCINFO(),
                        "Error while forking");

              }else if(pid==0){

                /* child executes this code after fork */

                /* reset signal handlers so that children exit nicely */
                CatchSignals(SignalExit);

                /* start timers for this tile */
                StartTimers(&tiletstart,&tilecputimestart);

                /* set up tile parameters */
                pid=getpid();
                info << pyre::journal::at(__HERE__)
                     << "Unwrapping tile at row " << nexttilerow
                     << ", column " << nexttilecol << " (pid "
                     << ((long )pid) << ")"
                     << pyre::journal::endl;
                SetupTile(nlines,linelen,iterparams,tileparams,
                          iteroutfiles,tileoutfiles,
                          nexttilerow,nexttilecol);
              
                /* unwrap the tile */
                UnwrapTile(iterinfiles,tileoutfiles,iterparams,tileparams,
                           nlines,linelen,tag);

                /* log elapsed time */
                DisplayElapsedTime(tiletstart,tilecputimestart);

                /* child exits when done unwrapping */
                exit(NORMAL_EXIT);

              }
              
              /* parent executes this code after fork */

              /* increment tile counters */
              if(++nexttilecol==ntilecol){
                nexttilecol=0;
                nexttilerow++;
              }

              /* increment counter of running child processes */
              if(pid!=iterparams->parentpid){
                nchildren++;
              }

            }else{

              /* wait for a child to finish (only parent gets here) */
              pid=wait(&childstatus);

              /* make sure child exited cleanly */
              if(!(WIFEXITED(childstatus)) || (WEXITSTATUS(childstatus))!=0){
                fflush(NULL);
                signal(SIGTERM,SIG_IGN);
                kill(0,SIGTERM);
                throw isce3::except::RuntimeError(ISCE_SRCINFO(),
                        "Unexpected or abnormal exit of child process " +
                        std::to_string(pid));
              }

              /* we're done if there are no more active children */
              /* shouldn't really need this sleep(), but be extra sure child */
              /*   outputs are really flushed and written to disk by OS */
              if(--nchildren==0){
                sleep(sleepinterval);
                break;
              }

            } /* end if free processor and tiles remaining */
          } /* end while loop */

          /* return signal handlers to default behavior */
          CatchSignals(SIG_DFL);

        }else{

          /* nonparallel code */

          /* loop over all tiles */
          for(nexttilerow=0;nexttilerow<ntilerow;nexttilerow++){
            for(nexttilecol=0;nexttilecol<ntilecol;nexttilecol++){
              if(dotilemask(nexttilerow,nexttilecol)){

                /* set up tile parameters */
                info << pyre::journal::at(__HERE__)
                     << "Unwrapping tile at row " << nexttilerow
                     << ", column " << nexttilecol
                     << pyre::journal::endl;
                SetupTile(nlines,linelen,iterparams,tileparams,
                          iteroutfiles,tileoutfiles,
                          nexttilerow,nexttilecol);
            
                /* unwrap the tile */
                UnwrapTile(iterinfiles,tileoutfiles,iterparams,tileparams,
                           nlines,linelen,tag);

              }
            }
          }

        } /* end if nthreads>1 */

      } /* end if !iterparams->assembleonly */

      /* reassemble tiles */
      AssembleTiles(iteroutfiles,iterparams,nlines,linelen,tag);
    
    } /* end if multiple tiles */

    /* remove temporary tile file if desired at end of second iteration */
    if(iterparams->rmtileinit && optiter>0){
      unlink(tileinitfile);
    }
    
  } /* end of optiter loop */
  
  /* done */
  return(0);
  
} /* end of Unwrap() */


/* function: UnwrapTile()
 * ----------------------
 * This is the main phase unwrapping function for a single tile.
 */
template<class CostTag>
static
int UnwrapTile(infileT *infiles, outfileT *outfiles, paramT *params,
               tileparamT *tileparams,  long nlines, long linelen, CostTag tag){

  /* variable declarations */
  long nrow, ncol, nnoderow, narcrow, n, ngroundarcs, iincrcostfile;
  long nflow, ncycle, mostflow, nflowdone;
  long candidatelistsize, candidatebagsize;
  long isource, nsource;
  long nincreasedcostiter;
  totalcostT totalcost, oldtotalcost, mintotalcost;
  nodeT *source;
  nodeT ground[1]={};
  signed char notfirstloop, allmasked;
  bucketT bkts[1]={};

  Array2D<float> mag, wrappedphase, unwrappedest;
  Array2D<short> flows;
  Array1D<candidateT> candidatebag, candidatelist;
  Array1D<int> nnodesperrow;
  Array1D<int> narcsperrow;
  Array2D<signed char> iscandidate;
  Array2D<nodeT*> apexes;
  Array2D<incrcostT> incrcosts;
  Array2D<short> mstcosts;

  using Cost=typename CostTag::Cost;
  Array2D<Cost> costs;

  auto info=pyre::journal::info_t("isce3.unwrap.snaphu");
  constexpr int output_detail_level=2;
  auto verbose=pyre::journal::info_t("isce3.unwrap.snaphu",output_detail_level);
  auto status=pyre::journal::info_t("isce3.unwrap.snaphu.status",output_detail_level);

  /* get size of tile */
  nrow=tileparams->nrow;
  ncol=tileparams->ncol;

  /* read input file (memory allocated by read function) */
  ReadInputFile(infiles,&mag,&wrappedphase,&flows,linelen,nlines,
                params,tileparams);

  /* read interferogram magnitude if specified separately */
  ReadMagnitude(mag,infiles,linelen,nlines,tileparams);

  /* read mask file and apply to magnitude */
  ReadByteMask(mag,infiles,linelen,nlines,tileparams,params);

  /* make sure we have at least one pixel that is not masked */
  allmasked=CheckMagMasking(mag,nrow,ncol);

  /* read the coarse unwrapped estimate, if provided */
  if(strlen(infiles->estfile)){
    ReadUnwrappedEstimateFile(&unwrappedest,infiles,linelen,nlines,
                              params,tileparams);

    /* subtract the estimate from the wrapped phase (and re-wrap) */
    FlattenWrappedPhase(wrappedphase,unwrappedest,nrow,ncol);

  }

  /* build the cost arrays */  
  BuildCostArrays(&costs,&mstcosts,mag,wrappedphase,unwrappedest,
                  linelen,nlines,nrow,ncol,params,tileparams,infiles,outfiles,tag);

  /* if in quantify-only mode, evaluate cost of unwrapped input then return */
  if(params->eval){
    mostflow=Short2DRowColAbsMax(flows,nrow,ncol);
    info << pyre::journal::at(__HERE__)
         << "Maximum flow on network: " << mostflow
         << pyre::journal::endl;
    Array1D<int> dummy;
    totalcost=EvaluateTotalCost(costs,flows,nrow,ncol,dummy,params,tag);
    info << pyre::journal::at(__HERE__) << "Total solution cost: "
         << std::fixed << std::setprecision(9) << ((double )totalcost)
         << pyre::journal::endl;
    return(1);
  }

  /* set network function pointers for grid network */
  SetGridNetworkFunctionPointers();

  /* initialize the flows (find simple unwrapping to get a feasible flow) */
  Array2D<nodeT> nodes;
  if(!params->unwrapped){

    /* see which initialization method to use */
    if(params->initmethod==MSTINIT){

      /* use minimum spanning tree (MST) algorithm */
      MSTInitFlows(wrappedphase,&flows,mstcosts,nrow,ncol,
                   &nodes,ground,params->initmaxflow);
    
    }else if(params->initmethod==MCFINIT){

      /* use minimum cost flow (MCF) algorithm */
      MCFInitFlows(wrappedphase,&flows,mstcosts,nrow,ncol);

    }else{
      fflush(NULL);
      throw isce3::except::InvalidArgument(ISCE_SRCINFO(),
              "Illegal initialization method");
    }

    /* integrate the phase and write out if necessary */
    if(params->initonly || strlen(outfiles->initfile)){
      info << pyre::journal::at(__HERE__)
           << "Integrating phase"
           << pyre::journal::endl;
      auto unwrappedphase=Array2D<float>(nrow,ncol);
      IntegratePhase(wrappedphase,unwrappedphase,flows,nrow,ncol);
      if(unwrappedest.size()){
        Add2DFloatArrays(unwrappedphase,unwrappedest,nrow,ncol);
      }
      FlipPhaseArraySign(unwrappedphase,params,nrow,ncol);

      /* return if called in init only; otherwise, free memory and continue */
      if(params->initonly){
        info << pyre::journal::at(__HERE__)
             << "Writing output to file " << outfiles->outfile
             << pyre::journal::endl;
        WriteOutputFile(mag,unwrappedphase,outfiles->outfile,outfiles,
                        nrow,ncol);
        return(1);
      }else{
        verbose << pyre::journal::at(__HERE__)
                << "Writing initialization to file " << outfiles->initfile
                << pyre::journal::endl;
        WriteOutputFile(mag,unwrappedphase,outfiles->initfile,outfiles,
                        nrow,ncol);
      }
    }
  }

  /* initialize network variables */
  InitNetwork(flows,&ngroundarcs,&ncycle,&nflowdone,&mostflow,&nflow,
              &candidatebagsize,&candidatebag,&candidatelistsize,
              &candidatelist,&iscandidate,&apexes,bkts,&iincrcostfile,
              &incrcosts,&nodes,ground,&nnoderow,&nnodesperrow,&narcrow,
              &narcsperrow,nrow,ncol,&notfirstloop,&totalcost,params);
  oldtotalcost=totalcost;
  mintotalcost=totalcost;
  nincreasedcostiter=0;

  /* regrow regions with -G parameter */
  if(params->regrowconncomps){

    /* grow connected components */
    GrowConnCompsMask(costs,flows,nrow,ncol,incrcosts,outfiles,params,tag);

    /* free up remaining memory and return */
    return(1);
  }

  /* mask zero-magnitude nodes so they are not considered in optimization */
  MaskNodes(nrow,ncol,nodes,ground,mag);

  /* if we have a single tile, trap signals for dumping results */
  if(params->ntilerow==1 && params->ntilecol==1){
    signal(SIGINT,SetDump);
    signal(SIGHUP,SetDump);
  }

  /* main loop: loop over flow increments and sources */
  if(!allmasked){
    info << pyre::journal::at(__HERE__)
         << "Running nonlinear network flow optimizer"
         << pyre::journal::endl
         << "Maximum flow on network: " << mostflow
         << pyre::journal::endl;
    verbose << pyre::journal::at(__HERE__)
            << "Number of nodes in network: " << ((nrow-1)*(ncol-1)+1)
            << pyre::journal::endl;
    while(TRUE){ 
 
      info << pyre::journal::at(__HERE__)
           << "Flow increment: " << nflow
           << "  (Total improvements: " << ncycle << ")"
           << pyre::journal::endl;

      /* set up the incremental (residual) cost arrays */
      SetupIncrFlowCosts(costs,incrcosts,flows,nflow,nrow,narcrow,narcsperrow,
                         params,tag);
      if(params->dumpall && params->ntilerow==1 && params->ntilecol==1){
        DumpIncrCostFiles(incrcosts,++iincrcostfile,nflow,nrow,ncol);
      }

      /* set the tree root (equivalent to source of shortest path problem) */
      Array1D<nodeT*> sourcelist;
      Array1D<long> nconnectedarr;
      nsource=SelectSources(nodes,mag,ground,nflow,flows,ngroundarcs,
                            nrow,ncol,params,&sourcelist,&nconnectedarr);

      /* set up network variables for tree solver */
      SetupTreeSolveNetwork(nodes,ground,apexes,iscandidate,
                            nnoderow,nnodesperrow,narcrow,narcsperrow,
                            nrow,ncol);
      
      /* loop over sources */
      n=0;
      for(isource=0;isource<nsource;isource++){

        /* set source */
        source=sourcelist[isource];

        /* show status if verbose */
        if(source->row==GROUNDROW){
          status << pyre::journal::at(__HERE__)
                 << "Source " << isource << ": (edge ground)"
                 << pyre::journal::endl;
        }else{
          status << pyre::journal::at(__HERE__)
                 << "Source " << isource << ": row, col = "
                 << source->row << ", " << source->col
                 << pyre::journal::endl;
        }

        /* run the solver, and increment nflowdone if no cycles are found */
        Array2D<nodesuppT> dummy;
        n+=TreeSolve(nodes,dummy,ground,source,
                     &candidatelist,&candidatebag,
                     &candidatelistsize,&candidatebagsize,
                     bkts,flows,costs,incrcosts,apexes,iscandidate,
                     ngroundarcs,nflow,mag,wrappedphase,outfiles->outfile,
                     nnoderow,nnodesperrow,narcrow,narcsperrow,nrow,ncol,
                     outfiles,nconnectedarr[isource],params,tag);
      }

      /* evaluate and save the total cost (skip if first loop through nflow) */
      Array1D<int> dummy;
      verbose << pyre::journal::at(__HERE__)
              << "Current solution cost: " << std::fixed << std::setprecision(16)
              << ((double )EvaluateTotalCost(costs,flows,nrow,ncol,dummy,params,tag))
              << pyre::journal::endl;
      if(notfirstloop){
        oldtotalcost=totalcost;
        totalcost=EvaluateTotalCost(costs,flows,nrow,ncol,dummy,params,tag);
        if(totalcost<mintotalcost){
          mintotalcost=totalcost;
        }
        if(totalcost>oldtotalcost || (n>0 && totalcost==oldtotalcost)){
          fflush(NULL);
          info << pyre::journal::at(__HERE__)
               << "Caution: Unexpected increase in total cost"
               << pyre::journal::endl;
        }
        if(totalcost > mintotalcost){
          nincreasedcostiter++;
        }else{
          nincreasedcostiter=0;
        }
      }

      /* consider this flow increment done if not too many neg cycles found */
      ncycle+=n;
      if(n<=params->maxnflowcycles){
        nflowdone++;
      }else{
        nflowdone=1;
      }

      /* find maximum flow on network, excluding arcs affected by masking */
      mostflow=MaxNonMaskFlow(flows,mag,nrow,ncol);
      if(nincreasedcostiter>=mostflow){
        fflush(NULL);
        auto warnings=pyre::journal::warning_t("isce3.unwrap.snaphu");
        warnings << pyre::journal::at(__HERE__)
                 << "WARNING: Unexpected sustained increase in total cost."
                 << "  Breaking loop"
                 << pyre::journal::endl;
        break;
      }

      /* break if we're done with all flow increments or problem is convex */
      if(nflowdone>=params->maxflow || nflowdone>=mostflow || params->p>=1.0){
        break;
      }

      /* update flow increment */
      nflow++;
      if(nflow>params->maxflow || nflow>mostflow){
        nflow=1;
        notfirstloop=TRUE;
      }
      verbose << pyre::journal::at(__HERE__)
              << "Maximum valid flow on network: " << mostflow
              << pyre::journal::endl;

      /* dump flow arrays if necessary */
      if(strlen(outfiles->flowfile)){
        FlipFlowArraySign(flows,params,nrow,ncol);
        Write2DRowColArray(flows,outfiles->flowfile,nrow,ncol,
                           sizeof(short));
        FlipFlowArraySign(flows,params,nrow,ncol);
      }

    } /* end loop until no more neg cycles */
  } /* end if all pixels masked */

  /* if we have single tile, return signal handlers to default behavior */
  if(params->ntilerow==1 && params->ntilecol==1){
    signal(SIGINT,SIG_DFL);
    signal(SIGHUP,SIG_DFL);
  }

  /* grow connected component mask */
  if(strlen(outfiles->conncompfile)){
    GrowConnCompsMask(costs,flows,nrow,ncol,incrcosts,outfiles,params,tag);
  }

  /* grow regions for tiling */
  if(params->ntilerow!=1 || params->ntilecol!=1){
    GrowRegions(costs,flows,nrow,ncol,incrcosts,outfiles,tileparams,params,tag);
  }

  /* evaluate and display the maximum flow and total cost */
  Array1D<int> dummy;
  totalcost=EvaluateTotalCost(costs,flows,nrow,ncol,dummy,params,tag);
  info << pyre::journal::at(__HERE__)
       << "Maximum flow on network: " << mostflow
       << pyre::journal::endl << "Total solution cost: "
       << std::fixed << std::setprecision(9) << ((double )totalcost)
       << pyre::journal::endl;

  /* integrate the wrapped phase using the solution flow */
  info << pyre::journal::at(__HERE__)
       << "Integrating phase"
       << pyre::journal::endl;
  auto unwrappedphase=Array2D<float>(nrow,ncol);
  IntegratePhase(wrappedphase,unwrappedphase,flows,nrow,ncol);

  /* reinsert the coarse estimate, if it was given */
  if(unwrappedest.size()){
    Add2DFloatArrays(unwrappedphase,unwrappedest,nrow,ncol);
  }

  /* flip the sign of the unwrapped phase array if it was flipped initially, */
  FlipPhaseArraySign(unwrappedphase,params,nrow,ncol);

  /* write the unwrapped output */
  info << pyre::journal::at(__HERE__)
       << "Writing output to file " << outfiles->outfile
       << pyre::journal::endl;
  WriteOutputFile(mag,unwrappedphase,outfiles->outfile,outfiles,
                  nrow,ncol);  

  /* free remaining memory and return */
  return(0);

} /* end of UnwrapTile() */

} // namespace isce3::unwrap
