/* 
 * File:   Clonable.h
 * Author: DrSobik
 *
 * Created on February 23, 2013, 11:55 PM
 */

#ifndef CLONABLE_H
#define	CLONABLE_H

class Clonable {
public:
    Clonable();
    Clonable(Clonable& orig);
    virtual ~Clonable();

    virtual Clonable* clone();

};

#endif	/* CLONABLE_H */

