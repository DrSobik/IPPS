/* 
 * File:   IterativeAlg.h
 * Author: DrSobik
 *
 * Description :	Class IterativeAlg is a basis class for different iterative
 *					algorithms.
 * 
 * * Contained data:
 *				 
 *				_maxiter
 *					-	maximum number of iterations of the search algorithm.
 * 
 *				_curiter
 *					-	current iteration of the search algortim.
 * 
 *				_maxiterdeclined
 *					-	maximum number of sequential iterations with the of the
 *						declined solutions.
 *				
 *				_curiterdeclined
 *					-	number of sequential iterations with the declined 
 *						solutions.
 *					
 *				_maxtimems
 *					-	maximal time period in ms in which one more iteration of
 *						the search algorithm can still be started.
 * 
 *				_curtime
 *					-	current time from the start of the iterations.
 * 
 * Created on February 27, 2012, 5:09 PM
 */

#ifndef ITERATIVEALG_H
#define	ITERATIVEALG_H

#include <QObject>
#include <QTime>

#include "MathExt"

#include "DebugExt.h"

using namespace Common;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE IterativeAlg /*: public QObject*/ {
    //Q_OBJECT
protected:
    Math::intUNI _maxiter;
    Math::intUNI _curiter;

    Math::intUNI _maxiterdeclined;
    Math::intUNI _curiterdeclined;

    Math::intUNI _maxtimems;
    QTime _curtime;

public:
    IterativeAlg();
    IterativeAlg(IterativeAlg& orig);
    virtual ~IterativeAlg();

    /** Set the maximum number of iterations of the search algorithm. */
    inline void maxIter(const Math::intUNI miter) {
        _maxiter = miter;
    }

    inline Math::intUNI maxIter() {
        return _maxiter;
    }

    /** Set the maximum number of sequential operations on which the solutions
     *	are declined. */
    inline void maxIterDecl(const Math::intUNI miterdecl) {
        _maxiterdeclined = miterdecl;
    }

    inline Math::intUNI maxIterDecl() {
        return _maxiterdeclined;
    }

    inline void maxTimeMs(const Math::intUNI mtimems) {
        _maxtimems = mtimems;
    }

    inline Math::intUNI maxTimeMs() {
        return _maxtimems;
    }

    /** Number of iterations elapsed since the start of the algorithm. */
    inline Math::intUNI iter() {
        return _curiter;
    }

	/** Number of milliseconds elapsed since the start of the algorithm. */
	inline Math::intUNI timeMs(){
		return _curtime.elapsed();
	}
	
    /** Number of sequential iterations on which the newly generated solutions
     *	have been declined. */
    inline Math::intUNI iterDecl() {
        return _curiterdeclined;
    }

    /** Initialize the planner. */
    virtual void init();

    /** One step of the algorithm. */
    virtual void step() {
        // Perform one step of the algorithm
        stepActions();

        // Update the iteration counter
        _curiter++;
    }

    /** Actions performed on each step of the algorithm. */
    virtual void stepActions() {
        Debugger::eDebug("IterativeAlg::step not implemented!");
    }

    /** Assess the results of the algorithms' step.*/
    inline void assess() {
        // Perform assessment actions
        assessActions();
    }

    /** Actions to be performed for the assessment of the newly 
     *  generated plan. */
    virtual void assessActions() {
        Debugger::eDebug("IterativeAlg::assessActions not implemented!");
    }

    /** Decide whether the newly generated plan will be accepted or declined. */
    virtual void acceptOrDecline() {
        if (acceptCondition()) {
            // Set the counter of the sequentially declined solutions to 0
            _curiterdeclined = 0;

            // Perform accept actions
            acceptActions();
        } else {
            // Perform decline actions
            declineActions();

            // Increase the counter of the sequentially declined solutions
            _curiterdeclined++;
        }
    }

    /** Condition of acceptance of the newly generated solution. */
    virtual bool acceptCondition() {
        Debugger::eDebug("IterativeAlg::acceptActions not implemented!");
        return false;
    }

    /** Actions to be preformed in case the newly generated step result accepted. */
    virtual void acceptActions() {
        Debugger::eDebug("IterativeAlg::acceptActions not implemented!");
    }

    /** Actions to be performed in case the step result is declined. */
    virtual void declineActions() {
        Debugger::eDebug("IterativeAlg::declineActions not implemented!");
    }

    /** Check whether the stopping condition for this algorithm is satisfied. */
    inline bool stop() {
        // Check the stopping condition
        if (stopCondition()) {

            // Perform actions associated with the stopping condition
            stopActions();

            return true;
        } else {
            return false;
        }

        return false;
    }

    /** The logical stopping condition of the algorithm. */
    virtual bool stopCondition() {
        //Debugger::warn << _curtime.elapsed() << " " << _maxtimems << ENDL;
        //Debugger::warn << _curiter << " " << _maxiter << ENDL;
        return _curiterdeclined >= _maxiterdeclined || _curiter >= _maxiter || _curtime.elapsed() > _maxtimems;
    }

    virtual void stopActions() {
        Debugger::wDebug("IterativeAlg::stopActions not implemented!");
    }

    /** Run the algorithm. */
    virtual void run();

    /** Call the preprocessing routine. */
    inline void preprocess() {
        // Preprocess 
        preprocessingActions();
    }

    /** Call the postprocessing routine. */
    inline void postprocess() {
        // Postprocess
        postprocessingActions();
    }

    /** Preprocessing actions for the search algorithm. */
    virtual void preprocessingActions() {
        Debugger::wDebug("IterativeAlg::preprocessingActions not implemented!");
    }

    /** Postprocessing actions for the search algorithm. */
    virtual void postprocessingActions() {
        Debugger::wDebug("IterativeAlg::postprocessingActions not implemented!");
    }
};

/** Event-driven iterative algorithm */
class EventIterativeAlg : public QObject, public IterativeAlg {
    Q_OBJECT

public:
    EventIterativeAlg();
    virtual ~EventIterativeAlg();

    /** Run the algorithm. */
    virtual void run();

signals:
    void sigInit();

    void sigInitFinished();

    /** Is emitted when a step of the algorithm should be made. */
    void sigStep();

    /** Should be emitted as soon as the step actions are finished. */
    void sigStepFinished();

    /** Is emitted when the assessment should be made. */
    void sigAssess();

    /** Should be emitted as soon as the assess actions are finished. */
    void sigAssessFinished();

    void sigAcceptOrDecline();

    void sigAcceptCondition();

    void sigAcceptConditionFinished(const bool&);

    void sigAccept();

    void sigAcceptFinished();

    void sigDecline();

    void sigDeclineFinished();

    /** Should be emitted as soon as the acceptance decision is made. */
    void sigAcceptOrDeclineFinished();

    void sigFinished();

	/** Notify that the complete iteration of the algorithm has finished (including assessment). */
	void sigCompleteStepFinished();
	
public slots:

    void slotInit();

    void slotInitFinished();

    void slotStep();

    void slotAssess();

    void slotAcceptOrDecline();

    void slotAcceptConditionFinished(const bool&);

    void slotAccept();

    void slotDecline();

    void slotAcceptFinished();

    void slotDeclineFinished();

    /** Must be called as soon as one step of an algorithm is finished. */
    void slotStepFinished();

    /** Called after each step in order to assess it. */
    void slotAssessFinished();

    /** Called after each step in order to assess it. */
    void slotAcceptOrDeclineFinished();

    virtual void slotFinished();

};

#endif	/* ITERATIVEALG_H */

