// MarkedList.cpp
#include "stdafx.h"

#include "MarkedList.h"
#include "../interface/HeeksObj.h"
#include "../interface/MarkedObject.h"
#include "../interface/PropertyInt.h"
#include "DigitizeMode.h"
#include "SelectMode.h"
#include "PointOrWindow.h"
#include "GripperSelTransform.h"
#include "GraphicsCanvas.h"
#include "HeeksFrame.h"
#include "ConversionTools.h"
#include <wx/clipbrd.h>
#include "../tinyxml/tinyxml.h"
#include <wx/stdpaths.h>
#include <fstream>
using namespace std;

MarkedList::MarkedList(){
	gripping = false;
	point_or_window = new PointOrWindow(true);
	gripper_marked_list_changed = false;
	ignore_coords_only = false;
	m_filter = -1;
}

MarkedList::~MarkedList(void){
	delete point_or_window;
	delete_move_grips(false);
}

void MarkedList::delete_move_grips(bool check_app_grippers){
	std::list<Gripper*>::iterator It;
	for(It = move_grips.begin(); It != move_grips.end(); It++){
		Gripper* gripper = *It;
		if(check_app_grippers){
			if(gripper == wxGetApp().cursor_gripper)wxGetApp().cursor_gripper = NULL;
			if(gripper == wxGetApp().drag_gripper)wxGetApp().drag_gripper = NULL;
		}
		delete gripper;
	}
	move_grips.clear();
}

void MarkedList::create_move_grips(){
	delete_move_grips();
	double pos[3];
	int number_of_grips_made = 0;
	std::list<HeeksObj*>::iterator Iter ;
	for(Iter = m_list.begin(); Iter != m_list.end() && number_of_grips_made<100; Iter++){
		HeeksObj* object = *Iter;
		if(object->GetType() == GripperType)continue;
		std::list<double> vl;
		std::list<double>::iterator It;
		object->GetGripperPositions(&vl, false);
		for(It = vl.begin(); It != vl.end() && number_of_grips_made<100; It++){
			EnumGripperType gripper_type = (EnumGripperType)((int)(*It));
			It++;
			pos[0] = *It;
			It++;
			pos[1] = *It;
			It++;
			pos[2] = *It;
			move_grips.push_back(new GripperSelTransform(make_point(pos), gripper_type));
			number_of_grips_made++;
		}
	}
}

void MarkedList::update_move_grips(){
	if(gripping)return;
	double pos[3];
	std::list<HeeksObj*>::iterator Iter ;
	std::list<Gripper*>::iterator Iter2;
	Iter2 = move_grips.begin();
	for(Iter = m_list.begin(); Iter != m_list.end(); Iter++){
		if(Iter2 == move_grips.end())break;
		HeeksObj* object = *Iter;
		if(object->GetType() == GripperType)continue;
		std::list<double> vl;
		std::list<double>::iterator It;
		object->GetGripperPositions(&vl, false);
		for(It = vl.begin(); It != vl.end(); It++){
			It++;
			pos[0] = *It;
			It++;
			pos[1] = *It;
			It++;
			pos[2] = *It;
			Gripper* gripper = *Iter2;
			gripper->position = make_point(pos);
			Iter2++;
			if(Iter2 == move_grips.end())break;
		}
	}
}

void MarkedList::render_move_grips(bool select, bool no_color){
	std::list<Gripper*>::iterator It;
	for(It = move_grips.begin(); It != move_grips.end(); It++){
		if(select)glPushName((unsigned int)(*It));
		(*It)->glCommands(select, false, no_color);
		if(select)glPopName();
	}
}

void MarkedList::create_grippers(){
	if(gripping)return;
	if(gripper_marked_list_changed)create_move_grips();
	else update_move_grips();
	gripper_marked_list_changed = false;
}

void MarkedList::GrippersGLCommands(bool select, bool no_color){
	if(size()>0){
		create_grippers();
		render_move_grips(select, no_color);
	}
}

void MarkedList::ObjectsInWindow( wxRect window, MarkedObject* marked_object, bool single_picking){
	int buffer_length = 16384;
	GLuint *data = (GLuint *)malloc( buffer_length * sizeof(GLuint) );
	if(data == NULL)return;
	int i, j;
	int half_window_width = 0;
	wxPoint window_centre;
	if(single_picking){
		half_window_width = (window.width)/2;
		window_centre.x = window.x + window.width/2;
		window_centre.y = window.y + window.height/2;
	}
	int window_mode = 0;
	while(1){
		if(single_picking){
			int window_size = half_window_width;
			if(window_mode == 0)window_size = 0;
			if(window_mode == 1)window_size = half_window_width/2;
			window.x = window_centre.x - window_size;
			window.y = window_centre.y - window_size;
			window.width = window_size * 2;
			window.height = window_size * 2;
		}
	    GLint num_hits = -1;
		while(num_hits < 0){
			glSelectBuffer(buffer_length, data);
			glRenderMode(GL_SELECT);
			glInitNames();
			wxGetApp().m_frame->m_graphics->m_view_point.SetViewport();
			wxGetApp().m_frame->m_graphics->m_view_point.SetPickProjection(window);
			wxGetApp().m_frame->m_graphics->m_view_point.SetModelview();
			wxGetApp().glCommands(true, false, false);
			GrippersGLCommands(true, false);
			glFlush();
			num_hits = glRenderMode(GL_RENDER); 
			if(num_hits<0){
				free(data);
				buffer_length *= 10;
				data = (GLuint *)malloc( buffer_length * sizeof(GLuint) );
				if(data == NULL)return;
			}
		}
		int pos = 0;
		for(i=0; i<num_hits; i++)
		{
			unsigned int names = data[pos];
			if(names == 0)break;
			pos++;
			unsigned int min_depth = data[pos];
			pos+=2;
			MarkedObject* current_found_object = marked_object;
			bool ignore_coords_only_found = false;
			for(j=0; j<(int)names; j++, pos++){
				if(!ignore_coords_only_found && current_found_object != NULL){
					HeeksObj *object = (HeeksObj *)(data[pos]);
					if(ignore_coords_only && wxGetApp().m_digitizing->OnlyCoords(object)){
						ignore_coords_only_found = true;
					}
					else{
						if(object->GetType() == GripperType || (object->GetMarkingMask() & m_filter)){
							int window_size = window.width;
							current_found_object = current_found_object->Add(object, min_depth, window_size);
						}
					}
				}
			}
		}
		window_mode++;
		if(!single_picking)break;
		if(window_mode > 2)break;
	}
	
	free(data);
}

void MarkedList::glCommands(){
	std::list<HeeksObj*>::iterator Iter ;
	int object_count = 0;
	for(Iter = m_list.begin(); Iter != m_list.end(); Iter++, object_count++){
		HeeksObj *object = (*Iter);
		object->glCommands(true, false, false);
	}
}

void MarkedList::Add(std::list<HeeksObj *> &list){
	std::list<HeeksObj *>::iterator It;
	for(It = list.begin(); It != list.end(); It++){
		HeeksObj *object = *It;
		m_list.push_back(object);
		m_set.insert(object);
	}
	OnChanged(false, false, &list, NULL);
}

void MarkedList::Remove(HeeksObj *object){
	std::list<HeeksObj *> list;
	list.push_back(object);
	Remove(list);
}

void MarkedList::Add(HeeksObj *object){
	std::list<HeeksObj *> list;
	list.push_back(object);
	Add(list);
}

void MarkedList::Remove(const std::list<HeeksObj *> &obj_list){
	std::list<HeeksObj *>::const_iterator It;
	for(It = obj_list.begin(); It != obj_list.end(); It++){
		HeeksObj *object = *It;
		if(m_set.find(object) != m_set.end()){
			m_list.remove(object);
		}
		m_set.erase(object);
	}
	OnChanged(false, false, NULL, &obj_list);
}

void MarkedList::Clear(void){ 
	m_list.clear();
	m_set.clear();
	OnChanged(false, true, NULL, NULL);
}

void MarkedList::FindMarkedObject(const wxPoint &point, MarkedObject* marked_object){
	if(marked_object == NULL)return;
	point_or_window->SetWithPoint(point);
	ObjectsInWindow(point_or_window->box_chosen, marked_object);
}

bool MarkedList::ObjectMarked(HeeksObj *object){
	return m_set.find(object) != m_set.end();
}

void MarkedList::OnChanged(bool all_marked, bool none_marked, const std::list<HeeksObj *>* added, const std::list<HeeksObj *>* removed){
	gripper_marked_list_changed = true;
	wxGetApp().ObserversMarkedListChanged(all_marked, none_marked, added, removed);
}

void MarkedList::set_ignore_onoff(HeeksObj* object, bool b){
	if(b)m_ignore_set.insert(object);
	else m_ignore_set.erase(object);
}

bool MarkedList::get_ignore(HeeksObj* object){
	if(m_ignore_set.find(object) != m_ignore_set.end())return true;
	return false;
}

void MarkedList::GetProperties(std::list<Property *> *list){
	if(m_list.size() == 1)
	{
		m_list.front()->GetProperties(list);
	}
	else
	{
		// multiple selection
		list->push_back(new PropertyInt(_T("Number of items selected"), m_list.size(), NULL));
	}
}

class DeleteMarkedListTool : public Tool
{
private:
	static wxBitmap* m_bitmap;
public:
	const wxChar* GetTitle() {return _T("Delete Marked Items");}
	void Run() {wxGetApp().DeleteMarkedItems();}
	wxBitmap* Bitmap(){if(m_bitmap == NULL){wxString exe_folder = wxGetApp().GetExeFolder();m_bitmap = new wxBitmap(exe_folder + _T("/bitmaps/delete.png"), wxBITMAP_TYPE_PNG);}return m_bitmap;}
} delete_marked_list_tool;
wxBitmap* DeleteMarkedListTool::m_bitmap = NULL;

class CopyMarkedList: public Tool
{
private:
	static wxBitmap* m_bitmap;
public:
	void Run();
	const wxChar* GetTitle(){return _T("Copy");}
	wxBitmap* Bitmap(){if(m_bitmap == NULL){wxString exe_folder = wxGetApp().GetExeFolder();m_bitmap = new wxBitmap(exe_folder + _T("/bitmaps/copy.png"), wxBITMAP_TYPE_PNG);}return m_bitmap;}
	const wxChar* GetToolTip(){return _T("Copies the selected items to the clipboard");}
} copy_marked_list;
wxBitmap* CopyMarkedList::m_bitmap = NULL;

void CopyMarkedList::Run()
{
	wxGetApp().m_marked_list->CopySelectedItems();
}

class PasteTool: public Tool
{
private:
	static wxBitmap* m_bitmap;
public:
	void Run();
	const wxChar* GetTitle(){return _T("Paste");}
	wxBitmap* Bitmap(){if(m_bitmap == NULL){wxString exe_folder = wxGetApp().GetExeFolder();m_bitmap = new wxBitmap(exe_folder + _T("/bitmaps/paste.png"), wxBITMAP_TYPE_PNG);}return m_bitmap;}
	const wxChar* GetToolTip(){return _T("Paste items from the clipboard to the drawing");}
} paste_tool;
wxBitmap* PasteTool::m_bitmap = NULL;

void PasteTool::Run()
{
	wxGetApp().Paste();
}

void MarkedList::GetTools(std::list<Tool*>* t_list, const wxPoint* p){
	if (m_list.size() > 1)
	{
		t_list->push_back(&delete_marked_list_tool);
		t_list->push_back(NULL);
	}

	if(m_list.size() == 1)
	{
		m_list.front()->GetTools(t_list, p);
	}

	GetConversionMenuTools(t_list);

	// cut and copy tools
	for(std::list<HeeksObj*>::iterator It = m_list.begin(); It != m_list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->CanBeCopied())
		{
			t_list->push_back(&copy_marked_list);
			break;
		}
	}

	// paste
	if (wxGetApp().IsPasteReady())t_list->push_back(&paste_tool);
}

void MarkedList::CutSelectedItems()
{
	wxGetApp().StartHistory(_T("Cut Selected Items"));
	CopySelectedItems();
	wxGetApp().DeleteUndoably(m_list);
	wxGetApp().EndHistory();
}

void MarkedList::CopySelectedItems()
{
	wxStandardPaths sp;
	sp.GetTempDir();
	wxString temp_file = sp.GetTempDir() + _T("/temp_Heeks_clipboard_file.xml");

	wxGetApp().SaveXMLFile(m_list, temp_file);

#if wxUSE_UNICODE
	wifstream ifs(temp_file);
#else
	ifstream ifs(temp_file);
#endif
	if(!ifs)return;

	wxString fstr;
	wxChar str[1024];
	while(!(ifs.eof())){
		ifs.getline(str, 1022);
		fstr.append(str);
		fstr.append(_T("\r\n"));
		if(!ifs)break;
	}

	if (wxTheClipboard->Open())
	{
		// This data object is held by the clipboard, 
		// so do not delete them in the app.
		wxTheClipboard->SetData( new wxTextDataObject(fstr));
		wxTheClipboard->Close();
	}
}

void MarkedList::Reset()
{
	delete_move_grips();
}