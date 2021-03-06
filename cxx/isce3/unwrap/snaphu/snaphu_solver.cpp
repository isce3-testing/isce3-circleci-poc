/*************************************************************************

  snaphu network-flow solver source file
  Written by Curtis W. Chen
  Copyright 2002 Board of Trustees, Leland Stanford Jr. University
  Please see the supporting documentation for terms of use.
  No warranty.

*************************************************************************/

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <limits>

#include <isce3/except/Error.h>
#include <isce3/unwrap/ortools/min_cost_flow.h>

#include "snaphu.h"

namespace isce3::unwrap {

/* static variables local this file */

/* pointers to functions for tailoring network solver to specific topologies */
static nodeT *(*NeighborNode)(nodeT *, long, long *, Array2D<nodeT>&, nodeT *, long *,
                              long *, long *, long, long, boundaryT *,
                              Array2D<nodesuppT>&);
static void (*GetArc)(nodeT *, nodeT *, long *, long *, long *, long, long,
                      Array2D<nodeT>&, Array2D<nodesuppT>&);

/* static (local) function prototypes */
static
void AddNewNode(nodeT *from, nodeT *to, long arcdir, bucketT *bkts,
                long nflow, Array2D<incrcostT>& incrcosts, long arcrow, long arccol,
                paramT *params);
static
void CheckArcReducedCost(nodeT *from, nodeT *to, nodeT *apex,
                         long arcrow, long arccol, long arcdir,
                         Array1D<candidateT>* candidatebagptr,
                         long *candidatebagnextptr,
                         long *candidatebagsizeptr, Array2D<incrcostT>& incrcosts,
                         Array2D<signed char>& iscandidate, paramT *params);
static
nodeT *InitBoundary(nodeT *source, Array2D<nodeT>& nodes,
                    boundaryT *boundary, Array2D<nodesuppT>& nodesupp, Array2D<float>& mag,
                    nodeT *ground, long ngroundarcs, long nrow, long ncol,
                    paramT *params, long *nconnectedptr);
static
long CheckBoundary(Array2D<nodeT>& nodes, Array2D<float>& mag, nodeT *ground, long ngroundarcs,
                   boundaryT *boundary, long nrow, long ncol,
                   paramT *params, nodeT *start);
static
int IsRegionEdgeArc(Array2D<float>& mag, long arcrow, long arccol,
                    long nrow, long ncol);
static
int IsRegionInteriorArc(Array2D<float>& mag, long arcrow, long arccol,
                        long nrow, long ncol);
static
int IsRegionArc(Array2D<float>& mag, long arcrow, long arccol,
                long nrow, long ncol);
static
int IsRegionEdgeNode(Array2D<float>& mag, long row, long col, long nrow, long ncol);
static
int CleanUpBoundaryNodes(nodeT *source, boundaryT *boundary, Array2D<float>& mag,
                         Array2D<nodeT>& nodes, nodeT *ground,
                         long nrow, long ncol, long ngroundarcs,
                         Array2D<nodesuppT>& nodesupp);
static
int DischargeBoundary(Array2D<nodeT>& nodes, nodeT *ground,
                      boundaryT *boundary, Array2D<nodesuppT>& nodesupp, Array2D<short>& flows,
                      Array2D<signed char>& iscandidate, Array2D<float>& mag,
                      Array2D<float>& wrappedphase, long ngroundarcs,
                      long nrow, long ncol);
static
int InitTree(nodeT *source, Array2D<nodeT>& nodes,
             boundaryT *boundary, Array2D<nodesuppT>& nodesupp,
             nodeT *ground, long ngroundarcs, bucketT *bkts, long nflow,
             Array2D<incrcostT>& incrcosts, long nrow, long ncol, paramT *params);
static
nodeT *FindApex(nodeT *from, nodeT *to);
static
int CandidateCompare(const void *c1, const void *c2);
static
nodeT *NeighborNodeGrid(nodeT *node1, long arcnum, long *upperarcnumptr,
                        Array2D<nodeT>& nodes, nodeT *ground, long *arcrowptr,
                        long *arccolptr, long *arcdirptr, long nrow,
                        long ncol, boundaryT *boundary, Array2D<nodesuppT>& nodesupp);
static inline
long GetArcNumLims(long fromrow, long *upperarcnumptr,
                   long ngroundarcs, boundaryT *boundary);
static
nodeT *NeighborNodeNonGrid(nodeT *node1, long arcnum, long *upperarcnumptr,
                           Array2D<nodeT>& nodes, nodeT *ground, long *arcrowptr,
                           long *arccolptr, long *arcdirptr, long nrow,
                           long ncol, boundaryT *boundary,
                           Array2D<nodesuppT>& nodesupp);
static
void GetArcGrid(nodeT *from, nodeT *to, long *arcrow, long *arccol,
                long *arcdir, long nrow, long ncol,
                Array2D<nodeT>& nodes, Array2D<nodesuppT>& nodesupp);
static
void GetArcNonGrid(nodeT *from, nodeT *to, long *arcrow, long *arccol,
                   long *arcdir, long nrow, long ncol,
                   Array2D<nodeT>& nodes, Array2D<nodesuppT>& nodesupp);
static
void NonDegenUpdateChildren(nodeT *startnode, nodeT *lastnode,
                            nodeT *nextonpath, long dgroup,
                            long ngroundarcs, long nflow, Array2D<nodeT>& nodes,
                            Array2D<nodesuppT>& nodesupp, nodeT *ground,
                            boundaryT *boundary,
                            Array2D<nodeT*>& apexes, Array2D<incrcostT>& incrcosts,
                            long nrow, long ncol, paramT *params);
static
long PruneTree(nodeT *source, Array2D<nodeT>& nodes, nodeT *ground, boundaryT *boundary,
               Array2D<nodesuppT>& nodesupp, Array2D<incrcostT>& incrcosts,
               Array2D<short>& flows, long ngroundarcs, long prunecostthresh,
               long nrow, long ncol);
static
int CheckLeaf(nodeT *node1, Array2D<nodeT>& nodes, nodeT *ground, boundaryT *boundary,
              Array2D<nodesuppT>& nodesupp, Array2D<incrcostT>& incrcosts,
              Array2D<short>& flows, long ngroundarcs, long nrow, long ncol,
              long prunecostthresh);
static
int GridNodeMaskStatus(long row, long col, Array2D<float>& mag);
static
int GroundMaskStatus(long nrow, long ncol, Array2D<float>& mag);
static
int InitBuckets(bucketT *bkts, nodeT *source, long nbuckets);
static
nodeT *MinOutCostNode(bucketT *bkts);
static
nodeT *SelectConnNodeSource(Array2D<nodeT>& nodes, Array2D<float>& mag,
                            nodeT *ground, long ngroundarcs,
                            long nrow, long ncol,
                            paramT *params, nodeT *start, long *nconnectedptr);
static
long ScanRegion(nodeT *start, Array2D<nodeT>& nodes, Array2D<float>& mag,
                nodeT *ground, long ngroundarcs,
                long nrow, long ncol, int groupsetting);
static
short GetCost(Array2D<incrcostT>& incrcosts, long arcrow, long arccol,
              long arcdir);
static
void SolveMST(Array2D<nodeT>& nodes, nodeT *source, nodeT *ground,
              bucketT *bkts, Array2D<short>& mstcosts, Array2D<signed char>& residue,
              Array2D<signed char>& arcstatus, long nrow, long ncol);
static
long DischargeTree(nodeT *source, Array2D<short>& mstcosts, Array2D<short>& flows,
                   Array2D<signed char>& residue, Array2D<signed char>& arcstatus,
                   Array2D<nodeT>& nodes, nodeT *ground, long nrow, long ncol);
static
signed char ClipFlow(Array2D<signed char>& residue, Array2D<short>& flows,
                     Array2D<short>& mstcosts, long nrow, long ncol,
                     long maxflow);



/* function: SetGridNetworkFunctionPointers()
 * ------------------------------------------
 */
int SetGridNetworkFunctionPointers(void){

  /* set static pointers to functions */
  NeighborNode=NeighborNodeGrid;
  GetArc=GetArcGrid;
  return(0);
  
}


/* function: SetNonGridNetworkFunctionPointers()
 * ---------------------------------------------
 */
int SetNonGridNetworkFunctionPointers(void){

  /* set static pointers to functions */
  NeighborNode=NeighborNodeNonGrid;
  GetArc=GetArcNonGrid;
  return(0);

}


/* function: TreeSolve()
 * ---------------------
 * Solves the nonlinear network optimization problem.
 */
template<class CostTag>
long TreeSolve(Array2D<nodeT>& nodes, Array2D<nodesuppT>& nodesupp, nodeT *ground,
               nodeT *source, Array1D<candidateT>* candidatelistptr,
               Array1D<candidateT>* candidatebagptr, long *candidatelistsizeptr,
               long *candidatebagsizeptr, bucketT *bkts, Array2D<short>& flows,
               Array2D<typename CostTag::Cost>& costs, Array2D<incrcostT>& incrcosts, 
               Array2D<nodeT*>& apexes, Array2D<signed char>& iscandidate, 
               long ngroundarcs, long nflow, Array2D<float>& mag, 
               Array2D<float>& wrappedphase, char *outfile, long nnoderow, 
               Array1D<int>& nnodesperrow, long /*narcrow*/,
               Array1D<int>& /*narcsperrow*/, long nrow, long ncol,
               outfileT *outfiles, long nconnected, paramT *params, CostTag tag){

  long i, row, col, arcrow, arccol, arcdir, arcnum, upperarcnum;
  long arcrow1, arccol1, arcdir1, arcrow2, arccol2, arcdir2;
  long treesize, candidatelistsize, candidatebagsize;
  long violation, groupcounter, fromgroup, group1, apexlistbase, apexlistlen;
  long cyclecost, outcostto, startlevel, dlevel, doutcost, dincost;
  long candidatelistlen, candidatebagnext;
  long inondegen, ipivots, nnewnodes, maxnewnodes, templong;
  long nmajor, nmajorprune, npruned, prunecostthresh;
  signed char fromside;
  nodeT *from, *to, *cycleapex, *node1, *node2, *leavingparent, *leavingchild;
  nodeT *root, *mntpt, *oldmntpt, *skipthread, *tempnode1, *tempnode2;
  nodeT *firstfromnode, *firsttonode;
  boundaryT boundary[1]={};

  auto firewall=pyre::journal::firewall_t("isce3.unwrap.snaphu");
  auto warnings=pyre::journal::warning_t("isce3.unwrap.snaphu");

  /* a uniquely named channel that logs the current status of the tree solver */
  /* (may produce very verbose output) */
  constexpr int output_detail_level=2;
  auto status=pyre::journal::info_t("isce3.unwrap.snaphu.status",output_detail_level);

  /* initialize some variables to zero to stop compiler warnings */
  from=NULL;
  to=NULL;
  cycleapex=NULL;
  node1=NULL;
  node2=NULL;
  leavingparent=NULL;
  leavingchild=NULL;
  root=NULL;
  mntpt=NULL;
  oldmntpt=NULL;
  skipthread=NULL;
  tempnode1=NULL;
  tempnode2=NULL;
  firstfromnode=NULL;
  firsttonode=NULL;

  /* dereference some pointers and store as local variables */
  auto candidatelist=*candidatelistptr;
  auto candidatebag=*candidatebagptr;
  candidatelistsize=*candidatelistsizeptr;
  candidatebagsize=*candidatebagsizeptr;
  candidatelistlen=0;
  candidatebagnext=0;

  Array1D<candidateT>* localcandidatelistptr=&candidatelist;
  Array1D<candidateT>* localcandidatebagptr=&candidatebag;
  Array1D<candidateT>* tempcandidateptr=nullptr;

  /* initialize boundary, which affects network structure */
  /* recompute number of connected nodes since setting boundary may make */
  /*   some nodes inaccessible */
  source=InitBoundary(source,nodes,boundary,nodesupp,mag,
                      ground,ngroundarcs,nrow,ncol,params,&nconnected);

  /* set up */
  bkts->curr=bkts->maxind;
  InitTree(source,nodes,boundary,nodesupp,ground,ngroundarcs,bkts,nflow,
           incrcosts,nrow,ncol,params);
  apexlistlen=INITARRSIZE;
  auto apexlist = Array1D<nodeT*>(apexlistlen);
  groupcounter=2;
  ipivots=0;
  inondegen=0;
  maxnewnodes=ceil(nconnected*params->maxnewnodeconst);
  nnewnodes=0;
  treesize=1;
  npruned=0;
  nmajor=0;
  nmajorprune=params->nmajorprune;
  prunecostthresh=params->prunecostthresh;;
  status << pyre::journal::at(__HERE__)
         << "Treesize: " << std::left << std::setw(10) << treesize
         << " Pivots: " << std::setw(11) << ipivots
         << " Improvements: " << std::setw(11) << inondegen
         << pyre::journal::endl;

  /* loop over each entering node (note, source already on tree) */
  while(treesize<nconnected){

    nnewnodes=0;
    while(nnewnodes<maxnewnodes && treesize<nconnected){

      /* get node with lowest outcost */
      to=MinOutCostNode(bkts);
      from=to->pred;

      /* add new node to the tree */
      GetArc(from,to,&arcrow,&arccol,&arcdir,nrow,ncol,nodes,nodesupp);
      to->group=1;
      to->level=from->level+1;
      to->incost=from->incost+GetCost(incrcosts,arcrow,arccol,-arcdir);
      to->next=from->next;
      to->prev=from;
      to->next->prev=to;
      from->next=to;

      /* scan new node's neighbors */
      from=to;
      arcnum=GetArcNumLims(from->row,&upperarcnum,ngroundarcs,boundary);
      while(arcnum<upperarcnum){

        /* get row, col indices and distance of next node */
        to=NeighborNode(from,++arcnum,&upperarcnum,nodes,ground,
                        &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);

        /* if to node is on tree */
        if(to->group>0){
          if(to!=from->pred){
            cycleapex=FindApex(from,to);
            apexes(arcrow,arccol)=cycleapex;
            CheckArcReducedCost(from,to,cycleapex,arcrow,arccol,arcdir,
                                localcandidatebagptr,&candidatebagnext,
                                &candidatebagsize,incrcosts,iscandidate,
                                params);
          }else{
            apexes(arcrow,arccol)=NULL;
          }

        }else if(to->group!=PRUNED && to->group!=MASKED){

          /* if to is not on tree, update outcost and add to bucket */
          AddNewNode(from,to,arcdir,bkts,nflow,incrcosts,arcrow,arccol,params);

        }
      }
      nnewnodes++;
      treesize++;
    }

    /* keep looping until no more arcs have negative reduced costs */
    while(candidatebagnext){

      /* if we received SIGINT or SIGHUP signal, dump results */
      /* keep this stuff out of the signal handler so we don't risk */
      /* writing a non-feasible solution (ie, if signal during augment) */
      /* signal handler disabled for all but primary (grid) networks */
      if(dumpresults_global){
        fflush(NULL);
        warnings << pyre::journal::at(__HERE__)
                 << pyre::journal::newline << pyre::journal::newline
                 << "Dumping current solution to file " << outfile
                 << pyre::journal::endl;
        auto unwrappedphase = Array2D<float>(nrow,ncol);
        IntegratePhase(wrappedphase,unwrappedphase,flows,nrow,ncol);
        FlipPhaseArraySign(unwrappedphase,params,nrow,ncol);
        WriteOutputFile(mag,unwrappedphase,outfiles->outfile,outfiles,
                        nrow,ncol);  
        if(requestedstop_global){
          fflush(NULL);
          throw isce3::except::RuntimeError(ISCE_SRCINFO(),
                  "Received interrupt or hangup signal");
        }
        dumpresults_global=FALSE;
        fflush(NULL);
        warnings << pyre::journal::at(__HERE__)
                 << pyre::journal::newline << pyre::journal::newline
                 << "Program continuing" << pyre::journal::endl;
      }

      /* swap candidate bag and candidate list pointers and sizes */
      tempcandidateptr=localcandidatebagptr;
      localcandidatebagptr=localcandidatelistptr;
      localcandidatelistptr=tempcandidateptr;
      templong=candidatebagsize;
      candidatebagsize=candidatelistsize;
      candidatelistsize=templong;
      candidatelistlen=candidatebagnext;
      candidatebagnext=0;

      /* sort candidate list by violation, with augmenting arcs always first */
      qsort((void *)localcandidatelistptr->data(),candidatelistlen,sizeof(candidateT),
            CandidateCompare);

      /* set all arc directions to be plus/minus 1 */
      for(i=0;i<candidatelistlen;i++){
        if((*localcandidatelistptr)[i].arcdir>1){
          (*localcandidatelistptr)[i].arcdir=1;
        }else if((*localcandidatelistptr)[i].arcdir<-1){
          (*localcandidatelistptr)[i].arcdir=-1;
        }
      }

      /* this doesn't seem to make it any faster, so just do all of them */
      /* set the number of candidates to process */
      /* (must change candidatelistlen to ncandidates in for loop below) */
      /*
      maxcandidates=MAXCANDIDATES;
      if(maxcandidates>candidatelistlen){
        ncandidates=candidatelistlen;
      }else{
        ncandidates=maxcandidates;
      }
      */

      /* now pivot for each arc in the candidate list */
      for(i=0;i<candidatelistlen;i++){

        /* get arc info */
        from=(*localcandidatelistptr)[i].from;
        to=(*localcandidatelistptr)[i].to;
        arcdir=(*localcandidatelistptr)[i].arcdir;
        arcrow=(*localcandidatelistptr)[i].arcrow;
        arccol=(*localcandidatelistptr)[i].arccol;

        /* unset iscandidate */
        iscandidate(arcrow,arccol)=FALSE;

        /* make sure the next arc still has a negative violation */
        outcostto=from->outcost+
          GetCost(incrcosts,arcrow,arccol,arcdir);
        cyclecost=outcostto + to->incost
          -apexes(arcrow,arccol)->outcost
          -apexes(arcrow,arccol)->incost;

        /* if violation no longer negative, check reverse arc */
        if(!((outcostto < to->outcost) || (cyclecost < 0))){
          from=to;
          to=(*localcandidatelistptr)[i].from;
          arcdir=-arcdir;
          outcostto=from->outcost+
            GetCost(incrcosts,arcrow,arccol,arcdir);
          cyclecost=outcostto + to->incost
            -apexes(arcrow,arccol)->outcost
            -apexes(arcrow,arccol)->incost;
        }

        /* see if the cycle is negative (see if there is a violation) */
        if((outcostto < to->outcost) || (cyclecost < 0)){

          /* make sure the group counter hasn't gotten too big */
          if(++groupcounter>MAXGROUPBASE){
            for(row=0;row<nnoderow;row++){
              for(col=0;col<nnodesperrow[row];col++){
                if(nodes(row,col).group>0){
                  nodes(row,col).group=1;
                }
              }
            }
            if(ground!=NULL && ground->group>0){
              ground->group=1;
            }
            if(boundary->node->group>0){
              boundary->node->group=1;
            }
            groupcounter=2;
          }

          /* if augmenting cycle (nondegenerate pivot) */
          if(cyclecost<0){

            /* augment flow along cycle and select leaving arc */
            /* if we are augmenting non-zero flow, any arc with zero flow */
            /* after the augmentation is a blocking arc */
            while(TRUE){
              fromside=TRUE;
              node1=from;
              node2=to;
              leavingchild=NULL;
              flows(arcrow,arccol)+=arcdir*nflow;
              ReCalcCost(costs,incrcosts,flows(arcrow,arccol),arcrow,arccol,
                         nflow,nrow,params,tag);
              violation=GetCost(incrcosts,arcrow,arccol,arcdir);
              if(node1->level > node2->level){
                while(node1->level != node2->level){
                  GetArc(node1->pred,node1,&arcrow1,&arccol1,&arcdir1,
                         nrow,ncol,nodes,nodesupp);
                  flows(arcrow1,arccol1)+=(arcdir1*nflow);
                  ReCalcCost(costs,incrcosts,flows(arcrow1,arccol1),
                             arcrow1,arccol1,nflow,nrow,params,tag);
                  if(leavingchild==NULL
                     && !flows(arcrow1,arccol1)){
                    leavingchild=node1;
                  }
                  violation+=GetCost(incrcosts,arcrow1,arccol1,arcdir1);
                  node1->group=groupcounter+1;
                  node1=node1->pred;
                }
              }else{
                while(node1->level != node2->level){
                  GetArc(node2->pred,node2,&arcrow2,&arccol2,&arcdir2,
                         nrow,ncol,nodes,nodesupp);
                  flows(arcrow2,arccol2)-=(arcdir2*nflow);
                  ReCalcCost(costs,incrcosts,flows(arcrow2,arccol2),
                             arcrow2,arccol2,nflow,nrow,params,tag);
                  if(!flows(arcrow2,arccol2)){
                    leavingchild=node2;
                    fromside=FALSE;
                  }
                  violation+=GetCost(incrcosts,arcrow2,arccol2,-arcdir2);
                  node2->group=groupcounter;
                  node2=node2->pred;
                }
              }
              while(node1!=node2){
                GetArc(node1->pred,node1,&arcrow1,&arccol1,&arcdir1,nrow,ncol,
                       nodes,nodesupp);
                GetArc(node2->pred,node2,&arcrow2,&arccol2,&arcdir2,nrow,ncol,
                       nodes,nodesupp);
                flows(arcrow1,arccol1)+=(arcdir1*nflow);
                flows(arcrow2,arccol2)-=(arcdir2*nflow);
                ReCalcCost(costs,incrcosts,flows(arcrow1,arccol1),
                           arcrow1,arccol1,nflow,nrow,params,tag);
                ReCalcCost(costs,incrcosts,flows(arcrow2,arccol2),
                           arcrow2,arccol2,nflow,nrow,params,tag);
                violation+=(GetCost(incrcosts,arcrow1,arccol1,arcdir1)
                            +GetCost(incrcosts,arcrow2,arccol2,-arcdir2));
                if(!flows(arcrow2,arccol2)){
                  leavingchild=node2;
                  fromside=FALSE;
                }else if(leavingchild==NULL
                         && !flows(arcrow1,arccol1)){
                  leavingchild=node1;
                }
                node1->group=groupcounter+1;
                node2->group=groupcounter;
                node1=node1->pred;
                node2=node2->pred;
              }
              if(violation>=0){
                break;
              }
            }
            inondegen++;

          }else{

            /* We are not augmenting flow, but just updating potentials. */
            /* Arcs with zero flow are implicitly directed upwards to */
            /* maintain a strongly feasible spanning tree, so arcs with zero */
            /* flow on the path between to node and apex are blocking arcs. */
            /* Leaving arc is last one whose child's new outcost is less */
            /* than its old outcost.  Such an arc must exist, or else */
            /* we'd be augmenting flow on a negative cycle. */
            
            /* trace the cycle and select leaving arc */
            fromside=FALSE;
            node1=from;
            node2=to;
            leavingchild=NULL;
            if(node1->level > node2->level){
              while(node1->level != node2->level){
                node1->group=groupcounter+1;
                node1=node1->pred;
              }
            }else{
              while(node1->level != node2->level){
                if(outcostto < node2->outcost){
                  leavingchild=node2;
                  GetArc(node2->pred,node2,&arcrow2,&arccol2,&arcdir2,
                         nrow,ncol,nodes,nodesupp);
                  outcostto+=GetCost(incrcosts,arcrow2,arccol2,-arcdir2);
                }else{
                  outcostto=VERYFAR;
                }
                node2->group=groupcounter;
                node2=node2->pred;
              }
            }
            while(node1!=node2){
              if(outcostto < node2->outcost){
                leavingchild=node2;
                GetArc(node2->pred,node2,&arcrow2,&arccol2,&arcdir2,nrow,ncol,
                       nodes,nodesupp);
                outcostto+=GetCost(incrcosts,arcrow2,arccol2,-arcdir2);
              }else{
                outcostto=VERYFAR;
              }
              node1->group=groupcounter+1;
              node2->group=groupcounter;
              node1=node1->pred;
              node2=node2->pred;
            }
          }
          cycleapex=node1;

          /* set leaving parent */ 
          if(leavingchild==NULL){
            fromside=TRUE;
            leavingparent=from;
          }else{
            leavingparent=leavingchild->pred;
          }

          /* swap from and to if leaving arc is on the from side */
          if(fromside){
            groupcounter++;
            fromgroup=groupcounter-1;
            tempnode1=from;
            from=to;
            to=tempnode1;
          }else{
            fromgroup=groupcounter+1;
          }

          /* if augmenting pivot */
          if(cyclecost<0){

            /* find first child of apex on either cycle path */
            firstfromnode=NULL;
            firsttonode=NULL;
            arcnum=GetArcNumLims(cycleapex->row,&upperarcnum,
                                 ngroundarcs,boundary);
            while(arcnum<upperarcnum){
              tempnode1=NeighborNode(cycleapex,++arcnum,&upperarcnum,nodes,
                                     ground,&arcrow,&arccol,&arcdir,nrow,ncol,
                                     boundary,nodesupp);
              if(tempnode1->group==groupcounter
                 && apexes(arcrow,arccol)==NULL){
                firsttonode=tempnode1;
                if(firstfromnode!=NULL){
                  break;
                }
              }else if(tempnode1->group==fromgroup
                       && apexes(arcrow,arccol)==NULL){
                firstfromnode=tempnode1;
                if(firsttonode!=NULL){
                  break;
                }
              }
            }

            /* update potentials, mark stationary parts of tree */
            cycleapex->group=groupcounter+2;
            if(firsttonode!=NULL){
              NonDegenUpdateChildren(cycleapex,leavingparent,firsttonode,0,
                                     ngroundarcs,nflow,nodes,nodesupp,ground,
                                     boundary,apexes,incrcosts,nrow,ncol,
                                     params); 
            }
            if(firstfromnode!=NULL){
              NonDegenUpdateChildren(cycleapex,from,firstfromnode,1,
                                     ngroundarcs,nflow,nodes,nodesupp,ground,
                                     boundary,apexes,incrcosts,nrow,ncol,
                                     params);
            }
            groupcounter=from->group;
            apexlistbase=cycleapex->group;

            /* children of cycleapex are not marked, so we set fromgroup */
            /*   equal to cycleapex group for use with apex updates below */
            /* all other children of cycle will be in apexlist if we had an */
            /*   augmenting pivot, so fromgroup only important for cycleapex */
            fromgroup=cycleapex->group;

          }else{

            /* set this stuff for use with apex updates below */
            cycleapex->group=fromgroup;
            groupcounter+=2;
            apexlistbase=groupcounter+1;
          }

          /* remount subtree at new mount point */
          if(leavingchild==NULL){

            skipthread=to;

          }else{

            root=from;
            oldmntpt=to;

            /* for each node on the path from to node to leaving child */
            while(oldmntpt!=leavingparent){

              /* remount the subtree at the new mount point */
              mntpt=root;
              root=oldmntpt;
              oldmntpt=root->pred;
              root->pred=mntpt;
              GetArc(mntpt,root,&arcrow,&arccol,&arcdir,nrow,ncol,
                     nodes,nodesupp);

              /* calculate differences for updating potentials and levels */
              dlevel=mntpt->level-root->level+1;
              doutcost=mntpt->outcost - root->outcost 
                + GetCost(incrcosts,arcrow,arccol,arcdir);
              dincost=mntpt->incost - root->incost 
                + GetCost(incrcosts,arcrow,arccol,-arcdir);

              /* update all children */
              /* group of each remounted tree used to reset apexes below */
              node1=root;
              startlevel=root->level;
              groupcounter++;
              while(TRUE){

                /* update the level, potentials, and group of the node */
                node1->level+=dlevel;
                node1->outcost+=doutcost;
                node1->incost+=dincost;
                node1->group=groupcounter;

                /* break when node1 is no longer descendent of the root */
                if(node1->next->level <= startlevel){
                  break;
                }
                node1=node1->next;
              }

              /* update threads */
              root->prev->next=node1->next;
              node1->next->prev=root->prev;
              node1->next=mntpt->next;  
              mntpt->next->prev=node1;
              mntpt->next=root;       
              root->prev=mntpt;

            }
            skipthread=node1->next;

            /* reset apex pointers for entering and leaving arcs */
            GetArc(from,to,&arcrow,&arccol,&arcdir,nrow,ncol,nodes,nodesupp);
            apexes(arcrow,arccol)=NULL;
            GetArc(leavingparent,leavingchild,&arcrow,&arccol,
                   &arcdir,nrow,ncol,nodes,nodesupp);
            apexes(arcrow,arccol)=cycleapex;

            /* make sure we have enough memory for the apex list */
            if(groupcounter-apexlistbase+1>apexlistlen){
              apexlistlen=1.5*(groupcounter-apexlistbase+1);
              apexlist.conservativeResize(apexlistlen);
            }

            /* set the apex list */
            /* the apex list is a look up table of apex node pointers indexed */
            /*   by the group number relative to a base group value */
            node2=leavingchild;
            for(group1=groupcounter;group1>=apexlistbase;group1--){
              apexlist[group1-apexlistbase]=node2;
              node2=node2->pred;
            }

            /* reset apex pointers on remounted tree */
            /* only nodes which are in different groups need new apexes */
            node1=to;
            startlevel=to->level;
            while(TRUE){

              /* loop over outgoing arcs */
              arcnum=GetArcNumLims(node1->row,&upperarcnum,
                                   ngroundarcs,boundary);
              while(arcnum<upperarcnum){
                node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                                   &arcrow,&arccol,&arcdir,nrow,ncol,
                                   boundary,nodesupp);

                /* if node2 on tree */
                if(node2->group>0){

                  /* if node2 is either not part of remounted tree or */
                  /*   it is higher on remounted tree than node1, */
                  /*   and arc isn't already on tree */
                  if(node2->group < node1->group
                     && apexes(arcrow,arccol)!=NULL){

                    /* if new apex in apexlist */
                    /* node2 on remounted tree, if nonaugmenting pivot */
                    if(node2->group >= apexlistbase){

                      apexes(arcrow,arccol)=apexlist[node2->group
                                                     -apexlistbase];

                    }else{

                      /* if old apex below level of cycleapex, */
                      /*   node2 is on "to" node's side of tree */
                      /* implicitly, if old apex above cycleapex, */
                      /*   we do nothing since apex won't change */
                      if(apexes(arcrow,arccol)->level > cycleapex->level){

                        /* since new apex not in apexlist (tested above), */
                        /* node2 above leaving arc so new apex is cycleapex */
                        apexes(arcrow,arccol)=cycleapex;

                      }else{

                        /* node2 not on "to" side of tree */
                        /* if old apex is cycleapex, node2 is on "from" side */
                        if(apexes(arcrow,arccol)==cycleapex){

                          /* new apex will be on cycle, so trace node2->pred */
                          /*   until we hit a node with group==fromgroup */
                          tempnode2=node2;
                          while(tempnode2->group != fromgroup){
                            tempnode2=tempnode2->pred;
                          }
                          apexes(arcrow,arccol)=tempnode2;

                        }
                      }
                    }

                    /* check outgoing arcs for negative reduced costs */
                    CheckArcReducedCost(node1,node2,apexes(arcrow,arccol),
                                        arcrow,arccol,arcdir,localcandidatebagptr,
                                        &candidatebagnext,&candidatebagsize,
                                        incrcosts,iscandidate,params);

                  } /* end if node2 below node1 and arc not on tree */

                }else if(node2->group!=PRUNED && node2->group!=MASKED){

                  /* node2 is not on tree, so put it in correct bucket */
                  AddNewNode(node1,node2,arcdir,bkts,nflow,incrcosts,
                             arcrow,arccol,params);

                } /* end if node2 on tree */
              } /* end loop over node1 outgoing arcs */

              /* move to next node in thread, break if we left the subtree */
              node1=node1->next;
              if(node1->level <= startlevel){
                break;
              }
            }
          } /* end if leavingchild!=NULL */

          /* if we had an augmenting cycle */
          /* we need to check outarcs from descendents of any cycle node */
          /* (except apex, since apex potentials don't change) */
          if(cyclecost<0){

            /* check descendents of cycle children of apex */
            while(TRUE){

              /* firstfromnode, firsttonode may have changed */
              if(firstfromnode!=NULL && firstfromnode->pred==cycleapex){
                node1=firstfromnode;
                firstfromnode=NULL;
              }else if(firsttonode!=NULL && firsttonode->pred==cycleapex){
                node1=firsttonode;
                firsttonode=NULL;
              }else{
                break;
              }
              startlevel=node1->level;

              /* loop over all descendents */
              while(TRUE){

                /* loop over outgoing arcs */
                arcnum=GetArcNumLims(node1->row,&upperarcnum,
                                     ngroundarcs,boundary);
                while(arcnum<upperarcnum){
                  node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                                     &arcrow,&arccol,&arcdir,nrow,ncol,
                                     boundary,nodesupp);

                  /* check for outcost updates or negative reduced costs */
                  if(node2->group>0){
                    if(apexes(arcrow,arccol)!=NULL
                       && (node2->group!=node1->group
                           || node1->group==apexlistbase)){
                      CheckArcReducedCost(node1,node2,apexes(arcrow,arccol),
                                          arcrow,arccol,arcdir,localcandidatebagptr,
                                          &candidatebagnext,&candidatebagsize,
                                          incrcosts,iscandidate,params);
                    }
                  }else if(node2->group!=PRUNED && node2->group!=MASKED){
                    AddNewNode(node1,node2,arcdir,bkts,nflow,incrcosts,
                               arcrow,arccol,params);
                  }                     
                }

                /* move to next node in thread, break if left the subtree */
                /*   but skip the remounted tree, since we checked it above */
                node1=node1->next;
                if(node1==to){
                  node1=skipthread;
                }
                if(node1->level <= startlevel){
                  break;
                }
              }
            }
          }
          ipivots++;
        } /* end if cyclecost<0 || outcostto<to->outcost */
      } /* end of for loop over candidates in list */

      /* this is needed only if we don't process all candidates above */
      /* copy remaining candidates into candidatebag */
      /*
      while(candidatebagnext+(candidatelistlen-ncandidates)>candidatebagsize){
        candidatebagsize+=CANDIDATEBAGSTEP;
        localcandidatebagptr->conservativeResize(candidatebagsize);
      }
      for(i=ncandidates;i<candidatelistlen;i++){
        (*localcandidatebagptr)[candidatebagnext++]=(*localcandidatelistptr)[i];
      }
      */

      /* display status */
      status << pyre::journal::at(__HERE__)
             << "Treesize: " << std::left << std::setw(10) << treesize
             << " Pivots: " << std::setw(11) << ipivots
             << " Improvements: " << std::setw(11) << inondegen
             << pyre::journal::endl;

    } /* end of while loop on candidatebagnext */    

    /* prune tree by removing unneeded leaf nodes */
    if(!((++nmajor) % nmajorprune)){
      npruned+=PruneTree(source,nodes,ground,boundary,nodesupp,incrcosts,flows,
                         ngroundarcs,prunecostthresh,nrow,ncol);
    }

  } /* end while treesize<number of total nodes */

  /* sanity check tree structure */
  node1=source->next;
  while(node1!=source){
    if(node1->pred->level!=node1->level-1){
      firewall << pyre::journal::at(__HERE__)
               << "Error detected: row " << node1->row << ", col " << node1->col
               << ", level " << node1->level << " has pred row " << node1->pred->row
               << ", col " << node1->pred->col << ", level " << node1->pred->level
               << pyre::journal::endl;
    }
    node1=node1->next;
  }
  
  /* discharge boundary */
  /* flow to edge of region goes along normal grid arcs, but flow along edge */
  /*   of region that should go along zero-cost arcs along edge is not */
  /*   captured in solver code above since nodes of edge are collapsed into */
  /*   single boundary node.  This accumulates surplus/demand at edge nodes. */
  /*   Here, find surplus/demand by balancing flow in/out of edge nodes and */
  /*   discharge sending flow along zero-cost edge arcs */
  DischargeBoundary(nodes,ground,boundary,nodesupp,flows,iscandidate,
                    mag,wrappedphase,ngroundarcs,nrow,ncol);

  /* sanity check that buckets are actually all empty after optimizer is done */
  for(i=0;i<bkts->size;i++){
    if(bkts->bucketbase[i]!=NULL){
      firewall << pyre::journal::at(__HERE__)
               << "ERROR: bucket " << i << " not empty after TreeSolve (row="
               << bkts->bucketbase[i]->row << ", col="
               << bkts->bucketbase[i]->col << ")"
               << pyre::journal::endl;
      break;
    }
  }

  /* reset group numbers of nodes along boundary */
  /* nodes in neighboring regions may have been set to be MASKED in */
  /*   in InitBoundary() to avoid reaching them across single line of */
  /*   masked pixels, so return mask status of nodes along boundary to normal */
  CleanUpBoundaryNodes(source,boundary,mag,nodes,ground,nrow,ncol,ngroundarcs,
                       nodesupp);

  /* clean up: set pointers for outputs */
  status << pyre::journal::at(__HERE__)
         << "Treesize: " << std::left << std::setw(10) << treesize
         << " Pivots: " << std::setw(11) << ipivots
         << " Improvements: " << std::setw(11) << inondegen
         << pyre::journal::endl;
  *candidatelistptr=*localcandidatelistptr;
  *candidatebagptr=*localcandidatebagptr;
  *candidatelistsizeptr=candidatelistsize;
  *candidatebagsizeptr=candidatebagsize;

  /* return the number of nondegenerate pivots (number of improvements) */
  return(inondegen);

}


/* function: AddNewNode()
 * ----------------------
 * Adds a node to a bucket if it is not already in a bucket.  Updates 
 * outcosts of to node if the new distance is less or if to's pred is
 * from (then we have to do the update).
 */
static
void AddNewNode(nodeT *from, nodeT *to, long arcdir, bucketT *bkts,
                long /*nflow*/, Array2D<incrcostT>& incrcosts, long arcrow, long arccol,
                paramT * /*params*/){

  long newoutcost;
  
  newoutcost=from->outcost
    +GetCost(incrcosts,arcrow,arccol,arcdir);
  if(newoutcost<to->outcost || to->pred==from){
    if(to->group==INBUCKET){      /* if to is already in a bucket */
      if(to->outcost<bkts->maxind){
        if(to->outcost>bkts->minind){
          BucketRemove(to,to->outcost,bkts);
        }else{
          BucketRemove(to,bkts->minind,bkts);
        }
      }else{
        BucketRemove(to,bkts->maxind,bkts);
      }
    }      
    to->outcost=newoutcost;
    to->pred=from;
    if(newoutcost<bkts->maxind){
      if(newoutcost>bkts->minind){
        BucketInsert(to,newoutcost,bkts);
        if(newoutcost<bkts->curr){
          bkts->curr=newoutcost;
        }
      }else{
        BucketInsert(to,bkts->minind,bkts);
        bkts->curr=bkts->minind;
      }
    }else{
      BucketInsert(to,bkts->maxind,bkts);
    }
    to->group=INBUCKET;
  }
  return;
}


/* function: CheckArcReducedCost()
 * -------------------------------
 * Given a from and to node, checks for negative reduced cost, and adds
 * the arc to the entering arc candidate bag if one is found.
 */
static
void CheckArcReducedCost(nodeT *from, nodeT *to, nodeT *apex,
                         long arcrow, long arccol, long arcdir,
                         Array1D<candidateT>* candidatebagptr,
                         long *candidatebagnextptr,
                         long *candidatebagsizeptr, Array2D<incrcostT>& incrcosts,
                         Array2D<signed char>& iscandidate, paramT * /*params*/){

  long apexcost, fwdarcdist, revarcdist, violation;
  nodeT *temp;


  /* do nothing if already candidate */
  /* illegal corner arcs have iscandidate=TRUE set ahead of time */
  if(iscandidate(arcrow,arccol)){
    return;
  }

  /* set the apex cost */
  apexcost=apex->outcost+apex->incost;

  /* check forward arc */
  fwdarcdist=GetCost(incrcosts,arcrow,arccol,arcdir);
  violation=fwdarcdist+from->outcost+to->incost-apexcost;
  if(violation<0){
    arcdir*=2;  /* magnitude 2 for sorting */
  }else{
    revarcdist=GetCost(incrcosts,arcrow,arccol,-arcdir);
    violation=revarcdist+to->outcost+from->incost-apexcost;
    if(violation<0){
      arcdir*=-2;  /* magnitude 2 for sorting */
      temp=from;
      from=to;
      to=temp;
    }else{
      violation=fwdarcdist+from->outcost-to->outcost;
      if(violation>=0){
        violation=revarcdist+to->outcost-from->outcost;
        if(violation<0){
          arcdir=-arcdir;
          temp=from;
          from=to;
          to=temp;
        }
      }
    }
  }

  /* see if we have a violation, and if so, add arc to candidate bag */
  if(violation<0){
    if((*candidatebagnextptr)>=(*candidatebagsizeptr)){
      (*candidatebagsizeptr)+=CANDIDATEBAGSTEP;
      candidatebagptr->conservativeResize(*candidatebagsizeptr);
    }
    (*candidatebagptr)[*candidatebagnextptr].violation=violation;
    (*candidatebagptr)[*candidatebagnextptr].from=from;
    (*candidatebagptr)[*candidatebagnextptr].to=to;
    (*candidatebagptr)[*candidatebagnextptr].arcrow=arcrow;
    (*candidatebagptr)[*candidatebagnextptr].arccol=arccol;
    (*candidatebagptr)[*candidatebagnextptr].arcdir=arcdir;
    (*candidatebagnextptr)++;
    iscandidate(arcrow,arccol)=TRUE;
  }

  /* done */
  return;
}


/* function: InitBoundary()
 * ------------------------
 * Initialize boundary structure for region to be unwrapped assuming
 * source is on boundary.
 *
 * This function makes several passes over the boundary nodes.  The
 * first pass finds nodes linked by zero-weight arcs that are candidates
 * for being on the boundary.  The second pass decides which of the
 * candidate nodes should actually point to the boundary node while
 * ensuring that no node has multiple valid arcs to the boundary node.
 * The third pass builds the neighbor list given the boundary pointer
 * nodes from the previous pass.  The fourth pass sets the group
 * members of boundary pointer nodes, which cannot be done earlier
 * since it would mess up how NeighborNodeGrid() works.
 *
 * Return pointer to source since source may become pointer to boundary.
 */
static
nodeT *InitBoundary(nodeT *source, Array2D<nodeT>& nodes,
                    boundaryT *boundary, Array2D<nodesuppT>& nodesupp, Array2D<float>& mag,
                    nodeT *ground, long ngroundarcs, long nrow, long ncol,
                    paramT *params, long *nconnectedptr){

  int iseligible, isinteriornode;
  long k, nlist, ninteriorneighbor;
  long nlistmem, nboundarymem, nneighbormem;
  long arcnum, upperarcnum;
  long neighborarcnum, neighborupperarcnum;
  long arcrow, arccol, arcdir;
  long nconnected;
  nodeT *from, *to, *end;

  /* initialize to null first */
  boundary->node->row=BOUNDARYROW;
  boundary->node->col=BOUNDARYCOL;
  boundary->node->next=NULL;
  boundary->node->prev=NULL;
  boundary->node->pred=NULL;
  boundary->node->level=0;
  boundary->node->group=0;
  boundary->node->incost=VERYFAR;
  boundary->node->outcost=VERYFAR;
  boundary->neighborlist=Array1D<neighborT>{};
  boundary->boundarylist=Array1D<nodeT*>{};
  boundary->nneighbor=0;
  boundary->nboundary=0;

  /* if this is non-grid network, do nothing */
  if(nodesupp.size()){
    return(source);
  }

  /* make sure magnitude exists */
  if(!mag.size()) {
    return(source);
  }

  /* scan region and mask any nodes that are not already masked but are not */
  /*   reachable through non-region arcs */
  /* such nodes can exist if there is single line of masked pixels separating */
  /*   regions */
  /* boundary is not yet set up, so scanning will search node neighbors as */
  /*   normal grid neighbors */
  nconnected=ScanRegion(source,nodes,mag,ground,ngroundarcs,nrow,ncol,MASKED);

  /* if source is ground, do nothing, since do not want boundary with ground */
  if(source==ground){
    return(source);
  }

  /* make sure source is on edge */
  /* we already know source is not ground from check above */
  if(!IsRegionEdgeNode(mag,source->row,source->col,nrow,ncol)){
    auto warnings=pyre::journal::warning_t("isce3.unwrap.snaphu");
    warnings << pyre::journal::at(__HERE__)
             << "WARNING: Non edge node as source in InitBoundary()"
             << pyre::journal::endl;
  }

  /* get memory for node list */
  nlistmem=NLISTMEMINCR;
  auto nodelist = Array1D<nodeT*>(nlistmem);
  nodelist[0]=source;
  nlist=1;

  /* first pass: build list of nodes on boundary */
  /* this should handle double-corner cases where all four arcs out of grid */
  /*   node will be region edge arcs (eg, mag[i][j] and mag[i+1][j+1] are */
  /*   both zero and mag[i+1][j] and mag[i][j+1] are both nonzero */
  source->next=NULL;
  source->group=BOUNDARYCANDIDATE;
  from=source;
  end=source;
  while(TRUE){

    /* see if neighbors can be reached through region-edge arcs */
    arcnum=GetArcNumLims(from->row,&upperarcnum,ngroundarcs,NULL);
    while(arcnum<upperarcnum){

      /* get neighboring node */
      to=NeighborNode(from,++arcnum,&upperarcnum,nodes,ground,
                      &arcrow,&arccol,&arcdir,nrow,ncol,NULL,nodesupp);

      /* see if node is reached through edge arc and node is not yet in list */
      if(IsRegionEdgeArc(mag,arcrow,arccol,nrow,ncol)
         && to->group!=BOUNDARYCANDIDATE){

        /* keep node in list */
        if(nlist==nlistmem){
          nlistmem+=NLISTMEMINCR;
          nodelist.conservativeResize(nlistmem);
        }
        nodelist[nlist++]=to;
        to->group=BOUNDARYCANDIDATE;

        /* add node to list of nodes to be searched */
        end->next=to;
        to->next=NULL;
        end=to;
        
      }
    }

    /* move to next node to search */
    if(from->next==NULL){
      break;
    }
    from=from->next;

  }

  /* get memory for boundary list */
  nboundarymem=NLISTMEMINCR;
  auto boundarylist = Array1D<nodeT*>(nboundarymem);

  /* second pass to avoid multiple arcs to same node */
  /* go through nodes in list and check criteria for including on boundary */
  for(k=0;k<nlist;k++){

    /* only consider if node is not edge ground */
    /* we do not want ground node to be a boundary node since ground already */
    /*   acts similar to boundary node */
    if(nodelist[k]->row!=GROUNDROW){
    
      /* loop over neighbors */
      iseligible=TRUE;
      ninteriorneighbor=0;
      arcnum=GetArcNumLims(nodelist[k]->row,&upperarcnum,ngroundarcs,NULL);
      while(arcnum<upperarcnum){

        /* get neighbor */
        from=NeighborNode(nodelist[k],++arcnum,&upperarcnum,nodes,ground,
                          &arcrow,&arccol,&arcdir,nrow,ncol,NULL,nodesupp);

        /* see if node can be reached through interior arc */
        /*   and node is not masked or on boundary */
        /* node may be on edge of island of masked pixels, so it does not */
        /*   need to be interior node in sense of not touching masked pixels  */
        isinteriornode=(IsRegionInteriorArc(mag,arcrow,arccol,nrow,ncol)
                        && from->group!=MASKED
                        && from->level!=BOUNDARYLEVEL);
        if(isinteriornode){
          ninteriorneighbor++;
        }
        
        /* scan neighbor's neighbors if neighbor is interior node */
        /*    or if it is edge node that is not yet on boundary  */
        /* edge node may have been reached through a non-region arc, but */
        /*    that is okay since that non-region arc will have zero cost */
        /*    given that non-region nodes will be masked; need to let this */
        /*    happen because solver will use such an arc too */
        if(isinteriornode || (from->group==BOUNDARYCANDIDATE
                              && from->level!=BOUNDARYLEVEL)){

          /* loop over neighbors of neighbor */
          neighborarcnum=GetArcNumLims(from->row,&neighborupperarcnum,
                                       ngroundarcs,NULL);
          while(neighborarcnum<neighborupperarcnum){

            /* get neighbor of neighbor */
            to=NeighborNode(from,++neighborarcnum,&neighborupperarcnum,
                            nodes,ground,&arcrow,&arccol,&arcdir,
                            nrow,ncol,NULL,nodesupp);

            /* see if neighbor of neighbor is on boundary already */
            if(to->level==BOUNDARYLEVEL){
              iseligible=FALSE;
              break;
            }
          }
        }

        /* break if already ineligible */
        if(!iseligible){
          break;
        }
      }

      /* see if we should include this node in boundary */
      if(iseligible && ninteriorneighbor>0){
        nodelist[k]->level=BOUNDARYLEVEL;
        if(++boundary->nboundary > nboundarymem){
          nboundarymem+=NLISTMEMINCR;
          boundarylist.conservativeResize(nboundarymem);
        }
        boundarylist[boundary->nboundary-1]=nodelist[k];
      }
    }
  }

  /* set groups of all edge nodes back to zero */
  for(k=0;k<nlist;k++){
    nodelist[k]->group=0;
    nodelist[k]->next=NULL;
  }

  /* punt if there were too few boundary nodes */
  /* region should be unwrapped with nodes behaving like normal grid nodes */
  /*   but with neighbors that are not part of region masked */
  if(boundary->nboundary<MINBOUNDARYSIZE){
    for(k=0;k<boundary->nboundary;k++){
      boundarylist[k]->level=0;
      boundarylist[k]->group=0;
    }
    boundary->node->row=BOUNDARYROW;
    boundary->node->col=BOUNDARYCOL;
    boundary->node->next=NULL;
    boundary->node->prev=NULL;
    boundary->node->pred=NULL;
    boundary->node->level=0;
    boundary->node->group=0;
    boundary->node->incost=VERYFAR;
    boundary->node->outcost=VERYFAR;
    boundary->neighborlist=Array1D<neighborT>{};
    boundary->boundarylist=Array1D<nodeT*>{};
    boundary->nneighbor=0;
    boundary->nboundary=0;
    return(source);
  }

  /* third pass */
  /* set up for creating neighbor list */
  nneighbormem=NLISTMEMINCR;
  auto neighborlist = Array1D<neighborT>(nneighbormem);

  /* now go through boundary pointer nodes and build neighbor list */
  for(k=0;k<boundary->nboundary;k++){

    /* loop over neighbors to keep in neighbor list */
    /* checks above should ensure that unmasked neighbors of this boundary */
    /*    pointer node are not reachable by any other boundary pointer node */
    arcnum=GetArcNumLims(boundarylist[k]->row,&upperarcnum,ngroundarcs,NULL);
    while(arcnum<upperarcnum){

      /* get neighbor */
      to=NeighborNode(boundarylist[k],++arcnum,&upperarcnum,nodes,ground,
                      &arcrow,&arccol,&arcdir,nrow,ncol,NULL,nodesupp);
      
      /* see if node is not masked and not a boundary node */
      /* node may or may not be reachable through region arc, but if */
      /*   non region arc is used, it is okay since it will have zero cost; */
      /*   solver would use these arcs if there were no boundary, so let */
      /*   those nodes stay in neighbor list of boundary */
      if(to->group!=MASKED && to->level!=BOUNDARYLEVEL){

        /* add neighbor as neighbor of boundary */
        boundary->nneighbor++;
        if(boundary->nneighbor>nneighbormem){
          nneighbormem+=NLISTMEMINCR;
          neighborlist.conservativeResize(nneighbormem);
        }
        neighborlist[boundary->nneighbor-1].neighbor=to;
        neighborlist[boundary->nneighbor-1].arcrow=arcrow;
        neighborlist[boundary->nneighbor-1].arccol=arccol;
        neighborlist[boundary->nneighbor-1].arcdir=arcdir;
      }
    }
  }

  /* fourth pass */
  /* now that boundary is properly set up, make one last pass to set groups */
  for(k=0;k<boundary->nboundary;k++){
    boundarylist[k]->group=BOUNDARYPTR;
    boundarylist[k]->level=0;
  }

  /* keep only needed memory and store pointers in boundary structure */
  neighborlist.conservativeResize(boundary->nneighbor);
  boundary->neighborlist = neighborlist;
  boundarylist.conservativeResize(boundary->nboundary);
  boundary->boundarylist = boundarylist;

  /* check boundary for consistency */
  /* count number of connected nodes, which should have changed by number */
  /*   outer edge nodes that got collapsed into single boundary node (minus 1) */
  nconnected=CheckBoundary(nodes,mag,ground,ngroundarcs,boundary,nrow,ncol,
                           params,boundary->node);
  if(nconnectedptr!=NULL){
    if(nconnected+boundary->nboundary-1!=(*nconnectedptr)){
      auto info=pyre::journal::info_t("isce3.unwrap.snaphu");
      info << pyre::journal::at(__HERE__)
           << "WARNING: Changed number of connected nodes in InitBoundary()"
           << pyre::journal::endl;
    }
    (*nconnectedptr)=nconnected;
  }

  /* done */
  return(boundary->node);
  
}


/* function: CheckBoundary()
 * -------------------------
 * Similar to SelectConnNodeSource, but reset group to zero and check boundary.
 */
static
long CheckBoundary(Array2D<nodeT>& nodes, Array2D<float>& /*mag*/, nodeT *ground, long ngroundarcs,
                   boundaryT *boundary, long nrow, long ncol,
                   paramT * /*params*/, nodeT *start){

  long arcrow, arccol, arcdir, arcnum, upperarcnum;
  long nontree, nboundaryarc, nboundarynode, nconnected;
  nodeT *node1, *node2, *end;

  Array2D<nodesuppT> nodesupp;

  /* if start node is not eligible, give error */
  if(start->group==MASKED){
    fflush(NULL);
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "Ineligible starting node in CheckBoundary()");
  }
  
  /* initialize local variables */
  nconnected=0;
  end=start;
  node1=start;
  node1->group=INBUCKET;

  /* loop to search for connected, unmasked nodes */
  while(node1!=NULL){

    /* loop over neighbors of current node */
    arcnum=GetArcNumLims(node1->row,&upperarcnum,ngroundarcs,boundary);
    while(arcnum<upperarcnum){
      node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                         &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);

      /* if neighbor is not masked or visited, add to list of nodes to search */
      if(node2->group!=MASKED && node2->group!=ONTREE
         && node2->group!=INBUCKET){

        node2->group=INBUCKET;
        end->next=node2;
        node2->next=NULL;
        end=node2;
      }
    }

    /* mark this node visited */
    node1->group=ONTREE;
    nconnected++;

    /* move to next node in list */
    node1=node1->next;

  }

  /* loop over connected nodes to check connectivity and reset group numbers */
  node1=start;
  nontree=0;
  nboundaryarc=0;
  nboundarynode=0;
  while(node1!=NULL){

    /* loop over neighbors of current node */
    arcnum=GetArcNumLims(node1->row,&upperarcnum,ngroundarcs,boundary);
    while(arcnum<upperarcnum){
      node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                         &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);

      /* see if we have an arc to boundary */
      /* this may or may not use region arc, but if non-region arc is used */
      /*   it should have zero cost */
      if(node2->row==BOUNDARYROW){
        nboundaryarc++;
      }
    }

    /* check number of boundary nodes */
    if(node1->row==BOUNDARYROW){
      nboundarynode++;
    }

    /* count total number of nodes */
    nontree++;
    
    /* reset group number */
    if(node1->group==ONTREE){
      node1->group=0;
    }

    /* move to next node in list */
    node1=node1->next;
    
  }

  /* check for consistency */
  if(nontree!=nconnected){
    fflush(NULL);
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "Inconsistent num connected nodes in CheckBoundary()");
  }
  if(nboundaryarc!=boundary->nneighbor){
    fflush(NULL);
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "Inconsistent num neighbor nodes in CheckBoundary()");
  }
  if(nboundarynode!=1){
    fflush(NULL);
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "Number of boundary nodes is not 1 in CheckBoundary()");
  }

  /* return number of connected nodes */
  return(nconnected);

}


/* function: IsRegionEdgeArc()
 * ---------------------------
 * Return TRUE if arc is on edge of region, FALSE otherwise.
 */
static
int IsRegionEdgeArc(Array2D<float>& mag, long arcrow, long arccol,
                    long nrow, long /*ncol*/){

  long row1, col1, row2, col2, nzeromag;

  /* if no magnitude, then everything is in single region */
  if(!mag.size()) {
    return(FALSE);
  }

  /* determine indices of pixels on either side of this arc */
  if(arcrow<nrow-1){
    row1=arcrow;
    row2=row1+1;
    col1=arccol;
    col2=col1;
  }else{
    row1=arcrow-(nrow-1);
    row2=row1;
    col1=arccol;
    col2=col1+1;
  }

  /* see whether exactly one pixel has zero magnitude */
  nzeromag=0;
  if(mag(row1,col1)==0){
    nzeromag++;
  }
  if(mag(row2,col2)==0){
    nzeromag++;
  }
  if(nzeromag==1){
    return(TRUE);
  }
  return(FALSE);

}


/* function: IsRegionInteriorArc()
 * -------------------------------
 * Return TRUE if arc goes between two nodes in same region such that
 * both pixel magnitudes on either side of arc are nonzero.
 */
static
int IsRegionInteriorArc(Array2D<float>& mag, long arcrow, long arccol,
                        long nrow, long ncol){

  long row1, col1, row2, col2;

  /* if no magnitude, everything is in single region */
  if(!mag.size()){
    return(TRUE);
  }

  /* determine indices of pixels on either side of this arc */
  if(arcrow<nrow-1){
    row1=arcrow;
    row2=row1+1;
    col1=arccol;
    col2=col1;
  }else{
    row1=arcrow-(nrow-1);
    row2=row1;
    col1=arccol;
    col2=col1+1;
  }

  /* see whether both pixels have nonzero magnitude */
  if(mag(row1,col1)>0 && mag(row2,col2)>0){
    return(TRUE);
  }
  return(FALSE);

}


/* function: IsRegionArc()
 * -----------------------
 * Return TRUE if arc goes between two nodes in same region such that
 * at least one pixel magnitude on either side of arc is nonzero.
 */
static
int IsRegionArc(Array2D<float>& mag, long arcrow, long arccol,
                long nrow, long ncol){

  long row1, col1, row2, col2;

  /* if no magnitude, everything is in single region */
  if(!mag.size()){
    return(TRUE);
  }

  /* determine indices of pixels on either side of this arc */
  if(arcrow<nrow-1){
    row1=arcrow;
    row2=row1+1;
    col1=arccol;
    col2=col1;
  }else{
    row1=arcrow-(nrow-1);
    row2=row1;
    col1=arccol;
    col2=col1+1;
  }

  /* see whether at least one pixel has nonzero magnitude */
  if(mag(row1,col1)>0 || mag(row2,col2)>0){
    return(TRUE);
  }
  return(FALSE);

}


/* function: IsRegionEdgeNode()
 * ----------------------------
 * Return TRUE if node touches at least one zero magnitude pixel and
 * at least one non-zero magnitude pixel, FALSE otherwise.
 */
static
int IsRegionEdgeNode(Array2D<float>& mag, long row, long col, long /*nrow*/, long /*ncol*/){

  int onezeromag, onenonzeromag;


  /* if there is no magnitude info, then no nodes are on edge */
  if(!mag.size()) {
    return(FALSE);
  }

  /* check for ground */
  if(row==GROUNDROW){
    return(FALSE);
  }

  /* check mag for one zero mag and one nonzero mag pixel around node */
  onezeromag=FALSE;
  onenonzeromag=FALSE;
  if(mag(row,col)==0 || mag(row+1,col)==0
     || mag(row,col+1)==0 || mag(row+1,col+1)==0){
    onezeromag=TRUE;
  }
  if(mag(row,col)!=0 || mag(row+1,col)!=0
     || mag(row,col+1)!=0 || mag(row+1,col+1)!=0){
    onenonzeromag=TRUE;
  }
  if(onezeromag && onenonzeromag){
    return(TRUE);
  }
  return(FALSE);
  
}


/* function: CleanUpBoundaryNodes()
 * --------------------------------
 * Unset group values indicating boundary nodes on network.  This is
 * necessary because InitBoundary() temporarily sets node groups to
 * MASKED if the node is in a different region than the current but
 * can be reached from the current region.  This can occur if two
 * regions are separated by a single row or column of masked pixels.
 */
static
int CleanUpBoundaryNodes(nodeT *source, boundaryT *boundary, Array2D<float>& mag,
                         Array2D<nodeT>& nodes, nodeT *ground,
                         long nrow, long ncol, long ngroundarcs,
                         Array2D<nodesuppT>& nodesupp){

  long nconnected;
  nodeT *start;

  /* do nothing if this is not a grid network */
  if(nodesupp.size()){
    return(0);
  }

  /* starting node should not be boundary */
  if(source->row==BOUNDARYROW){
    start=boundary->neighborlist[0].neighbor;
  }else{
    start=source;
  }

  /* scan region and unmask any nodes that touch good pixels since they */
  /*   may have been masked by the ScanRegion() call in InitBoundaries() */
  nconnected=ScanRegion(start,nodes,mag,ground,ngroundarcs,nrow,ncol,0);

  /* done */
  return(nconnected);
}


/* function: DischargeBoundary()
 * -----------------------------
 * Find nodes and arcs along edge of region (defined by zero magnitude) and
 * compute surplus/demand by balancing flow in/out of nodes.  Then discharge 
 * surplus/demand by sending flow along zero-cost arcs along region edge.
 */
static
int DischargeBoundary(Array2D<nodeT>& nodes, nodeT *ground,
                      boundaryT *boundary, Array2D<nodesuppT>& nodesupp, Array2D<short>& flows,
                      Array2D<signed char>& iscandidate, Array2D<float>& mag,
                      Array2D<float>& wrappedphase, long ngroundarcs,
                      long nrow, long ncol){

  long nedgenode;
  long row, col, fromrow, fromcol, todir;
  long arcnum, upperarcnum, arcrow, arccol, arcdir, narccol;
  long surplus, residue, excess;
  nodeT *from, *to, *nextnode;


  /* do nothing if we have no boundary */
  if(nodesupp.size() || boundary==NULL
     || boundary->nboundary==0 || boundary->nneighbor==0){
    return(0);
  }
  
  /* find initial region edge node */
  nextnode=boundary->boundarylist[0];
  row=nextnode->row;
  col=nextnode->col;
  if(!IsRegionEdgeNode(mag,row,col,nrow,ncol)){
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "DischargeBoundary() start node " + std::to_string(row) +
            ", " + std::to_string(col) + " not on edge");
  }

  /* silence compiler warnings */
  row=0;
  col=0;
  todir=0;

  /* make sure iscandidate is zero */
  /* temporarily set illegal corner arcs to 0 to simplify logic (reset later) */
  for(row=0;row<2*nrow-1;row++){
    if(row<nrow-1){
      narccol=ncol;
    }else{
      narccol=ncol-1;
    }
    for(col=0;col<narccol;col++){
      iscandidate(row,col)=0;
    }
  }

  /* initialize counter of edge nodes to 1 */
  /* (tree root doesn't get push or counter increment) */
  nedgenode=1;
  
  /* use similar algorithm as DischargeTree() to descend region edge arcs */
  /* this will form tree (do not need to close cycle if we discharge */
  /*   all nodes--last arc that would close to form cycle will have no flow) */
  /* tree could branch (not just single chain) because of double corner cases */
  /*   where mag[i][j] and mag[i+1][j+1] are zero and mag[i+1][j] and */
  /*   mag[i][j+1] are nonzero */
  while(TRUE){

    /* get next node */
    /* mark with -1 cost to denote that they have been visited */
    from=nextnode;
    from->outcost=-1;
    nextnode=NULL;

    /* loop over outgoing arcs */
    /* pass NULL to NeighborNode() for boundary so as not to follow */
    /*   node pointers to boundary node */
    arcnum=GetArcNumLims(from->row,&upperarcnum,ngroundarcs,NULL);
    while(arcnum<upperarcnum){

      /* get neighbor node */
      to=NeighborNode(from,++arcnum,&upperarcnum,nodes,ground,
                      &arcrow,&arccol,&arcdir,nrow,ncol,NULL,nodesupp);

      /* see if arc is on region edge and not done */
      if(IsRegionEdgeArc(mag,arcrow,arccol,nrow,ncol)
         && (iscandidate(arcrow,arccol)==-1 ||
             (iscandidate(arcrow,arccol)==0 && to->outcost!=-1))){

        /* save arc */
        nextnode=to;
        row=arcrow;
        col=arccol;
        todir=arcdir;

        /* stop and follow arc if arc not yet followed */
        if(!iscandidate(arcrow,arccol)){
          break;
        }

      }
    }

    /* break if no unfollowed arcs (ie, we are done examining tree) */
    if(nextnode==NULL){
      break;
    }

    /* if we found leaf and we're moving back up the tree, do a push */
    /* otherwise, just mark the path by decrementing iscandidate */
    if((--iscandidate(row,col))==-2){

      /* integrate flow into current node */
      fromrow=from->row;
      fromcol=from->col;
      surplus=(flows(fromrow,fromcol)
               -flows(fromrow,fromcol+1)
               +flows(fromrow+nrow-1,fromcol)
               -flows(fromrow+1+nrow-1,fromcol));

      /* compute residue from wrapped phase */
      residue=NodeResidue(wrappedphase,fromrow,fromcol);

      /* compute excess as surplus plus residue */
      excess=surplus+residue;

      /* augment flow */
      flows(row,col)+=todir*excess;

      /* increment counter of edge nodes */
      nedgenode++;
      
    }

  }

  /* reset all iscandidate and outcost values */
  /* set illegal corner arc iscandidate values back to TRUE */
  /* outcost of region edge nodes should be zero if source was on edge */
  /*   and arc costs along boundary are all zero */
  for(row=0;row<nrow;row++){
    for(col=0;col<ncol;col++){
      if(row<nrow-1){
        if(iscandidate(row,col)){
          if(col>0){
            nodes(row,col-1).outcost=0;
          }
          if(col<ncol-1){
            nodes(row,col).outcost=0;
          }
        }
        iscandidate(row,col)=FALSE;
      }
      if(col<ncol-1){
        if(iscandidate(row+nrow-1,col)){
          if(row>0){
            nodes(row-1,col).outcost=0;
          }
          if(row<nrow-1){
            nodes(row,col).outcost=0;
          }
        }
        iscandidate(row+nrow-1,col)=FALSE;
      }
    }
  }
  iscandidate(nrow-1,0)=TRUE;
  iscandidate(2*nrow-2,0)=TRUE;
  iscandidate(nrow-1,ncol-2)=TRUE;
  iscandidate(2*nrow-2,ncol-2)=TRUE;

  /* done */
  return(nedgenode);
         
}


/* function: InitTree()
 * --------------------
 * Initialize tree for tree solver.
 */
static
int InitTree(nodeT *source, Array2D<nodeT>& nodes,
             boundaryT *boundary, Array2D<nodesuppT>& nodesupp,
             nodeT *ground, long ngroundarcs, bucketT *bkts, long nflow,
             Array2D<incrcostT>& incrcosts, long nrow, long ncol, paramT *params){

  long arcnum, upperarcnum, arcrow, arccol, arcdir;
  nodeT *to;


  /* put source on tree */
  source->group=1;
  source->outcost=0;
  source->incost=0;
  source->pred=NULL;
  source->prev=source;
  source->next=source;
  source->level=0;

  /* loop over outgoing arcs and add to buckets */
  arcnum=GetArcNumLims(source->row,&upperarcnum,ngroundarcs,boundary);
  while(arcnum<upperarcnum){
    
    /* get node reached by outgoing arc */
    to=NeighborNode(source,++arcnum,&upperarcnum,nodes,ground,
                    &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);

    /* add to node to bucket */
    if(to->group!=PRUNED && to->group!=MASKED){
      AddNewNode(source,to,arcdir,bkts,nflow,incrcosts,arcrow,arccol,params);
    }

  }

  /* done */
  return(0);
  
}


/* function: FindApex()
 * --------------------
 * Given pointers to two nodes on a spanning tree, the function finds
 * and returns a pointer to their deepest common ancestor, the apex of
 * a cycle formed by joining the two nodes with an arc.
 */
static
nodeT *FindApex(nodeT *from, nodeT *to){

  if(from->level > to->level){
    while(from->level != to->level){
      from=from->pred;
    }
  }else{
    while(from->level != to->level){
      to=to->pred;
    }
  }
  while(from != to){
    from=from->pred;
    to=to->pred;
  }
  return(from);
}


/* function: CandidateCompare()
 * ----------------------------
 * Compares the violations of candidate arcs for sorting.  First checks
 * if either candidate has an arcdir magnitude greater than 1, denoting 
 * an augmenting cycle.  Augmenting candidates are always placed before 
 * non-augmenting candidates.  Otherwise, returns positive if the first  
 * candidate has a greater (less negative) violation than the second, 0 
 * if they are the same, and negative otherwise.  
 */
static
int CandidateCompare(const void *c1, const void *c2){

  if(labs(((candidateT *)c1)->arcdir) > 1){
    if(labs(((candidateT *)c2)->arcdir) < 2){
      return(-1);
    }
  }else if(labs(((candidateT *)c2)->arcdir) > 1){
    return(1);
  }

  return(((candidateT *)c1)->violation - ((candidateT *)c2)->violation);

  /*
  if(((candidateT *)c1)->violation > ((candidateT *)c2)->violation){
    return(1);
  }else if(((candidateT *)c1)->violation < ((candidateT *)c2)->violation){
    return(-1);
  }else{
    return(0);
  }
  */
}


/* function: GetArcNumLims()
 * -------------------------
 * Get the initial and ending values for arcnum to find neighbors of
 * the passed node.
 */
static inline
long GetArcNumLims(long fromrow, long *upperarcnumptr,
                   long ngroundarcs, boundaryT *boundary){

  long arcnum;
  
  /* set arcnum limits based on node type */
  if(fromrow<0){
    arcnum=-1;
    if(fromrow==GROUNDROW){
      *upperarcnumptr=ngroundarcs-1;
    }else{
      *upperarcnumptr=boundary->nneighbor-1;
    }
  }else{
    arcnum=-5;
    *upperarcnumptr=-1;
  }
  return(arcnum);
  
}


/* function: NeighborNodeGrid()
 * ----------------------------
 * Return the neighboring node of the given node corresponding to the
 * given arc number for a grid network with a ground node.
 */
static
nodeT *NeighborNodeGrid(nodeT *node1, long arcnum, long * /*upperarcnumptr*/,
                        Array2D<nodeT>& nodes, nodeT *ground, long *arcrowptr,
                        long *arccolptr, long *arcdirptr, long nrow,
                        long ncol, boundaryT *boundary, Array2D<nodesuppT>& /*nodesupp*/){
  long row, col;
  nodeT *neighbor;

  /* get starting node row and col for convenience */
  row=node1->row;
  col=node1->col;

  /* see if starting node is boundary node */
  if(row==BOUNDARYROW){

    /* get neighbor info from boundary structure */
    neighbor=boundary->neighborlist[arcnum].neighbor;
    *arcrowptr=boundary->neighborlist[arcnum].arcrow;
    *arccolptr=boundary->neighborlist[arcnum].arccol;
    *arcdirptr=boundary->neighborlist[arcnum].arcdir;
    
  }else{
  
    /* starting node is normal node */
    switch(arcnum){
    case -4:
      *arcrowptr=row;
      *arccolptr=col+1;
      *arcdirptr=1;
      if(col==ncol-2){
        neighbor=ground;
      }else{
        neighbor=&nodes(row,col+1);
      }
      break;
    case -3:
      *arcrowptr=nrow+row;
      *arccolptr=col;
      *arcdirptr=1;
      if(row==nrow-2){
        neighbor=ground;
      }else{
        neighbor=&nodes(row+1,col);
      }
      break;
    case -2:
      *arcrowptr=row;
      *arccolptr=col;
      *arcdirptr=-1;
      if(col==0){
        neighbor=ground;
      }else{
        neighbor=&nodes(row,col-1);
      }
      break;

    case -1:
      *arcrowptr=nrow-1+row;
      *arccolptr=col;
      *arcdirptr=-1;
      if(row==0){
        neighbor=ground;
      }else{
        neighbor=&nodes(row-1,col);
      }
      break;
    default:
      if(arcnum<nrow-1){
        *arcrowptr=arcnum;
        *arccolptr=0;
        *arcdirptr=1;
        neighbor=&nodes(*arcrowptr,0);
      }else if(arcnum<2*(nrow-1)){
        *arcrowptr=arcnum-(nrow-1);
        *arccolptr=ncol-1;
        *arcdirptr=-1;
        neighbor=&nodes(*arcrowptr,ncol-2);
      }else if(arcnum<2*(nrow-1)+ncol-3){
        *arcrowptr=nrow-1;
        *arccolptr=arcnum-2*(nrow-1)+1;
        *arcdirptr=1;
        neighbor=&nodes(0,*arccolptr);
      }else{
        *arcrowptr=2*nrow-2;
        *arccolptr=arcnum-(2*(nrow-1)+ncol-3)+1;
        *arcdirptr=-1;
        neighbor=&nodes(nrow-2,*arccolptr);
      }
      break;
    }

    /* get boundary node if neighbor is pointer one */
    if(neighbor->group==BOUNDARYPTR && boundary!=NULL){
      neighbor=boundary->node;
    }

  }

  /* return neighbor */
  return(neighbor);

}


/* function: NeighborNodeNonGrid()
 * -------------------------------
 * Return the neighboring node of the given node corresponding to the
 * given arc number for a nongrid network (ie, arbitrary topology).
 */
static
nodeT *NeighborNodeNonGrid(nodeT *node1, long arcnum, long *upperarcnumptr,
                           Array2D<nodeT>& /*nodes*/, nodeT * /*ground*/, long *arcrowptr,
                           long *arccolptr, long *arcdirptr, long /*nrow*/,
                           long /*ncol*/, boundaryT * /*boundary*/,
                           Array2D<nodesuppT>& nodesupp){

  long tilenum, nodenum;
  scndryarcT *outarc;

  /* set up */
  tilenum=node1->row;
  nodenum=node1->col;
  *upperarcnumptr=nodesupp(tilenum,nodenum).noutarcs-5;

  /* set the arc row (tilenumber) and column (arcnumber) */
  outarc=nodesupp(tilenum,nodenum).outarcs[arcnum+4];
  *arcrowptr=outarc->arcrow;
  *arccolptr=outarc->arccol;
  if(node1==outarc->from){
    *arcdirptr=1;
  }else{
    *arcdirptr=-1;
  }

  /* return the neighbor node */
  return(nodesupp(tilenum,nodenum).neighbornodes[arcnum+4]);

}


/* function: GetArcGrid()
 * ----------------------
 * Given a from node and a to node, sets pointers for indices into
 * arc arrays, assuming primary (grid) network.
 */
static
void GetArcGrid(nodeT *from, nodeT *to, long *arcrow, long *arccol,
                long *arcdir, long nrow, long ncol,
                Array2D<nodeT>& nodes, Array2D<nodesuppT>& /*nodesupp*/){

  long fromrow, fromcol, torow, tocol;


  fromrow=from->row;
  fromcol=from->col;
  torow=to->row;
  tocol=to->col;
  
  if(fromcol==tocol-1){           /* normal arcs (neither endpoint ground) */
    *arcrow=fromrow;
    *arccol=fromcol+1;
    *arcdir=1;
  }else if(fromcol==tocol+1){
    *arcrow=fromrow;
    *arccol=fromcol;
    *arcdir=-1;
  }else if(fromrow==torow-1){
    *arcrow=fromrow+1+nrow-1;
    *arccol=fromcol;
    *arcdir=1;
  }else if(fromrow==torow+1){
    *arcrow=fromrow+nrow-1;
    *arccol=fromcol;
    *arcdir=-1;
  }else if(fromrow==BOUNDARYROW){ /* arc from boundary pointer */
    if(tocol<ncol-2 && nodes(torow,tocol+1).group==BOUNDARYPTR){
      *arcrow=torow;
      *arccol=tocol+1;
      *arcdir=-1;
    }else if(tocol>0 && nodes(torow,tocol-1).group==BOUNDARYPTR){
      *arcrow=torow;
      *arccol=tocol;
      *arcdir=1;
    }else if(torow<nrow-2 && nodes(torow+1,tocol).group==BOUNDARYPTR){
      *arcrow=torow+1+nrow-1;
      *arccol=tocol;
      *arcdir=-1;
    }else{
      *arcrow=torow+nrow-1;
      *arccol=tocol;
      *arcdir=1;
    }
  }else if(torow==BOUNDARYROW){   /* arc to boundary pointer */
    if(fromcol<ncol-2 && nodes(fromrow,fromcol+1).group==BOUNDARYPTR){
      *arcrow=fromrow;
      *arccol=fromcol+1;
      *arcdir=1;
    }else if(fromcol>0 && nodes(fromrow,fromcol-1).group==BOUNDARYPTR){
      *arcrow=fromrow;
      *arccol=fromcol;
      *arcdir=-1;
    }else if(fromrow<nrow-2 && nodes(fromrow+1,fromcol).group==BOUNDARYPTR){
      *arcrow=fromrow+1+nrow-1;
      *arccol=fromcol;
      *arcdir=1;
    }else{
      *arcrow=fromrow+nrow-1;
      *arccol=fromcol;
      *arcdir=-1;
    }
  }else if(fromcol==0){           /* arcs to ground */
    *arcrow=fromrow;
    *arccol=0;
    *arcdir=-1;
  }else if(fromcol==ncol-2){
    *arcrow=fromrow;
    *arccol=ncol-1;
    *arcdir=1;
  }else if(fromrow==0){
    *arcrow=nrow-1;
    *arccol=fromcol;
    *arcdir=-1;
  }else if(fromrow==nrow-2){
    *arcrow=2*(nrow-1);
    *arccol=fromcol;
    *arcdir=1;
  }else if(tocol==0){             /* arcs from ground */
    *arcrow=torow;
    *arccol=0;
    *arcdir=1;
  }else if(tocol==ncol-2){
    *arcrow=torow;
    *arccol=ncol-1;
    *arcdir=-1;
  }else if(torow==0){
    *arcrow=nrow-1;
    *arccol=tocol;
    *arcdir=1;
  }else{
    *arcrow=2*(nrow-1);
    *arccol=tocol;
    *arcdir=-1;
  }
  return;
}


/* function: GetArcNonGrid()
 * -------------------------
 * Given a from node and a to node, sets pointers for indices into
 * arc arrays, assuming secondary (arbitrary topology) network.
 */
static
void GetArcNonGrid(nodeT *from, nodeT *to, long *arcrow, long *arccol,
                   long *arcdir, long /*nrow*/, long /*ncol*/,
                   Array2D<nodeT>& /*nodes*/, Array2D<nodesuppT>& nodesupp){

  long tilenum, nodenum, arcnum;
  scndryarcT *outarc;

  /* get tile and node numbers for from node */
  tilenum=from->row;
  nodenum=from->col;

  /* loop over all outgoing arcs of from node */
  arcnum=0;
  while(TRUE){
    outarc=nodesupp(tilenum,nodenum).outarcs[arcnum++];
    if(outarc->from==to){
      *arcrow=outarc->arcrow;
      *arccol=outarc->arccol;
      *arcdir=-1;
      return;
    }else if(outarc->to==to){
      *arcrow=outarc->arcrow;
      *arccol=outarc->arccol;
      *arcdir=1;
      return;
    }
  }
  return;
}


/* Function: NonDegenUpdateChildren()
 * ----------------------------------
 * Updates potentials and groups of all childredn along an augmenting path, 
 * until a stop node is hit.
 */
static
void NonDegenUpdateChildren(nodeT *startnode, nodeT *lastnode,
                            nodeT *nextonpath, long dgroup,
                            long ngroundarcs, long /*nflow*/, Array2D<nodeT>& nodes,
                            Array2D<nodesuppT>& nodesupp, nodeT *ground,
                            boundaryT *boundary,
                            Array2D<nodeT*>& /*apexes*/, Array2D<incrcostT>& incrcosts,
                            long nrow, long ncol, paramT * /*params*/){

  nodeT *node1, *node2;
  long dincost, doutcost, arcnum, upperarcnum, startlevel;
  long group1, pathgroup, arcrow, arccol, arcdir;

  /* loop along flow path  */
  node1=startnode;
  pathgroup=lastnode->group;
  while(node1!=lastnode){

    /* update potentials along the flow path by calculating arc distances */
    node2=nextonpath;
    GetArc(node2->pred,node2,&arcrow,&arccol,&arcdir,nrow,ncol,
           nodes,nodesupp);
    doutcost=node1->outcost - node2->outcost
      + GetCost(incrcosts,arcrow,arccol,arcdir);
    node2->outcost+=doutcost;
    dincost=node1->incost - node2->incost
      + GetCost(incrcosts,arcrow,arccol,-arcdir);
    node2->incost+=dincost;
    node2->group=node1->group+dgroup;

    /* update potentials of children of this node in the flow path */
    node1=node2;
    arcnum=GetArcNumLims(node1->row,&upperarcnum,ngroundarcs,boundary);
    while(arcnum<upperarcnum){
      node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                         &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);
      if(node2->pred==node1 && node2->group>0){
        if(node2->group==pathgroup){
          nextonpath=node2;
        }else{
          startlevel=node2->level;
          group1=node1->group;
          while(TRUE){
            node2->group=group1;
            node2->incost+=dincost;
            node2->outcost+=doutcost;
            node2=node2->next;
            if(node2->level <= startlevel){
              break;
            }
          }
        }
      }
    }
  }
  return;
}


/* function: PruneTree()
 * ---------------------
 * Descends the tree from the source and finds leaves that can be 
 * removed.
 */
static
long PruneTree(nodeT *source, Array2D<nodeT>& nodes, nodeT *ground, boundaryT *boundary,
               Array2D<nodesuppT>& nodesupp, Array2D<incrcostT>& incrcosts,
               Array2D<short>& flows, long ngroundarcs, long prunecostthresh,
               long nrow, long ncol){

  long npruned;
  nodeT *node1;

  /* set up */
  npruned=0;

  /* descend tree and look for leaves to prune */
  node1=source->next;
  while(node1!=source){
    
    /* see if current node is a leaf that should be pruned */
    if(CheckLeaf(node1,nodes,ground,boundary,nodesupp,incrcosts,flows,
                 ngroundarcs,nrow,ncol,prunecostthresh)){

      /* remove the current node from the tree */
      node1->prev->next=node1->next;
      node1->next->prev=node1->prev;
      node1->group=PRUNED;
      npruned++;

      /* see if last node checked was current node's parent */
      /*   if so, it may need pruning since its child has been pruned */
      if(node1->prev->level < node1->level){
        node1=node1->prev;
      }else{
        node1=node1->next;
      }

    }else{

      /* move on to next node */
      node1=node1->next;

    }
  }

  /* show status */
  constexpr int output_detail_level=2;
  auto status=pyre::journal::info_t("isce3.unwrap.snaphu.status",output_detail_level);
  status << pyre::journal::at(__HERE__)
          << pyre::journal::newline << "  Pruned " << npruned << " nodes"
          << pyre::journal::endl;
  
  /* return number of pruned nodes */
  return(npruned);

}


/* function: CheckLeaf()
 * ---------------------
 * Checks to see if the passed node should be pruned from the tree.  
 * The node should be pruned if it is a leaf and if all of its outgoing
 * arcs have very high costs and only lead to other nodes that are already
 *  on the tree or are already pruned.
 */
static
int CheckLeaf(nodeT *node1, Array2D<nodeT>& nodes, nodeT *ground, boundaryT *boundary,
              Array2D<nodesuppT>& nodesupp, Array2D<incrcostT>& incrcosts,
              Array2D<short>& flows, long ngroundarcs, long nrow, long ncol,
              long prunecostthresh){

  long arcnum, upperarcnum, arcrow, arccol, arcdir;
  nodeT *node2;


  /* first, check to see if node1 is a leaf */
  if(node1->next->level > node1->level){
    return(FALSE);
  }

  /* loop over outgoing arcs */
  arcnum=GetArcNumLims(node1->row,&upperarcnum,ngroundarcs,boundary);
  while(arcnum<upperarcnum){
    node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                       &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);

    /* current node should not be pruned if adjacent node */
    /*   has not been added to tree yet */
    /*   or if incremental costs of arc are too small */
    /*   or there is nonzero flow on the arc */
    if(node2->group==0 || node2->group==INBUCKET
       || incrcosts(arcrow,arccol).poscost<prunecostthresh
       || incrcosts(arcrow,arccol).negcost<prunecostthresh
       || flows(arcrow,arccol)!=0){
      return(FALSE);
    }
  }

  /* all conditions for pruning are satisfied */
  return(TRUE);

}


/* function: InitNetowrk()
 * -----------------------
 */
int InitNetwork(Array2D<short>& flows, long *ngroundarcsptr, long *ncycleptr,
                long *nflowdoneptr, long *mostflowptr, long *nflowptr,
                long *candidatebagsizeptr, Array1D<candidateT>* candidatebagptr,
                long *candidatelistsizeptr, Array1D<candidateT>* candidatelistptr,
                Array2D<signed char>* iscandidateptr, Array2D<nodeT*>* apexesptr,
                bucketT *bkts, long *iincrcostfileptr,
                Array2D<incrcostT>* incrcostsptr, Array2D<nodeT>* nodesptr, nodeT *ground,
                long *nnoderowptr, Array1D<int>* nnodesperrowptr, long *narcrowptr,
                Array1D<int>* narcsperrowptr, long nrow, long ncol,
                signed char *notfirstloopptr, totalcostT *totalcostptr,
                paramT *params){

  long i;

  /* get and initialize memory for nodes */
  if(ground!=NULL && !nodesptr->size()){
    *nodesptr = Array2D<nodeT>(nrow-1, ncol-1);
    InitNodeNums(nrow-1,ncol-1,*nodesptr,ground);
  }

  /* take care of ambiguous flows to ground at corners */
  if(ground!=NULL){
    flows(0,0)+=flows(nrow-1,0);
    flows(nrow-1,0)=0;
    flows(0,ncol-1)-=flows(nrow-1,ncol-2);
    flows(nrow-1,ncol-2)=0;
    flows(nrow-2,0)-=flows(2*nrow-2,0);
    flows(2*nrow-2,0)=0;
    flows(nrow-2,ncol-1)+=flows(2*nrow-2,ncol-2);
    flows(2*nrow-2,ncol-2)=0;
  }

  /* initialize network solver variables */
  *ncycleptr=0;
  *nflowptr=1;
  *candidatebagsizeptr=INITARRSIZE;
  *candidatebagptr= Array1D<candidateT>(*candidatebagsizeptr);
  *candidatelistsizeptr=INITARRSIZE;
  *candidatelistptr = Array1D<candidateT>(*candidatelistsizeptr);
  if(ground!=NULL){
    *nflowdoneptr=0;
    *mostflowptr=Short2DRowColAbsMax(flows,nrow,ncol);
    if(*mostflowptr*params->nshortcycle>LARGESHORT){
      auto info=pyre::journal::info_t("isce3.unwrap.snaphu");
      info << pyre::journal::at(__HERE__)
           << "Maximum flow on network: " << *mostflowptr
           << pyre::journal::endl;
      fflush(NULL);
      throw isce3::except::RuntimeError(ISCE_SRCINFO(),
              "((Maximum flow) * NSHORTCYCLE) too large");
    }
    if(ncol>2){
      *ngroundarcsptr=2*(nrow+ncol-2)-4; /* don't include corner column arcs */
    }else{
      *ngroundarcsptr=2*(nrow+ncol-2)-2;
    }
    *iscandidateptr = MakeRowColArray2D<signed char>(nrow, ncol);
    *apexesptr = MakeRowColArray2D<nodeT*>(nrow, ncol);
  }

  /* set up buckets for TreeSolve (MSTInitFlows() has local set of buckets) */
  if(ground!=NULL){
    bkts->minind=-LRound((params->maxcost+1)*(nrow+ncol)
                               *NEGBUCKETFRACTION);
    bkts->maxind=LRound((params->maxcost+1)*(nrow+ncol)
                              *POSBUCKETFRACTION);
  }else{
    bkts->minind=-LRound((params->maxcost+1)*(nrow)
                               *NEGBUCKETFRACTION);
    bkts->maxind=LRound((params->maxcost+1)*(nrow)
                              *POSBUCKETFRACTION);
  }
  bkts->size=bkts->maxind-bkts->minind+1;
  bkts->bucketbase = Array1D<nodeT*>(bkts->size);
  bkts->bucket = &(bkts->bucketbase[-bkts->minind]);
  for(i=0;i<bkts->size;i++){
    bkts->bucketbase[i]=NULL;
  }

  /* get memory for incremental cost arrays */
  *iincrcostfileptr=0;
  if(ground!=NULL){
    *incrcostsptr = MakeRowColArray2D<incrcostT>(nrow, ncol);
  }

  /* set number of nodes and arcs per row */
  if(ground!=NULL){
    (*nnoderowptr)=nrow-1;
    *nnodesperrowptr = Array1D<int>(nrow-1);
    for(i=0;i<nrow-1;i++){
      (*nnodesperrowptr)[i]=ncol-1;
    }
    (*narcrowptr)=2*nrow-1;
    *narcsperrowptr = Array1D<int>(2*nrow-1);
    for(i=0;i<nrow-1;i++){
      (*narcsperrowptr)[i]=ncol;
    }
    for(i=nrow-1;i<2*nrow-1;i++){
      (*narcsperrowptr)[i]=ncol-1;
    }
  }

  /* initialize variables for main optimizer loop */
  (*notfirstloopptr)=FALSE;
  (*totalcostptr)=INITTOTALCOST;

  /* done */
  return(0);

}


/* function: SetupTreeSolveNetwork()
 * ---------------------------------
 */
long SetupTreeSolveNetwork(Array2D<nodeT>& nodes, nodeT *ground, Array2D<nodeT*>& apexes,
                           Array2D<signed char>& iscandidate, long nnoderow,
                           Array1D<int>& nnodesperrow, long narcrow, Array1D<int>& narcsperrow,
                           long nrow, long ncol){

  long row, col, nnodes;


  /* loop over each node and initialize values */
  nnodes=0;
  for(row=0;row<nnoderow;row++){
    for(col=0;col<nnodesperrow[row];col++){
      if(nodes(row,col).group!=MASKED){
        nodes(row,col).group=0;
        nnodes++;
      }
      nodes(row,col).incost=VERYFAR;
      nodes(row,col).outcost=VERYFAR;
      nodes(row,col).pred=NULL;
    }
  }

  /* initialize the ground node */
  if(ground!=NULL){
    if(ground->group!=MASKED){
      ground->group=0;
      nnodes++;
    }
    ground->incost=VERYFAR;
    ground->outcost=VERYFAR;
    ground->pred=NULL;
  }

  /* initialize arcs */
  for(row=0;row<narcrow;row++){
    for(col=0;col<narcsperrow[row];col++){
      apexes(row,col)=NONTREEARC;
      iscandidate(row,col)=FALSE;
    }
  }

  /* if in grid mode, ground will exist */
  if(ground!=NULL){

    /* set iscandidate=TRUE for illegal corner arcs so they're never used */
    iscandidate(nrow-1,0)=TRUE;
    iscandidate(2*nrow-2,0)=TRUE;
    iscandidate(nrow-1,ncol-2)=TRUE;
    iscandidate(2*nrow-2,ncol-2)=TRUE;

  }

  /* return the number of nodes in the network */
  return(nnodes);
      
}


/* function: CheckMagMasking()
 * ---------------------------
 * Check magnitude masking to make sure we have at least one pixel
 * that is not masked; return 0 on success, or 1 if all pixels masked.
 */
signed char CheckMagMasking(Array2D<float>& mag, long nrow, long ncol){

  long row, col;

  /* loop over pixels and see if any are unmasked */
  for(row=0;row<nrow;row++){
    for(col=0;col<ncol;col++){
      if(mag(row,col)>0){
        return(0);
      }
    }
  }
  return(1);

}


/* function: MaskNodes()
 * ---------------------
 * Set group numbers of nodes to MASKED if they are surrounded by
 * zero-magnitude pixels, 0 otherwise.
 */
int MaskNodes(long nrow, long ncol, Array2D<nodeT>& nodes, nodeT *ground,
              Array2D<float>& mag){

  long row, col;

  /* loop over grid nodes and see if masking is necessary */
  for(row=0;row<nrow-1;row++){
    for(col=0;col<ncol-1;col++){
      nodes(row,col).group=GridNodeMaskStatus(row,col,mag);
    }
  }

  /* check whether ground node should be masked */
  ground->group=GroundMaskStatus(nrow,ncol,mag);

  /* done */
  return(0);

}


/* function: GridNodeMaskStatus()
 * ---------------------------------
 * Given row and column of grid node, return MASKED if all pixels around node
 * have zero magnitude, and 0 otherwise.
 */
static
int GridNodeMaskStatus(long row, long col, Array2D<float>& mag){

  /* return 0 if any pixel is not masked */
  if(mag(row,col) || mag(row,col+1)
     || mag(row+1,col) || mag(row+1,col+1)){
    return(0);
  }
  return(MASKED);

}


/* function: GroundMaskStatus()
 * ----------------------------
 * Return MASKED if all pixels around grid edge have zero magnitude, 0
 * otherwise.
 */
static
int GroundMaskStatus(long nrow, long ncol, Array2D<float>& mag){

  long row, col;

  /* check all pixels along edge */
  for(row=0;row<nrow;row++){
    if(mag(row,0) || mag(row,ncol-1)){
      return(0);
    }
  }
  for(col=0;col<ncol;col++){
    if(mag(0,col) || mag(nrow-1,col)){
      return(0);
    }
  }

  return(MASKED);

}


/* function: MaxNonMaskFlowMag()
 * -----------------------------
 * Return maximum flow magnitude that does not traverse an arc adjacent
 * to a masked interferogram pixel.
 */
long MaxNonMaskFlow(Array2D<short>& flows, Array2D<float>& mag, long nrow, long ncol){

  long row, col;
  long mostflow, flowvalue;

  /* find max flow by checking row arcs then col arcs */
  mostflow=0;
  for(row=0;row<nrow-1;row++){
    for(col=0;col<ncol;col++){
      flowvalue=labs((long )flows(row,col));
      if(flowvalue>mostflow && mag(row,col)>0 && mag(row+1,col)>0){
        mostflow=flowvalue;
      }
    }
  }
  for(row=nrow-1;row<2*nrow-1;row++){
    for(col=0;col<ncol-1;col++){
      flowvalue=labs((long )flows(row,col));
      if(flowvalue>mostflow
         && mag(row-nrow+1,col)>0 && mag(row-nrow+1,col+1)>0){
          mostflow=flowvalue;
      }
    }
  }
  return(mostflow);
}


/* function: InitNodeNums()
 * ------------------------
 */
int InitNodeNums(long nrow, long ncol, Array2D<nodeT>& nodes, nodeT *ground){

  long row, col;

  /* loop over each element and initialize values */
  for(row=0;row<nrow;row++){
    for(col=0;col<ncol;col++){
      nodes(row,col).row=row;
      nodes(row,col).col=col;
    }
  }

  /* initialize the ground node */
  if(ground!=NULL){
    ground->row=GROUNDROW;
    ground->col=GROUNDCOL;
  }
  
  /* done */
  return(0);

}
      

/* function: InitBuckets()
 * -----------------------
 */
static
int InitBuckets(bucketT *bkts, nodeT *source, long nbuckets){
  
  long i;

  /* set up bucket array parameters */
  bkts->curr=0;
  bkts->wrapped=FALSE;

  /* initialize the buckets */
  for(i=0;i<nbuckets;i++){
    bkts->bucketbase[i]=NULL;
  }

  /* put the source in the zeroth distance index bucket */
  bkts->bucket[0]=source;
  source->next=NULL;
  source->prev=NULL;
  source->group=INBUCKET;
  source->outcost=0;

  /* done */
  return(0);
  
}


/* function: InitNodes()
 * ---------------------
 */
int InitNodes(long nnrow, long nncol, Array2D<nodeT>& nodes, nodeT *ground){

  long row, col;

  /* loop over each element and initialize values */
  for(row=0;row<nnrow;row++){
    for(col=0;col<nncol;col++){
      nodes(row,col).group=NOTINBUCKET;
      nodes(row,col).incost=VERYFAR;
      nodes(row,col).outcost=VERYFAR;
      nodes(row,col).pred=NULL;
    }
  }

  /* initialize the ground node */
  if(ground!=NULL){
    ground->group=NOTINBUCKET;
    ground->incost=VERYFAR;
    ground->outcost=VERYFAR;
    ground->pred=NULL;
  }

  /* done */
  return(0);
  
}


/* function: BucketInsert()
 * ------------------------
 */
void BucketInsert(nodeT *node, long ind, bucketT *bkts){

  /* put node at beginning of bucket list */
  node->next=bkts->bucket[ind];
  if((bkts->bucket[ind])!=NULL){
    bkts->bucket[ind]->prev=node;
  }
  bkts->bucket[ind]=node;
  node->prev=NULL;

  /* mark node in bucket array */
  node->group=INBUCKET;

  /* done */
  return;

}

  
/* function: BucketRemove()
 * ------------------------
 */
void BucketRemove(nodeT *node, long ind, bucketT *bkts){
  
  /* remove node from doubly linked list */
  if((node->next)!=NULL){
    node->next->prev=node->prev;
  }
  if(node->prev!=NULL){
    node->prev->next=node->next;
  }else if(node->next==NULL){    
    bkts->bucket[ind]=NULL;
  }else{
    bkts->bucket[ind]=node->next;
  }

  /* done */
  return;

}


/* function: ClosestNode()
 * -----------------------
 */
nodeT *ClosestNode(bucketT *bkts){

  nodeT *node;

  /* find the first bucket with nodes in it */
  while(TRUE){

    /* see if we got to the last bucket */
    if((bkts->curr)>(bkts->maxind)){
        return(NULL);
    }

    /* see if we found a nonempty bucket; if so, return it */
    if((bkts->bucket[bkts->curr])!=NULL){
      node=bkts->bucket[bkts->curr];
      node->group=ONTREE;
      bkts->bucket[bkts->curr]=node->next;
      if((node->next)!=NULL){
        node->next->prev=NULL;
      }
      return(node);
    }

    /* move to next bucket */
    bkts->curr++;
  
  }
}


/* function: ClosestNodeCircular()
 * -------------------------------
 * Similar to ClosestNode(), but assumes circular buckets.  This
 * function should NOT be used if negative arc weights exist on the 
 * network; initial value of bkts->minind should always be zero.
 */
/* 
 * This function is no longer used 
 */
#if 0
static
nodeT *ClosestNodeCircular(bucketT *bkts);
static
nodeT *ClosestNodeCircular(bucketT *bkts){

  nodeT *node;

  /* find the first bucket with nodes in it */
  while(TRUE){

    /* see if we got to the last bucket */
    if((bkts->curr+bkts->minind)>(bkts->maxind)){
      if(bkts->wrapped){
        bkts->wrapped=FALSE;
        bkts->curr=0;
        bkts->minind+=bkts->size;
        bkts->maxind+=bkts->size;
      }else{
        return(NULL);
      }
    }

    /* see if we found a nonempty bucket; if so, return it */
    if((bkts->bucket[bkts->curr])!=NULL){
      node=bkts->bucket[bkts->curr];
      node->group=ONTREE;
      bkts->bucket[bkts->curr]=node->next;
      if((node->next)!=NULL){
        node->next->prev=NULL;
      }
      return(node);
    }

    /* move to next bucket */
    bkts->curr++;
  
  }
}
#endif


/* function: MinOutCostNode()
 * --------------------------
 * Similar to ClosestNode(), but always returns closest node even if its
 * outcost is less than the minimum bucket index.  Does not handle circular
 * buckets.  Does not handle no nodes left condition (this should be handled 
 * by calling function).
 */
static
nodeT *MinOutCostNode(bucketT *bkts){

  long minoutcost;
  nodeT *node1, *node2;

  /* move to next non-empty bucket */
  while(bkts->curr<bkts->maxind && bkts->bucket[bkts->curr]==NULL){
    bkts->curr++;
  }

  /* scan the whole bucket if it is the overflow or underflow bag */
  if(bkts->curr==bkts->minind || bkts->curr==bkts->maxind){

    node2=bkts->bucket[bkts->curr];
    node1=node2;
    minoutcost=node1->outcost;
    while(node2!=NULL){
      if(node2->outcost<minoutcost){
        minoutcost=node2->outcost;
        node1=node2;
      }
      node2=node2->next;
    }
    BucketRemove(node1,bkts->curr,bkts);

  }else{

    node1=bkts->bucket[bkts->curr];
    bkts->bucket[bkts->curr]=node1->next;
    if(node1->next!=NULL){
      node1->next->prev=NULL;
    }

  }

  return(node1);

}


/* function: SelectSources()
 * -------------------------
 * Create a list of node pointers to be sources for each set of
 * connected pixels (not disconnected by masking).  Return the number
 * of sources (ie, the number of connected sets of pixels).
 */
long SelectSources(Array2D<nodeT>& nodes, Array2D<float>& mag, nodeT *ground, long /*nflow*/,
                   Array2D<short>& /*flows*/, long ngroundarcs,
                   long nrow, long ncol, paramT *params,
                   Array1D<nodeT*>* sourcelistptr, Array1D<long>* nconnectedarrptr){

  long row, col, nsource, nsourcelistmem, nconnected;
  nodeT *source;


  /* initialize local variables */
  nsource=0;
  nsourcelistmem=0;
  Array1D<nodeT*> sourcelist;
  Array1D<long> nconnectedarr;

  /* loop over nodes to initialize */
  if(ground->group!=MASKED && ground->group!=BOUNDARYPTR){
    ground->group=0;
  }
  ground->next=NULL;
  for(row=0;row<nrow-1;row++){
    for(col=0;col<ncol-1;col++){
      if(nodes(row,col).group!=MASKED && nodes(row,col).group!=BOUNDARYPTR){
        nodes(row,col).group=0;
      }
      nodes(row,col).next=NULL;
    }
  }

  /* check ground node (NULL for boundary since not yet defined) */
  source=SelectConnNodeSource(nodes,mag,
                              ground,ngroundarcs,nrow,ncol,params,
                              ground,&nconnected);
  if(source!=NULL){
      
    /* get more memory for source list if needed */
    if(++nsource>nsourcelistmem){
      nsourcelistmem+=NSOURCELISTMEMINCR;
      sourcelist.conservativeResize(nsourcelistmem);
      nconnectedarr.conservativeResize(nsourcelistmem);
    }

    /* store source in list */
    sourcelist[nsource-1]=source;
    nconnectedarr[nsource-1]=nconnected;
    
  }
    
  /* loop over nodes to find next set of connected nodes */
  for(row=0;row<nrow-1;row++){
    for(col=0;col<ncol-1;col++){

      /* check pixel */
      /*   boundary not yet defined, so connectivity via grid arcs only */
      source=SelectConnNodeSource(nodes,mag,
                                  ground,ngroundarcs,nrow,ncol,params,
                                  &nodes(row,col),&nconnected);
      if(source!=NULL){

        /* get more memory for source list if needed */
        if(++nsource>nsourcelistmem){
          nsourcelistmem+=NSOURCELISTMEMINCR;
          sourcelist.conservativeResize(nsourcelistmem);
          nconnectedarr.conservativeResize(nsourcelistmem);
        }

        /* store source in list */
        sourcelist[nsource-1]=source;
        nconnectedarr[nsource-1]=nconnected;

      }
    }
  }

  /* show message about number of connected regions */
  auto info=pyre::journal::info_t("isce3.unwrap.snaphu");
  info << pyre::journal::at(__HERE__)
       << "Found " << nsource << " valid set(s) of connected nodes"
       << pyre::journal::endl;

  /* reset group values for all nodes */
  if(ground->group!=MASKED && ground->group!=BOUNDARYPTR){
    ground->group=0;
  }
  ground->next=NULL;
  for(row=0;row<nrow-1;row++){
    for(col=0;col<ncol-1;col++){
      if(nodes(row,col).group==INBUCKET
         || nodes(row,col).group==NOTINBUCKET
         || nodes(row,col).group==BOUNDARYCANDIDATE
         || nodes(row,col).group==PRUNED){
        fflush(NULL);
        auto firewall=pyre::journal::firewall_t("isce3.unwrap.snaphu");
        firewall << pyre::journal::at(__HERE__)
                 << "WARNING: weird nodes[" << row << "][" << col
                 << "].group=" << nodes(row,col).group << " in SelectSources()"
                 << pyre::journal::endl;
      }

      if(nodes(row,col).group!=MASKED && nodes(row,col).group!=BOUNDARYPTR){
        nodes(row,col).group=0;
      }
      nodes(row,col).next=NULL;
    }
  }


  /* done */
  if(sourcelistptr!=NULL){
    *sourcelistptr = sourcelist;
  }else{
    fflush(NULL);
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "NULL sourcelistptr in SelectSources()");
  }
  if(nconnectedarrptr!=NULL){
    *nconnectedarrptr = nconnectedarr;
  }else{
    fflush(NULL);
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "NULL nconnectedarrptr SelectSources()");
  }
  return(nsource);

}


/* function: SelectConnNodeSource()
 * --------------------------------
 * Select source from among set of connected nodes specified by
 * starting node.  Return NULL if the start node is masked or already
 * part of another connected set, or if the connected set is too
 * small.
 */
static
nodeT *SelectConnNodeSource(Array2D<nodeT>& nodes, Array2D<float>& mag,
                            nodeT *ground, long ngroundarcs,
                            long nrow, long ncol,
                            paramT *params, nodeT *start, long *nconnectedptr){

  long nconnected;
  nodeT *source;

  /* if start node is not eligible, just return NULL */
  if(start->group==MASKED || start->group==ONTREE){
    return(NULL);
  }

  /* find all nodes for this set of connected pixels and mark them on tree */
  nconnected=ScanRegion(start,nodes,mag,ground,ngroundarcs,nrow,ncol,ONTREE);
  
  /* see if number of nodes in this connected set is big enough */
  if(nconnected>params->nconnnodemin){

    /* set source to first node in chain */
    /* this ensures that the soruce is the ground node or on the edge */
    /*   of the connected region, which tends to be faster */
    source=start;

  }else{
    source=NULL;
  }

  /* set number of connected nodes and return source */
  if(nconnectedptr!=NULL){
    (*nconnectedptr)=nconnected;
  }
  return(source);

}


/* function: ScanRegion()
 * ----------------------
 * Find all connected grid nodes of region, defined by reachability without
 * crossing non-region arcs, and set group according to desired
 * behavior defined by groupsetting, which should be either ONTREE for
 * call from SelectConnNodeSourcre(), MASKED for a call from
 * InitBoundary(), or 0 for a call from CleanUpBoundaryNodes().  Return
 * number of connected nodes.
 */
static
long ScanRegion(nodeT *start, Array2D<nodeT>& nodes, Array2D<float>& mag,
                nodeT *ground, long ngroundarcs,
                long nrow, long ncol, int groupsetting){

  nodeT *node1, *node2, *end;
  long arcrow, arccol, arcdir, arcnum, upperarcnum, nconnected;
  boundaryT *boundary;

  Array2D<nodesuppT> nodesupp;

  /* set up */
  nconnected=0;
  end=start;
  boundary=NULL;
  node1=start;
  node1->group=INBUCKET;

  /* loop to search for connected nodes */
  while(node1!=NULL){

    /* loop over neighbors of current node */
    arcnum=GetArcNumLims(node1->row,&upperarcnum,ngroundarcs,boundary);
    while(arcnum<upperarcnum){
      node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                         &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);

      /* if neighbor is pointer to boundary, unset so we scan grid nodes */
      if(node2->group==BOUNDARYPTR){
        node2->group=0;
      }

      /* see if neighbor is in region */
      if(IsRegionArc(mag,arcrow,arccol,nrow,ncol)){

        /* if neighbor is in region and not yet in list to be scanned, add it */
        if(node2->group!=ONTREE && node2->group!=INBUCKET){
          node2->group=INBUCKET;
          end->next=node2;
          node2->next=NULL;
          end=node2;
        }
      }
    }

    /* mark this node visited */
    node1->group=ONTREE;

    /* make sure level is initialized */
    if(groupsetting==ONTREE){
      node1->level=0;
    }

    /* count this node */
    nconnected++;

    /* move to next node in list */
    node1=node1->next;

  }
  
  /* for each node in region, scan neighbors to mask or unmask unreachable */
  /*   nodes that may be in other regions and therefore not yet masked */
  if(groupsetting!=ONTREE){

    /* loop over nodes in region */
    node1=start;
    while(node1!=NULL){

      /* loop over neighbors of current node */
      arcnum=GetArcNumLims(node1->row,&upperarcnum,ngroundarcs,boundary);
      while(arcnum<upperarcnum){
        node2=NeighborNode(node1,++arcnum,&upperarcnum,nodes,ground,
                           &arcrow,&arccol,&arcdir,nrow,ncol,boundary,nodesupp);

        /* see if neighbor is not in region */
        if(node2->group!=ONTREE){

          /* set mask status according to desired behavior */
          if(groupsetting==MASKED){
            node2->group=MASKED;
          }else if(groupsetting==0){
            if(node2->row==GROUNDROW){
              node2->group=GroundMaskStatus(nrow,ncol,mag);
            }else{
              node2->group=GridNodeMaskStatus(node2->row,node2->col,mag);
            }
          }
        }
      }

      /* move to next node */
      node1=node1->next;

    }

    /* reset groups of all nodes within region */
    node1=start;
    while(node1!=NULL){
      node1->group=0;
      node1=node1->next;
    }
  }

  /* return number of connected nodes */
  return(nconnected);

}


/* function: GetCost()
 * -------------------
 * Returns incremental flow cost for current flow increment dflow from
 * lookup array.
 */
static
short GetCost(Array2D<incrcostT>& incrcosts, long arcrow, long arccol,
              long arcdir){

  /* look up cost and return it for the appropriate arc direction */
  /* we may want add a check here for clipped incremental costs */
  if(arcdir>0){
    return(incrcosts(arcrow,arccol).poscost);
  }else{
    return(incrcosts(arcrow,arccol).negcost);
  }
}


/* function: ReCalcCost()
 * ----------------------
 * Updates the incremental cost for an arc.
 */
template<class CostTag>
long ReCalcCost(Array2D<typename CostTag::Cost>& costs, Array2D<incrcostT>& incrcosts, long flow,
                long arcrow, long arccol, long nflow, long nrow,
                paramT *params, CostTag tag){

  long poscost, negcost, iclipped;

  /* calculate new positive and negative nflow costs, as long ints */
  CalcCost(costs,flow,arcrow,arccol,nflow,nrow,params,
           &poscost,&negcost,tag);

  /* clip costs to short int */
  iclipped=0;
  if(poscost>LARGESHORT){
    incrcosts(arcrow,arccol).poscost=LARGESHORT;
    iclipped++;
  }else{
    if(poscost<-LARGESHORT){
      incrcosts(arcrow,arccol).poscost=-LARGESHORT;
      iclipped++;
    }else{
      incrcosts(arcrow,arccol).poscost=poscost;
    }
  }
  if(negcost>LARGESHORT){
    incrcosts(arcrow,arccol).negcost=LARGESHORT;
    iclipped++;
  }else{
    if(negcost<-LARGESHORT){
      incrcosts(arcrow,arccol).negcost=-LARGESHORT;
      iclipped++;
    }else{
      incrcosts(arcrow,arccol).negcost=negcost;
    }
  }

  /* return the number of clipped incremental costs (0, 1, or 2) */
  return(iclipped);
}


/* function: SetupIncrFlowCosts()
 * ------------------------------
 * Calculates the costs for positive and negative dflow flow increment
 * if there is zero flow on the arc.
 */
template<class CostTag>
int SetupIncrFlowCosts(Array2D<typename CostTag::Cost>& costs, Array2D<incrcostT>& incrcosts, 
                       Array2D<short>& flows, long nflow, long nrow, long narcrow,
                       Array1D<int>& narcsperrow, paramT *params, CostTag tag){

  long arcrow, arccol, iclipped, narcs;
  char pl[2];

  /* loop over all rows and columns */
  narcs=0;
  iclipped=0;
  for(arcrow=0;arcrow<narcrow;arcrow++){
    narcs+=narcsperrow[arcrow];
    for(arccol=0;arccol<narcsperrow[arcrow];arccol++){

      /* calculate new positive and negative nflow costs, as long ints */
      iclipped+=ReCalcCost(costs,incrcosts,flows(arcrow,arccol),
                           arcrow,arccol,nflow,nrow,params,tag);
    }
  }

  /* print overflow warning if applicable */
  if(iclipped){
    if(iclipped>1){
      strcpy(pl,"s");
    }else{
      strcpy(pl,"");
    }
    fflush(NULL);
    auto warnings=pyre::journal::warning_t("isce3.unwrap.snaphu");
    warnings << pyre::journal::at(__HERE__)
             << iclipped << " incremental cost" << pl
             << " clipped to avoid overflow ("
             << std::fixed << std::setprecision(3)
             << ((double )iclipped)/(2*narcs) << "%)"
             << pyre::journal::endl;
  }

  /* done */
  return(0);

}


/* function: EvaluateTotalCost()
 * -----------------------------
 * Computes the total cost of the flow array and prints it out.  Pass nrow
 * and ncol if in grid mode (primary network), or pass nrow=ntiles and
 * ncol=0 for nongrid mode (secondary network).
 */
template<class CostTag>
totalcostT EvaluateTotalCost(Array2D<typename CostTag::Cost>& costs, Array2D<short>& flows, 
                             long nrow, long ncol, Array1D<int>& narcsperrow, paramT *params, 
                             CostTag tag){

  totalcostT rowcost, totalcost;
  long row, col, maxrow, maxcol;

  /* sum cost for each row and column arc */
  totalcost=0;
  if(ncol){
    maxrow=2*nrow-1;
  }else{
    maxrow=nrow;
  }
  for(row=0;row<maxrow;row++){
    rowcost=0;
    if(ncol){
      if(row<nrow-1){
        maxcol=ncol;
      }else{
        maxcol=ncol-1;
      }
    }else{
      maxcol=narcsperrow[row];
    }
    for(col=0;col<maxcol;col++){
      rowcost+=EvalCost(costs,flows,row,col,nrow,params,tag);
    }
    totalcost+=rowcost;
  }

  return(totalcost);
}


/* function: MSTInitFlows()
 * ------------------------
 * Initializes the flow on a the network using minimum spanning tree
 * algorithm.
 */
int MSTInitFlows(Array2D<float>& wrappedphase, Array2D<short>* flowsptr,
                 Array2D<short>& mstcosts, long nrow, long ncol,
                 Array2D<nodeT>* nodesptr, nodeT *ground, long maxflow){

  long row, col, i, maxcost;
  nodeT *source;
  bucketT bkts[1]={};

  auto info=pyre::journal::info_t("isce3.unwrap.snaphu");

  /* get and initialize memory for ground, nodes, buckets, and child array */
  *nodesptr = Array2D<nodeT>(nrow-1, ncol-1);
  InitNodeNums(nrow-1,ncol-1,*nodesptr,ground);

  /* find maximum cost */
  maxcost=0;
  for(row=0;row<2*nrow-1;row++){
    if(row<nrow-1){
      i=ncol;
    }else{
      i=ncol-1;
    }
    for(col=0;col<i;col++){
      if(mstcosts(row,col)>maxcost
         && !((row==nrow-1 || 2*nrow-2) && (col==0 || col==ncol-2))){
        maxcost=mstcosts(row,col);
      }
    }
  }

  /* get memory for buckets and arc status */
  bkts->size=LRound((maxcost+1)*(nrow+ncol+1));
  bkts->bucketbase = Array1D<nodeT*>(bkts->size);
  bkts->minind=0;
  bkts->maxind=bkts->size-1;
  bkts->bucket = bkts->bucketbase.data();
  auto arcstatus = MakeRowColArray2D<signed char>(nrow, ncol);

  /* calculate phase residues (integer numbers of cycles) */
  info << pyre::journal::at(__HERE__)
       << "Initializing flows with MST algorithm"
       << pyre::journal::endl;
  auto residue = Array2D<signed char>(nrow-1, ncol-1);
  CycleResidue(wrappedphase,residue,nrow,ncol);

  /* get memory for flow arrays */
  *flowsptr = MakeRowColArray2D<short>(nrow, ncol);
  auto& flows=*flowsptr;

  /* loop until no flows exceed the maximum flow */
  constexpr int output_detail_level=2;
  auto verbose=pyre::journal::info_t("isce3.unwrap.snaphu",output_detail_level);
  verbose << pyre::journal::at(__HERE__)
          << "Running approximate minimum spanning tree solver"
          << pyre::journal::endl;
  while(TRUE){

    /* set up the source to be the first non-zero residue that we find */
    source=NULL;
    for(row=0;row<nrow-1 && source==NULL;row++){
      for(col=0;col<ncol-1 && source==NULL;col++){
        if(residue(row,col)){
          source=&(*nodesptr)(row,col);
        }
      }
    }
    if(source==NULL){
      info << pyre::journal::at(__HERE__)
           << "No residues found"
           << pyre::journal::endl;
      break;
    }

    /* initialize data structures */
    InitNodes(nrow-1,ncol-1,*nodesptr,ground);
    InitBuckets(bkts,source,bkts->size);
    
    /* solve the mst problem */
    SolveMST(*nodesptr,source,ground,bkts,mstcosts,residue,arcstatus,
             nrow,ncol);
    
    /* find flows on minimum tree (only one feasible flow exists) */
    DischargeTree(source,mstcosts,flows,residue,arcstatus,
                  *nodesptr,ground,nrow,ncol);
    
    /* do pushes to clip the flows and make saturated arcs ineligible */
    /* break out of loop if there is no flow greater than the limit */
    if(ClipFlow(residue,flows,mstcosts,nrow,ncol,maxflow)){
      break;
    }
  }

  return(0);

}


/* function: SolveMST()
 * --------------------
 * Finds tree which spans all residue nodes of approximately minimal length.
 * Note that this function may produce a Steiner tree (tree may split at 
 * non-residue node), though finding the exactly minimum Steiner tree is 
 * NP-hard.  This function uses Prim's algorithm, nesting Dijkstra's 
 * shortest path algorithm in each iteration to find next closest residue 
 * node to tree.  See Ahuja, Orlin, and Magnanti 1993 for details.  
 *
 * Dijkstra implementation and some associated functions adapted from SPLIB 
 * shortest path codes written by Cherkassky, Goldberg, and Radzik.
 */
static
void SolveMST(Array2D<nodeT>& nodes, nodeT *source, nodeT *ground,
              bucketT *bkts, Array2D<short>& mstcosts, Array2D<signed char>& residue,
              Array2D<signed char>& arcstatus, long nrow, long ncol){

  nodeT *from, *to, *pathfrom, *pathto;
  long fromdist, newdist, arcdist, ngroundarcs, groundcharge;
  long fromrow, fromcol, row, col, arcnum, upperarcnum, maxcol;
  long pathfromrow, pathfromcol;
  long arcrow, arccol, arcdir;

  Array2D<nodesuppT> nodesupp;

  /* calculate the number of ground arcs */
  ngroundarcs=2*(nrow+ncol-2)-4;

  /* calculate charge on ground */
  groundcharge=0;
  for(row=0;row<nrow-1;row++){
    for(col=0;col<ncol-1;col++){
      groundcharge-=residue(row,col);
    }
  }

  /* initialize arc status array */
  for(arcrow=0;arcrow<2*nrow-1;arcrow++){
    if(arcrow<nrow-1){
      maxcol=ncol;
    }else{
      maxcol=ncol-1;
    }
    for(arccol=0;arccol<maxcol;arccol++){
      arcstatus(arcrow,arccol)=0;
    }
  }

  /* loop until there are no more nodes in any bucket */
  while((from=ClosestNode(bkts))!=NULL){

    /* info for current node */
    fromrow=from->row;
    fromcol=from->col;

    /* if we found a residue */
    if(((fromrow!=GROUNDROW && residue(fromrow,fromcol)) ||
       (fromrow==GROUNDROW && groundcharge)) && from!=source){

      /* set node and its predecessor */
      pathto=from;
      pathfrom=from->pred;

      /* go back and make arcstatus -1 along path */
      while(TRUE){

        /* give to node zero distance label */
        pathto->outcost=0;

        /* get arc indices for arc between pathfrom and pathto */
        GetArc(pathfrom,pathto,&arcrow,&arccol,&arcdir,nrow,ncol,
               nodes,nodesupp);

        /* set arc status to -1 to mark arc on tree */
        arcstatus(arcrow,arccol)=-1;

        /* stop when we get to a residue */
        pathfromrow=pathfrom->row;
        pathfromcol=pathfrom->col;
        if((pathfromrow!=GROUNDROW && residue(pathfromrow,pathfromcol))
           || (pathfromrow==GROUNDROW && groundcharge)){
          break;
        }
        
        /* move up to previous node pair in path */
        pathto=pathfrom;
        pathfrom=pathfrom->pred;

      } /* end while loop marking costs on path */
      
    } /* end if we found a residue */

    /* set a variable for from node's distance */
    fromdist=from->outcost;

    /* scan from's neighbors */
    arcnum=GetArcNumLims(fromrow,&upperarcnum,ngroundarcs,NULL);
    while(arcnum<upperarcnum){

      /* get row, col indices and distance of next node */
      to=NeighborNode(from,++arcnum,&upperarcnum,nodes,ground,
                      &arcrow,&arccol,&arcdir,nrow,ncol,NULL,nodesupp);
      row=to->row;
      col=to->col;

      /* get cost of arc to new node (if arc on tree, cost is 0) */
      if(arcstatus(arcrow,arccol)<0){
        arcdist=0;
      }else if((arcdist=mstcosts(arcrow,arccol))==LARGESHORT){
        arcdist=VERYFAR;
      }

      /* compare distance of new nodes to temp labels */
      if((newdist=fromdist+arcdist)<(to->outcost)){

        /* if to node is already in a bucket, remove it */
        if(to->group==INBUCKET){
          if(to->outcost<bkts->maxind){
            BucketRemove(to,to->outcost,bkts);
          }else{
            BucketRemove(to,bkts->maxind,bkts);
          }
        }
                
        /* update to node */
        to->outcost=newdist;
        to->pred=from;

        /* insert to node into appropriate bucket */
        if(newdist<bkts->maxind){
          BucketInsert(to,newdist,bkts);
          if(newdist<bkts->curr){
            bkts->curr=newdist;
          }
        }else{
          BucketInsert(to,bkts->maxind,bkts);
        }
        
      } /* end if newdist < old dist */
      
    } /* end loop over outgoing arcs */
  } /* end while ClosestNode()!=NULL */

}


/* function: DischargeTree()
 * -------------------------
 * Does depth-first search on result tree from SolveMST.  Integrates
 * charges from tree leaves back up to set arc flows.  This implementation
 * is non-recursive; a recursive implementation might be faster, but 
 * would also use much more stack memory.  This method is equivalent to 
 * walking the tree, so it should be nore more than a factor of 2 slower.
 */
static
long DischargeTree(nodeT *source, Array2D<short>& /*mstcosts*/, Array2D<short>& flows,
                   Array2D<signed char>& residue, Array2D<signed char>& arcstatus,
                   Array2D<nodeT>& nodes, nodeT *ground, long nrow, long ncol){

  long row, col, todir, arcrow, arccol, arcdir;
  long arcnum, upperarcnum, ngroundarcs;
  nodeT *from, *to, *nextnode;
  
  Array2D<nodesuppT> nodesupp;

  /* set up */
  /* use outcost member of node structure to temporarily store charge */
  nextnode=source;
  ground->outcost=0;
  for(row=0;row<nrow-1;row++){
    for(col=0;col<ncol-1;col++){
      nodes(row,col).outcost=residue(row,col);
      ground->outcost-=residue(row,col);
    }
  }
  ngroundarcs=2*(nrow+ncol-2)-4;
  todir=0;

  /* keep looping unitl we've walked the entire tree */
  while(TRUE){
    
    from=nextnode;
    nextnode=NULL;

    /* loop over outgoing arcs from this node */
    arcnum=GetArcNumLims(from->row,&upperarcnum,ngroundarcs,NULL);
    while(arcnum<upperarcnum){

      /* get row, col indices and distance of next node */
      to=NeighborNode(from,++arcnum,&upperarcnum,nodes,ground,
                      &arcrow,&arccol,&arcdir,nrow,ncol,NULL,nodesupp);

      /* see if the arc is on the tree and if it has been followed yet */
      if(arcstatus(arcrow,arccol)==-1){

        /* arc has not yet been followed: move down the tree */
        nextnode=to;
        row=arcrow;
        col=arccol;
        break;

      }else if(arcstatus(arcrow,arccol)==-2){

        /* arc has already been followed and leads back up the tree: */
        /* save it, but keep looking for downwards arc */
        nextnode=to;
        row=arcrow;
        col=arccol;
        todir=arcdir;

      }
    }

    /* break if no unfollowed arcs (ie, we are done examining the tree) */
    if(nextnode==NULL){
      break;
    }

    /* if we found leaf and we're moving back up the tree, do a push */
    /* otherwise, just mark the path by decrementing arcstatus */
    if((--arcstatus(row,col))==-3){
      flows(row,col)+=todir*from->outcost;
      nextnode->outcost+=from->outcost;
      from->outcost=0;
    }
  }

  /* finish up */
  return(from->outcost);
  
} /* end of DischargeTree() */


/* function: ClipFlow()
 * ---------------------
 * Given a flow, clips flow magnitudes to a computed limit, resets 
 * residues so sum of solution of network problem with new residues 
 * and solution of clipped problem give total solution.  Upper flow limit
 * is 2/3 the maximum flow on the network or the passed value maxflow, 
 * whichever is greater.  Clipped flow arcs get costs of passed variable 
 * maxcost.  Residues should have been set to zero by DischargeTree().
 */
static
signed char ClipFlow(Array2D<signed char>& residue, Array2D<short>& flows,
                     Array2D<short>& mstcosts, long nrow, long ncol,
                     long maxflow){

  long row, col, cliplimit, maxcol, excess, tempcharge, sign;
  long mostflow, maxcost;

  constexpr int output_detail_level=2;
  auto verbose=pyre::journal::info_t("isce3.unwrap.snaphu",output_detail_level);

  /* find maximum flow */
  mostflow=Short2DRowColAbsMax(flows,nrow,ncol);

  /* if there is no flow greater than the maximum, return TRUE */
  if(mostflow<=maxflow){
    return(TRUE);
  }
  verbose << pyre::journal::at(__HERE__)
          << "Maximum flow on network: " << mostflow
          << pyre::journal::endl;

  /* set upper flow limit */
  cliplimit=(long )ceil(mostflow*CLIPFACTOR)+1;
  if(maxflow>cliplimit){
    cliplimit=maxflow;
  }

  /* find maximum cost (excluding ineligible corner arcs) */
  maxcost=0;
  for(row=0;row<2*nrow-1;row++){
    if(row<nrow-1){
      maxcol=ncol;
    }else{
      maxcol=ncol-1;
    }
    for(col=0;col<maxcol;col++){
      if(mstcosts(row,col)>maxcost && mstcosts(row,col)<LARGESHORT){
        maxcost=mstcosts(row,col);
      }
    }
  }

  /* set the new maximum cost and make sure it doesn't overflow short int */
  maxcost+=INITMAXCOSTINCR;
  if(maxcost>=LARGESHORT){
    fflush(NULL);
    auto warnings=pyre::journal::warning_t("isce3.unwrap.snaphu");
    warnings << pyre::journal::at(__HERE__)
             << "WARNING: escaping ClipFlow loop to prevent cost overflow"
             << pyre::journal::endl;
    return(TRUE);
  }

  /* clip flows and do pushes */
  for(row=0;row<2*nrow-1;row++){
    if(row<nrow-1){
      maxcol=ncol;
    }else{
      maxcol=ncol-1;
    }
    for(col=0;col<maxcol;col++){
      if(labs(flows(row,col))>cliplimit){
        if(flows(row,col)>0){
          sign=1;
          excess=flows(row,col)-cliplimit;
        }else{
          sign=-1;
          excess=flows(row,col)+cliplimit;
        }
        if(row<nrow-1){
          if(col!=0){
            tempcharge=residue(row,col-1)+excess;
            if(tempcharge>MAXRES || tempcharge<MINRES){
              fflush(NULL);
              throw isce3::except::RuntimeError(ISCE_SRCINFO(),
                      "Overflow of residue data type");
            }
            residue(row,col-1)=tempcharge;
          }
          if(col!=ncol-1){
            tempcharge=residue(row,col)-excess;
            if(tempcharge<MINRES || tempcharge>MAXRES){
              fflush(NULL);
              throw isce3::except::RuntimeError(ISCE_SRCINFO(),
                      "Overflow of residue data type");
            }
            residue(row,col)=tempcharge;
          }
        }else{
          if(row!=nrow-1){
            tempcharge=residue(row-nrow,col)+excess;
            if(tempcharge>MAXRES || tempcharge<MINRES){
              fflush(NULL);
              throw isce3::except::RuntimeError(ISCE_SRCINFO(),
                      "Overflow of residue data type");
            }
            residue(row-nrow,col)=tempcharge;
          }
          if(row!=2*nrow-2){
            tempcharge=residue(row-nrow+1,col)-excess;
            if(tempcharge<MINRES || tempcharge>MAXRES){
              fflush(NULL);
              throw isce3::except::RuntimeError(ISCE_SRCINFO(),
                      "Overflow of residue data type");
            }
            residue(row-nrow+1,col)=tempcharge;
          }
        }
        flows(row,col)=sign*cliplimit;
        mstcosts(row,col)=maxcost;
      }
    }
  }

  /* return value indicates that flows have been clipped */
  verbose << pyre::journal::at(__HERE__)
          << "Flows clipped to " << cliplimit << ". Rerunning MST solver."
          << pyre::journal::endl;
  return(FALSE);

}


/* function: MCFInitFlows()
 * ------------------------
 * Initializes the flow on the network using a minimum cost flow
 * algorithm.
 */
int MCFInitFlows(Array2D<float>& wrappedphase, Array2D<short>* flowsptr,
                 Array2D<short>& mstcosts, long nrow, long ncol){

  auto info=pyre::journal::info_t("isce3.unwrap.snaphu");
  info << pyre::journal::at(__HERE__)
       << "Initializing flows with MCF algorithm"
       << pyre::journal::endl;

  /* number of rows & cols of nodes in the residue network */
  const auto m=nrow-1;
  const auto n=ncol-1;

  /* calculate phase residues (integer numbers of cycles) */
  auto residue=Array2D<signed char>(m,n);
  CycleResidue(wrappedphase,residue,nrow,ncol);

  /* total number of nodes and directed arcs in the network */
  const auto nnodes=m*n+1;
  const auto narcs=2*((m+1)*n+(n+1)*m);

  /* the solver uses 32-bit integers for node & arc indices */
  /* check for possible overflow */
  using operations_research::NodeIndex;
  using operations_research::ArcIndex;
  if(nnodes>std::numeric_limits<NodeIndex>::max()){
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "Number of MCF network nodes exceeds maximum representable value");
  }
  if(narcs>std::numeric_limits<ArcIndex>::max()){
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "Number of MCF network arcs exceeds maximum representable value");
  }

  /* begin building the network topology and setting up the MCF problem */
  using Network=operations_research::SimpleMinCostFlow;
  auto network=Network(nnodes,narcs);

  /* assigns a positive integer label to each grid node */
  /* grid node indices begin at 1 (index 0 is used for the ground node) */
  auto GetNodeIndex=[=](long i, long j)->NodeIndex{
    return 1+i*n+j;
  };
  constexpr NodeIndex ground=0;

  /* adds a pair of forward & reverse arcs to the network connecting two nodes */
  /* sister arcs have equal cost and capacity */
  using operations_research::CostValue;
  using operations_research::FlowQuantity;
  auto AddSisterArcs=[&](NodeIndex node1, NodeIndex node2, CostValue cost){
    constexpr static auto capacity=static_cast<FlowQuantity>(ARCUBOUND);
    network.AddArcWithCapacityAndUnitCost(node2,node1,capacity,cost);
    network.AddArcWithCapacityAndUnitCost(node1,node2,capacity,cost);
  };

  /* break down arc costs into row (horizontal) & col (vertical) cost arrays */
  const auto rowcosts=mstcosts.topLeftCorner(m,n+1);
  const auto colcosts=mstcosts.bottomLeftCorner(m+1,n);

  /* arcs are assigned sequential indices (starting from 0) in the order that
     they're added to the network */
  /* we rely on this fact later on when extracting flows from the network */

  /* begin adding horizontal arcs to the network */
  for(long i=0;i<m;++i){
    /* add a pair of arcs between the left border node and the ground node */
    {
      const auto node=GetNodeIndex(i,0);
      const auto cost=static_cast<CostValue>(rowcosts(i,0));
      AddSisterArcs(ground,node,cost);
    }

    /* add a pair of horizontal arcs between each adjacent grid node */
    for(long j=0;j<n-1;++j){
      const auto node1=GetNodeIndex(i,j);
      const auto node2=GetNodeIndex(i,j+1);
      const auto cost=static_cast<CostValue>(rowcosts(i,j+1));
      AddSisterArcs(node1,node2,cost);
    }

    /* add a pair of arcs between the right border node and the ground node */
    {
      const auto node=GetNodeIndex(i,n-1);
      const auto cost=static_cast<CostValue>(rowcosts(i,n));
      AddSisterArcs(node,ground,cost);
    }
  }

  /* begin adding vertical arcs to the network */
  /* add a pair of arcs between each top border node and the ground node */
  for(long j=0;j<n;++j){
    const auto node=GetNodeIndex(0,j);
    const auto cost=static_cast<CostValue>(colcosts(0,j));
    AddSisterArcs(ground,node,cost);
  }
  /* add a pair of vertical arcs between each adjacent grid node */
  for(long i=0;i<m-1;++i){
    for(long j=0;j<n;++j){
      const auto node1=GetNodeIndex(i,j);
      const auto node2=GetNodeIndex(i+1,j);
      const auto cost=static_cast<CostValue>(colcosts(i+1,j));
      AddSisterArcs(node1,node2,cost);
    }
  }
  /* add a pair of arcs between each bottom border node and the ground node */
  for(long j=0;j<n;++j){
    const auto node=GetNodeIndex(m-1,j);
    const auto cost=static_cast<CostValue>(colcosts(m,j));
    AddSisterArcs(node,ground,cost);
  }

  /* add node supplies to the network */
  FlowQuantity totalsupply=0;
  for(long i=0;i<m;++i){
    for(long j=0;j<n;++j){
      auto node=GetNodeIndex(i,j);
      auto supply=static_cast<FlowQuantity>(residue(i,j));
      network.SetNodeSupply(node,supply);
      totalsupply+=supply;
    }
  }

  /* add enough demand to the ground node to balance the network */
  network.SetNodeSupply(ground,-totalsupply);

  /* run the solver to produce L1-optimal flows */
  if(network.Solve() != Network::OPTIMAL){
    throw isce3::except::RuntimeError(ISCE_SRCINFO(),
            "MCF initialization failed");
  }

  *flowsptr=MakeRowColArray2D<short>(nrow,ncol);

  /* break down arc flows into row (horizontal) & col (vertical) flow arrays */
  auto rowflows=flowsptr->topLeftCorner(m,n+1);
  auto colflows=flowsptr->bottomLeftCorner(m+1,n);

  /* extract arc flows from the network */
  /* the easiest way to do this is in the exact order in which the arcs were
     added to the network (relying implicitly on the sequential ordering of arc
     indices) */

  /* extract horizontal flows from the network */
  ArcIndex arcidx=0;
  for(long i=0;i<m;++i){
    for(long j=0;j<n+1;++j){
      /* Compute eastward-minus-westward net flow */
      const auto x1=network.Flow(arcidx++);
      const auto x2=network.Flow(arcidx++);
      rowflows(i,j)=x2-x1;
    }
  }

  /* extract vertical flows from the network */
  for(long i=0;i<m+1;++i){
    for(long j=0;j<n;++j){
      /* Compute southward-minus-northward net flow */
      const auto x1=network.Flow(arcidx++);
      const auto x2=network.Flow(arcidx++);
      colflows(i,j)=x2-x1;
    }
  }

  /* done */
  return(0);
}


#define INSTANTIATE_TEMPLATES(T) \
  template long TreeSolve(Array2D<nodeT>&, Array2D<nodesuppT>&, nodeT*, \
                          nodeT*, Array1D<candidateT>*, \
                          Array1D<candidateT>*, long*, \
                          long*, bucketT*, Array2D<short>&, \
                          Array2D<typename T::Cost>&, Array2D<incrcostT>&, Array2D<nodeT*>&, \
                          Array2D<signed char>&, long, long, \
                          Array2D<float>&, Array2D<float>&, char*, \
                          long, Array1D<int>&, long, \
                          Array1D<int>&, long, long, \
                          outfileT*, long, paramT*, T); \
  template long ReCalcCost(Array2D<typename T::Cost>&, Array2D<incrcostT>&, long, \
                           long, long, long, long, \
                           paramT*, T); \
  template int SetupIncrFlowCosts(Array2D<typename T::Cost>&, Array2D<incrcostT>&, Array2D<short>&, \
                       long, long, long, \
                       Array1D<int>&, paramT*, T); \
  template totalcostT EvaluateTotalCost(Array2D<typename T::Cost>&, Array2D<short>&, long, long, \
                             Array1D<int>&, paramT*, T);
INSTANTIATE_TEMPLATES(TopoCostTag)
INSTANTIATE_TEMPLATES(DefoCostTag)
INSTANTIATE_TEMPLATES(SmoothCostTag)
INSTANTIATE_TEMPLATES(L0CostTag)
INSTANTIATE_TEMPLATES(L1CostTag)
INSTANTIATE_TEMPLATES(L2CostTag)
INSTANTIATE_TEMPLATES(LPCostTag)
INSTANTIATE_TEMPLATES(L0BiDirCostTag)
INSTANTIATE_TEMPLATES(L1BiDirCostTag)
INSTANTIATE_TEMPLATES(L2BiDirCostTag)
INSTANTIATE_TEMPLATES(LPBiDirCostTag)
INSTANTIATE_TEMPLATES(NonGridCostTag)

} // namespace isce3::unwrap
