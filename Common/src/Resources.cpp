/* 
 * File:   Machines.cpp
 * Author: DrSobik
 * 
 * Created on July 1, 2011, 11:08 AM
 */

#include <stdlib.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qvariant.h>

#include "Resources.h"

/* #########################  Machine  ###################################### */

Machine::Machine() {
	ID = -1;

	timeAvailable(0.0);

	init();
}

Machine::Machine(Machine& orig) {
	copy(orig);
}

Machine::~Machine() {
	//QTextStream out(stdout);

	//out << "Machine::~Machine : Deleting machine " << ID << endl;

	clear();
}

void Machine::init() {
	//ID = -1;

	//type2speed.clear();

	//t = 0.0;

	t = timeAvail;

	topt = 0.0;

	operations.clear();
}

void Machine::clear() {
	//QTextStream out(stdout);

	//out << "Machine::clear : Clearing machine " << ID << endl;

	timeAvail = 0.0;

	t = 0.0;

	topt = 0.0;

	operations.clear();
}

void Machine::copy(Machine& other) {
	ID = other.ID;

	topt = other.topt;

	type2speed = other.type2speed;

	t = other.t;

	timeAvail = other.timeAvail;

	// IMPORTANT!!! Only the pointers but not real operations are copied!!!
	operations = other.operations;
	// Clear the operations
	//for (int i = 0; i < operations.size(); i++) {
	//delete operations[i];
	//}

	// Copy the operations
	//operations.clear();
	//for (int i = 0; i < other.operations.size(); i++) {
	//operations.append(new Operation(*(other.operations[i])));
	//}
}

const Machine& Machine::operator=(Machine& other) {
	copy(other);
	return *this;
}

void Machine::fromDOMElement(const QDomElement &melem) {
	QDomNodeList tts_list = melem.elementsByTagName("type2speed");
	QStringList ts_str;

	this->init();
	this->type2speed.clear();

	ID = melem.attribute("id").toInt();

	for (int i = 0; i < tts_list.size(); i++) {
		ts_str = tts_list.item(i).toElement().text().split(",");
		type2speed[ts_str[0].toInt()] = ts_str[1].toDouble();
	}

}

QTextStream& operator<<(QTextStream &out, Machine &m) {
	out << "Mac: ["
			<< "<"
			<< "ID=" << m.ID << ","
			<< "Ty=";

	for (QHash<int, double>::iterator hit = m.type2speed.begin(); hit != m.type2speed.end(); hit++) {
		out << hit.key() << ",";
	}

	out << "s=";
	for (QHash<int, double>::iterator hit = m.type2speed.begin(); hit != m.type2speed.end(); hit++) {
		out << hit.value() << ",";
	}

	out << "A=" << m.timeAvailable() << ",";
	out << "t=" << m.t << ">" << endl << endl;

	// Now output the operation assigned to this machine

	for (QList<Operation*>::iterator opit = m.operations.begin(); opit != m.operations.end(); opit++) {
		(*opit)->write(out);
		out << endl;
	}

	out << endl << "]";

	return out;
}

QXmlStreamReader& operator>>(QXmlStreamReader& reader, Machine& m) {

	// IMPORTANT!!! It is assumed that a token "machine" has just been read

	m.ID = reader.attributes().value("id").toString().toInt(); // ID of the machine

	double timeAvail = reader.attributes().value("availAt").toString().toDouble(); // Time when the machine becomes available
	m.timeAvailable(timeAvail);

	while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "machine")) { // Parse the processing characteristics

		reader.readNext();

		if (reader.name() == "type2speed") { // type2speed

			QString elText = reader.readElementText(); // the type2speed string

			if (elText != "") {

				QStringList sl = elText.split(",");

				m.type2speed[sl[0].toInt()] = sl[1].toDouble();

			}

		}

	}

	return reader;
}

QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Machine& m) {

	composer.writeStartElement("machine");
	composer.writeAttribute("id", QString::number(m.ID));
	composer.writeAttribute("availAt", QString::number(m.time()));

	for (QHash<int, double>::iterator iter = m.type2speed.begin(); iter != m.type2speed.end(); iter++) {
		int curOperType = iter.key();
		double curOperSpeed = iter.value();

		composer.writeTextElement("type2speed", QString::number(curOperType) + "," + QString::number(curOperSpeed));
	}

	composer.writeEndElement(); // machine

	return composer;
}

Machine& Machine::operator<<(Operation *o) {
	// Ignore the fictive operations of type 0
	if (o->type == 0) return *this;

#ifdef DEBUG
	if (!type2speed.contains(o->type)) {
		Debugger::eDebug("Trying to schedule operation on a wrong machine!");
	};

	/*
	for (int i = 0; i < operations.size(); i++) {
		if (operations[i]->type == 0) {
			Debugger::eDebug("Operation of type 0 is scheduled!");
		}
	}
	 */
#endif

	// The processing time of the operation on this machine
	double pt = procTime(o);

	// Increase the total clean operation processing time
	topt += pt;

	// Set the machine's availability time
	o->machAvailTime(this->timeAvail);

	// Set the operation's start and processing times
	o->sp(Math::max(t, o->r()), pt);

	// Update the next time the machine will become available
	t = o->c();

	// Assign the machine's ID to the operation
	o->machID = this->ID;

	operations.append(o);

	return *this;
}

double Machine::procTime(const Operation* o) {
	// If the speed is 0 then the processing time will be infinity!!!
	if (!this->type2speed.contains(o->type)) {
		Debugger::err << "Machine::procTime : Machine " << ID << " is not able to process operation type " << o->type << ENDL;
	}

	return o->pe() / this->type2speed[o->type];
}

double Machine::util() {

	if (time() == 0.0) return 0.0; // No operations are scheduled

	return topt / time();
}

/** A global qHash function which is able to return hash-values for Machine 
 *  based on its ID. */

inline uint qHash(const Machine& key) {
	return qHash(key.ID);
}

inline uint qHash(const Machine* key) {
	return qHash(key->ID);
}

/* ########################################################################## */

/* ######################  ToolGroup  ####################################### */

ToolGroup::ToolGroup() {
	ID = -1;
	init();
}

ToolGroup::ToolGroup(ToolGroup& orig) {
	this->copy(orig);
}

ToolGroup::~ToolGroup() {
	//QTextStream out(stdout);

	//out << "ToolGroup::~ToolGroup : Deleting MG " << ID << endl;

	clear();
}

void ToolGroup::init() {

	for (int i = 0; i < _machines.size(); i++) {
		_machines[i]->init();
	}
}

void ToolGroup::clear() {
	//QTextStream out(stdout);

	//out << "ToolGroup::clear : Clearing MG " << ID << endl;

	// Delete the machines
	for (int i = 0; i < _machines.size(); i++) {
		delete _machines[i];
	}

	types.clear();
	_machines.clear();
	mid2idx.clear();
}

void ToolGroup::copy(ToolGroup& other) {
	ID = other.ID;

	types = other.types;

	// Clear the machines
	for (int i = 0; i < _machines.size(); i++) {
		delete _machines[i];
	}

	// Copy the machines
	_machines.resize(other.machines().size());
	for (int i = 0; i < other._machines.size(); i++) {
		_machines[i] = new Machine(*(other._machines[i]));
	}

	// Copy the other data
	mid2idx = other.mid2idx;

}

ToolGroup& ToolGroup::operator=(ToolGroup& other) {
	this->copy(other);

	return *this;
}

void ToolGroup::fromDOMElement(const QDomElement &tgelem) {
	Machine *m;
	QDomNodeList mach_list = tgelem.elementsByTagName("machine");

	ID = tgelem.attribute("id").toInt();

	this->clear();

	for (int i = 0; i < mach_list.size(); i++) {
		m = new Machine;
		m->fromDOMElement(mach_list.item(i).toElement());
		*this << m;
		m = NULL;
	}
}

QTextStream& operator<<(QTextStream &out, ToolGroup &tg) {
	out << "TG: ("
			<< "<<"
			<< "ID=" << tg.ID << ","
			<< "Ty=";


	for (QSet<int>::iterator sit = tg.types.begin(); sit != tg.types.end(); sit++) {
		out << *sit;
		if (sit != --tg.types.end())
			out << ",";
	}
	out << ">>" << endl << endl;

	// Now output the operation assigned to this machine
	for (QVector<Machine*>::iterator mit = tg._machines.begin(); mit != tg._machines.end(); mit++) {
		out << **mit;
		out << endl;
	}
	out << endl << ")";

	return out;
}

Machine& ToolGroup::randomMachine() {
	//int rndidx = Rand::rndInt(0, _machines.size() - 1);
	int rndidx = Rand::rnd<Math::uint32>(0, _machines.size() - 1);

	return *(_machines.at(rndidx));
}

Machine& ToolGroup::nextAvailable() {
	int sz = _machines.size();
	double t = Math::MAX_DOUBLE;
	int idx = -1;

	for (int i = 0; i < sz; i++)
		if (_machines.at(i)->time() < t) {
			t = _machines.at(i)->time();
			idx = i;
		}

	return *(_machines.at(idx));
}

Machine& ToolGroup::nextAvailable(const int &type) {
	int sz = _machines.size();
	double t = Math::MAX_DOUBLE;
	int idx = -1;

	for (int i = 0; i < sz; i++)
		if (_machines.at(i)->type2speed.contains(type)) {
			if (_machines.at(i)->time() < t) {
				t = _machines.at(i)->time();
				idx = i;
			}
		}

	return *(_machines.at(idx));
}

Machine& ToolGroup::fastestAvailable(const double &t, const int &type) {
	// IMPORTANT!!! If some machine is not able to process some operation type then the corresponding speed value is automatically set to 0.0

	int sz = _machines.size();
	double s = 0.0;
	int idx = -1;
	int ncapable = 0; // Number of machines capable to process the operation

	for (int i = 0; i < sz; i++)
		if (_machines.at(i)->type2speed.contains(type)) {
			if (_machines.at(i)->time() <= t && s < _machines.at(i)->type2speed[type]) {
				s = _machines.at(i)->type2speed[type];
				idx = i;
			}
			ncapable++;
		}

	if (ncapable == 0) {
		Debugger::err << "ToolGroup::fastestAvailable : Failed to find capable machine in TG " << ID << " for operation type " << type << ENDL;
	} else {
		if (idx == -1) return fastestMachine(type); //nextAvailable();
	}

	return *(_machines.at(idx));
}

Machine& ToolGroup::fastestMachine(const int &type) {
	return fastestAvailable(double(Math::MAX_DOUBLE), type);
}

Machine& ToolGroup::slowestMachine(const int &type) {
	// IMPORTANT!!! If some machine is not able to process some operation type then the corresponding speed value is automatically set to 0.0

	int sz = _machines.size();
	double s = Math::MAX_DOUBLE;
	int idx = -1;
	int ncapable = 0; // Number of machines capable to process the operation

	for (int i = 0; i < sz; i++) {
		if (_machines.at(i)->type2speed.contains(type)) {
			if (s > _machines.at(i)->type2speed[type]) {
				s = _machines.at(i)->type2speed[type];
				idx = i;
			}

			ncapable++;
		}
	}

	if (ncapable == 0) {
		Debugger::err << "ToolGroup::fastestAvailable : Failed to find capable machine in TG " << ID << " for operation type " << type << ENDL;
	}

	return *(_machines.at(idx));
}

Machine& ToolGroup::earliestToFinish(const Operation *o) {
	//QTextStream out(stdout);

	int sz = _machines.size();
	double s;
	double p;
	double c = Math::MAX_DOUBLE;
	int idx = -1;

	for (int i = 0; i < sz; i++) {
		if (_machines.at(i)->type2speed.contains(o->type)) {
			p = _machines.at(i)->procTime(o);
			s = Math::max(_machines.at(i)->time(), o->r());

			//out << "s=" << s << endl;
			//out << "p=" << p << endl;
			//out << "c=" << c << endl;

			if (c > s + p) {
				c = s + p;
				idx = i;
			}
		}
	}

	if (idx == -1) {

		//out << *this << endl;
		//out << *o << endl;

		Debugger::err << "ToolGroup::earliestToFinish : Failed to find earliest to finish machine in TG " << ID << " for operation type " << o->type << ENDL;
	}

	return *(_machines.at(idx));
}

double ToolGroup::expectedSpeed(const int& type) {
	double exps = 0.0; // Expected speed of processing of the operation

	int sz = _machines.size();
	int nmachs = 0;
	for (int i = 0; i < sz; i++) {
		if (_machines.at(i)->type2speed.contains(type)) {
			exps += _machines.at(i)->type2speed[type];
			nmachs++;
		}
	}

	if (nmachs == 0) {
		Debugger::err << "ToolGroup::expectedSpeed : TG " << ID << " can not process operation type " << type << ENDL;
	}

	exps /= double(nmachs);

	return exps;
}

double ToolGroup::expectedProcTime(Operation *o) {
	// IMPORTANT!!! If some machine is not able to process some operation type then it is not accounted

	//return shortestProcTime(o);

	/*
	int sz = _machines.size();
	double exps = 0.0; // Expected speed of processing of the operation

	int nmachs = 0;
	for (int i = 0; i < sz; i++) {
		if (_machines.at(i)->type2speed.contains(o->type)) {
			exps += _machines.at(i)->type2speed[o->type];
			nmachs++;
		}
	}

	if (nmachs == 0) {
		Debugger::err << "ToolGroup::expectedProcTime : TG " << ID << " can not process operation type " << o->type << ENDL;
	}

	exps /= double(nmachs);
	 */
	double exps = expectedSpeed(o->type);

	return o->pe() / exps;
}

double ToolGroup::shortestProcTime(Operation *o) {
	return o->pe() / fastestMachine(o->type).type2speed[o->type];
}

double ToolGroup::longestProcTime(Operation *o) {
	return o->pe() / slowestMachine(o->type).type2speed[o->type];
}

double ToolGroup::avgUtil() {
	double tutil = 0.0;
	for (int i = 0; i < _machines.size(); i++) {
		tutil += _machines[i]->util();
	}

	if (_machines.size() == 0) return 0.0;

	return tutil / double(_machines.size());
}

inline uint qHash(const ToolGroup& key) {
	return qHash(key.ID);
}

inline uint qHash(const ToolGroup* key) {
	return qHash(key->ID);
}

/* ########################################################################## */

/* #########################  Resources  #################################### */

int Resources::rcCreated = 0;
int Resources::rcDeleted = 0;

Resources::Resources() {
	init();

	rcCreated++;
}

Resources::Resources(Resources& orig) {
	*this = orig;
}

Resources::~Resources() {
	//QTextStream out(stdout);

	//out << "Resources::~Resources : Deleting resources " << ID << endl;

	clear();

	rcDeleted++;

	//out << "RC created : " << rcCreated << endl;
	//out << "RC deleted : " << rcDeleted << endl;
}

Resources& Resources::operator=(Resources& other) {
	this->ID = other.ID;

	// Clear the current tool groups
	//	for (int i = 0; i < tools.size(); i++) {
	//		delete tools[i];
	//	}
	clear();

	// Copy the resources
	tools.resize(other.tools.size());
	for (int i = 0; i < other.tools.size(); i++) {
		tools[i] = new ToolGroup(*(other.tools[i]));
	}

	tid2idx = other.tid2idx;
	type2idcs = other.type2idcs;

	return *this;
}

void Resources::init() {

	for (int i = 0; i < tools.size(); i++) {
		tools[i]->init();
	}
}

void Resources::clear() {
	//QTextStream out(stdout);

	//out << "Resources::~clear : Clearing resources " << ID << endl;

	// Delete the tool groups
	for (int i = 0; i < tools.size(); i++) {
		delete tools[i];
	}

	tools.clear();
	tid2idx.clear();
	type2idcs.clear();
}

void Resources::mergeToolGroups() {
	QTextStream out(stdout);

	QVector<ToolGroup*> newTools;
	QHash<int, int> newToolID2Idx;
	QHash<int, QList<int> > newType2Idcs;

	out << "Resources::mergeToolGroups : Merging machine groups ... " << endl;

	ToolGroup* dummyTG = NULL;

	ToolGroup* bigTG = new ToolGroup;
	bigTG->ID = 1;

	for (int i = 0; i < tools.size(); i++) {
		ToolGroup* curTG = tools[i];

		Machine* curNewMach;

		if (curTG->ID == 0) { // Leave the dummy machine

			dummyTG = new ToolGroup(*curTG);

		} else {

			QList<Machine*> curMachs = curTG->machines();

			// Iterate over all machines of the current TG
			for (int j = 0; j < curMachs.size(); j++) {
				Machine* curMach = curMachs[j];
				curNewMach = new Machine(*curMach);

				*bigTG << curNewMach;
			}

		}

	}

	// Clear the old data
	this->clear();

	// Append the dummy and the big machine groups
	*this << dummyTG;
	*this << bigTG;

	out << "Machine groups after the merge : " << endl;

	for (int i = 0; i < tools.size(); i++) {
		ToolGroup* curTG = tools[i];

		out << *curTG << endl;
	}

	//getchar();

}

void Resources::fromDOMElement(const QDomElement &rcelem) {
	ToolGroup *tg;

	QDomNodeList tgelems = rcelem.elementsByTagName("toolgroup");

	ID = rcelem.attribute("id").toInt();

	tools.clear();
	tid2idx.clear();
	type2idcs.clear();

	for (int i = 0; i < tgelems.size(); i++) {
		tg = new ToolGroup;
		tg->fromDOMElement(tgelems.item(i).toElement());
		*this << tg;
		tg = NULL;
	}
}

QTextStream& operator<<(QTextStream &out, Resources &rc) {
	out << "RC: ||"
			<< "<<<"
			<< "ID=" << rc.ID
			<< ">>>" << endl << endl;

	// Now output the operation assigned to this machine

	for (QVector<ToolGroup*>::iterator tgit = rc.tools.begin(); tgit != rc.tools.end(); tgit++) {
		out << **tgit;
		out << endl;
	}

	out << endl << "||" << endl;

	return out;
}

Machine& Resources::nextAvailable() {
	int sz = tools.size();
	double t = Math::MAX_DOUBLE;
	//int idx = -1;
	Machine *m = NULL;

	for (int i = 0; i < sz; i++) {
		m = &(tools.at(i)->nextAvailable());
		if (m->time() < t) {
			t = m->time();
			//idx = i;
		}
	}

	return *m;
}

/* ########################################################################## */