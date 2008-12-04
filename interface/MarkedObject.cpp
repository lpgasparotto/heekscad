// MarkedObject.cpp
#include "stdafx.h"
#include "MarkedObject.h"
#include "HeeksObj.h"
#include "ToolList.h"

MarkedObject::MarkedObject()
{
	m_depth = 0;
	m_object = NULL;
	m_window_size = -1;
}

MarkedObject::MarkedObject(unsigned long depth, HeeksObj* object, int window_size)
{
	m_object = object;
	m_depth = depth;
	m_window_size = window_size;
}

MarkedObject::MarkedObject(const MarkedObject &so)
{
	operator=(so);
}

MarkedObject::~MarkedObject(){
	Clear();
}

const MarkedObject& MarkedObject::operator=(const MarkedObject &so)
{
	m_depth = so.m_depth;
	m_object = so.m_object;
	return (*this);
}

unsigned long MarkedObject::GetDepth()
{
	if(m_map.size() == 0)return m_depth;
	if(CurrentIt == m_map.end())return m_depth;
	return CurrentIt->second->GetDepth();
}

void MarkedObject::Clear()
{
	for(std::map<HeeksObj*, MarkedObject*>::iterator It = m_map.begin(); It != m_map.end(); It++)
	{
		MarkedObject* object = It->second;
		delete object;
	}
	m_map.clear();
	m_types.clear();
}

MarkedObject* MarkedObject::Add(HeeksObj* object, unsigned long depth, int window_size)
{
	std::map<HeeksObj*, MarkedObject*>::iterator FindIt = m_map.find(object);

	MarkedObject* marked_object = NULL;
	if(FindIt == m_map.end())
	{
		std::map<int, std::map<int, MarkedObject*> >::iterator FindIt2 = m_types.end();
		if(single_type())
		{
			FindIt2 = m_types.find(0);
			if(FindIt2 != m_types.end())
			{
				std::map<int, MarkedObject*>& map = FindIt2->second;
				std::map<int, MarkedObject*>::iterator TypeFindIt = map.find(object->GetType());
				if(TypeFindIt != FindIt2->second.end())
				{
					MarkedObject* so = TypeFindIt->second;
					if(depth < so->GetDepth() && window_size<= so->GetWindowSize())
					{
						map.erase(TypeFindIt);
						m_map.erase(so->GetObject());
						so->Clear();
						delete so;
					}
					else{
						return NULL;
					}
				}
			}
		}
		if(single_type())
		{
			marked_object = new MarkedObjectOneOfEach(depth, object, window_size);
		}
		else
		{
			marked_object = new MarkedObjectManyOfSame(depth, object, window_size);
		}
		m_map.insert(std::pair<HeeksObj*, MarkedObject*>(object, marked_object));
		CurrentIt = m_map.end();
		if(single_type())
		{
			std::pair<int, MarkedObject*> type_entry(object->GetType(), marked_object);
			if(FindIt2 != m_types.end())
			{
				FindIt2->second.insert(type_entry);
			}
			else{
				std::map<int, MarkedObject*> new_map;
				new_map.insert(type_entry);
				m_types.insert(std::pair< int, std::map<int, MarkedObject*> >(0, new_map));
			}
		}
	}
	else
	{
		marked_object = FindIt->second;
		if(depth<marked_object->m_depth)marked_object->m_depth = depth;
	}
	return marked_object;
}

void MarkedObject::SetFirst(EnumStackedType stacked_type)
{
	m_stacked_type = stacked_type;
	if(m_map.size() > 0)
	{
		CurrentIt = m_map.begin();
	}
	switch(m_stacked_type)
	{
		case EverythingStackedType:
		case BottomOnlyStackedType:
		case OneFromLevelStackedType:
			{
				if(m_map.size() > 0){
					CurrentIt = m_map.begin();
					for(std::map<HeeksObj*, MarkedObject*>::iterator It = m_map.begin(); It != m_map.end(); It++)
					{
						It->second->SetFirst(m_stacked_type);
					}
				}
			}
			break;
	}
	m_processed = false;
}

HeeksObj* MarkedObject::GetFirstOfEverything()
{
	SetFirst(EverythingStackedType);
	return Increment();
}

HeeksObj* MarkedObject::GetFirstOfTopOnly()
{
	SetFirst(TopOnlyStackedType);
	return Increment();
}

HeeksObj* MarkedObject::GetFirstOfBottomOnly()
{
	SetFirst(BottomOnlyStackedType);
	return Increment();
}

HeeksObj* MarkedObject::GetFirstOfOneFromLevel()
{
	SetFirst(OneFromLevelStackedType);
	return Increment();
}

HeeksObj* MarkedObject::Increment()
{
	if(m_stacked_type == TopOnlyStackedType)
	{
		if(m_map.size() == 0)return NULL;
		if(!m_processed)
		{
			m_processed = true;
			CurrentIt = m_map.begin();
		}
		else
		{
			if(CurrentIt == m_map.end())return NULL;
			CurrentIt++;
			if(CurrentIt == m_map.end())return NULL;
		}
		return CurrentIt->first;
	}
	if(m_stacked_type == BottomOnlyStackedType)
	{
		if(!m_processed && m_map.size() == 0)
		{
			m_processed = true;
			return m_object;
		}
		if(m_map.size() == 0)return NULL;
		if(!m_processed)
		{
			m_processed = true;
			CurrentIt = m_map.begin();
		}
		HeeksObj* object = CurrentIt->second->Increment();
		if(object)return object;
		if(CurrentIt == m_map.end())return NULL;
		CurrentIt++;
		if(CurrentIt == m_map.end())return NULL;
		return Increment();
	}
	if(m_stacked_type == OneFromLevelStackedType)
	{
		if(!m_processed)
		{
			m_processed = true;
			if(m_object)return m_object;
		}
		if(m_map.size() == 0)return NULL;
		return m_map.begin()->second->Increment();
	}
	if(!m_processed)
	{
		m_processed = true;
		if(m_stacked_type == 0 && m_object)return m_object;
	}
	if(m_map.size() == 0)return NULL;
	if(CurrentIt == m_map.end())return NULL;
	HeeksObj* object = CurrentIt->second->Increment();
	if(object)return object;
	CurrentIt++;
	return Increment();
}
