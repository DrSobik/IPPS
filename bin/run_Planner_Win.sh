echo "Hello!";

executable="Planner/Planner.exe";

# DLL search path
PATH=./DLL/Common:./DLL/Solvers/VNSPlanner:./DLL/Solvers/LocalSearchPM:./DLL/Solvers/PriorityScheduler:./DLL/Solvers/CombinedScheduler
echo "Current PATH: $PATH"
export PATH

# run time limits
globmaxiter=500;
globmaxtimem=300;

# randomizing
randseed=1325467;

# initialization rule on the planning level
planninginitrule=TPT_RANK; #TPT_RANK;

# neighborhood structure
ns=N1;

# speps withing the neighborhood structures
stepN1=2;
stepN2=2;
stepN3=2;

# number of neighbors to be generated at a time
numneigh=1;

# string for the averager over the runs
averagerstring="";

for ddFactor in Middle #Tight Middle Loose
do

for instSize in Middle #Small Middle Large
do

for testnum in 01 #02 03 04 05 06 07 08 09 10 #11 12 13 14 15 16 17 18 19 20
do

for objective in TWT@Common
do

  testpref="";
  #if [ $testnum -lt 10 ]; then testpref="0"; fi;

  #testdir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/Test_$testnum/";
  #testdir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/EURO_2013/Common/ipps_0$testnum/";
  #testdir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/EURO_2013/Common/From_Capek/RCPSP_like/ipps_$testnum/";
  #testdir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/EURO_2013/IPPS/Middle/Small/ipps_${testpref}$testnum/";
  #testdir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/EURO_2013/IPPS/Middle/Middle/ipps_${testpref}$testnum/";
  #testdir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/EURO_2013/IPPS/Loose/Large/ipps_${testpref}$testnum/";
  #testdir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Medium_MISTA_2013/Test_$testnum/";
  #curTestDir="/home/DrSobik/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/EURO_2013/IPPS/${ddFactor}/${instSize}/ipps_${testpref}$testnum/";
  curTestDir="e:/Projects/IPPS/Tests/Integer_Tests/Tests_Joint/EURO_2013/IPPS/${ddFactor}/${instSize}/ipps_${testpref}$testnum/";
  
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
    
  
  for globIter  in 100 #500 250
  do
  
  # the protocol file
  for numNeigh in 2 #1 #2 4
  do
    for planningInitRule in TPTRANK #RND TPTRANK
    do
      for ns in N2N1 #N2N3 #N1 N2
      do
	#for scheduler in CSLS2k #RND FIFO WFIFO EDD WEDD MDD WMDD ATC_K #RND FIFO WFIFO EDD WEDD MDD WMDD ATC #RND FIFO  #WFIFO EDD WEDD WMDD #ATC
	#do
	
	  for schedStrategyIdx in 6 #5 6 7 8 9 10 11 12 13 14 #1 2 3 4 5 6 7 8 9 10 11 12 13 14
	  do 
	  
	  mkdir "${curTestDir}ProtocolsModularTesting";
	
	  for run in 1 #2 3 4 5
	  do
	  
	    if [ $run == 1 ]; then randseed=1325467; echo $randseed; fi;
	    if [ $run == 2 ]; then randseed=12345; echo $randseed; fi;
	    if [ $run == 3 ]; then randseed=654321; echo $randseed; fi;
	    if [ $run == 4 ]; then randseed=872348; echo $randseed; fi;
	    if [ $run == 5 ]; then randseed=765198; echo $randseed; fi;
	  
	    # set the name of the protocol
	    protoFileName="ProtocolsModularTesting/proto_${objective}_GlobIter_${globIter}_NS_${ns}_NumNeigh_${numNeigh}_Init_${planningInitRule}_SchedStrategy_${schedStrategyIdx}_Run_${run}"
	    protoFile="${protoFileName}.xml";
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
	    
	    $executable "SchedStrategy:${schedStrategy[${schedStrategyIdx}]}" "Objective:${objective}" "Algorithm:VNS" "InitScheduler:${initSchedulerSetting}" "NS:${ns}" "StepN1:${stepN1}" "StepN2:${stepN2}" "StepN3:${stepN3}" "NumNeigh:${numNeigh}" "PlannerInitRule:${planningInitRule}" "RandSeed:${randseed}" "GlobMaxIter:${globIter}" "GlobMaxTimeM:${globmaxtimem}" "Test:${curTestDir}" "Protocol:${protoFile}";
	    echo "Finished the main program!";
	  
	  done # runs
	
	  done # strategy
	
	#done # scheduler
      done # ns	
    done # planningInitRule
  done # numNeigh
  done # globIter

done # objective
  
done # testnum

done # instSize

done # ddFactor

echo "Finished!";
