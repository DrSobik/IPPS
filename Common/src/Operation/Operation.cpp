/* 
 * File:   Operation.cpp
 * Author: DrSobik
 * 
 * Created on July 1, 2011, 10:05 AM
 */

#include "Operation.h"

int Operation::operCreated = 0;
int Operation::operDeleted = 0;

Operation::Operation(){
	_pe = 1.0;
	init();

	operCreated++;
}

Operation::Operation(const double &etalon_proc_time) {
	_pe = etalon_proc_time;
	init();

	operCreated++;
}

Operation::Operation(const Operation& orig) {
	copy(orig);
	
	operCreated++;
}

Operation::Operation(Operation& orig) {
	copy(orig);
	
	operCreated++;
}

Operation::~Operation() {
	//QTextStream out(stdout);

	operDeleted++;

//	out << "Operations created : " << operCreated << endl;
//	out << "Operations deleted : " << operDeleted << endl;
}

void Operation::init() {
	ID = -1;
	OID = -1;
	OT = -1;
	BID = -1;
	IID = -1;
	IT = -1;
	RID = -1;
	SI = -1;

	type = -1;
	toolID = -1;
	machID = -1;

	_ir = 0.0;
	_r = 0.0;
	_mA = 0.0;
	_d = 0.0;
	_s = 0.0;
	_p = 0.0;
	_w = 0.0;
	_c = 0.0;
}

void Operation::copy(const Operation& other) {
	ID = other.ID;
	OID = other.OID;
	OT = other.OT;
	BID = other.BID;
	IID = other.IID;
	IT = other.IT;
	RID = other.RID;
	SI = other.SI;

	type = other.type;
	toolID = other.toolID;
	machID = other.machID;

	_ir = other._ir;
	_r = other._r;
	_mA = other._mA;
	_d = other._d;
	_s = other._s;
	_pe = other._pe;
	_p = other._p;
	_w = other._w;
	_c = other._c;
}

const Operation& Operation::operator=(const Operation& other) {
	copy(other);
	return *this;
}

/** Write information about the operation. */
void Operation::write(QTextStream &out) {
	out << "Operation : {ID=" << ID << ", "
			<< "OID=" << OID << ","
			<< "OT=" << OT << ","
			<< "BID=" << BID << ","
			<< "IID=" << IID << ","
			<< "IT=" << IT << ","
			<< "RID=" << RID << ","
			<< "SI=" << SI << ","
			<< "Ty=" << type << ","
			<< "To=" << toolID << ","
			<< "Ma=" << machID << ","
			<< "w=" << _w << ","
			<< "ir=" << _ir << ","
			<< "r=" << _r << ","
			<< "mA=" << _mA << ","
			<< "s=" << _s << ","
			<< "p=" << _p << ","
			<< "c=" << _c << ","
			<< "d=" << _d << ","
			<< "wT=" << wT() << "}";
}

QXmlStreamWriter& operator<<(QXmlStreamWriter& composer, Operation& o) {
	composer.writeStartElement("operation");
	composer.writeAttribute("id", QString::number(o.ID));

	composer.writeTextElement("unitID", QString::number(o.OID));

	composer.writeTextElement("unitType", QString::number(o.OT));

	composer.writeTextElement("bomID", QString::number(o.BID));

	composer.writeTextElement("partID", QString::number(o.IID));

	composer.writeTextElement("partType", QString::number(o.IT));

	composer.writeTextElement("routeID", QString::number(o.RID));

	composer.writeTextElement("stepIdx", QString::number(o.SI));

	composer.writeTextElement("type", QString::number(o.type));

	composer.writeTextElement("toolGroup", QString::number(o.toolID));

	composer.writeTextElement("machID", QString::number(o.machID));

	composer.writeTextElement("weight", QString::number(o.w()));

	composer.writeTextElement("releaseTime", QString::number(o.ir()));

	composer.writeTextElement("startTime", QString::number(o.s()));

	composer.writeTextElement("procTime", QString::number(o.p()));

	composer.writeTextElement("dueTime", QString::number(o.d()));

	composer.writeTextElement("completionTime", QString::number(o.c()));

	composer.writeEndElement(); // operation

	return composer;
}

QXmlStreamReader& operator>>(QXmlStreamReader& reader, Operation& o) {
	// IMPORTANT!!! It is assumed that the reader's current token is "operation"
	o.ID = reader.attributes().value("id").toString().toInt();

	while (!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "operation")) {

		reader.readNext();


		if (reader.name() == "unitID") {
			reader.readNext();
			o.OID = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "unitType") {
			reader.readNext();
			o.OT = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "bomID") {
			reader.readNext();
			o.BID = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "partID") {
			reader.readNext();
			o.IID = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "partType") {
			reader.readNext();
			o.IT = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "routeID") {
			reader.readNext();
			o.RID = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "stepIdx") {
			reader.readNext();
			o.SI = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "type") {
			reader.readNext();
			o.type = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "toolGroup") {
			reader.readNext();
			o.toolID = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "machID") {
			reader.readNext();
			o.machID = reader.text().toString().toInt();
			reader.readNext();
		}

		if (reader.name() == "weight") {
			reader.readNext();
			o.w(reader.text().toString().toDouble());
			reader.readNext();
		}

		if (reader.name() == "releaseTime") {
			reader.readNext();
			o.ir(reader.text().toString().toDouble());
			reader.readNext();
		}

		if (reader.name() == "startTime") {
			reader.readNext();
			o.s(reader.text().toString().toDouble());
			reader.readNext();
		}

		if (reader.name() == "procTime") {
			reader.readNext();
			o.p(reader.text().toString().toDouble());
			reader.readNext();
		}

		if (reader.name() == "dueTime") {
			reader.readNext();
			o.d(reader.text().toString().toDouble());
			reader.readNext();
		}

	}

	return reader;
}