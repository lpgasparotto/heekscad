// HCircle.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#ifdef WIN32
#pragma once
#endif
#include "../interface/HeeksObj.h"
#include "../interface/HeeksColor.h"
#include <gp_Circ.hxx>

class HCircle: public HeeksObj{
private:
	HeeksColor color;
	static wxIcon* m_icon;

public:
	gp_Circ m_circle;

	~HCircle(void);
	HCircle(const gp_Circ &c, const HeeksColor* col);
	HCircle(const HCircle &c);

	const HCircle& operator=(const HCircle &c);

	// HeeksObj's virtual functions
	int GetType()const{return CircleType;}
	long GetMarkingMask()const{return MARKING_FILTER_CIRCLE;}
	void glCommands(bool select, bool marked, bool no_color);
	void GetBox(CBox &box);
	const wxChar* GetTypeString(void)const{return _("Circle");}
	HeeksObj *MakeACopy(void)const;
	wxString GetIcon(){return _T("circle");}
	bool ModifyByMatrix(const double *mat);
	void SetColor(const HeeksColor &col){color = col;}
	const HeeksColor* GetColor()const{return &color;}
	void GetGripperPositions(std::list<double> *list, bool just_for_endof);
	void GetProperties(std::list<Property *> *list);
	bool FindNearPoint(const double* ray_start, const double* ray_direction, double *point);
	bool FindPossTangentPoint(const double* ray_start, const double* ray_direction, double *point);
	bool Stretch(const double *p, const double* shift);
	void GetSegments(void(*callbackfunc)(const double *p), double pixels_per_mm, bool want_start_point = true)const;
	bool GetCentrePoint(double* pos);
	void WriteXML(TiXmlNode *root);
	int Intersects(const HeeksObj *object, std::list< double > *rl)const;

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	static bool GetLineTangentPoints(const gp_Circ& c1, const gp_Circ& c2, const gp_Pnt& a, const gp_Pnt& b, gp_Pnt& p1, gp_Pnt& p2);
	static bool GetLineTangentPoint(const gp_Circ& c, const gp_Pnt& a, const gp_Pnt& b, gp_Pnt& p);
	static bool GetArcTangentPoints(const gp_Circ& c, const gp_Lin &line, const gp_Pnt& p, double radius, gp_Pnt& p1, gp_Pnt& p2, gp_Pnt& centre, gp_Dir& axis);
	static bool GetArcTangentPoints(const gp_Circ& c1, const gp_Circ &c2, const gp_Pnt& a, const gp_Pnt& b, double radius, gp_Pnt& p1, gp_Pnt& p2, gp_Pnt& centre, gp_Dir& axis);
	static bool GetArcTangentPoints(const gp_Lin& l1, const gp_Lin &l2, const gp_Pnt& a, const gp_Pnt& b, double radius, gp_Pnt& p1, gp_Pnt& p2, gp_Pnt& centre, gp_Dir& axis);
	static bool GetArcTangentPoint(const gp_Lin& l, const gp_Pnt& a, const gp_Pnt& b, const gp_Vec *final_direction, double* radius, gp_Pnt& p, gp_Pnt& centre, gp_Dir& axis);
	static bool GetArcTangentPoint(const gp_Circ& c, const gp_Pnt& a, const gp_Pnt& b, const gp_Vec *final_direction, double* radius, gp_Pnt& p, gp_Pnt& centre, gp_Dir& axis);
};
