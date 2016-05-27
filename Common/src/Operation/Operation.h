/* 
 * File:   Operation.h
 * Author: DrSobik
 *
 * Created on July 1, 2011, 10:05 AM
 * 
 * Description: Class Operation represents operations to be executed.
 * 
 * Contained data:
 *
 * 				ID	-	unique ID of the operation in the execution system.
 *						Each operation has its unique ID.
 *
 * 				OID	-	ID of the order, which contains the operation.
 *						(Can be used for execution status feedback to the order
 *						agent).
 * 
 *				type-	type of the operation. It defines, which operation
 *						exactly must be executed and by which tool group
 * 
 *				toolID-	points to the tool group, which the operation must be 
 *						scheduled on (ID of the tool group).
 * 
 *				machID-	points to the machine in the production system, on which
 *						the operation must be executed (ID of the machine).
 * 
 *				_ir -	initial ready time of the operation. Convenient while
 *						changing the ready times of the operations within 
 *						different scheduling methods.
 * 
 *				_r	-	ready time of the operation.
 * 
 *				_d	-	due date of the operation.
 * 
 *				_s	-	start time of the operation.
 * 
 *				_pe	-	processing time of the operation on the etalon machine. 
 *						It depends only on the job. The processing time of the 
 *						operation is proportional to p and inverse proportional 
 *						to the speed of the machine on which this operation is 
 *						processed.
 * 
 *				_p	-	Actual processing time of the operation. Depends on the
 *						operation and the processing machine.
 * 
 *				_w	-	weight or priority of the operation.
 * 
 *				_c	-	completion time of the operation.
 * 
 *				scheduled -
 *						indicates whether the operations is considered to be
 *						scheduled. 
 */


#ifndef OPERATION_H
#define	OPERATION_H

#include <QTextStream>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "DebugExt.h"
#include "MathExt"

using namespace Common;

#if defined DLL_EXPORT
#define DLL_EXPORT_INTEFACE Q_DECL_EXPORT
#else
#define DLL_EXPORT_INTEFACE Q_DECL_IMPORT
#endif

class DLL_EXPORT_INTEFACE Operation {
public:
	int ID;
	int OID;
	int OT; // Type of the order which this operation belongs to
	int BID; // ID of the BOM according to which the operation is added
	int IID; // ID of the item containing the operation
	int IT; // Type of the item which this operation belongs to
	int RID; // ID of the routes which this operation belongs to
	int SI; // Step index corresponding to this operation in the route

	int type;

	int toolID;
	int machID;

	double _ir;
	double _r;
	double _mA; // Availability time of the machine which the operation is assigned to. "0.0" if the operation is not assigned
	double _d;
	double _s;
	double _pe;
	double _p;
	double _w;
	double _c;

	static int operCreated;
	static int operDeleted;
	
public:
	Operation();
	Operation(const double &etalon_proc_time);
	Operation(const Operation& orig);
	Operation(Operation& orig);
	virtual ~Operation();

	/* -----------------------  Utils  -------------------------------------- */

	/** Initializes this operation. */
	virtual void init();

	/** Assigns all of the data of the "other" operation to this one. */
	virtual void copy(const Operation& other);

	/** Copy operator. */
	const Operation& operator=(const Operation& other);

	/** Friend operator that checks whether the operations are the same. 
	 * Comparison is based on the unique IDs of the operations. */
	friend bool operator==(const Operation& o1, const Operation& o2) {
		return o1.ID == o2.ID;
	}

	inline void orderID(const int& ordID) {
		OID = ordID;
	}

	inline int orderID() {
		return OID;
	}

	inline void orderType(const int& ordType) {
		OT = ordType;
	}

	inline int orderType() {
		return OT;
	}

	inline void bomID(const int& bID) {
		BID = bID;
	}

	inline int bomID() {
		return BID;
	}

	inline void itemID(const int& itmID) {
		IID = itmID;
	}

	inline int itemID() {
		return IID;
	}

	inline void itemType(const int& itmType) {
		IT = itmType;
	}

	inline int itemType() {
		return IT;
	}

	inline void routeID(const int& rtID) {
		RID = rtID;
	}

	inline int routeID() {
		return RID;
	}

	inline void stepIdx(const int& idx) {
		SI = idx;
	}

	inline int stepIdx() {
		return SI;
	}

	/** Write information about the operation. */
	virtual void write(QTextStream &out);

    DLL_EXPORT_INTEFACE friend QTextStream& operator<<(QTextStream &out, Operation o) {
		o.write(out);
		return out;
	}

    DLL_EXPORT_INTEFACE friend QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Operation& o);
    DLL_EXPORT_INTEFACE friend QXmlStreamReader& operator>>(QXmlStreamReader& reader, Operation& o);


	/* ---------------------------------------------------------------------- */

	/* ------------------------  Scheduling relevant  ----------------------- */

	/** Set the machine availability time for the operation. */
	inline void machAvailTime(const double& mA, const bool& update = true) {
		if (update) {
			// Check whether the current start time of the machine correctly in view of the machine's availability time
			_mA = mA;
			s(Math::max(s(), mA));
		} else {
			// Just set the machine's availability time
			_mA = mA;
		}
	}

	/** Returns the machine's availability time for this operation. */
	inline double machAvailTime(){
		return _mA;
	}
	
	/** Set the start time for the operations' processing. Will update the 
	 *  completion time by adding the processing time. 
	 */
	inline void s(const double &start_time, const bool &update = true) {
		if (update) {
			_s = start_time;
			_c = _s + _p;
		} else {
			_s = start_time;
		}
	}

	/** Start time of the operation. */
	inline double s() const {
		return _s;
	}

	/** Etalon processing time of the operation. */
	inline double pe() const {
		return _pe;
	}

	/** Set the actual processing time of the operation. Will update the 
	 * completion time by adding the start time.*/
	inline void p(const double &proc_time, const bool &update = true) {
		if (update) {
			_p = proc_time;
			_c = _s + proc_time;
		} else {
			_p = proc_time;
		}
	}

	/** Processing time of the operation. */
	inline double p() const {
		return _p;
	}

	/** Set start and processing time of the operation. Will update the
	 *  completion time. */
	inline void sp(const double &start_time, const double proc_time, const bool &update = true) {
		if (update) {
			_p = proc_time;
			_s = start_time;
			_c = _s + proc_time;
		} else {
			_p = proc_time;
			_s = start_time;
		}
	}

	/** Completion time of the operation. */
	inline double c() const {
		return _c;
	}

	/** Set the initial ready time of the operation. */
	inline void ir(const double &init_ready_time) {
		_ir = init_ready_time;
		if (_ir > r()) {
			r(_ir);
		}
	}

	/** Initial ready time of the operation. */
	inline double ir() const {
		return _ir;
	}

	/** Set the ready time of the operation. */
	inline void r(const double &ready_time, const bool &check_update = true) {
		if (check_update) {
			_r = Math::max(ready_time, ir());
			s(Math::max(_s, _r));
		} else {
			_r = ready_time;
		}
	}

	/** Ready time of the operation. */
	inline double r() const {
		return _r;
	}

	/** Set the due date of the operation. */
	inline void d(const double &due_date) {
		_d = due_date;
	}

	/** Due date of the operation. */
	inline double d() const {
		return _d;
	}

	/** Set the weight of the operation. */
	inline void w(const double &weight) {
		_w = weight;
	}

	/** Weight of the operation. */
	inline double w() const {
		return _w;
	}

	/** Weighted tardiness of the operation. Based on the completion time and
	 *  of the weight of the operation. */
	inline double wT() const {
		return _w * Math::max(0.0, _c - _d);
	}

	/* ---------------------------------------------------------------------- */

private:

};

#endif	/* OPERATION_H */

