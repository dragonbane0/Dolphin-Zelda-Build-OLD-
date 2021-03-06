// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

//Dragonbane

#include <wx/defs.h>
#include <wx/dialog.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/translation.h>
#include <wx/windowid.h>
#include <wx/artprov.h>
#include <wx/stattext.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/frame.h>


#include "Common/CommonTypes.h"
#include <string>


class wxWindow;
class wxButton;

class TWWSuperswim : public wxDialog
{
private:

	DECLARE_EVENT_TABLE();

protected:
	wxButton* m_button1;
	wxChoice* m_choice_isle;
	wxStaticText* m_staticText8;
	wxPanel* m_panel1;
	wxStaticText* m_staticText10;
	wxTextCtrl* m_textCtrl_XPos;
	wxStaticText* m_staticText12;
	wxTextCtrl* m_textCtrl_ZPos;
	wxButton* m_button4;
	wxButton* m_button5;

public:

	TWWSuperswim(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Superswim Script by Trog"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style  = wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP);
	void OnSelectionChanged(wxCommandEvent& event);
	void OnButtonPressed(wxCommandEvent& event);
	void OnCloseWindow(wxCloseEvent& event);
};
