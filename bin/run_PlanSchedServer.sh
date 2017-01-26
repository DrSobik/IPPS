echo "Hello!";

# detect platform
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  
  curPlatform="linux";
  
elif [[ "$OSTYPE" == "linux" ]]; then
  
  curPlatform="linux";
  
elif [[ "$OSTYPE" == "darwin"* ]]; then
  
  # Mac OSX
  curPlatform="darwin";
  
elif [[ "$OSTYPE" == "cygwin" ]]; then
  
  # POSIX compatibility layer and Linux environment emulation for Windows
  curPlatform="cygwin";
  
elif [[ "$OSTYPE" == "msys" ]]; then
  
  # Lightweight shell and GNU utilities compiled for Windows (part of MinGW)
  curPlatform="windows";
  
elif [[ "$OSTYPE" == "win32" ]]; then
  
  # Not sure this can happen.
  curPlatform="windows";

elif [[ "$OSTYPE" == "windows" ]]; then
  
  # Not sure this can happen.
  curPlatform="windows";  
  
elif [[ "$OSTYPE" == "freebsd"* ]]; then

  curPlatform="freebsd";  
  
else
  # Unknown.
  curPlatform="UNKNOWN";
fi

# the right executable
if [[ "$curPlatform" == "linux" ]]; then
  
  executable="PlanSchedServer/PlanSchedServer";
  
elif [[ "$curPlatform" == "windows" ]]; then
  
  executable="PlanSchedServer/PlanSchedServer.exe";

fi

# the right DLL search path
if [[ "$curPlatform" == "linux" ]]; then
  
  LD_LIBRARY_PATH=./DLL/Common:./DLL/Solvers/VNSPlanner:./DLL/Solvers/LocalSearchPM:./DLL/Solvers/PriorityScheduler:./DLL/Solvers/CombinedScheduler
  echo "Current LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
  export LD_LIBRARY_PATH
  
elif [[ "$curPlatform" == "windows" ]]; then
  
  PATH=./DLL/Common:./DLL/Solvers/VNSPlanner:./DLL/Solvers/LocalSearchPM:./DLL/Solvers/PriorityScheduler:./DLL/Solvers/CombinedScheduler
  echo "Current PATH: $PATH"
  export PATH

fi

# run time limits
globIter=10;
globMaxTimeM=60;

# randomizing
randseed=1325467;

# initialization rule on the planning level
planningInitRule=SOL_INIT_RANK; #TPTRANK; #TPT_RANK;

# neighborhood structure
ns=NS_N2N3; #N2N1

# speps withing the neighborhood structures
stepN1=2;
stepN2=2;
stepN3=2;

# number of neighbors to be generated at a time
numNeigh=2;

# scheduling strategy to be used by the solver algorithm
schedStrategyIdx=6; 

# objective
objective=TWT@Common;


# only SDR
schedStrategy[1]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6)}";
  
# CS = SDR + ATC
#schedStrategy[2]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7)}";
SS2RNDSchedulerStr="RNDScheduler(RNDScheduler_ID=1;RNDScheduler_PRIMARY_OBJECTIVE=TWT@Common)@PriorityScheduler";
SS2FIFOSchedulerSrt="WFIFOScheduler(WFIFOScheduler_ID=2;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=false)@PriorityScheduler";
SS2WFIFOSchedulerSrt="WFIFOScheduler(WFIFOScheduler_ID=3;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=true)@PriorityScheduler";
SS2EODSchedulerStr="WEODScheduler(WEODScheduler_ID=4;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=false)@PriorityScheduler";
SS2WEODSchedulerStr="WEODScheduler(WEODScheduler_ID=5;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=true)@PriorityScheduler";
SS2MODSchedulerStr="WMODScheduler(WMODScheduler_ID=6;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=false)@PriorityScheduler";
SS2WMODSchedulerStr="WMODScheduler(WMODScheduler_ID=7;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=true)@PriorityScheduler";
SS2ATCFFFIFOSrt="WFIFOScheduler(WFIFOScheduler_ID=1;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=false)@PriorityScheduler";
SS2ATCFFWFIFOSrt="WFIFOScheduler(WFIFOScheduler_ID=2;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=true)@PriorityScheduler";
SS2ATCFFEODStr="WEODScheduler(WEODScheduler_ID=3;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=false)@PriorityScheduler";
SS2ATCFFWEODStr="WEODScheduler(WEODScheduler_ID=4;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=true)@PriorityScheduler";
SS2ATCFFMODStr="WMODScheduler(WMODScheduler_ID=5;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=false)@PriorityScheduler";
SS2ATCFFWMODStr="WMODScheduler(WMODScheduler_ID=6;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=true)@PriorityScheduler";
SS2ATCFFMDDStr="WMDDScheduler(WMDDScheduler_ID=7;WMDDScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMDDScheduler_WEIGHTED=false)@PriorityScheduler";
SS2ATCFFWMDDStr="WMDDScheduler(WMDDScheduler_ID=8;WMDDScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMDDScheduler_WEIGHTED=true)@PriorityScheduler";
SS2ATCFFEstimatorStr="CombinedScheduler(CS_PRIMARY_OBJECTIVE=TWT@Common;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7&8;CS_SCHEDULERS=[${SS2ATCFFFIFOSrt};${SS2ATCFFWFIFOSrt};${SS2ATCFFEODStr};${SS2ATCFFWEODStr};${SS2ATCFFMODStr};${SS2ATCFFWMODStr};${SS2ATCFFMDDStr};${SS2ATCFFWMDDStr}])@CombinedScheduler";
SS2ATCSchedulerStr="ATCANScheduler(ATCANScheduler_ID=8;ATCANScheduler_PRIMARY_OBJECTIVE=TWT@Common;ATCANScheduler_KAPPA=2.0;ATCANScheduler_KAPPA_OPT=true;ATCANScheduler_CONSIDER_SUCC=false;ATCANScheduler_FF_ESTIMATOR=${SS2ATCFFEstimatorStr})@PriorityScheduler";
SS2CS="CombinedScheduler(CS_PRIMARY_OBJECTIVE=TWT@Common;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7&8;CS_SCHEDULERS=[${SS2RNDSchedulerStr};${SS2FIFOSchedulerSrt};${SS2WFIFOSchedulerSrt};${SS2EODSchedulerStr};${SS2WEODSchedulerStr};${SS2MODSchedulerStr};${SS2WMODSchedulerStr};${SS2ATCSchedulerStr}])@CombinedScheduler";
schedStrategy[2]="PLANNER{[0%-100%):(VNSPLANNER_BEST_PLAN=false),[100%-100%]:(VNSPLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:${SS2CS}}";
#schedStrategy[2]="PLANNER{[0%-100%):(VNSPLANNER_BEST_PLAN=false),[100%-100%]:(VNSPLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:${SS2FIFOSchedulerSrt}}";
 
# CS+LS(20)
schedStrategy[3]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20)}";
 
# CS+LS(200)
schedStrategy[4]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=200)}";
  
# CS+LS(2000)
schedStrategy[5]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=2000)}";
 
# CS+LS(20000)
#schedStrategy[6]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
SS6RNDSchedulerStr="RNDScheduler(RNDScheduler_ID=1;RNDScheduler_PRIMARY_OBJECTIVE=TWT@Common)@PriorityScheduler";
SS6FIFOSchedulerSrt="WFIFOScheduler(WFIFOScheduler_ID=2;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=false)@PriorityScheduler";
SS6WFIFOSchedulerSrt="WFIFOScheduler(WFIFOScheduler_ID=3;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=true)@PriorityScheduler";
SS6EODSchedulerStr="WEODScheduler(WEODScheduler_ID=4;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=false)@PriorityScheduler";
SS6WEODSchedulerStr="WEODScheduler(WEODScheduler_ID=5;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=true)@PriorityScheduler";
SS6MODSchedulerStr="WMODScheduler(WMODScheduler_ID=6;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=false)@PriorityScheduler";
SS6WMODSchedulerStr="WMODScheduler(WMODScheduler_ID=7;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=true)@PriorityScheduler";
SS6ATCFFFIFOSrt="WFIFOScheduler(WFIFOScheduler_ID=1;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=false)@PriorityScheduler";
SS6ATCFFWFIFOSrt="WFIFOScheduler(WFIFOScheduler_ID=2;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=true)@PriorityScheduler";
SS6ATCFFEODStr="WEODScheduler(WEODScheduler_ID=3;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=false)@PriorityScheduler";
SS6ATCFFWEODStr="WEODScheduler(WEODScheduler_ID=4;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=true)@PriorityScheduler";
SS6ATCFFMODStr="WMODScheduler(WMODScheduler_ID=5;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=false)@PriorityScheduler";
SS6ATCFFWMODStr="WMODScheduler(WMODScheduler_ID=6;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=true)@PriorityScheduler";
SS6ATCFFMDDStr="WMDDScheduler(WMDDScheduler_ID=7;WMDDScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMDDScheduler_WEIGHTED=false)@PriorityScheduler";
SS6ATCFFWMDDStr="WMDDScheduler(WMDDScheduler_ID=8;WMDDScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMDDScheduler_WEIGHTED=true)@PriorityScheduler";
SS6ATCFFEstimatorStr="CombinedScheduler(CS_PRIMARY_OBJECTIVE=TWT@Common;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7&8;CS_SCHEDULERS=[${SS6ATCFFFIFOSrt};${SS6ATCFFWFIFOSrt};${SS6ATCFFEODStr};${SS6ATCFFWEODStr};${SS6ATCFFMODStr};${SS6ATCFFWMODStr};${SS6ATCFFMDDStr};${SS6ATCFFWMDDStr}])@CombinedScheduler";
SS6ATCSchedulerStr="ATCANScheduler(ATCANScheduler_ID=8;ATCANScheduler_PRIMARY_OBJECTIVE=TWT@Common;ATCANScheduler_KAPPA=2.0;ATCANScheduler_KAPPA_OPT=true;ATCANScheduler_CONSIDER_SUCC=false;ATCANScheduler_FF_ESTIMATOR=${SS6ATCFFEstimatorStr})@PriorityScheduler";
SS6LSInitScheduler="CombinedScheduler(CS_PRIMARY_OBJECTIVE=TWT@Common;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7&8;CS_SCHEDULERS=[${SS6RNDSchedulerStr};${SS6FIFOSchedulerSrt};${SS6WFIFOSchedulerSrt};${SS6EODSchedulerStr};${SS6WEODSchedulerStr};${SS6MODSchedulerStr};${SS6WMODSchedulerStr};${SS6ATCSchedulerStr}])@CombinedScheduler";
#SS6LSInitScheduler="${SS6RNDSchedulerStr}";
SS6LS="LocalSearchPM(LS_PRIMARY_OBJECTIVE=TWT@Common;LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;LS_MAX_ITER=20000;LS_BEST_POS_TO_MOVE=false;LS_INIT_SCHEDULER=${SS6LSInitScheduler})@LocalSearchPM";
schedStrategy[6]="PLANNER{[0%-100%):(VNSPLANNER_BEST_PLAN=false),[100%-100%]:(VNSPLANNER_BEST_PLAN=true)},SCHEDULER{[0%-100%]:${SS6LS}}";
  
# [0%-50%):SDR, [50%-100%]:CS
schedStrategy[7]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-50%):CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6),[50%-100%]:CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7)}";

# [0%-50%):SDR, [50%-80%):CS, [80%-100%]:CS+LS(20000)
schedStrategy[8]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-50%):CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6),[50%-80%):CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7),[80%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
  
# [0%-50%):SDR, [50%-80%):CS+LS(20), [80%-100%]:CS+LS(20000)
schedStrategy[9]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-50%):CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6),[50%-80%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20),[80%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
  
# [0%-80%):CS,  [80%-95%):CS+LS(20), [95%-100%]:CS+LS(20000)
schedStrategy[10]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-80%):CS,[80%-95%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20),[95%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
  
# [0%-90%):SDR, [90%-100%):CS+LS(20), [100%-100%]:CS+LS(20000)
schedStrategy[11]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-90%):CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6),[90%-100%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20),[100%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
  
# [0%-90%):CS,  [90%-100%):CS+LS(20), [100%-100%]:CS+LS(20000)
schedStrategy[12]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-90%):CS(CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7),[90%-100%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20),[100%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
  
# [0%-90%):SDR+LS(20), [90%-100%):CS+LS(2000), [100%-100%]:CS+LS(20000)
schedStrategy[13]="PLANNER{[0%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-90%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6;LS_MAX_ITER=20),[90%-100%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=2000),[100%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
  
# [0%-90%):SDR+LS(20), [90%-100%):CS+LS(2000), [100%-100%]:CS+LS(20000) // Planner options variate
schedStrategy[14]="PLANNER{[0%-50%):(PLANNER_BEST_PLAN=false),[50%-55%):(PLANNER_BEST_PLAN=true),[55%-80%):(PLANNER_BEST_PLAN=false),[80%-85%):(PLANNER_BEST_PLAN=true),[85%-100%):(PLANNER_BEST_PLAN=false),[100%-100%]:(PLANNER_BEST_PLAN=true)},SCHEDULER{[0%-90%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6;LS_MAX_ITER=20),[90%-100%):CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=2000),[100%-100%]:CSLS(LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=false;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7;LS_MAX_ITER=20000)}";
        
# set the name of the protocol
# the right DLL search path
if [[ "$curPlatform" == "linux" ]]; then
  
  protoFileDir="PlanSchedServer/";
  
elif [[ "$curPlatform" == "windows" ]]; then
  
  protoFileDir="PlanSchedServer/"; #"c:/Users/sobeyko/Projects/IPPS/CPPALG/bin/PlanSchedServer";

fi
protoFileName="Protocols/proto_${objective/\@/AT}_GlobIter_${globIter}_NS_${ns}_NumNeigh_${numNeigh}_Init_${planningInitRule}_SchedStrategy_${schedStrategyIdx}_Run_ALL"
#protoFile="${protoFileName}.xml";
protoFile="${protoFileDir}${protoFileName}.xml";
echo $protoFile;

# run the prograFm
echo "Current scheduling strategy: ${schedStrategy[${schedStrategyIdx}]}";
   
# Initial scheduler
RNDSchedulerStr="RNDScheduler(RNDScheduler_ID=1;RNDScheduler_PRIMARY_OBJECTIVE=TWT@Common)@PriorityScheduler";
FIFOSchedulerStr="WFIFOScheduler(WFIFOScheduler_ID=2;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=false)@PriorityScheduler";
WFIFOSchedulerStr="WFIFOScheduler(WFIFOScheduler_ID=3;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=true)@PriorityScheduler";
EODSchedulerStr="WEODScheduler(WEODScheduler_ID=4;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=false)@PriorityScheduler";
WEODSchedulerStr="WEODScheduler(WEODScheduler_ID=5;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=true)@PriorityScheduler";
MODSchedulerStr="WMODScheduler(WMODScheduler_ID=6;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=false)@PriorityScheduler";
WMODSchedulerStr="WMODScheduler(WMODScheduler_ID=7;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=true)@PriorityScheduler";
    
## FF estimators for ATC
ATCFFFIFOSrt="WFIFOScheduler(WFIFOScheduler_ID=1;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=false)@PriorityScheduler";
ATCFFWFIFOSrt="WFIFOScheduler(WFIFOScheduler_ID=2;WFIFOScheduler_PRIMARY_OBJECTIVE=TWT@Common;WFIFOScheduler_WEIGHTED=true)@PriorityScheduler";
ATCFFEODStr="WEODScheduler(WEODScheduler_ID=3;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=false)@PriorityScheduler";
ATCFFWEODStr="WEODScheduler(WEODScheduler_ID=4;WEODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WEODScheduler_WEIGHTED=true)@PriorityScheduler";
ATCFFMODStr="WMODScheduler(WMODScheduler_ID=5;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=false)@PriorityScheduler";
ATCFFWMODStr="WMODScheduler(WMODScheduler_ID=6;WMODScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMODScheduler_WEIGHTED=true)@PriorityScheduler";
ATCFFMDDStr="WMDDScheduler(WMDDScheduler_ID=7;WMDDScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMDDScheduler_WEIGHTED=false)@PriorityScheduler";
ATCFFWMDDStr="WMDDScheduler(WMDDScheduler_ID=8;WMDDScheduler_PRIMARY_OBJECTIVE=TWT@Common;WMDDScheduler_WEIGHTED=true)@PriorityScheduler";
ATCFFEstimatorStr="CombinedScheduler(CS_PRIMARY_OBJECTIVE=TWT@Common;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7&8;CS_SCHEDULERS=[${ATCFFFIFOSrt};${ATCFFWFIFOSrt};${ATCFFEODStr};${ATCFFWEODStr};${ATCFFMODStr};${ATCFFWMODStr};${ATCFFMDDStr};${ATCFFWMDDStr}])@CombinedScheduler";
##ATC
ATCSchedulerStr="ATCANScheduler(ATCANScheduler_ID=8;ATCANScheduler_PRIMARY_OBJECTIVE=TWT@Common;ATCANScheduler_KAPPA=2.0;ATCANScheduler_KAPPA_OPT=true;ATCANScheduler_CONSIDER_SUCC=false;ATCANScheduler_FF_ESTIMATOR=${ATCFFEstimatorStr})@PriorityScheduler";
  
#CS
#initSchedulerSetting="CombinedScheduler(CS_PRIMARY_OBJECTIVE=TWT@Common;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7&8;CS_SCHEDULERS=[${RNDSchedulerStr};${FIFOSchedulerStr};${WFIFOSchedulerStr};${EODSchedulerStr};${WEODSchedulerStr};${MODSchedulerStr};${WMODSchedulerStr};${ATCSchedulerStr}])@CombinedScheduler";
 
#CSLS
LSInitSchedulerStr="CombinedScheduler(CS_PRIMARY_OBJECTIVE=TWT@Common;CS_ALLOWED_SCHEDULERS=1&2&3&4&5&6&7&8;CS_SCHEDULERS=[${RNDSchedulerStr};${FIFOSchedulerStr};${WFIFOSchedulerStr};${EODSchedulerStr};${WEODSchedulerStr};${MODSchedulerStr};${WMODSchedulerStr};${ATCSchedulerStr}])@CombinedScheduler";
#LSInitSchedulerStr="${RNDSchedulerStr}";
initSchedulerSetting="LocalSearchPM(LS_PRIMARY_OBJECTIVE=TWT@Common;LS_CRIT_NODES_UPDATE_FREQ=100;LS_CHK_COR=true;LS_MAX_ITER=20000;LS_BEST_POS_TO_MOVE=false;LS_INIT_SCHEDULER=${LSInitSchedulerStr})@LocalSearchPM";

# algorithm for IPPS
planSchedSolver="VNSPlanner(VNSPLANNER_PROTOCOLFILE=${protoFile};VNSPLANNER_SOLINITRULE=${planningInitRule};VNSPLANNER_NS=${ns};VNSPLANNER_STEPN1=${stepN1};VNSPLANNER_STEPN2=${stepN2};VNSPLANNER_STEPN3=${stepN3};VNSPLANNER_NUMNEIGH=${numNeigh};VNSPLANNER_GLOBMAXITER=${globIter};VNSPLANNER_GLOBMAXTIMEMINUTES=${globMaxTimeM};VNSPLANNER_SCHEDSTRATEGY=${schedStrategy[${schedStrategyIdx}]};VNSPLANNER_INITSCHEDULER=${initSchedulerSetting};VNSPLANNER_PRIMARY_OBJECTIVE=${objective})@VNSPlanner";

$executable "PlanSchedSolver:${planSchedSolver}" "RandSeed:${randseed}" "Host:localhost" "Port:5555" ;
#$executable "PlanSchedSolver:${planSchedSolver}" "RandSeed:${randseed}" "Host:192.168.1.100" "Port:5555" ;
    
echo "Finished!";
