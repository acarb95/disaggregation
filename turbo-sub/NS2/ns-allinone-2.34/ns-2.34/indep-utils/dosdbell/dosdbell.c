//This file outputs a ns file for a dumbell network to be used for
//DDOS experiments.
/*the input file is a script file which is read by the program to get
the parameters of the network.

A typical file example
d (diameter of the network) H (hurtz parameter)  
n1 (no of incoming TCP nodes) rate (rate) 
n2 (no. of DDOS attackers in the subtree) master (0/1) rate (rate)
*/

#include<stdio.h>
#include<stdlib.h>
#include "dosdbell.h"


void set_Topology(FILE *filer);
void TCL_Write_Initial(FILE *);
void TCL_Write_Nodes(FILE *);
void TCL_Write_Connections(FILE*);
void TCL_Write_Agents(FILE *);
void TCL_Write_DDOS_Agent(FILE*);
void TCL_Write_Final(FILE*);

struct Topology topo;
int diameter;
double TCP_start_time;
double DDOS_start_time;
double finish_time;
double bw;
int pack_size;
float bn_delay;
float bn_bw ; 

main(int argc, char **argv){
  FILE *filer;
 
  if(argc !=3){
    printf("USAGE: %s parameter_filename tcloutput_filename\n",argv[0]);
    exit(0);
  }
  else{
    filer = fopen(argv[1],"r");
    set_Topology(filer);
    fclose(filer);
    filer = fopen(argv[2],"w");
    TCL_Write_Initial(filer);
    TCL_Write_Nodes(filer);
    TCL_Write_Connections(filer);
    TCL_Write_Agents(filer);
    TCL_Write_DDOS_Agent(filer);
    TCL_Write_Final(filer);
    fclose(filer);
  }  
}


  
void set_Topology(FILE *filer){
  double perT, perFTP, perCBR, perV;
  int i;
  double H;
  double rnd;
  int attackNo,master;
  double DDOSRate;
  double hop;  

  fscanf(filer,"%d %f %f\n",&diameter,&bn_bw,&bn_delay);
  fscanf(filer,"%d %lf %lf\n",&topo.TCPNodes.NoNodes,&bw,&TCP_start_time);  
  topo.TCPNodes.TCPNode = (struct TCP_Node_Info *)malloc(topo.TCPNodes.NoNodes*sizeof(struct TCP_Node_Info));

  for(i=0;i<topo.TCPNodes.NoNodes;i++){
    
    /* Based on the topology, get the number of hops from the source to destination.
       Since a dumbbell has a minimum of three hops, the number of hops returned 
       should be greater than 3 
    */ 

    do{
      topo.TCPNodes.TCPNode[i].delayFrom = (double) Inet_default_no_hops(diameter);
    }while(topo.TCPNodes.TCPNode[i].delayFrom<=3);

    /* Calculate the delay in for both the links */
    topo.TCPNodes.TCPNode[i].delayTo = ceil(drand48()*(topo.TCPNodes.TCPNode[i].delayFrom-2));
    topo.TCPNodes.TCPNode[i].delayFrom -= topo.TCPNodes.TCPNode[i].delayTo;
    
    // Maximum delay between the start of all sources 
    rnd = ((double)(drand48()*TCP_start_time));
    topo.TCPNodes.TCPNode[i].startTime = rnd;
  }

  fscanf(filer,"%d %lf %d %d %lf\n",&topo.CBRTraffic.NoAttackers,&topo.CBRTraffic.DDOSRate,&pack_size,&master,&DDOS_start_time); 
  CBRTrafficInet(diameter,topo.CBRTraffic.NoAttackers, master, topo.CBRTraffic.DDOSRate, &topo.CBRTraffic.DDOSTraffic);
  if(master==0)
    topo.CBRTraffic.master = NoMaster;
  else
    topo.CBRTraffic.master = Master;
  topo.CBRTraffic.MaxTime = topo.CBRTraffic.DDOSTraffic.Events[topo.CBRTraffic.DDOSTraffic.NoEvents-1].delay;
  fscanf(filer,"%lf\n",&finish_time);
  printf("Topology Generated Using:\n Diameter=%d BottleNeck: BW=%.3fMbps Delay=%.3fms\n \
Background Traffic: TCP Nodes=%d BW=%.3lfMbps Starttime=%.1lfs\n \
Dos Traffic       : Attacker= %d BW=%.3lfMbps Starttime=%.1lfs\n \
PacketSize=%dB  Master = %d  Finishtime = %.1lfs\n", \
	 diameter,bn_bw,bn_delay,topo.TCPNodes.NoNodes,bw,TCP_start_time, \
	 topo.CBRTraffic.NoAttackers, \
         topo.CBRTraffic.DDOSRate, DDOS_start_time,pack_size,master,finish_time);
}

void TCL_Write_Initial(FILE *filew){
  fprintf(filew, "set ns [new Simulator]\n\n\n");
  /* Incase we need nam support */
  //fprintf(filew, "set nf [open out.nam w]\n");
  //fprintf(filew, "$ns namtrace-all $nf\n\n\n");
  //fprintf(filew, "proc finish {} { \n  global ns nf \n  $ns flush-trace \n  close $nf \n  exec nam out.nam & \n  exit 0 \n  }\n\n\n");
  /***till here***/
  fprintf(filew, "proc finish {} {\n  exit 0 \n  }\n\n\n");
}


void TCL_Write_Nodes(FILE *filew){
  fprintf(filew, "for {set i 0} {$i < %d} {incr i} { \n  set TCPSend($i) [$ns node] \n  }\n\n",topo.TCPNodes.NoNodes);
  fprintf(filew, "for {set i 0} {$i < %d} {incr i} { \n  set TCPRecv($i) [$ns node] \n  }\n\n",topo.TCPNodes.NoNodes);

  
   fprintf(filew,"set BotLeft [$ns node] \n");
   fprintf(filew,"set BotRight [$ns node] \n");
   fprintf(filew,"set DDOSNode [$ns node] \n");
   fprintf(filew,"set Victim [$ns node] \n");

   fprintf(filew,"\n\n");
}


void TCL_Write_Connections(FILE *filew){
  int i;
  for(i=0;i<topo.TCPNodes.NoNodes;i++){
    fprintf(filew,"$ns duplex-link $TCPSend(%d) $BotLeft %.3lfMb %.3lfms DropTail\n",i,bw,topo.TCPNodes.TCPNode[i].delayTo*bn_delay); 
    fprintf(filew,"$ns duplex-link $TCPRecv(%d) $BotRight %.3lfMb %.3lfms DropTail\n",i,bw,topo.TCPNodes.TCPNode[i].delayFrom*bn_delay); 
  }

   fprintf(filew,"\n\n");
   fprintf(filew,"$ns duplex-link $BotLeft $BotRight %.3lfMb %.3lfms DropTail\n",bn_bw,bn_delay); 
   fprintf(filew,"$ns duplex-link $BotLeft $DDOSNode %.3lfMb %.3lfms DropTail\n",10*bn_bw,bn_delay);
   fprintf(filew,"$ns duplex-link $BotRight $Victim %.3lfMb %.3lfms DropTail\n",10*bn_bw,bn_delay);
   fprintf(filew,"\n\n");
}


void TCL_Write_Agents(FILE *filew){
  int i;
  double time;

  fprintf(filew, "for {set i 0} {$i < %d} {incr i} { \n    \
set TCPSendAg($i) [new Agent/TCP] \n  \
$ns attach-agent $TCPSend($i) $TCPSendAg($i) \n     \
set TCPSendTraffic($i) [new Application/FTP] \n     \
$TCPSendTraffic($i) attach-agent $TCPSendAg($i) \n   \
set TCPRecvAg($i) [new Agent/TCPSink] \n    \
$ns attach-agent $TCPRecv($i) $TCPRecvAg($i) \n   \
$ns connect $TCPSendAg($i) $TCPRecvAg($i) \n  \
$TCPSendAg($i) set class_ 1 \n}\n\n",topo.TCPNodes.NoNodes);

  fprintf(filew,"\n\n\n");

  for(i=0;i<topo.TCPNodes.NoNodes;i++){
    fprintf(filew,"$ns at %lf \"$TCPSendTraffic(%d) start\"\n",topo.TCPNodes.TCPNode[i].startTime,i);
  }
  fprintf(filew,"\n\n");
}



void TCL_Write_DDOS_Agent(FILE* filew){
  int i;

  fprintf(filew,"\n\n");
  fprintf(filew,"set DDOSUDP [new Agent/UDP]\n");
  fprintf(filew,"$ns attach-agent $DDOSNode $DDOSUDP\n");
  fprintf(filew,"set DDOSTraffic [new Application/Traffic/CBR]\n");
  fprintf(filew,"$DDOSTraffic attach-agent $DDOSUDP\n");
  fprintf(filew,"set VictimAgent [new Agent/Null]\n");
  fprintf(filew,"$ns attach-agent $Victim $VictimAgent\n");
  fprintf(filew,"$ns connect $DDOSUDP $VictimAgent\n");
  fprintf(filew,"$DDOSUDP set class_ 2\n");


  fprintf(filew,"\n\n\n");

  DDOS_start_time*=1000;
  fprintf(filew,"$ns at %lf \"$DDOSTraffic set rate_ %lfMb \"\n",ceil(DDOS_start_time+topo.CBRTraffic.DDOSTraffic.Events[0].delay*bn_delay)/1000, \
	                                   topo.CBRTraffic.DDOSTraffic.Events[0].NetCBR);
  fprintf(filew,"$ns at %lf \"$DDOSTraffic set packet_size_ %d \"\n",\
	                                   ceil(DDOS_start_time+topo.CBRTraffic.DDOSTraffic.Events[0].delay*bn_delay)/1000,pack_size);
  fprintf(filew,"$ns at %lf \"$DDOSTraffic start\"\n",ceil(DDOS_start_time+topo.CBRTraffic.DDOSTraffic.Events[0].delay*bn_delay)/1000);

  for(i=1;i<topo.CBRTraffic.DDOSTraffic.NoEvents;i++){
    fprintf(filew,"$ns at %lf \"$DDOSTraffic set rate_ %lfMb\"\n",\
	                                   ceil(DDOS_start_time+topo.CBRTraffic.DDOSTraffic.Events[i].delay*bn_delay)/1000, 
	                                   topo.CBRTraffic.DDOSTraffic.Events[i].NetCBR);
  }

  fprintf(filew,"\n\n");

}


void TCL_Write_Final(FILE *filew){
  fprintf(filew,"\n\n");
  fprintf(filew,"set outfile [open qtrace.tr w]\n$ns trace-queue $BotLeft $BotRight $outfile\n\n\n");
  fprintf(filew,"$ns at %lf \"finish\"\n",finish_time);
  fprintf(filew, "$ns run\n");
}






